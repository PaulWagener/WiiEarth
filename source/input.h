#ifndef _INPUT_H_
#define _INPUT_H_

#include <ogcsys.h>
#include <wiiuse/wpad.h>
#include "GRRLIB/GRRLIB.h"

#include "gfx/cursor_grab.h"
#include "gfx/cursor_hand.h"
#include "world.h"

float cursor_x;
float cursor_y;
float cursor_rot;
bool cursor_visible;

u32 wpaddown;
u32 wpadheld;

int SCREEN_WIDTH;
#define SCREEN_HEIGHT 480

#define SCREEN_XCENTER SCREEN_WIDTH / 2
#define SCREEN_YCENTER SCREEN_HEIGHT / 2  

#define CURSOR_WIDTH 96
#define CURSOR_HEIGHT 96
#define CURSOR_OPACITY 255

GRRLIB_texImg tex_cursor_grab;
GRRLIB_texImg tex_cursor_hand;

void initializeinput();
void updateinput();
void drawcursor();

#endif /* _INPUT_H_ */
