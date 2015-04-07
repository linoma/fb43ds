#include <stdlib.h>
#include <stdio.h>
#include <3ds.h>
#include "webrequest.h"
#include "fb.h"
#include "utils.h"

extern u32* gxCmdBuf;
u32* gpuOut = (u32*)0x1F119400;
u32* gpuDOut = (u32*)0x1F370800;

static touchPosition lt;
static circlePosition lcp;
//---------------------------------------------------------------------------
int main(int argc, char** argv)
{
	touchPosition lastTouch;
	int frame=0,lp_frame=0;
	
	CWebRequest::InitializeClient();	
	gfxInitDefault();
	GPU_Init(NULL);
	gfxSet3D(false);
	srand(svcGetSystemTick());
	CFBClient::Initialize();
	while(aptMainLoop()){
		hidScanInput();		
		u32 press = hidKeysDown();
		u32 held = hidKeysHeld();
		u32 release = hidKeysUp();
		if((press & ~KEY_TOUCH)){
			CFBClient::onKeysPressEvent(press,1);
			hidCircleRead(&lcp);
		}
		if((release & ~KEY_TOUCH)){
			CFBClient::onKeysUpEvent(press,1);
			hidCircleRead(&lcp);
		}
		if (held & KEY_TOUCH){
			hidTouchRead(&lt);
			if(!lp_frame){
				lastTouch=lt;
				lp_frame++;
			}
			else{
				int i=0;
				
				if(abs(lt.px-lastTouch.px) <= 5){
					if(abs(lt.py-lastTouch.py) <= 5)
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
//---------------------------------------------------------------------------
int getCursorPos(LPPOINT p)
{
	if(!p)
		return -1;
	p->x = lt.px;
	p->y = lt.py;
	return 0;
}