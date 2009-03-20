#include "startscreen.h"
#include <wiiuse/wpad.h>
#include "http.h"

static u32 *xfb[2] = { NULL, NULL };
static GXRModeObj *vmode;

//Start up console
void initialize() {
	vmode = VIDEO_GetPreferredMode(NULL);
	xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
	xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
	console_init(xfb[0],20,20,vmode->fbWidth,vmode->xfbHeight,vmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(vmode);
	VIDEO_SetNextFramebuffer(xfb[0]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(vmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
}

void startscreen()
{
	initialize();
	
	printf("\n\n\n\n\n\n\n\n");
	printf("      Wii Earth 2.1\n");
	printf("         Images are from openstreetmap.org, maps.google.com and maps.live.com\n\n");
	
    printf("      Waiting for network to initialize...");
	
	s32 ip;
    while ((ip = net_init()) == -EAGAIN)
	{
		printf(".");
		usleep(100 * 1000); //100ms
	}
	printf("\n");
	
    if(ip < 0) {
		printf("      Error while initialising, error is %i, exiting\n", ip);
		usleep(1000 * 1000 * 1); //1 sec
		exit(0);
	}
        
	char myIP[16];
    if (if_config(myIP, NULL, NULL, true) < 0) {
		printf("      Error reading IP address, exiting");
		usleep(1000 * 1000 * 1); //1 sec
		exit(0);
	}
	printf("      Network initialised.\n");
	printf("      IP: %s\n\n", myIP);
	printf("      Have Fun!");
	
	usleep(1000 * 1000 * 1); //1 sec
}
