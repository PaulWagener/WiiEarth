#include "tile.h"
#include "http.h"
#include "world.h"
#include "input.h"
#include "GRRLIB/GRRLIB.h"

#include <ogc/lwp.h> // Thread

struct tile* createtile(enum tile_source type, int x, int y, int zoom);
bool tiledownloaded(enum tile_source type, int x, int y, int zoom);
void deletetile(struct tile* tile);
void* download_and_place_tile(void* tile);
void* downloadtile(void* tile);
int getleastrelevanttile();

enum tile_source current_tilesource = LIVE_MAP;
struct tile* downloading_tile = NULL;
struct tile* downloading_zoom1tile = NULL;

static lwp_t downloadthread;


/**
 * Method that adds new relevant tiles
 *
 * Its first priority is to download a 3x3 grid of tiles that fills the screen
 * After that it will download all tiles 'above' the current view so that if the user quickly
 * zooms out there are tiles there to prevent a lot of black screen.
 */
void updatetiles()
{
	//Fade new tiles in
	int i;
	for(i = 0; i < NUM_TILES; i++)
	{
		if(tiles[i] != NULL)
		{
			if(tiles[i]->opacity < 255)
				tiles[i]->opacity += 10;
			
			if(tiles[i]->opacity > 255)
				tiles[i]->opacity = 255;
		}
	}
	
	
	//Check if there are tiles that are done downloading
	if(downloading_tile != NULL && downloading_tile->status == VISIBLE)
	{
		//Delete the tile that is most out of view and replace it with the newly downloaded tile
		int i = getleastrelevanttile();
	
		if(tiles[i] != NULL)
			deletetile(tiles[i]);

		//Insert new tile
		tiles[i] = downloading_tile;
		tiles[i]->opacity = 0; //code in updatetiles() will fade the tile slowly into view
		downloading_tile = NULL;
	}
	

	if(downloading_zoom1tile != NULL && downloading_zoom1tile->status == VISIBLE)
	{
		downloading_zoom1tile->opacity = 255;
		if(downloading_zoom1tile->x == 0 && downloading_zoom1tile->y == 0) zoom1_tiles[0] = downloading_zoom1tile;
		if(downloading_zoom1tile->x == 0 && downloading_zoom1tile->y == 1) zoom1_tiles[1] = downloading_zoom1tile;
		if(downloading_zoom1tile->x == 1 && downloading_zoom1tile->y == 0) zoom1_tiles[2] = downloading_zoom1tile;
		if(downloading_zoom1tile->x == 1 && downloading_zoom1tile->y == 1) zoom1_tiles[3] = downloading_zoom1tile;

		downloading_zoom1tile = NULL;
	}
	
	int zoom = world_zoom_target < 1 ? 1 : world_zoom_target;
	const int numtiles_across = pow(2, zoom); //Number of tiles from top to bottom on this zoomlevel
	
	//Coordinates for the tile on the center of the screen
	const int center_x = world_x * numtiles_across;
	const int center_y = world_y * numtiles_across;
	
	//No tile being downloaded, download a new tile
	if(downloading_tile == NULL && downloading_zoom1tile == NULL)
	{
		//Find a tile in the 3x3 grid of tiles around the center of the screen
		//that is closest to the center (and not yet downloaded)
		int x, y,
			local_x, local_y,
			tile_x, tile_y;
		bool tile_found = FALSE;
		float best_distance;

		for(local_x = -1; local_x <= 1; local_x++)
		{
			for(local_y = -1; local_y <= 1; local_y++)
			{
				x = center_x + local_x;
				y = center_y + local_y;
				
				if(x < 0) x = 0;
				if(y < 0) y = 0;
				if(x >= numtiles_across) x = numtiles_across-1;
				if(y >= numtiles_across) y = numtiles_across-1;
							
				//If the tile is not yet downloaded it forms a candidate to be downloaded
				if(!tiledownloaded(current_tilesource, x, y, zoom))
				{
					//The best tile to download is one that has the least distance to the center of the screen
					float tilecenter_x = (x / (float)numtiles_across) + (1 / (float)numtiles_across) / 2,
						  tilecenter_y = (y / (float)numtiles_across) + (1 / (float)numtiles_across) / 2;

					float distance = powf(world_x - tilecenter_x, 2) + powf(world_y - tilecenter_y, 2);

					if(!tile_found)
					{
						tile_found = TRUE;
						tile_x = x;
						tile_y = y;
						best_distance = distance;
					} else {
						if(distance < best_distance)
						{
							tile_x = x;
							tile_y = y;
							best_distance = distance;
						}						
					}
				}
				
			}
		}
	
		if(tile_found)
		{
			downloading_tile = createtile(current_tilesource, tile_x, tile_y, zoom);
			if(downloading_tile != NULL)
			{
				LWP_CreateThread(&downloadthread, downloadtile, downloading_tile, NULL, 0, 80);
			}
		}
		
	}
	
	//Download the 4 tiles in zoom level 1 that show the whole world
	//(displayed as a background)
	if(downloading_tile == NULL && downloading_zoom1tile == NULL)
	{
		int i;
		for(i = 0; i < 4; i++)
		{
			if(zoom1_tiles[i] == NULL)
			{
				switch(i)
				{
					case 0: downloading_zoom1tile = createtile(current_tilesource, 0, 0, 1); break;
					case 1: downloading_zoom1tile = createtile(current_tilesource, 0, 1, 1); break;
					case 2: downloading_zoom1tile = createtile(current_tilesource, 1, 0, 1); break;
					case 3: downloading_zoom1tile = createtile(current_tilesource, 1, 1, 1); break;
				}
				LWP_CreateThread(&downloadthread, downloadtile, downloading_zoom1tile, NULL, 0, 80);
				break;
			}
		}
	}
	
	enum tile_source old_tilesource = current_tilesource;
    
	//Switch tiletype
	if(wpaddown & WPAD_BUTTON_2)
	{
        if(current_tilesource >= NUM_TILE_SOURCES - 1)
            current_tilesource = 0;
        else
            current_tilesource++;
    }
    
    if(wpaddown & WPAD_BUTTON_1)
    {
        if(current_tilesource == 0)
            current_tilesource = NUM_TILE_SOURCES - 1;
        else
            current_tilesource--;
    }
	
	if(current_tilesource != old_tilesource) {
		//Delete all current tiles
		for(i = 0; i < NUM_TILES; i++)
		{
			if(tiles[i] != NULL)
			{
				deletetile(tiles[i]);
				tiles[i] = NULL;
			}
		}
		
		for(i = 0; i < 4; i++)
		{
			if(zoom1_tiles[i] != NULL)
			{
				deletetile(zoom1_tiles[i]);
				zoom1_tiles[i] = NULL;
			}
		}
		
	}
	
}

void deletetile(struct tile* tile)
{
	if(tile->texture.data != NULL)
		free(tile->texture.data);
	
	free(tile);
}

/**
 * Returns if a tile is already in the 3x3 block of tiles
 */
bool tiledownloaded(enum tile_source source, int x, int y, int zoom)
{
	int i;
	for(i = 0; i < NUM_TILES; i++)
	{
		if(tiles[i] != NULL &&
			tiles[i]->source == source &&
			tiles[i]->x == x &&
			tiles[i]->y == y &&
			tiles[i]->zoom == zoom)
		{
			return TRUE;
		}
	}
	return FALSE;
}

GRRLIB_texImg url2texture(char *url)
{
	struct block file = downloadfile(url);
	
	if(file.data == NULL) {
		return empty_texture;
		
	}
	GRRLIB_texImg texture;
	if(file.data[0]==0xFF && file.data[1]==0xD8 && file.data[2]==0xFF) {
        texture = GRRLIB_LoadTextureJPEG(file.data, file.size);
    }
    else {
        texture = GRRLIB_LoadTexture(file.data);
    }
	free(file.data);
	return texture;
}

GRRLIB_texImg getlivehybridtile(char hcode[])
{
	char url[150];
	int randomserver = abs(hcode[strlen(hcode)-1]) % 4;
	int result = sprintf(url, "http://h%i.ortho.tiles.virtualearth.net/tiles/h%s.jpeg?g=167", randomserver, hcode);

	if(result < 0)
		return empty_texture;

	return url2texture(url);
}

GRRLIB_texImg getlivemaptile(char rcode[], int zoom)
{
	char url[150];
	int randomserver = abs(rcode[strlen(rcode)-1]) % 4;
	
	int result = sprintf(url, "http://r%i.ortho.tiles.virtualearth.net/tiles/r%s.png?g=174&shading=hill", randomserver, rcode);

	if(result < 0)
		return empty_texture;

	return url2texture(url);
}

GRRLIB_texImg getgooglemaptile(float lattitude, float longitude, int zoom)
{
	char url[300];

	int result = sprintf(url, "http://maps.google.com/staticmap?center=%.5f,%.5f&zoom=%i&size=256x256&maptype=map&format=jpg&sensor=false&key=ABQIAAAAC0oGO8iGcGLO1jeaET0fbhTLp7rNJYmgRqV7WukaV0vQ79jYwRQBAhP9xmVSeNw0BbDVzVYWf7NurA", lattitude, longitude, zoom);

	if(result < 0)
		return empty_texture;

	return url2texture(url);
}

GRRLIB_texImg getgooglesatellitetile(float lattitude, float longitude, int zoom)
{
	char url[300];
	int result = sprintf(url, "http://maps.google.com/staticmap?center=%.5f,%.5f&zoom=%i&size=256x256&maptype=hybrid&format=jpg&sensor=false&key=ABQIAAAAC0oGO8iGcGLO1jeaET0fbhTLp7rNJYmgRqV7WukaV0vQ79jYwRQBAhP9xmVSeNw0BbDVzVYWf7NurA", lattitude, longitude, zoom);

	if(result < 0)
		return empty_texture;

	return url2texture(url);
}

GRRLIB_texImg getgoogleterraintile(float lattitude, float longitude, int zoom)
{
	char url[300];

	int result = sprintf(url, "http://maps.google.com/staticmap?center=%.5f,%.5f&zoom=%i&size=256x256&maptype=terrain&format=jpg&sensor=false&key=ABQIAAAAC0oGO8iGcGLO1jeaET0fbhTLp7rNJYmgRqV7WukaV0vQ79jYwRQBAhP9xmVSeNw0BbDVzVYWf7NurA", lattitude, longitude, zoom);

	if(result < 0)
		return empty_texture;

	return url2texture(url);
}

/**
 * See here: http://wiki.openstreetmap.org/wiki/QuadTiles
 * for an explanation what quadtiles are
 */
char* converttoquadtiles(int x, int y, int zoom, char codes[])
{
	char *code = malloc(zoom + 1);
	if(code == NULL)
		return NULL;
			
	int tiles = pow(2, zoom);
	int halftiles = tiles / 2;
	int i = 0;
			
	for(; i < zoom; i++)
	{
		bool top = true;
		bool left = true;
		if(x >= halftiles) {
			left = false;
			x -= halftiles;
		}
		if(y >= halftiles) {
			top = false;
			y -= halftiles;
		}
		char letter;
		if(top && left) letter = codes[0];
		if(top && !left) letter = codes[1];
		if(!top && left) letter = codes[2];
		if(!top && !left) letter = codes[3];
		code[i] = letter;
				
		tiles = halftiles;
		halftiles = tiles / 2;
	}
	code[i] = '\0';
	return code;

}

/**
 * This is the interface through which all tile objects should be created
 * It creates a blank tile ready to be downloaded
 */
struct tile* createtile(enum tile_source source, int x, int y, int zoom)
{
    //Initialize a new tile
	struct tile *tile = malloc(sizeof(struct tile));
        
    if(tile == NULL)
		return NULL;

    tile->x = x;
    tile->y = y;
    tile->zoom = zoom;
    tile->texture = empty_texture;
    tile->source = source;
	tile->status = INITIALIZED;
	
	tile->opacity = 0;

    //Calculate the world position of the tile
    float tiles = pow(2, zoom);
    float xworld = x / tiles;
    float yworld = y / tiles;
    float width = 1 / tiles;

    tile->left = xworld;
    tile->right = xworld + width;
    tile->top = yworld;
	tile->bottom = yworld + width;

    return tile;
}


/**
 * Takes the tile and fills the texture property of it (and also updates the status)
 * After the tile is downloaded it will find another tile that is most out of view and delete it
 
 * The idea is that you call this method in a seperate thread because it blocks until the download and textureconversion is complete
 */
void* downloadtile(void* tilepointer)
{
	struct tile* tile = tilepointer;
	float longitude, lattitude;
	
	if(tile->source == GOOGLE_MAP || tile->source == GOOGLE_SATELLITE || tile->source == GOOGLE_TERRAIN) {
		const float PI = 3.1415f;

		float centerx = tile->left + (tile->right - tile->left) / 2;
		float centery = tile->top + (tile->bottom - tile->top) / 2;
		longitude = centerx * 360.0 - 180.0;
		float lattitude_rad = atan(sinh( PI * -(centery*2 - 1)));
		lattitude = lattitude_rad * 180.0 / PI;
	}
	
	tile->status = DOWNLOADING;
	
	//Downloads the tile by calling the appropriate API
	switch(tile->source)
	{
		case LIVE_SATELLITE:
		{
			char* code = converttoquadtiles(tile->x, tile->y, tile->zoom, "0123");
			tile->texture = getlivehybridtile(code);
			free(code);
			break;
		}
		case LIVE_MAP:
		{
			char* code = converttoquadtiles(tile->x, tile->y, tile->zoom, "0123");
			tile->texture = getlivemaptile(code, tile->zoom);
			free(code);
			break;
		}
		case GOOGLE_MAP:
		{
			tile->texture = getgooglemaptile(lattitude, longitude, tile->zoom);
			break;
		}		
		case GOOGLE_SATELLITE:
		{
			tile->texture = getgooglesatellitetile(lattitude, longitude, tile->zoom);
			break;
		}
		case GOOGLE_TERRAIN:
		{
			tile->texture = getgoogleterraintile(lattitude, longitude, tile->zoom);
			break;
		}
            
        case NUM_TILE_SOURCES:
            break;
	}
	
	tile->status = VISIBLE;
	return NULL;
}

/**
 * Find an index in the array of a tile that is least visible from all tiles on screen
 * By calling this function you assume that such a tile exists outside of the 3x3 grid
 */
int getleastrelevanttile()
{
	int i;
	
	//First try to find empty spaces in the tile array
	for(i = 0; i < NUM_TILES; i++)
	{
		if(tiles[i] == NULL)
			return i;
	}
	
	const struct point center = screen2world(SCREEN_XCENTER, SCREEN_YCENTER);
	const int numtiles_across_worldzoom = pow(2, world_zoom_target); //Number of tiles from top to bottom on this zoomlevel
	const int center_x = floor(center.x * numtiles_across_worldzoom),
			  center_y = floor(center.y * numtiles_across_worldzoom);
	
	//Iterate over all the tiles and get the zoomlevel that is furthest from the current zoomlevel,
	//either above or below the current zoomlevel
	int minzoom = tiles[0]->zoom;
	int maxzoom = minzoom;
	
	for(i = 0; i < NUM_TILES; i++)
	{
		if(tiles[i] != NULL)
		{
			if(tiles[i]->zoom < minzoom)
				minzoom = tiles[i]->zoom;
			
			if(tiles[i]->zoom > maxzoom)
				maxzoom = tiles[i]->zoom;
		}
	}
	
	int furthest_zoom;
	if(world_zoom_target - minzoom > maxzoom - world_zoom_target)
		furthest_zoom = minzoom;
	else
		furthest_zoom = maxzoom;
		

	//Find a tile in that zoomlevel that has the largest distance from the center		
	float furthest_distance = 0;
	int furthest_tile = 0;

	for(i = 0; i < NUM_TILES; i++)
	{
		if(tiles[i]->zoom == furthest_zoom	//Get a tile from the least relevant zoomlevel,
											//but do NOT touch upon the tiles that are in the 3x3 grid of tiles in the center
											//		(We are trying to download those, not delete them!)
			&& !(tiles[i]->zoom == world_zoom_target && tiles[i]->source == current_tilesource && tiles[i]->x >= center_x-1  && tiles[i]->x <= center_x+1 && tiles[i]->y >= center_y-1 && tiles[i]->y <= center_y+1))
		{
			const int numtiles_across = pow(2, tiles[i]->zoom); //Number of tiles from top to bottom on this zoomlevel
			const float tilecenter_x = (tiles[i]->x / (float)numtiles_across) + (1 / (float)numtiles_across) / 2,
						tilecenter_y = (tiles[i]->y / (float)numtiles_across) + (1 / (float)numtiles_across) / 2;
				  
			//Technically to get the real distance we should take the square root, but we are only using it for comparison.
			float distance = powf(world_x - tilecenter_x, 2) + powf(world_y - tilecenter_y, 2);
			if(distance > furthest_distance)
			{
				furthest_distance = distance;
				furthest_tile = i;
			}
		}
	}
	
	return furthest_tile;
}
