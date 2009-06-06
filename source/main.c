
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <errno.h>
#include <unistd.h>

#include "http.h"
#include "world.h"
#include "input.h"
#include "startscreen.h"
#include "GRRLIB/GRRLIB.h"
#include "overlay.h"

Mtx GXmodelView2D;

int fadeout_opacity = 0;
bool fadeout = false;

u8 HWButton;
void WiiResetPressed() { HWButton = SYS_RETURNTOMENU; }
void WiiPowerPressed() { HWButton = SYS_POWEROFF_STANDBY;}
void WiimotePowerPressed(s32 chan) { HWButton = SYS_POWEROFF_STANDBY; }

int main(){
	SYS_SetResetCallback(WiiResetPressed);
	SYS_SetPowerCallback(WiiPowerPressed);
	WPAD_SetPowerButtonCallback(WiimotePowerPressed);
	
	VIDEO_Init();

	if(CONF_GetAspectRatio())
	{
		SCREEN_WIDTH = 720;
	} else {
		SCREEN_WIDTH = 640;
	}
	
	
	initializeinput();
	initializeworld();
	
	GRRLIB_InitVideo();
	GRRLIB_Start();
	
	startscreen();
		
    while(1){
		if(!fadeout)
		{
			updateinput();
			updateworld();
			updatetiles();
			updateoverlay();
		}
		
		drawworld();
		drawcursor();
		drawoverlay();

		//When hitting home do a slow fade to black,
		//if it is totally black do the actual exit
		if (wpadheld & WPAD_BUTTON_HOME || HWButton) fadeout = true;
		if(fadeout)
		{
			fadeout_opacity += 5;
			if(fadeout_opacity >= 270) {
				if(HWButton)
					SYS_ResetSystem(HWButton, 0, 0);
					
				return 0;
			}
				
			GRRLIB_Rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, fadeout_opacity > 255 ? 255 << 24 : fadeout_opacity << 24, true);
			
		}
		
		GRRLIB_Render();
    }
    return 0;
}

