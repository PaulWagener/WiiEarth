#include "world.h"
#include "tile.h"
#include <math.h>

inline float coordinates2pixels(float degrees);
inline float pixels2coordinates(float pixels);
struct point world2screen(float lattitude, float longitude);
struct point screen2world(float x, float y);


/**
 * Wraps the GRRLIB_DrawImg function with parameters for width & height instead of scaleX & scaleY
 */
void DrawImg(float x, float y, float width, float height, u8 data[], u8 opacity)
{
        float scaleX = width / TILE_WIDTH;
        float scaleY = height / TILE_HEIGHT;

        //Adjust for the fact that images scale at their center
        float newX = x - (TILE_WIDTH / 2) + (width / 2);
        float newY = y - (TILE_HEIGHT / 2) + (height / 2); 
        
        GRRLIB_DrawImg(newX, newY, TILE_WIDTH, TILE_HEIGHT, data, 0, scaleX, scaleY, opacity);
} 

void stopslide()
{
	world_speed_x = 0;
	world_speed_y = 0;
	world_zooming_to = FALSE;
}

void initializeworld()
{
	world_x = 0.5;
	world_y = 0.5;
	world_zoom = -2;
	world_zoom_target = 0;
	world_zooming_to = FALSE;
	
	tex_loading = GRRLIB_LoadTexture(loading);
}


/**
 * Process controllerinput into a new mapview
 */
void updateworld()
{
	//Zooming
	if((wpaddown & WPAD_BUTTON_PLUS) && world_zoom_target < MAX_ZOOM) {
		stopslide();
		world_zoom_target += 1;
	}
	
	if((wpaddown & WPAD_BUTTON_MINUS) && world_zoom_target > MIN_ZOOM) {
		stopslide();
		world_zoom_target -= 1;
	}
	//Let world_zoom approach world_zoom_target
	world_zoom = world_zoom + ((world_zoom_target - world_zoom) / 10);
	
	world_width = coordinates2pixels(1); //Used within rendertile();
	
	//Dragging
	if(wpaddown & WPAD_BUTTON_A) {
		stopslide();
		struct point cursorworld = screen2world(cursor_x, cursor_y);
		world_grab_x = cursorworld.x;
		world_grab_y = cursorworld.y;
		
		world_grab_x -= floor(world_grab_x);
	}
	world_grabbed = wpadheld & WPAD_BUTTON_A;
	
	//Zooming towards a specific spot
	if(wpaddown & WPAD_BUTTON_B) {
		stopslide();
		if(world_zoom_target < MAX_ZOOM) world_zoom_target++;
		
		struct point cursorworld = screen2world(cursor_x, cursor_y);
		world_zoomto_x = cursorworld.x;
		world_zoomto_y = cursorworld.y;
		world_zoomto_x -= floor(world_zoomto_x);
		if(world_zoomto_y < 0) world_zoomto_y = 0;
		if(world_zoomto_y > 1) world_zoomto_y = 1;
		world_zooming_to = TRUE;
	}
	
	//If the user is holding the world then move the world so that the world_grab_x & y variables
	//are always right under the cursor
	if(world_grabbed)
	{
		float cursor_xcenter_distance = pixels2coordinates(cursor_x - SCREEN_XCENTER);
		float cursor_ycenter_distance = pixels2coordinates(cursor_y - SCREEN_YCENTER);
		
		float newworld_x = world_grab_x - cursor_xcenter_distance;
		float newworld_y = world_grab_y - cursor_ycenter_distance;
		
		//Keep it between 0 and 1
		newworld_x -= floor(newworld_x);
		
		if(newworld_y > 1) newworld_y = 1;
		if(newworld_y < 0) newworld_y = 0;

		//Store the speed in case the user lets go
		world_speed_x = newworld_x - world_x;
		world_speed_y = newworld_y - world_y;
		
		world_speed_x -= (int)(world_speed_x);		


		
		world_x = newworld_x;
		world_y = newworld_y;
	} else {
		world_speed_x /= world_speed_x < pixels2coordinates(SPEED_CUTOFFPOINT) ? HIGHSPEED_DECAYRATE : LOWSPEED_DECAYRATE;
		world_speed_y /= world_speed_y < pixels2coordinates(SPEED_CUTOFFPOINT) ? HIGHSPEED_DECAYRATE : LOWSPEED_DECAYRATE;
		
		world_x += world_speed_x;
		world_y += world_speed_y;
	}
	
	//D-pad movement
	if(!world_grabbed)
	{
		if(wpadheld & WPAD_BUTTON_UP) {
			stopslide();
			world_y -= pixels2coordinates(DPAD_SPEED);
		}
		
		if(wpadheld & WPAD_BUTTON_DOWN) {
			stopslide();
			world_y += pixels2coordinates(DPAD_SPEED);
		}
		
		if(wpadheld & WPAD_BUTTON_LEFT) {
			stopslide();
			world_x -= pixels2coordinates(DPAD_SPEED);
		}
		
		if(wpadheld & WPAD_BUTTON_RIGHT) {
			stopslide();
			world_x += pixels2coordinates(DPAD_SPEED);
		}
	}
	
	//Move to point
	if(world_zooming_to)
	{
		world_x += (world_zoomto_x - world_x)  / 10;
		world_y += (world_zoomto_y - world_y)  / 10;
	}
	
	//Correct for panning across the earth
	world_x -= floor(world_x);
	if(world_y > 1) world_y = 1;
	if(world_y < 0) world_y = 0;	

	//Update screen variables
	struct point topleft = screen2world(0, 0);
	struct point bottomright = screen2world(SCREEN_WIDTH, SCREEN_HEIGHT);
	screen_left = topleft.x;
	screen_top = topleft.y;
	screen_right = bottomright.x;
	screen_bottom = bottomright.y;
}

/**
 * Display all tiles as a map
 */
void drawworld()
{
	GX_SetZMode(GX_DISABLE,GX_ALWAYS,GX_FALSE);

	struct tile* tile = firsttile;
	while(tile != NULL)
	{
		drawtile(tile);
			
		tile = tile->nexttile;
	}
}

/**
 * Draws a single tile onto the screen
 * Also changes the status from VISIBLE to 
 */
void drawtile(struct tile* tile)
{
	if(tile->texture == NULL)
		return;
		
	struct point topleft = world2screen(tile->top, tile->left);
	struct point bottomright = world2screen(tile->bottom, tile->right);
	float width = bottomright.x - topleft.x;
	float height = bottomright.y - topleft.y;
	
	DrawImg(topleft.x, topleft.y, width, height, tile->texture, tile->opacity);

	//Draw extra copies of this tile to the left and to the right
	//Should only happed if the view is so zoomed out that the world can be seen multiple times
	
	//Draw copies to the left
	float extratilesx = topleft.x - world_width;
	while(extratilesx + width > 0)
	{
		DrawImg(extratilesx, topleft.y, width, height, tile->texture, tile->opacity);
		extratilesx -= world_width;
	}
	
	//Draw copies to the right
	extratilesx = topleft.x + world_width;
	while(extratilesx < SCREEN_WIDTH) {
		DrawImg(extratilesx, topleft.y, width, height, tile->texture, tile->opacity);
		extratilesx += world_width;
	}
}

inline float coordinates2pixels(float degrees)
{
	return degrees * WORLD_SIZE_AT_ZOOM0 * powf(2, world_zoom);
}

inline float pixels2coordinates(float pixels)
{
	return pixels / WORLD_SIZE_AT_ZOOM0 / powf(2, world_zoom);
}

struct point world2screen(float lattitude, float longitude) {
	float xcenter_distance = coordinates2pixels(longitude - world_x);
	float ycenter_distance = coordinates2pixels(lattitude - world_y);
	
	struct point screen;
	screen.x = SCREEN_XCENTER + xcenter_distance;
	screen.y = SCREEN_YCENTER + ycenter_distance;
	return screen;
}

struct point screen2world(float x, float y)
{
	float xcenter_distance = x - SCREEN_XCENTER;
	float ycenter_distance = y - SCREEN_YCENTER;
	
	struct point world;
	world.x = world_x + pixels2coordinates(xcenter_distance);
	world.y = world_y + pixels2coordinates(ycenter_distance);
	return world;
}
