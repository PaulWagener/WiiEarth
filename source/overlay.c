#include "overlay.h"
#include "tile.h"

#define DISPLAY_TIME 200
#define FADEIN_SPEED 25
#define FADEOUT_SPEED 40


u8 *tiletype_texture = NULL;

enum tiletype overlay_tiletype = OSM;
u8 overlay_opacity = 0;
u32 overlay_timedisplayed = 0;

u8* get_tiletype_texture(enum tiletype tiletype)
{
	u8 *texture;
	switch(tiletype)
	{
		case OSM:
			texture = GRRLIB_LoadTexture(openstreetmap);
			break;
			
		case LIVE_HYBRID:
			texture = GRRLIB_LoadTexture(live_hybrid);
			break;
			
		case LIVE_MAP:
			texture = GRRLIB_LoadTexture(live_maps);
			break;
	}
	return texture;
}

void updateoverlay()
{
	if(tiletype_texture == NULL)
		tiletype_texture = get_tiletype_texture(tiletype_current);
		
	//If the tiletype changed fade out the old overlay and once it is faded out put in a new texture
	if(tiletype_current != overlay_tiletype)
	{
		if(overlay_opacity > FADEOUT_SPEED) {
			overlay_opacity -= FADEOUT_SPEED;
		} else {
			if(tiletype_texture != NULL)
				free(tiletype_texture);
				
			tiletype_texture = get_tiletype_texture(tiletype_current);
				
			overlay_tiletype = tiletype_current;
			overlay_timedisplayed = 0;
		}
		
	//Fade in, wait for DISPLAY_TIME, fade out
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
}

void drawoverlay()
{
	GRRLIB_DrawImg(SCREEN_XCENTER - 100, 30, 200, 30, tiletype_texture, 0, 1, 1, overlay_opacity);
}

