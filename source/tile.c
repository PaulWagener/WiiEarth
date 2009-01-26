#include "tile.h"
#include "http.h"
#include "world.h"
#include "input.h"
#include "GRRLIB/GRRLIB.h"

#include <ogc/lwp.h> // Thread

struct tile* createtile(enum tiletype type, int x, int y, int zoom);
bool tiledownloaded(enum tiletype type, int x, int y, int zoom);
void deletetile(struct tile* tile);
void* downloadtile(void* tile);
int getleastrelevanttile();

enum tiletype tiletype_current = OSM;
struct tile* downloading_tile = NULL;

/**
 * Method that adds new relevant tiles
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
	
	int zoom = world_zoom_target < 1 ? 1 : world_zoom_target;
	const int numtiles_across = pow(2, zoom); //Number of tiles from top to bottom on this zoomlevel
	
	//Coordinates for the tile on the center of the screen
	const int center_x = world_x * numtiles_across;
	const int center_y = world_y * numtiles_across;
	
	//No tile being downloaded, download a new tile
	if(downloading_tile == NULL)
	{
		//Find a tile in the 3x3 grid of tiles around the center of the screen
		//that is closest to the center (and not yet downloaded)
		int x, y,
			local_x, local_y,
			besttile_x, besttile_y;
		bool besttile_found = FALSE;
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
				if(!tiledownloaded(tiletype_current, x, y, zoom))
				{
					//The best tile to download is one that has the least distance to the center of the screen
					float tilecenter_x = (x / (float)numtiles_across) + (1 / (float)numtiles_across) / 2,
						  tilecenter_y = (y / (float)numtiles_across) + (1 / (float)numtiles_across) / 2;

					float distance = powf(world_x - tilecenter_x, 2) + powf(world_y - tilecenter_y, 2);

					if(!besttile_found)
					{
						besttile_found = TRUE;
						besttile_x = x;
						besttile_y = y;
						best_distance = distance;
					} else {
						if(distance < best_distance)
						{
							besttile_x = x;
							besttile_y = y;
							best_distance = distance;
						}						
					}
				}
				
			}
		}
		
		if(besttile_found)
		{
			downloading_tile = createtile(tiletype_current, besttile_x, besttile_y, zoom);
			if(downloading_tile != NULL)
			{
				lwp_t thread;
				LWP_CreateThread(&thread, downloadtile, downloading_tile, NULL, 0, 80);
			}
		}
	}
	
	//Switch tiletype
	if(wpaddown & WPAD_BUTTON_2)
	{
		switch(tiletype_current)
		{
			case OSM:
				tiletype_current = LIVE_HYBRID;
				break;
				
			case LIVE_HYBRID:
				tiletype_current = LIVE_MAP;
				break;
				
			case LIVE_MAP:
				tiletype_current = OSM;
				break;
		}
		
		/*
		//Delete all current tiles
		for(i = 0; i < NUM_TILES; i++)
		{
			if(tiles[i] != NULL)
			{
				deletetile(tiles[i]);
				tiles[i] = NULL;
			}
		}
		*/
	}
	
}

void deletetile(struct tile* tile)
{
	if(tile->texture != NULL)
		free(tile->texture);
	
	free(tile);
}

/**
 * Check if a certain tile is already in the list
 */
bool tiledownloaded(enum tiletype type, int x, int y, int zoom)
{
	int i;
	for(i = 0; i < NUM_TILES; i++)
	{
		if(tiles[i] != NULL &&
			tiles[i]->type == type &&
			tiles[i]->x == x &&
			tiles[i]->y == y &&
			tiles[i]->zoom == zoom)
		{
			return TRUE;
		}
	}
	return FALSE;
}

u8* pngurl2texture(char *url)
{
	struct block file = downloadfile(url);
	
	if(file.data == NULL)
		return NULL;
	
	//Transform to PNG
	u8* texture = GRRLIB_LoadTexture(file.data);
	free(file.data);
	return texture;
}

u8* jpegurl2texture(char *url)
{
	struct block file = downloadfile(url);
	
	if(file.data == NULL)
		return NULL;

	//These are values libjpeg cannot cope with and wich
	//are sometimes returned by google for 404 pages or captcha pages
	//libjpeg will exit() when it cannot parse a file which we do not want
	if(file.data[0] != 0xff)
	{
		free(file.data);
		return NULL;
	}
	
	//Transform to JPEG
	u8* texture = GRRLIB_LoadTextureJPEG(file.data, file.size);
	free(file.data);
	return texture;
}

u8* getosmtile(int zoom, int x, int y)
{
	char url[50];
	int result = sprintf(url, "http://tile.openstreetmap.org/%i/%i/%i.png", zoom, x, y);

	if(result < 0)
		return NULL;

	return pngurl2texture(url);
}

u8* getlivehybridtile(char hcode[])
{
	char url[150];
	int randomserver = abs(hcode[strlen(hcode)-1]) % 4;
	int result = sprintf(url, "http://h%i.ortho.tiles.virtualearth.net/tiles/h%s.jpeg?g=167", randomserver, hcode);

	if(result < 0)
		return NULL;

	return jpegurl2texture(url);
}

u8* getlivemaptile(char rcode[], int zoom)
{
	char url[150];
	int randomserver = abs(rcode[strlen(rcode)-1]) % 4;
	
	int result = sprintf(url, "http://r%i.ortho.tiles.virtualearth.net/tiles/r%s.png?g=174&shading=hill", randomserver, rcode);

	if(result < 0)
		return NULL;

	if( zoom <= 13)
		return jpegurl2texture(url);
	else
		return pngurl2texture(url);
}

char* converttoquartercode(int x, int y, int zoom, char codes[])
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
struct tile* createtile(enum tiletype type, int x, int y, int zoom)
{
    //Initialize a new tile
	struct tile *tile = malloc(sizeof(struct tile));
        
    if(tile == NULL)
		return NULL;

    tile->x = x;
    tile->y = y;
    tile->zoom = zoom;
    tile->texture = NULL;
    tile->type = type;
	
	tile->texture = tex_loading;
	tile->opacity = 100;

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
	
	if(tile == NULL)
		return NULL;

	//Downloads the tile by calling the appropriate API
	switch(tile->type)
	{
		case OSM:
			tile->texture = getosmtile(tile->zoom, tile->x, tile->y);
			break;
			
		case LIVE_HYBRID:
		{
			char* code = converttoquartercode(tile->x, tile->y, tile->zoom, "0123");
			tile->texture = getlivehybridtile(code);
			free(code);
			break;
		}
		case LIVE_MAP:
		{
			char* code = converttoquartercode(tile->x, tile->y, tile->zoom, "0123");
			
			
			u8* texture = getlivemaptile(code, tile->zoom);
			
			if(tile == NULL)
				exit(0);
				
			tile->texture = texture;
			free(code);
			break;
		}
	}

	//Delete the tile that is most out of view and replace it with the newly downloaded tile
	int i = getleastrelevanttile();

	if(tiles[i] != NULL)
	{
		deletetile(tiles[i]);
	}

	//Insert new tile
	tile->opacity = 0; //code in updatetiles() will fade the tile slowly into view
	tiles[i] = tile;
	
	downloading_tile = NULL;
	return NULL;
}

/**
 * Find an index in the array of a tile that is least visible from all tiles on screen
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
		if(tiles[i]->zoom == furthest_zoom	//Get a tile from the least relevant zoomlevel, but do NOT touch upon the tiles that are in the 3x3 grid of tiles in the center
			&& !(tiles[i]->zoom == world_zoom_target && tiles[i]->type == tiletype_current && tiles[i]->x >= center_x-1  && tiles[i]->x <= center_x+1 && tiles[i]->y >= center_y-1 && tiles[i]->y <= center_y+1))
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
