#include <stdlib.h>
#include <stdio.h>
#include <3ds.h>
#include "webrequest.h"
#include "fb.h"


extern u32* gxCmdBuf;
u32* gpuOut = (u32*)0x1F119400;
u32* gpuDOut = (u32*)0x1F370800;

extern LPDEFFUNC pfn_State;

int main(int argc, char** argv)
{
	touchPosition lastTouch,lt;
	int frame=0,lp_frame=0;
	
	srvInit();	
	aptInit();	
	CWebRequest::InitializeClient();	
	gfxInit();
	hidInit(NULL);	
	GPU_Init(NULL);
	gfxSet3D(false);
	fsInit();
	srand(svcGetSystemTick());
	CFBClient::Initialize();
	pfn_State = CFBClient::main;
	while(aptMainLoop()){
		hidScanInput();		
		u32 press = hidKeysDown();
		u32 held = hidKeysHeld();
		u32 release = hidKeysUp();
		if (held & KEY_TOUCH){
			hidTouchRead(&lt);
			if(!lp_frame)
				lastTouch=lt;
			else{
				int i=0,d;
				
				d = abs(lt.px-lastTouch.px);
				if(d <= 5){
					d = abs(lt.py-lastTouch.py);
					if(d <= 5)
						i = 1;
				}
				if(i)
					lp_frame++;
				else{
					lp_frame = 0;
					CFBClient::onTouchEvent(&lt,2);
				}
			}
			if(!frame)
				CFBClient::onTouchEvent(&lt,1);
			frame++;
		}
		else{
			if(frame)
				CFBClient::onTouchEvent(&lt,lp_frame > 120 ? 8 : 4);
			frame = 0;
			lp_frame = 0;
		}
		CFBClient::main(0);
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
	CFBClient::Destroy();
	CWebRequest::DestroyClient();
	fsExit();
	hidExit();
	gfxExit();
	aptExit();
	srvExit();
	return 0;
}