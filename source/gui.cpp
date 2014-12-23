#include "gui.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "widgets.h"
#include "gfxdraw.h"

CDesktop *top,*bottom;
//---------------------------------------------------------------------------
int gui_init()
{
	top = new CTopDesktop();
	bottom = new CBottomDesktop();
	return 0;
}
//---------------------------------------------------------------------------
int gui_destroy()
{
	return 0;
}
//---------------------------------------------------------------------------
extern "C" int widgets_draws()
{
	top->draw(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL));
	bottom->draw(gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL));
	top->IncrementTimers();
	bottom->IncrementTimers();
	return 0;
}
//---------------------------------------------------------------------------
extern "C" int widgets_touch_events(touchPosition *p)
{
	return bottom->onTouchEvent(p);	
}
//---------------------------------------------------------------------------
CTopDesktop::CTopDesktop() : CDesktop(GFX_TOP)
{
	bkcolor = 0xFFd3d8e8;
}
//---------------------------------------------------------------------------
int CTopDesktop::draw(u8 *screen)
{
	if(!isInvalidate())
		gfxGradientFillRect(&sz,0,1,0xFFFFFFFF,bkcolor,screen);
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win)
		(*win)->draw(screen);
	return 0;	
}
//---------------------------------------------------------------------------
CBottomDesktop::CBottomDesktop() : CDesktop(GFX_BOTTOM)
{
	bkcolor = 0xFF3a5795;
}
