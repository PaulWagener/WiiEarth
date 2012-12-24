#include "overlay.h"
#include "tile.h"
#include "GRRLIB/GRRLIB.h"


#define DISPLAY_TIME 200
#define FADEIN_SPEED 25
#define FADEOUT_SPEED 40

GRRLIB_texImg tiletype_texture = {0,0,NULL};

enum tile_source overlay_tilesource = LIVE_MAP;
u8 overlay_opacity = 0;
u32 overlay_timedisplayed = 0;

GRRLIB_texImg get_tiletype_texture(enum tile_source tiletype)
{
	GRRLIB_texImg texture;
	switch(tiletype)
	{
		case LIVE_MAP:
			texture = GRRLIB_LoadTexture(live_maps);
			break;
	
		case LIVE_SATELLITE:
			texture = GRRLIB_LoadTexture(live_satellite);
			break;
			
		case GOOGLE_MAP:
			texture = GRRLIB_LoadTexture(google_maps);
			break;

		case GOOGLE_SATELLITE:
			texture = GRRLIB_LoadTexture(google_satellite);
			break;
			
		case GOOGLE_TERRAIN:
			texture = GRRLIB_LoadTexture(google_terrain);
			break;
            
        case NUM_TILE_SOURCES:
            break;
	}
	return texture;
}

void updateoverlay()
{
	
	//If the tiletype changed fade out the old overlay and once it is faded out put in a new texture
	if(current_tilesource != overlay_tilesource)
	{
		//The free() used to be before loading a new texture, this somehow leads to graphical glitches
		//Probably related to using the same memory as the previous texture.	
		u8* freetex = NULL;
		if(tiletype_texture.data != NULL)
		{
			freetex = tiletype_texture.data;
		}

		tiletype_texture = get_tiletype_texture(current_tilesource);
		
		if(freetex)
			free(freetex);
		
		overlay_tilesource = current_tilesource;
		overlay_timedisplayed = 0;
		overlay_opacity = 0;
		
	//1. Fade in
	//2. Wait for DISPLAY_TIME
	//3. Fade out
	//4. ???
	//5. Profit!
	} else {
		if(overlay_timedisplayed < DISPLAY_TIME) {
			if(overlay_opacity < (255 - FADEIN_SPEED))
				overlay_opacity += FADEIN_SPEED;
			overlay_timedisplayed++;
			
		} else {
			if(overlay_opacity > FADEOUT_SPEED)
				overlay_opacity -= FADEOUT_SPEED;
			else
				overlay_opacity = 0;
		}
	}
	
	if(tiletype_texture.data == NULL)
		tiletype_texture = get_tiletype_texture(current_tilesource);
}

void drawoverlay()
{
	if(tiletype_texture.data != NULL)
		GRRLIB_DrawImg(SCREEN_XCENTER - 100, 30, tiletype_texture, 0, 1, 1, 0xFFFFFF00 | overlay_opacity);
}
