
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
#include "dns.h"
#include "world.h"
#include "input.h"
#include "startscreen.h"
#include "GRRLIB/GRRLIB.h"


Mtx GXmodelView2D;

int fadeout_opacity = 0;
bool fadeout = false;

int oldzoomtarget = 1000;

int main(){
	
	VIDEO_Init();
	WPAD_Init();

	startscreen();
	
	GRRLIB_InitVideo();
	GRRLIB_Start();

	initializedownload();
	initializedns();
	
	initializeinput();
	initializeworld();

    while(1){
		if(!fadeout)
		{
			updateinput();
			updateworld();
			updatetiles();
		}
		
		drawworld();
		drawcursor();
		
		//When hitting home do a slow fade to black,
		//if it is totally black do the actual exit
		if (wpadheld & WPAD_BUTTON_HOME) fadeout = true;
		if(fadeout)
		{
			fadeout_opacity += 5;
			if(fadeout_opacity >= 255)
				return 0;
				
			GRRLIB_Rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, fadeout_opacity << 24, true);
			
		}
		
		GRRLIB_Render();
    }
    return 0;
}

