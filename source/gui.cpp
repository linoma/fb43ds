#include "gui.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "widgets.h"
#include "gfxdraw.h"
#include "gfxtext.h"

CDesktop *top,*bottom;
//---------------------------------------------------------------------------
int gui_init()
{
	top = new CTopDesktop();
	bottom = new CBottomDesktop();
	top->ShowDialog(bottom);
	bottom->ShowDialog(bottom);
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
	bkcolor = 0xFFFFFFFF;//0xFFd3d8e8
	CContainerWindow *c = new CStatusBar();	
	c->create(0,0,rcWin.right,20,2);	
	add(c);
	CBaseWindow *w = new CLabel("fb43ds");
	w->create(2,2,50,10,3);
	c->add(w);
}
//---------------------------------------------------------------------------
int CBottomDesktop::EraseBkgnd(u8 *screen)
{
	if(!isInvalidate()){
		gfxGradientFillRect(&rcWin,0,1,0xFFFFFFFF,bkcolor,screen);
		return 0;
	}
	return -1;
}
//---------------------------------------------------------------------------
int CBottomDesktop::draw(u8 *screen)
{
	int i = CDesktop::draw(screen);
	if(!i && keyboard)
		keyboard->draw(screen);
	return i;	
}
//---------------------------------------------------------------------------
CBottomDesktop::CBottomDesktop() : CDesktop(GFX_BOTTOM)
{
	//bkcolor = 0xFF3a5795;
	bkcolor=0xFFd3d8e8;
	CContainerWindow *c = new CStatusBar();	
	c->create(0,rcWin.bottom-14,rcWin.right,14,2);	
	add(c);
}
//---------------------------------------------------------------------------
int CBottomDesktop::onTouchEvent(touchPosition *p)
{
	if(keyboard && !keyboard->onTouchEvent(p))
		return 0;
	return CDesktop::onTouchEvent(p);
}