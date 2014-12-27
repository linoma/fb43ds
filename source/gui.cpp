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

	keyboard = new CKeyboard();
	keyboard->init(bottom);
	
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
	c->create(0,0,sz.right,20,2);	
	add(c);
	CWindow *w = new CLabel("fb43ds");
	w->create(0,0,50,10,3);
	c->add(w);
}
//---------------------------------------------------------------------------
int CTopDesktop::draw(u8 *screen)
{
	CBaseWindow::draw(screen);
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win)
		(*win)->draw(screen);
	return 0;	
}
//---------------------------------------------------------------------------
int CBottomDesktop::draw(u8 *screen)
{
	int i = 0;
	if(!isInvalidate()){
		gfxGradientFillRect(&sz,0,1,0xFFFFFFFF,bkcolor,screen);
		i = 1;
	}
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		if(!(*win)->draw(screen))
			i = 1;
	}
	if(i)
		keyboard->draw(screen);
	return 0;	
}
//---------------------------------------------------------------------------
CBottomDesktop::CBottomDesktop() : CDesktop(GFX_BOTTOM)
{
	//bkcolor = 0xFF3a5795;
	bkcolor=0xFFd3d8e8;
	CContainerWindow *c = new CStatusBar();	
	c->create(0,sz.bottom-14,sz.right,14,2);	
	add(c);
}
