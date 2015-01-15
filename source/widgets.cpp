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
CCursor::CCursor(CContainerWindow *w) : CAnimation(500000000)
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
	set_Enabled(1);
	draw();
	return 0;
}
//---------------------------------------------------------------------------
int CCursor::Hide()
{
	set_Enabled(0);
	if(status&4)
		draw();
	return 0;
}
//---------------------------------------------------------------------------
int CCursor::set_Pos(int x, int y)
{
	if(status&4)
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
	status ^= 4;
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
	text_len = 0;
	font = NULL;
	Invalidate();	
}
//---------------------------------------------------------------------------
CBaseWindow::CBaseWindow(gfxScreen_t s)
{
	color = 0xFF000000;
	bkcolor = 0xFFFFFFFF;
	scr = s;
	text = NULL;
	text_len = 0;
	font = NULL;
	Invalidate();
}
//---------------------------------------------------------------------------
CBaseWindow::~CBaseWindow()
{
	destroy();
}
//---------------------------------------------------------------------------
int CBaseWindow::is_invalidate()
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
	if(is_invalidate())
		return -1;
	if((bkcolor >> 24) == 0)
		return -2;
	gfxFillRect(rcWin.left,rcWin.top,rcWin.right,rcWin.bottom,bkcolor,screen);
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::fire_event(const char *key)
{
	if(!key || !key[0] || events.count(key)==0)
		return -1;
	void *v = events[key];
	if(!v)
		return -2;
	return ((int (*)(CBaseWindow *))v)(this);
}
//---------------------------------------------------------------------------
int CBaseWindow::set_Events(char *key,void *value)
{
	std::string s;
	
	if(!key || !key[0])
		return -1;
	s = "";
	for(;*key != 0;key++)
		s += tolower(*key);
	events[s] = value;
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
int CBaseWindow::onTouchEvent(touchPosition *p,u32 flags)
{
	if(!(flags&4))
		return -5;
	status &= ~1;
	if(p->px < rcWin.left)
		return -1;
	if(p->px > rcWin.right)
		return -2;
	if(p->py < rcWin.top)
		return -3;
	if(p->py > rcWin.bottom)
		return -4;
	fire_event("clicked");
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
int CBaseWindow::onKeysUpEvent(u32 press)
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
int CBaseWindow::Invalidate(int flags)
{
	if(flags&1)
		redraw=2;
	else
		redraw += 2;
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
	text_len = 0;
	if(s){
		len = strlen(s);
		text = (char *)malloc(len+1);
		if(text){
			strcpy(text,s);
			text_len = len;
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
		text_len = 0;
	}
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::get_WindowRect(LPRECT prc)
{
	prc->left = rcWin.left;
	prc->top = rcWin.top;
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
			res = res;
	}
	return res;
}
//---------------------------------------------------------------------------
int CContainerWindow::EraseBkgnd(u8 *screen)
{
	return CBaseWindow::draw(screen);
}
//---------------------------------------------------------------------------
int CContainerWindow::Invalidate(int flags)
{
	flags |=1;
	CBaseWindow::Invalidate(flags);
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win)
		(*win)->Invalidate(flags);
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
CBaseWindow * CContainerWindow::get_Window(u32 id)
{
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		if((*win)->get_ID() == id)
			return (*win);
	}
	return NULL;
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
	for (std::vector<CTimer *>::iterator t = timers.begin(); t != timers.end(); ++t)
		(*t)->onCounter();
	return 0;
}
//---------------------------------------------------------------------------
int CDesktop::onTouchEvent(touchPosition *p,u32 flags)
{
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		if(!(*win)->onTouchEvent(p,flags)){
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
	for (std::vector<CTimer *>::iterator t = timers.begin(); t != timers.end(); ++t){
		if((*t) == p)
			return -2;
	}		
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
		SetTimer(cursor);
	}
	cursor->Show(w,x,y);
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::SetCursorPos(int x,int y)
{
	if(cursor == NULL)
		return -1;
	return cursor->set_Pos(x,y);
}
//---------------------------------------------------------------------------	
int CDesktop::HideCursor()
{
	if(cursor != NULL)
		cursor->Hide();
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
int CDesktop::Invalidate(int flags)
{
	int res = CContainerWindow::Invalidate(flags);
	if(dlg_win)
		dlg_win->Invalidate(flags);
	return res;
}
//---------------------------------------------------------------------------
CWindow::CWindow() : CBaseWindow()
{
	color = 0xFF000000;
	bkcolor = 0xFFFFFFFF;
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
			gfxDrawText(screen,font,text,&rcWin,0);
		}
		return 0;
	}
	return -1;
}
//---------------------------------------------------------------------------
CLabel::CLabel(char *c) : CBaseWindow()
{
	bkcolor &= ~0xFF000000;
	color = 0xffffffff;
	set_Text(c);
}
//---------------------------------------------------------------------------
int CLabel::draw(u8 *screen)
{
	if(is_invalidate())
		return -1;
	if(!text || !text[0])
		return -2;	
	gfxSetTextColor(color);
	gfxDrawText(screen,font, text,&rcWin,0);
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
	if(is_invalidate())
		return -1;
	gfxFillRect(rcWin.left,rcWin.top,rcWin.right,rcWin.bottom,0xff606060,screen);
	gfxFillRect(rcWin.left,rcWin.top,rcWin.right-2,rcWin.bottom-2,0xfff0f0f0,screen);
	gfxFillRect(rcWin.left+2,rcWin.top+2,rcWin.right-2,rcWin.bottom-2,bkcolor,screen);	
	if(text){
		gfxSetTextColor(color);
		gfxDrawText(screen,font,text,&rcWin,1);
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
int CStatusBar::EraseBkgnd(u8 *screen)
{
	if(!is_invalidate()){
		gfxGradientFillRect(&rcWin,0,1,0xFF3a5795,bkcolor,screen);
		return 0;
	}
	return -1;
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
	char_pos = 0;
	ptCursor.x = ptCursor.y = 0;
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
		if(text){
			RECT rc;
			
			CopyRect(rc,rcWin);
			InflateRect(rc,2,2);
			gfxDrawText(screen,NULL,&text[char_pos],&rc,2);
		}
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
	ptCursor.x = x;
	ptCursor.y = y;
	d->ShowCursor(this,x,y);
	return 0;
}
//---------------------------------------------------------------------------
int CEditText::onActivate(int v)
{
	CBaseWindow::onActivate(v);
	if(v)
		ShowCursor(rcWin.left+2,rcWin.top+2);
	else
		HideCursor();
	return 0;
}
//---------------------------------------------------------------------------
int CEditText::onCharEvent(u8 c)
{
	char b[10],*p;
	int len,right,dx;
	font_s *f;
	CDesktop *d;
	
	len = text ? text_len : 0;
	p = (char *)malloc(len + 10);
	*((u32 *)p) = 0;
	if(c == BSP){
		if(text_len == 0)
			return 0;
		text_len--;
		text[text_len] = 0;
		Invalidate();
	}
	else{
		if(text != NULL)
			strcpy(p,text);
		b[0] = c;
		b[1] = 0;
		strcat(p,b);
		set_Text(p);
		free(p);
	}
	if(!(f = font))
		f = &fontDefault;
	len = text_len;
	right = (rcWin.right - rcWin.left) - 2;
	p = &text[--len];
	for(dx=2;len >= 0;len--){
		char c = *p--;
		if(c == '\n')
			continue;
		charDesc_s* cd = &f->desc[(int)c];
		if(!cd->data)
			continue;
		dx += cd->xa;
		if(dx >= right){
			dx -= cd->xa;
			break;
		}
	}	
	char_pos = ++len;
	ptCursor.x = rcWin.left+dx;
	d = (CDesktop *)get_Desktop();
	if(d)
		d->SetCursorPos(ptCursor.x,ptCursor.y);
	sprintf(b,"%d %d",len,ptCursor.x);
	svcOutputDebugString(b,strlen(b));
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
	int w,h;
	
	if(is_invalidate() || pImage == NULL)
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