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

//---------------------------------------------------------------------------
CCursor::CCursor(CContainerWindow *w) : CAnimation(200000000)
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
	b += (239-pos.y + pos.x*240)*3;
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
CBaseWindow::CBaseWindow()
{
	color = 0xFF000000;
	bkcolor = 0xFFFFFFFF;
	scr=GFX_TOP;
	text = NULL;
	font=NULL;
	Invalidate();	
}
//---------------------------------------------------------------------------
CBaseWindow::CBaseWindow(gfxScreen_t s)
{
	color = 0xFF000000;
	bkcolor = 0xFFFFFFFF;
	scr = s;
	text = NULL;
	font = NULL;
	Invalidate();
}
//---------------------------------------------------------------------------
CBaseWindow::~CBaseWindow()
{
	destroy();
}
//---------------------------------------------------------------------------
int CBaseWindow::isInvalidate()
{
	if(!redraw)
		return -1;
	redraw--;
	if(!redraw)
		status &= ~2;
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::draw(u8 *screen)
{
	if(isInvalidate())
		return -1;
	if((bkcolor >> 24) == 0)
		return -2;
	gfxFillRect(rcWin.left,rcWin.top,rcWin.right,rcWin.bottom,bkcolor,screen);
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
	status &= ~1;
	if(p->px < rcWin.left)
		return -1;
	if(p->px > rcWin.right)
		return -2;
	if(p->py < rcWin.top)
		return -3;
	if(p->py > rcWin.bottom)
		return -4;
	//we are inside the widget
	status |= 1;
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow:: onKeysPressEvent(u32 press)
{
	return -1;
}
//---------------------------------------------------------------------------
int CBaseWindow:: onKeysUpEvent(u32 press)
{
	return -1;
}
//---------------------------------------------------------------------------
int CBaseWindow::onActivate(int v)
{
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::set_Pos(int x,int y)
{
	rcWin.right -= rcWin.left;
	rcWin.bottom -= rcWin.top;
	rcWin.left = x;
	rcWin.top = y;
	rcWin.right += x;
	rcWin.bottom += y;
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::Invalidate()
{
	redraw = 3;
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
	rcWin.left = x;
	rcWin.top = y;
	rcWin.right = x+w;
	rcWin.bottom = y+h;
	ID = id;
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::onCharEvent(u8 c)
{
	return -1;
}
//---------------------------------------------------------------------------
int CBaseWindow::set_Text(char *s)
{
	int len;
	
	if(text)
		free(text);
	text = NULL;
	if(s){
		len = strlen(s);
		text = (char *)malloc(len+1);
		if(text){
			memset(text,0,len+1);
			strcpy(text,s);
		}
	}
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::destroy()
{	
	if(text){
		free(text);
		text=NULL;
	}
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::get_WindowRect(LPRECT prc)
{
	prc->left = rcWin.left;
	prc->top= rcWin.top;
	prc->bottom = rcWin.bottom;
	prc->right = rcWin.right;
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
	int res = EraseBkgnd(screen);
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		if(!(*win)->draw(screen))
			res=0;
	}
	return res;
}
//---------------------------------------------------------------------------
int CContainerWindow::EraseBkgnd(u8 *screen)
{
	return CBaseWindow::draw(screen);
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
	RECT rc;
	
	if(!w)
		return -1;
	w->get_WindowRect(&rc);
	wins.push_back(w);
	w->set_Parent(this);
	w->set_Pos(rcWin.left+rc.left,rcWin.top+rc.top);
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
CDesktop::CDesktop(gfxScreen_t s) : CContainerWindow(s)
{
	u16 w,h;
	
	bkcolor = 0xFF000000;
	gfxGetFramebuffer(s,GFX_LEFT,(u16 *)&w,(u16 *)&h);
	rcWin.left = rcWin.top = 0;
	rcWin.right = h-1;
	rcWin.bottom = w - 1;
	a_win = dlg_win = NULL;
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
	if(a_win != win){
		if(a_win != NULL)
			a_win->onActivate(0);
		a_win = win;
		win->onActivate(1);
	}
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
int CDesktop::SetTimer(CTimer *p)
{
	if(p == NULL)
		return -1;
	timers.push_back(p);
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::SetTimer(LPDEFFUNC f,u64 val,u32 p)
{
	CTimer *t = new CTimer(f,val,p);	
	return SetTimer(t);
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
	keyboard->show(w);
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::HideCursor()
{
	if(cursor != NULL)
		cursor->Hide();
	if(keyboard != NULL)
		keyboard->hide();
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::HideDialog()
{
	dlg_win = NULL;
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::ShowDialog(CBaseWindow *w)
{
	dlg_win = w;
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::draw(u8 *screen)
{
	int res = CContainerWindow::draw(screen);
	if(dlg_win == NULL)
		return res;
	if(!res)
		gfxFillRect(rcWin.left,rcWin.top,rcWin.right,rcWin.bottom,0x80000000,screen);
	dlg_win->draw(screen);
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::Invalidate()
{
	int res = CContainerWindow::Invalidate();
	if(dlg_win)
		dlg_win->Invalidate();
	return res;
}
//---------------------------------------------------------------------------
CWindow::CWindow() : CBaseWindow()
{
	color = 0xFF000000;
	bkcolor = 0xFFFFFFFF;
	cb = NULL;	
}
//---------------------------------------------------------------------------
CWindow::~CWindow()
{
	destroy();
}
//---------------------------------------------------------------------------
int CWindow::draw(u8 *screen)
{
	if(!CBaseWindow::draw(screen)){
		if(text){
			gfxSetTextColor(color);
			gfxDrawText(screen,0,text,&rcWin,0);
		}
		return 0;
	}
	return -1;
}
//---------------------------------------------------------------------------
CLabel::CLabel(char *c) : CBaseWindow()
{
	bkcolor &= ~0xFF000000;
	color=0xffffffff;
	set_Text(c);
}
//---------------------------------------------------------------------------
int CLabel::draw(u8 *screen)
{
	if(isInvalidate())
		return -1;
	if(!text || !text[0])
		return -2;	
	gfxSetTextColor(color);
	gfxDrawText(screen,NULL, text,&rcWin,0);
	return 0;
}
//---------------------------------------------------------------------------
CButton::CButton(char *c) : CBaseWindow()
{
	bkcolor = 0xFFDDDDDD;
	accel = 0;
	set_Text(c);
}
//---------------------------------------------------------------------------
CButton::CButton() : CBaseWindow()
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
	if(isInvalidate())
		return -1;
	gfxFillRect(rcWin.left,rcWin.top,rcWin.right,rcWin.bottom,0xff606060,screen);
	gfxFillRect(rcWin.left,rcWin.top,rcWin.right-2,rcWin.bottom-2,0xfff0f0f0,screen);
	gfxFillRect(rcWin.left+2,rcWin.top+2,rcWin.right-2,rcWin.bottom-2,bkcolor,screen);	
	if(text){
		gfxSetTextColor(color);
		gfxDrawText(screen,NULL,text,&rcWin,1);
	}
	return 0;
}
//---------------------------------------------------------------------------
int CButton::onKeysPressEvent(u32 press)
{
	if(press & accel){
		status |= 1;
		return 0;
	}
	return CBaseWindow::onKeysPressEvent(press);
}
//---------------------------------------------------------------------------
CStatusBar::CStatusBar() : CContainerWindow()
{
	bkcolor = 0xFF0000a0;
}
//---------------------------------------------------------------------------
CStatusBar::~CStatusBar()
{
}
//---------------------------------------------------------------------------
int CStatusBar::draw(u8 *screen)
{
	int res = -1;
	if(!isInvalidate()){
		gfxGradientFillRect(&rcWin,0,1,0xFF3a5795,bkcolor,screen);
		res = 0;
	}
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win)
		(*win)->draw(screen);
	return res;
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
CEditText::CEditText() : CBaseWindow()
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
	return CBaseWindow::create(x,y,w,sz.cy+4,id);
}
//---------------------------------------------------------------------------
int CEditText::draw(u8 *screen)
{
	int res = CBaseWindow::draw(screen);
	if(!res){
		gfxRect(rcWin.left,rcWin.top,rcWin.right,rcWin.bottom,0xffa0a0a0,screen);
		if(text)
			gfxDrawText(screen,NULL,text,&rcWin,0);
	}
	return res;
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
	CBaseWindow::onActivate(v);
	if(v)
		ShowCursor(rcWin.left+10,rcWin.top+2);
	else
		HideCursor();
	return 0;
}
//---------------------------------------------------------------------------
int CEditText::onCharEvent(u8 c)
{
	char b[2],*s;
	int len;
	
	len = text ? strlen(text) : 0;
	s = (char *)malloc(len + 10);
	memset(s,0,len+10);
	if(text != NULL)
		strcpy(s,text);
	b[0] = c;
	b[1] = 0;
	strcat(s,b);
	set_Text(s);
	free(s);
	return 0;
}
//---------------------------------------------------------------------------
CImageWindow::CImageWindow() : CBaseWindow()
{
	pImage = NULL;
	bkcolor &= ~0xFF000000;
}
//---------------------------------------------------------------------------
int CImageWindow::draw(u8 *screen)
{
	int x,y,w,h;
	
	if(isInvalidate() || pImage == NULL)
		return -1;
	CBaseWindow::draw(screen);
	w = rcWin.right-rcWin.left;
	h = rcWin.bottom-rcWin.top;
	return pImage->draw(screen,rcWin.left,rcWin.top,w,h);
}
//---------------------------------------------------------------------------
int CImageWindow::load(u8 *src,int w,int h)
{
	CImage *p;
	
	if(!src)
		return -1;
	p = NULL;
	if(src[0] == 0x47 && src[1] == 0x49 && src[2] == 0x46 && src[3] == 0x38)
		p = new CImageGif();
	if(!p)
		return -2;
	if(pImage)
		delete pImage;
	pImage = p;
	return p->load(src,w,h);
}