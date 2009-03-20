#ifndef _TILE_H_
#define _TILE_H_

#include "http.h"

enum tiletype {OSM, LIVE_SATELLITE, LIVE_MAP, GOOGLE_MAP, GOOGLE_SATELLITE, GOOGLE_TERRAIN};

extern enum tiletype tiletype_current;

#define NUM_TILES 9
struct tile* tiles[NUM_TILES];
struct tile* downloading_tile;

//A tile is an image of a piece of the earth
struct tile {

	//Position of tile on global map (in 0.0 1.0 coordinate system)
	float top;
	float left;
	
	float bottom;
	float right;
	
	//Tile identity
	int x;
	int y;
	int zoom;
	
	//Texture in 4x4RGBA8 format
	u8 *texture;
	
	int opacity; //Between 0 and 255
	
	enum tiletype type;
};

void updatetiles();
void drawtiles();

#endif /* _TILE_H_ */
