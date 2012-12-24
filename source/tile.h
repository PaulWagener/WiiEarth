#ifndef _TILE_H_
#define _TILE_H_

#include "http.h"
#include "world.h"
#include "GRRLIB/GRRLIB.h"

enum tile_source {LIVE_MAP, LIVE_SATELLITE, GOOGLE_MAP, GOOGLE_SATELLITE, GOOGLE_TERRAIN, NUM_TILE_SOURCES};

enum tile_status {INITIALIZED, DOWNLOADING, VISIBLE};

extern enum tile_source current_tilesource;

#define NUM_TILES 9
struct tile* tiles[NUM_TILES];


struct tile* zoom1_tiles[4];

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
	GRRLIB_texImg texture;
	
	int opacity; //Between 0 and 255
	
	enum tile_source source;
	
	enum tile_status status;
};

void updatetiles();
void drawtiles();

#endif /* _TILE_H_ */
