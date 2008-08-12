#include "input.h"

void initializeinput()
{
	WPAD_Init();
	WPAD_SetDataFormat(0, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(0, 640, 480);

	cursor_x = 0;
	cursor_y = 0;
	cursor_rot = 0;
	cursor_visible = FALSE;
	
	tex_cursor_grab=GRRLIB_LoadTexture(cursor_grab);
	tex_cursor_hand=GRRLIB_LoadTexture(cursor_hand);
}

void updateinput()
{
	//Update cursor placement
    u32 type;
    int res = WPAD_Probe(0, &type);
	
    if(res == WPAD_ERR_NONE) {
		WPADData *wd = WPAD_Data(WPAD_CHAN_0);
		
		if(wd->ir.valid)
		{
			cursor_x = (wd->ir.x / SCREEN_WIDTH * (SCREEN_WIDTH + CURSOR_WIDTH * 2)) - CURSOR_WIDTH;
			cursor_y = (wd->ir.y / SCREEN_HEIGHT * (SCREEN_HEIGHT + CURSOR_HEIGHT * 2)) - CURSOR_HEIGHT;
		}
		cursor_rot = wd->orient.roll;
		cursor_visible = wd->ir.valid;
	} else {
		cursor_visible = false;
	}
	
	//Update keys pressed
	WPAD_ScanPads();
	wpadheld = WPAD_ButtonsHeld(WPAD_CHAN_0);
	wpaddown = WPAD_ButtonsDown(WPAD_CHAN_0);	
}

void drawcursor()
{
	if(cursor_visible)
	{
		u8 *cursortexture = world_grabbed ? tex_cursor_grab : tex_cursor_hand;
	
		GRRLIB_DrawImg(cursor_x - (CURSOR_WIDTH / 2), cursor_y - (CURSOR_HEIGHT / 2), CURSOR_WIDTH, CURSOR_HEIGHT, cursortexture, cursor_rot, 1, 1, CURSOR_OPACITY );
	}


}

