#ifndef _TILE_H_
#define _TILE_H_

#include "http.h"

extern struct tile* firsttile;

enum tilestatus {INITIALIZED, DOWNLOADING, VISIBLE};

enum tiletype {GOOGLE_MAP, GOOGLE_SATELLITE, LIVE_HYBRID, LIVE_MAP};

extern enum tiletype tiletype_current;

#define PADTILES_TOP 1
#define PADTILES_BOTTOM 1
#define PADTILES_LEFT 1
#define PADTILES_RIGHT 1

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
	
	enum tilestatus status;

	struct tile *nexttile;
};

void updatetiles();

#endif /* _TILE_H_ */
