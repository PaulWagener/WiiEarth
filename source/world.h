#ifndef _WORLD_H_
#define _WORLD_H_

#include <math.h>
#include "tile.h"
#include "input.h"

#include "gfx/loading.h"
#include "GRRLIB/GRRLIB.h"

#define TILE_WIDTH 256
#define TILE_HEIGHT 256

#define MIN_ZOOM -1
#define MAX_ZOOM 18

#define WORLD_SIZE_AT_ZOOM0 512 //Width and height in pixels of the worldmap at zoom 0

//The user can 'fling' the map by grabbing and throwing it
//The speed the map gains from this is gradually reduced to 0
//To make for a smooth ride the lower speeds are reduced quicker then the higher speeds
#define HIGHSPEED_DECAYRATE 1.06
#define LOWSPEED_DECAYRATE 1.02
#define SPEED_CUTOFFPOINT 7 //In pixels per Vsync

#define DPAD_SPEED 5

//Generic structure to pass x,y values
struct point {
	float x;
	float y;
};

u8 *tex_loading;

//World_x and world_y represent the point that the map should be centered on
//It is a simple coordinate system with the topleft of google's map on 0.0, 0.0
//and the bottomright on 1.0, 1.0.
float world_x;
float world_y;

//Each zoomlevel going up means that both the width and the height of the view are halfed,
//0 is the zoomlevel that shows the whole earth, zoom 1 shows 1/4 of the earth, zoom 2 shows 1/8, etc
float world_zoom;

//This is the zoomlevel in discrete steps, world_zoom will always approach this value
int world_zoom_target;

//Variable derived from world_zoom
//states how many pixels the world is wide on the screen
float world_width;

//If the user gives the world a push these values will be filled with how fast the earth should slide across the screen
//Positive values means the map is moving to the topleft
float world_speed_x;
float world_speed_y;

//If the user is 'holding on' to the world with his Wiimote these values
//will tell the exact coordinates where he grabbed. It uses the same coordinate system as world_x & world_y
float world_grab_x;
float world_grab_y;
bool world_grabbed;

//Zooming towards a destination
bool world_zooming_to;
float world_zoomto_x;
float world_zoomto_y;

//Edges of the map on screen in world coordinates
float screen_top;
float screen_left;
float screen_bottom;
float screen_right;

void initializeworld();
void updateworld();
void drawworld();
void drawtile();

#endif /* _WORLD_H_ */
