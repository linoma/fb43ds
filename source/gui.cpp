#include "gui.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "widgets.h"

CDesktop *top,*bottom;
//---------------------------------------------------------------------------
int gui_init()
{
	top = new CDesktop(GFX_TOP);
	top->set_BkColor(0xFFFFFFFF);
	bottom = new CDesktop(GFX_BOTTOM);
	bottom->set_BkColor(0xFF0000FF);

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
