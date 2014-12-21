#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <vector>
#include <3ds.h>
#include "widgets.h"
#include "gfxdraw.h"
#include "gfxtext.h"
#include "gui.h"

using namespace std;

//---------------------------------------------------------------------------
CTimer::CTimer(LPDEFFUNC f,u64 i,u32 p)
{
	elapsed = svcGetSystemTick();
	interval = i;
	fnc = f;
	param=p;
	status=1;
}
//---------------------------------------------------------------------------
int CTimer::onCounter()
{
	if((status & 1) == 0)
		return -1;
	elapsed += svcGetSystemTick();
	if(elapsed >= interval){
		while(elapsed >=interval)
			elapsed -= interval;
		if(fnc != NULL)
			fnc(param);
	}
	return 0;
}
//---------------------------------------------------------------------------
int CTimer::set_Param(u32 p)
{
	param=p;
	return 0;
}
//---------------------------------------------------------------------------
int CTimer::set_Enabled(int v)
{
	if(v)
		status |= 1;
	else
		status &= ~1;
	return 0;
}
//---------------------------------------------------------------------------
CCursor::CCursor(CContainerWindow *w) : CTimer(onTimer,200000,(u32)this)
{
	desk = w;
	win = NULL;
	status &= ~5;
	gfxGetTextExtent(NULL,"X",&sz);
}
//---------------------------------------------------------------------------
int CCursor::Show(CBaseWindow *v,int x,int y)
{
	win = v;
	pos.x = x;
	pos.y = y;
	status |= 1|4;
	draw();
	return 0;
}
//---------------------------------------------------------------------------
int CCursor::Hide()
{
	status &= ~5;
	draw();
	return 0;
}
//---------------------------------------------------------------------------
int CCursor::set_Pos(int x, int y)
{
	status &= ~4;
	draw();
	pos.x = x;
	pos.y = y;
	return 0;
}
//---------------------------------------------------------------------------
int CCursor::draw()
{
	CDesktop *s;
	u8 *b;
	int y;
	
	s = ((CDesktop *)desk);
	if(s == NULL)
		return -1;
	b = s->get_Buffer();
	if(b == NULL)
		return -2;
	b += (240-pos.y + pos.x*240)*3;
	for(y=0;y<sz.cy;y++,b -= 3){
		b[0] ^= 0xFF;
		b[1] ^= 0xFF;
		b[2] ^= 0xFF;
	}
	return 0;
}
//---------------------------------------------------------------------------
int CCursor::onTimer()
{
	status ^= 4;
	draw();
	return 0;
}
//---------------------------------------------------------------------------
int CCursor::onTimer(u32 param)
{
	return ((CCursor *)param)->onTimer();
}
//---------------------------------------------------------------------------
CBaseWindow::CBaseWindow()
{
	bkcolor = 0xFF000000;
	color = 0xFFFFFFFF;
	scr=GFX_TOP;
	Invalidate();	
}
//---------------------------------------------------------------------------
CBaseWindow::CBaseWindow(gfxScreen_t s)
{
	bkcolor = 0xFF000000;
	color = 0xFFFFFFFF;
	scr = s;
	Invalidate();
}
//---------------------------------------------------------------------------
CBaseWindow::~CBaseWindow()
{
}
//---------------------------------------------------------------------------
int CBaseWindow::draw(u8 *screen)
{
	u8 r,g,b;
	
	if(!redraw)
		return -1;
	redraw--;
	if(!redraw)
		status &= ~2;
	if((bkcolor >> 24) == 0)
		return 0;
	r = (u8)(bkcolor >> 16);
	g = (u8)(bkcolor >> 8);
	b = (u8)bkcolor;
	gfxFillRect(sz.left,sz.top,sz.right,sz.bottom,r,g,b,screen);
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::set_BkColor(u32 c)
{
	bkcolor = c;
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::set_TextColor(u32 c)
{
	color = c;
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::onTouchEvent(touchPosition *p)
{
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow:: onKeysPressEvent(u32 press)
{
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow:: onKeysUpEvent(u32 press)
{
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::onActivate(int v)
{
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::Invalidate()
{
	redraw = 2;
	status |= 2;		
	return 0;
}
//---------------------------------------------------------------------------
CBaseWindow *CBaseWindow::get_Desktop()
{
	CBaseWindow *w,*w1;
	
	w = parent;
	w1 = NULL;
	while(w){
		w1 = w;
		w = w1->get_Parent();
	}
	return w1;
}
//---------------------------------------------------------------------------
int CBaseWindow::create(u32 x,u32 y,u32 w,u32 h,u32 id)
{
	sz.left = x;
	sz.top = y;
	sz.right = x+w;
	sz.bottom = y+h;
	ID = id;
	return 0;
}
//---------------------------------------------------------------------------
CContainerWindow::CContainerWindow() : CBaseWindow()
{
}
//---------------------------------------------------------------------------
CContainerWindow::CContainerWindow(gfxScreen_t s) : CBaseWindow(s)
{
}
//---------------------------------------------------------------------------
int CContainerWindow::draw(u8 *screen)
{
	CBaseWindow::draw(screen);
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win)
		(*win)->draw(screen);
	return 0;
}
//---------------------------------------------------------------------------
int CContainerWindow::Invalidate()
{
	CBaseWindow::Invalidate();
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win)
		(*win)->Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CContainerWindow::add(CBaseWindow *w)
{
	wins.push_back(w);
	w->set_Parent(this);
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
CDesktop::CDesktop(gfxScreen_t s) : CContainerWindow(s)
{
	u16 w,h;
	
	bkcolor = 0xFF000000;
	gfxGetFramebuffer(s,GFX_LEFT,(u16 *)&w,(u16 *)&h);
	sz.left = sz.top = 0;
	sz.right = h-1;
	sz.bottom = w - 1;
	a_win = NULL;
}
//---------------------------------------------------------------------------
u8 *CDesktop::get_Buffer()
{
	return gfxGetFramebuffer(scr,GFX_LEFT,NULL,NULL);
}
//---------------------------------------------------------------------------
int CDesktop::IncrementTimers()
{
	for (std::vector<CTimer *>::iterator t = timers.begin(); t != timers.end(); ++t){
		(*t)->onCounter();
	}
	return 0;
}
//---------------------------------------------------------------------------
int CDesktop::onTouchEvent(touchPosition *p)
{
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		if(!(*win)->onTouchEvent(p)){
			onActivateWindow(*win);
			return 0;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------	
int CDesktop::onActivateWindow(CBaseWindow *win)
{
	if(a_win != NULL)
		a_win->onActivate(0);
	a_win = win;
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::onKeysPressEvent(u32 press)
{
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		if(!(*win)->onKeysPressEvent(press)){
			onActivateWindow(*win);
			return 0;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------	
int CDesktop::onKeysUpEvent(u32 press)
{
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		if(!(*win)->onKeysUpEvent(press))
			return 0;
	}
	return -1;
}
//---------------------------------------------------------------------------	
int CDesktop::SetTimer(LPDEFFUNC f,u64 val,u32 p)
{
	CTimer *t = new CTimer(f,val,p);
	timers.push_back(t);
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::ShowCursor(CBaseWindow *w,int x,int y)
{
	if(cursor == NULL){
		cursor = new CCursor(this);
		if(cursor == NULL)
			return -1;
		timers.push_back(cursor);
	}
	cursor->Show(w,x,y);
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::HideCursor()
{
	return 0;
}
//---------------------------------------------------------------------------
CWindow::CWindow() : CBaseWindow()
{
	color = 0xFF000000;
	bkcolor = 0xFFFFFFFF;
	text = NULL;
	cb = NULL;	
}
//---------------------------------------------------------------------------
CWindow::~CWindow()
{
	destroy();
}
//---------------------------------------------------------------------------
int CWindow::destroy()
{
	if(text){
		free(text);
		text = NULL;
	}
	return 0;
}
//---------------------------------------------------------------------------
int CWindow::draw(u8 *screen)
{
	CBaseWindow::draw(screen);
	return 0;
}
//---------------------------------------------------------------------------
int CWindow::onTouchEvent(touchPosition *p)
{
	u32 s = status;
		
	status &= ~1;
	if(p->px < sz.left)
		return -1;
	if(p->px > sz.right)
		return -2;
	if(p->py < sz.top)
		return -3;
	if(p->py > sz.bottom)
		return -4;
	//we are inside the widget
	status |= 1;
	if((s & 1) == 0)
		onActivate(1);
	if(cb != NULL)
		cb(this);
	return 0;
}
//---------------------------------------------------------------------------
int CWindow::set_Text(char *s)
{
	if(text != NULL)
		free(text);
	text=NULL;
	if(s){
		text = (char *)malloc(strlen(s)+1);
		if(text == NULL)
			return -1;
		text[0]=0;
		strcpy(text,s);
	}
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
CLabel::CLabel() : CWindow()
{
	bkcolor &= ~0xFF000000;
}
//---------------------------------------------------------------------------
int CLabel::draw(u8 *screen)
{
	CWindow::draw(screen);
	return 0;
}
//---------------------------------------------------------------------------
CButton::CButton() : CWindow()
{
	bkcolor = 0xFFDDDDDD;
	accel = 0;
}
//---------------------------------------------------------------------------
CButton::~CButton()
{
}
//---------------------------------------------------------------------------
int CButton::draw(u8 *screen)
{
	CBaseWindow::draw(screen);	
	return 0;
}
//---------------------------------------------------------------------------
int CButton::onKeysPressEvent(u32 press)
{
	if(press & accel){
		status |= 1;
		return 0;
	}
	return CWindow::onKeysPressEvent(press);
}
//---------------------------------------------------------------------------
CStatusBar::CStatusBar() : CContainerWindow()
{
	bkcolor = 0xFF0000FF;
}
//---------------------------------------------------------------------------
CStatusBar::~CStatusBar()
{
}
//---------------------------------------------------------------------------
int CStatusBar::draw(u8 *screen)
{
	CBaseWindow::draw(screen);
	return 0;
}
//---------------------------------------------------------------------------
CMenuBar::CMenuBar() : CContainerWindow()
{
	bkcolor = 0xFF0000FF;
}
//---------------------------------------------------------------------------
int CMenuBar::draw(u8 *screen)
{
	return CBaseWindow::draw(screen);
}
//---------------------------------------------------------------------------
CEditText::CEditText() : CWindow()
{
}
//---------------------------------------------------------------------------
CEditText::~CEditText()
{
}
//---------------------------------------------------------------------------
int CEditText::create(u32 x,u32 y,u32 w,u32 h,u32 id)
{
	SIZE sz;
	
	gfxGetTextExtent(NULL,"X",&sz);
	return CWindow::create(x,y,w,sz.cy+4,id);
}
//---------------------------------------------------------------------------
int CEditText::draw(u8 *screen)
{
	CBaseWindow::draw(screen);
	gfxRect(sz.left,sz.top,sz.right,sz.bottom,0xa0,0xa0,0xa0,screen);
	return 0;
}
//---------------------------------------------------------------------------
int CEditText::HideCursor()
{
	CDesktop *d;
	
	d = (CDesktop *)get_Desktop();
	d->HideCursor();
	return 0;
}
//---------------------------------------------------------------------------
int CEditText::ShowCursor(int x,int y)
{
	CDesktop *d;
	
	d = (CDesktop *)get_Desktop();
	d->ShowCursor(this,x,y);
	return 0;
}
//---------------------------------------------------------------------------
int CEditText::onActivate(int v)
{
	CWindow::onActivate(v);
	if(v){
		ShowCursor(sz.left+10,sz.top);
	}
	else{
		HideCursor();
	}
	return 0;
}
