#include "tile.h"
#include "http.h"
#include "world.h"
#include "input.h"
#include "GRRLIB/GRRLIB.h"

#include <ogc/lwp.h> // Thread


//The start of a linked list of all tiles that should be displayed
struct tile *firsttile = NULL;

struct tile* createtile(enum tiletype type, int x, int y, int zoom);
bool checktile(enum tiletype type, int x, int y, int zoom);
void deletetile(struct tile* tile, struct tile* previoustile);
void* downloadtile(void* tile);

enum tiletype tiletype_current = GOOGLE_MAP;

/**
 * Method that adds new relevant tiles
 */
void updatetiles()
{
	int tiles = pow(2, world_zoom_target);
	const int zoom = world_zoom_target < 0 ? 0 : world_zoom_target;
	
	const int x = world_x * tiles;
	const int y = world_y * tiles;
	
		//Go check on each tile to update and/or remove it
	struct tile* tile = firsttile;
	struct tile* previoustile = NULL;
	while(tile != NULL)
	{
		//Opacity
		int endopacity = tile->status == DOWNLOADING ? 100 : 255;
		if(tile->opacity < endopacity)
			tile->opacity += 10;
			
		if (tile->opacity > endopacity)
			tile->opacity = endopacity;
			
		//These tiles are not yet ready to deleted
		if(tile->status == INITIALIZED || tile->status == DOWNLOADING)
		{
			previoustile = tile;
			tile = tile->nexttile;
			continue;
		}
		

			
		//Get a pointer to the nexttile in case it will be gone
		struct tile* nexttile = tile->nexttile;
		bool tiledeleted = false;
		
		
		//Delete tiles out of padding area
		if(tile->zoom == zoom && 
			(tile->x < x - PADTILES_LEFT
			|| tile->x > x + PADTILES_RIGHT
			|| tile->y < y - PADTILES_TOP
			|| tile->y > y + PADTILES_BOTTOM))
		{
			deletetile(tile, previoustile);
			tiledeleted = true;
		}
		
		//Different tiletypes cannot peacefully coexist
		if(!tiledeleted && tile->type != tiletype_current)
		{
			deletetile(tile, previoustile);
			tiledeleted = true;
		}
	
		//Delete all tiles that aren't in the current zoom or the zoom above that
		if(!tiledeleted && tile->zoom != zoom  && tile->zoom != zoom - 1) {
			deletetile(tile, previoustile);
			tiledeleted = true;
		}
		
		//Delete tiles in the zoom above as soon as they are out of view
		if(!tiledeleted && tile->zoom == zoom - 1)
		{
			if(tile->bottom < screen_top || tile->top > screen_bottom
				|| tile->right < screen_left || tile->left > screen_right)
			{
				deletetile(tile, previoustile);
				tiledeleted = true;
			}
		}
				

		//Move on to the next tile in the list
		if(tiledeleted)
		{
			//previoustile remains the same
			tile = nexttile;
		} else {
			previoustile = tile;
			tile = tile->nexttile;
		}
	}
	
	int localx, localy;
	struct tile* lasttile = firsttile;
	while(lasttile != NULL && lasttile->nexttile != NULL)
		lasttile = lasttile->nexttile;
		
	for(localy = -PADTILES_TOP; localy <= PADTILES_BOTTOM; localy++)
	{
		for(localx = -PADTILES_LEFT; localx <= PADTILES_RIGHT; localx++)
		{
			int tilex = x + localx;
			int tiley = y + localy;
			
			if(tilex < 0 || tilex >= tiles
			|| tiley < 0 || tiley >= tiles)
			   continue;
			   
			if(!checktile(tiletype_current, tilex, tiley, zoom))
			{
				
				//Add the tile to the list
				struct tile* newtile = createtile(tiletype_current, tilex, tiley, zoom);
				if(newtile == NULL)
					continue;
					
				if(lasttile == NULL)
				{
					firsttile = newtile;
				} else {
					lasttile->nexttile = newtile;
					
				}
				lasttile = newtile;
				
				lwp_t thread;
				LWP_CreateThread(&thread, downloadtile, newtile, NULL, 0, 80);
			}
		}
	}
	
	
	//Switch tiletype
	if(wpaddown & WPAD_BUTTON_2)
	{
		switch(tiletype_current)
		{
			case GOOGLE_MAP:
				tiletype_current = LIVE_HYBRID;
				break;
				
			case LIVE_HYBRID:
				tiletype_current = LIVE_MAP;
				break;
				
			case LIVE_MAP:
				tiletype_current = GOOGLE_MAP; //GOOGLE_SATELLITE  (Google satellite temporarily disabled, it causes quite a few problems)
				break;
			
			case GOOGLE_SATELLITE:    
				tiletype_current = GOOGLE_MAP;
				break;
		}
	}
	
}

/**
 * Deletes a tile from the list
 */
void deletetile(struct tile* tile, struct tile* previoustile)
{
	if(previoustile != NULL)
		previoustile->nexttile = tile->nexttile;
		
	if(tile == firsttile) {
		if(tile->nexttile == NULL) {
			firsttile = NULL;
		} else {
			firsttile = tile->nexttile;
		}
	}
	
	if(tile->texture != NULL)
		free(tile->texture);
	
	free(tile);
}

/**
 * Check if a certain tile is already in the list
 */
bool checktile(enum tiletype type, int x, int y, int zoom)
{
	struct tile *tile = firsttile;
	while(tile != NULL)
	{
		if(tile->type == type
			&& tile->x == x 
			&& tile->y == y
			&& tile->zoom == zoom)
		{
			return true;
		}
		
		tile = tile->nexttile;
	}
	return false;
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
	if(file.data[0] != 0xff)// || file.data[0] == 0x3c)
	{
		free(file.data);
		return NULL;
	}
	
	//Transform to JPEG
	u8* texture = GRRLIB_LoadTextureJPEG(file.data, file.size);
	printf("%p\n", texture);
	free(file.data);
	return texture;
}

/**
 *
 */
u8* getgooglemaptile(int x, int y, int zoom)
{
	const char urlformat[] = "http://mt%i.google.com/mt?v=w2.80&hl=nl&x=%i&y=%i&zoom=%i&s=Galileo";
	
	char url[strlen(urlformat) + 30];
	//Google Maps spreads the load between servers mt0.google.com to mt3.google.com
	//We also do this, but to support url caching we let the server depend on the variables
	int randomserver = abs((x * y * zoom)) % 4;
	int result = sprintf(url, urlformat, randomserver, x, y, zoom);
	
	if(result < 0)
		return NULL;
		
	//Download tile
	return pngurl2texture(url);
	
}

u8* getgooglesattelitetile(char tcode[])
{
	const char urlformat[] = "http://khm%i.google.com/kh?v=30&t=%s";
	char url[strlen(urlformat) + 30];
	int randomserver = abs(tcode[strlen(tcode)-1]) % 4;
	int result = sprintf(url, urlformat, randomserver, tcode);

	if(result < 0)
		return NULL;

	printf("%s\n", url);
	
	return jpegurl2texture(url);
}

u8* getlivehybridtile(char hcode[])
{
	const char urlformat[] = "http://h%i.ortho.tiles.virtualearth.net/tiles/h%s.jpeg?g=167";
	char url[strlen(urlformat) + strlen(hcode) + 30];
	int randomserver = abs(hcode[strlen(hcode)-1]) % 4;
	int result = sprintf(url, urlformat, randomserver, hcode);

	if(result < 0)
		return NULL;

	return jpegurl2texture(url);
}


u8* getlivemaptile(char rcode[])
{
	const char urlformat[] = "http://r%i.ortho.tiles.virtualearth.net/tiles/r%s.png?g=174&shading=hill";
	char url[strlen(urlformat) + strlen(rcode) + 30];
	int randomserver = abs(rcode[strlen(rcode)-1]) % 4;
	int result = sprintf(url, urlformat, randomserver, rcode);

	if(result < 0)
		return NULL;

	return jpegurl2texture(url);
}

/**
 * This is the interface through which all tile objects should be created
 * It creates a blank tile ready to be downloaded
 */
struct tile* createtile(enum tiletype type, int x, int y, int zoom)
{
	//Initialize a new tile
	struct tile *tile = malloc(sizeof(struct tile));
	
	if(tile == NULL) {
		return NULL;
	}
	tile->nexttile = NULL;
	tile->x = x;
	tile->y = y;
	tile->zoom = zoom;
	tile->texture = NULL;
	tile->opacity = 0;
	tile->type = type;
	
	//Calculate the world position of the tile
	float tiles = pow(2, zoom);
	float xworld = x / tiles;
	float yworld = y / tiles;
	float width = 1 / tiles;

	tile->left = xworld;
	tile->right = xworld + width;
	tile->top = yworld;
	tile->bottom = yworld + width;
	
	tile->status = INITIALIZED;
	
	return tile;
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
 * Takes the tile and fills the texture property of it (and also updates the status)
 * The idea is that you call this method in a seperate thread because it blocks until the download and textureconversion is complete
 */
void* downloadtile(void* tilepointer)
{
	struct tile* tile = tilepointer;
	tile->status = DOWNLOADING;
	tile->texture = tex_loading;
	
	//Downloads the tile by calling the appropriate API
	switch(tile->type)
	{
		case GOOGLE_MAP:
			tile->texture = getgooglemaptile(tile->x, tile->y, 17 - tile->zoom);
			break;

		case GOOGLE_SATELLITE:
		{
			char* code = converttoquartercode(tile->x, tile->y, tile->zoom, "qrts");
			tile->texture = getgooglesattelitetile(code);
			break;
		}
		
		case LIVE_HYBRID:
		{
			char* code = converttoquartercode(tile->x, tile->y, tile->zoom, "0123");
			tile->texture = getlivehybridtile(code);
			break;
		}
		case LIVE_MAP:
		{
			char* code = converttoquartercode(tile->x, tile->y, tile->zoom, "0123");
			tile->texture = getlivemaptile(code);
			break;
		}
			
	}
	tile->opacity = 0;
	tile->status = VISIBLE;
	
	return NULL;
}

