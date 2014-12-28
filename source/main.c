#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "gfxtext.h"


extern u32* gxCmdBuf;
u32* gpuOut = (u32*)0x1F119400;
u32* gpuDOut = (u32*)0x1F370800;
u8 *linear_buffer;

int (*pfn_State)();
extern int widgets_draws();
extern int fb_init();
extern int widgets_touch_events(touchPosition *p);

int main(int argc, char** argv)
{
	touchPosition lastTouch;
	
	srvInit();	
	aptInit();
	gfxInit();
	hidInit(NULL);	
	GPU_Init(NULL);
	gfxSet3D(false);
	pfn_State = fb_init;
	linear_buffer= linearAlloc(0x80000);
	srand(svcGetSystemTick());
	while(aptMainLoop()){
		hidScanInput();		
		u32 press = hidKeysDown();
		u32 held = hidKeysHeld();
		u32 release = hidKeysUp();
		if (held & KEY_TOUCH){			
			hidTouchRead(&lastTouch);
			widgets_touch_events(&lastTouch);
			held &= ~KEY_TOUCH;
		}
		if(pfn_State)
			pfn_State();			
		widgets_draws();
		widgets_draws();
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForEvent(GSPEVENT_VBlank0, false);
	}
	hidExit();
	gfxExit();
	aptExit();
	srvExit();
	return 0;
}