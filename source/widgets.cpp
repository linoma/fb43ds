#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <vector>
#include <3ds.h>
#include "widgets.h"
#include "gfxtext.h"
#include "gui.h"
#include "fb.h"
#include "utils.h"

//---------------------------------------------------------------------------
CCursor::CCursor(CContainerWindow *w) : CAnimation(500)
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
	Init();
	Invalidate();	
}
//---------------------------------------------------------------------------
CBaseWindow::CBaseWindow(gfxScreen_t s)
{
	Init();
	scr = s;
	Invalidate();
}
//---------------------------------------------------------------------------
void CBaseWindow::Init()
{
	color = 0xFF000000;
	bkcolor = 0xFFFFFFFF;
	scr = GFX_TOP;
	text = NULL;
	text_len = 0;
	font = NULL;
	status = 0;
	alpha = 255;
	parent = NULL;
}
//---------------------------------------------------------------------------
CBaseWindow::~CBaseWindow()
{
	destroy();
}
//---------------------------------------------------------------------------
u32 CBaseWindow::adjust_AlphaColor(u32 col)
{
	u32 a;
	
	a = (col >> 24);
	col &= 0xFFFFFF;
	a = (a * alpha) >> 8;
	col |= (a << 24);
	return col;
}
//---------------------------------------------------------------------------
int CBaseWindow::set_Alpha(u8 val)
{
	alpha = val;
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::Show()
{
	if((status & 4) == 0)
		return 0;
	status &= ~4;
	CBaseWindow *w = get_Desktop();
	if(w)
		w->Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::Hide()
{
	if(status & 4)
		return 0;
	status |= 4;
	CBaseWindow *w = get_Desktop();
	if(w)
		w->Invalidate();	
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::is_invalidate()
{
	if(!redraw)
		return -1;
	redraw--;
	if(!redraw)
		status &= ~2;
	if(status & 4)
		return -2;
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::draw(u8 *screen)
{
	if(is_invalidate())
		return -1;
	if((bkcolor >> 24) == 0)
		return 0;
	gfxFillRect(&rcWin,bkcolor,screen);
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::has_event(const char *key)
{
	if(!key || !key[0] || events.count(key)==0)
		return -1;
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::fire_event(const char *key,u32 param)
{
	if(has_event(key))
		return -1;
	LPEVENTFUNC v = (LPEVENTFUNC)events[key];
	if(!v)
		return -2;
	return v(this,param);
}
//---------------------------------------------------------------------------
int CBaseWindow::set_Events(char *key,LPEVENTFUNC value)
{
	char *p;
	
	if(!key || !key[0])
		return -1;
	p = (char *)malloc(strlen(key)+1);
	if(!p)
		return -2;
	strcpy(p,key);
	events[strtolower(p)] = (void *)value;
	free(p);
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
CBaseWindow *CBaseWindow::onTouchEvent(touchPosition *p,u32 flags)
{
	if((flags & 4) == 0 || (status&4))
		return 0;
	status &= ~1;
	if(p->px < rcWin.left)
		return 0;
	if(p->px > rcWin.right)
		return 0;
	if(p->py < rcWin.top)
		return 0;
	if(p->py > rcWin.bottom)
		return 0;
	fire_event("clicked");
	//we are inside the widget
	status |= 1;
	return this;
}
//---------------------------------------------------------------------------
CBaseWindow *CBaseWindow::onKeysPressEvent(u32 press,u32 flags)
{
	return 0;
}
//---------------------------------------------------------------------------
CBaseWindow *CBaseWindow::onKeysUpEvent(u32 press,u32 flags)
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
	if(flags & 1)
		redraw = 2;
	else
		redraw += 2;
	redraw=2;
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
		if(len){
			text = (char *)malloc(len+1);
			if(text){
				strcpy(text,s);
				text_len = len;
			}
		}
	}
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::get_Text(char *s,u32 len)
{
	if(!text)
		return 0;
	if(!s || len<text_len)
		return text_len;
	strcpy(s,text);
	return text_len;
}
//---------------------------------------------------------------------------
int CBaseWindow::destroy()
{	
	if(text){
		free(text);
		text = NULL;
		text_len = 0;
	}
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::get_WindowRect(LPRECT prc)
{
	if(!prc)
		return -1;
	prc->left = rcWin.left;
	prc->top = rcWin.top;
	prc->bottom = rcWin.bottom;
	prc->right = rcWin.right;
	return 0;
}
//---------------------------------------------------------------------------
int CBaseWindow::get_ClientRect(LPRECT prc,u32 flags)
{
	int res = get_WindowRect(prc);
	if(res)
		return res;
	if(flags&1)
		return 0;
	prc->right -= prc->left;
	prc->bottom -= prc->top;
	prc->left=prc->top=0;
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
int CContainerWindow::set_Alpha(u8 val)
{
	for(std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win)
		(*win)->set_Alpha(val);
	alpha = val;
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CContainerWindow::draw(u8 *screen)
{
	if(status & 4)
		return -3;
	int res = EraseBkgnd(screen);
	for(std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win)
		(*win)->draw(screen);
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
	int i=0;
	CBaseWindow::Invalidate(flags);
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win,i++)
		(*win)->Invalidate(flags);
	return 0;
}
//---------------------------------------------------------------------------
int CContainerWindow::set_Pos(int x, int y)
{
	RECT rc;
	int xx,yy,x0,y0;
	
	xx = rcWin.left;
	yy = rcWin.top;	
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		(*win)->get_WindowRect(&rc);
		x0=rc.left-xx;
		y0=rc.top-yy;
		(*win)->set_Pos(x+x0,y+y0);
	}
	CBaseWindow::set_Pos(x,y);
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
	recalc_layout();
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CContainerWindow::remove(CBaseWindow *w)
{
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		if((*win) == w){
			wins.erase(win);
			return 0;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
int CContainerWindow::remove(u32 id)
{
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		if((*win)->get_ID() == id){
			wins.erase(win);
			return 0;
		}
	}
	return -1;
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
int CContainerWindow::recalc_layout()
{
	return 0;
}
//---------------------------------------------------------------------------
CBaseWindow *CContainerWindow::onTouchEvent(touchPosition *p,u32 flags)
{
	if(status&4)
		return 0;
	for (std::vector<CBaseWindow *>::reverse_iterator win = wins.rbegin(); win != wins.rend(); ++win){
		if((*win)->onTouchEvent(p,flags))
			return *win;
	}
	return CBaseWindow::onTouchEvent(p,flags);
}
//---------------------------------------------------------------------------	
CBaseWindow *CContainerWindow::onKeysPressEvent(u32 press,u32 flags)
{
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		if(!(*win)->onKeysPressEvent(press,flags)){
			return *win;
		}
	}
	return 0;
}
//---------------------------------------------------------------------------	
CBaseWindow *CContainerWindow::onKeysUpEvent(u32 press,u32 flags)
{
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		if(!(*win)->onKeysUpEvent(press,flags))
			return *win;
	}
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
CBaseWindow *CDesktop::onKeysPressEvent(u32 press,u32 flags)
{
	CBaseWindow *w;
	
	if(a_win){
		if(a_win->onKeysPressEvent(press,flags))
			return a_win;
	}
	if((press & (KEY_A|KEY_B)) == 0)
		return NULL;
	w = CContainerWindow::onKeysPressEvent(press,flags);
	if(w != NULL)
		onActivateWindow(w);
	return w;
}
//---------------------------------------------------------------------------
CBaseWindow *CDesktop::onTouchEvent(touchPosition *p,u32 flags)
{
	for (std::vector<CBaseWindow *>::reverse_iterator win = wins.rbegin(); win != wins.rend(); ++win){
		if((*win)->onTouchEvent(p,flags)){
			onActivateWindow(*win);
			return *win;
		}
	}
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::onActivateWindow(CBaseWindow *win)
{
	if(a_win != win){
		CBaseWindow *w;
		
		w = a_win;
		a_win = win;
		if(w != NULL)
			w->onActivate(0);
		win->onActivate(1);
	}
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::ShowCursor(CBaseWindow *w,int x,int y)
{
	if(cursor == NULL){
		cursor = new CCursor(this);
		if(cursor == NULL)
			return -1;
		fb->SetTimer(cursor);
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
	if(dlg_win != NULL){
		((CDialog *)dlg_win)->Hide();
		dlg_win = NULL;
	}
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::ShowDialog(CDialog *w)
{
	dlg_win = w;
	w->set_Parent(this);
	w->Show();
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
		gfxFillRect(&rcWin,0x60000000,screen);
	dlg_win->draw(screen);
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::Invalidate(int flags)
{
	flags |=1;
	int res = CContainerWindow::Invalidate(flags);
	if(dlg_win)
		dlg_win->Invalidate(flags);
	return res;
}
//---------------------------------------------------------------------------	
int CDesktop::ActiveWindow(CBaseWindow *w)
{
	if(w==NULL)
		return -1;
	if(BringWinTop(w))
		return -2;
	onActivateWindow(w);
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------	
int CDesktop::BringWinTop(CBaseWindow *w)
{
	if(w == wins.back())
		return 0;
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		if((*win) == w){
			wins.erase(win);
			wins.push_back(w);
			Invalidate();
			return 0;
		}
	}
	return -2;
}
//---------------------------------------------------------------------------
CWindow::CWindow() : CContainerWindow()
{
	color = 0x60000000;
	bkcolor = 0xFFf8f8f8;
	gfxGetTextExtent(NULL,"X",&szCaption);
}
//---------------------------------------------------------------------------
CWindow::~CWindow()
{
	destroy();
}
//---------------------------------------------------------------------------
int CWindow::EraseBkgnd(u8 *screen)
{
	RECT rc;
	
	if(is_invalidate())
		return -1;
	gfxFillRoundRect(&rcWin,5,color,bkcolor,screen);
	if(text){
		rc.left = rcWin.left+1;
		rc.right = rcWin.right-1;
		rc.top = rcWin.top + 1;
		rc.bottom = rc.top + szCaption.cy + 1;
		gfxGradientFillRect(&rc,4,3,0xFFdddddd,0xFFb0b0b0,screen);
		gfxSetTextColor(color|0xff000000);
		gfxDrawText(screen,font,text,&rc,DT_VCENTER|DT_CENTER);
	}
	return 0;
}
//---------------------------------------------------------------------------
int CWindow::get_ClientRect(LPRECT prc,u32 flags)
{
	if(!prc)
		return -1;
	prc->left = 2;
	prc->right = rcWin.right-rcWin.left-prc->left*2;
	prc->top = 3;
	if(text)
		prc->top += szCaption.cy;
	prc->bottom = rcWin.bottom-rcWin.top-prc->left-prc->top;
	if(flags&1){
		prc->left += rcWin.left;
		prc->right += rcWin.left;
		prc->top += rcWin.top;
		prc->bottom += rcWin.top;
	}
	return 0;
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
	LPEVENTFUNC pfn;
	int res;
	
	if(is_invalidate())
		return -1;
	if(!text || !text[0])
		return -2;
	res = 1;
	if(!has_event("drawitem")){
		if((pfn = (LPEVENTFUNC)events["drawitem"]) != NULL){
			DRAWITEM dw;
			
			dw.value = text;
			dw.prcItem = &rcWin;
			dw.screen = screen;
			res = pfn(this,(u32)&dw);
		}
	}
	if(res){
		gfxSetTextColor(color);
		gfxDrawText(screen,font,text,&rcWin,0);
	}
	return 0;
}
//---------------------------------------------------------------------------
CButton::CButton(char *c) : CBaseWindow()
{
	bkcolor = 0xFFb0b0b0;
	accel = 0;
	set_Text(c);
}
//---------------------------------------------------------------------------
CButton::CButton() : CBaseWindow()
{
	bkcolor = 0xFFb0b0b0;
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
	gfxGradientFillRect(&rcWin,0,1,0xFFDDDDDD,bkcolor,screen);
	gfxRect(&rcWin,0x80aaaaaa,screen);
	if(text){
		gfxSetTextColor(color);
		gfxDrawText(screen,font,text,&rcWin,DT_VCENTER|DT_CENTER);
	}
	return 0;
}
//---------------------------------------------------------------------------
CBaseWindow *CButton::onKeysPressEvent(u32 press,u32 flags)
{
	if(press & accel){
		status |= 1;
		return this;
	}
	return CBaseWindow::onKeysPressEvent(press,flags);
}
//---------------------------------------------------------------------------
CStatusBar::CStatusBar() : CContainerWindow()
{
	bkcolor = 0xFF2a2a2a;
}
//---------------------------------------------------------------------------
CStatusBar::~CStatusBar()
{
}
//---------------------------------------------------------------------------
int CStatusBar::EraseBkgnd(u8 *screen)
{
	if(!is_invalidate()){
		gfxGradientFillRect(&rcWin,0,1,0xFF4a4a4a,bkcolor,screen);
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
	
	gfxGetTextExtent(font,"X",&sz);
	return CBaseWindow::create(x,y,w,sz.cy+4,id);
}
//---------------------------------------------------------------------------
int CEditText::draw(u8 *screen)
{
	int res = CBaseWindow::draw(screen);
	if(!res){
		gfxRect(&rcWin,0xffa0a0a0,screen);
		if(text){
			RECT rc;
			
			CopyRect(rc,rcWin);
			InflateRect(rc,2,2);
			gfxDrawText(screen,NULL,&text[char_pos],&rc,DT_SINGLELINE|DT_VCENTER);
		}
	}
	return res;
}
//---------------------------------------------------------------------------
CBaseWindow *CEditText::onKeysPressEvent(u32 press,u32 flags)
{	
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
	
	if(is_invalidate())
		return -1;
	CBaseWindow::draw(screen);
	if(!pImage)
		return 0;
	w = rcWin.right-rcWin.left;
	h = rcWin.bottom-rcWin.top;
	return pImage->draw(screen,rcWin.left,rcWin.top,w,h);
}
//---------------------------------------------------------------------------
int CImageWindow::load(CImage *src)
{
	if(!src)
		return -1;
	if(pImage)
		pImage->release();
	pImage = src;
	src->add_ref();
	return 0;
}
//---------------------------------------------------------------------------
int CImageWindow::destroy()
{
	int res = CBaseWindow::destroy();
	if(pImage){
		pImage->release();
		pImage=NULL;
	}
	return res;
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
		pImage->release();
	pImage = p;
	return p->load(src,w,h);
}
//---------------------------------------------------------------------------
CDialog::CDialog() : CContainerWindow()
{
	bkcolor = 0xFFDDDDDD;
}
//---------------------------------------------------------------------------
CDialog::~CDialog()
{
}
//---------------------------------------------------------------------------
CToolBar::CToolBar() : CContainerWindow()
{
	bkcolor = 0xFF4e69a2;
}
//---------------------------------------------------------------------------
CToolBar::~CToolBar()
{
}
//---------------------------------------------------------------------------
int CToolBar::recalc_layout()
{
	RECT rc;
	int x,y;
	
	x = 2;
	y = rcWin.bottom-rcWin.top;
	for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
		(*win)->get_WindowRect(&rc);
		rc.right -= rc.left;
		rc.bottom -= rc.top;		
		(*win)->set_Pos(x,rcWin.top + ((y - rc.bottom) >> 1));
		x += rc.right + 2;		
	}
	return 0;
}
//---------------------------------------------------------------------------
int CToolBar::EraseBkgnd(u8 *screen)
{
	if(is_invalidate())
		return -1;
	gfxGradientFillRect(&rcWin,0,1,0xff3b5998,bkcolor,screen);
//	gfxLine(rcWin.left,rcWin.bottom-1,rcWin.right,rcWin.bottom-1,0x80a0a0a0,screen);
//	gfxLine(rcWin.left,rcWin.bottom,rcWin.right,rcWin.bottom,0x80404040,screen);
	return 0;
}
//---------------------------------------------------------------------------
CBaseWindow *CToolBar::onTouchEvent(touchPosition *p,u32 flags)
{
	for (std::vector<CBaseWindow *>::reverse_iterator win = wins.rbegin(); win != wins.rend(); ++win){
		if((*win)->onTouchEvent(p,flags))
			return *win;
	}
	return 0;
}
//---------------------------------------------------------------------------
CToolButton::CToolButton() : CImageWindow()
{
	bkcolor = 0x00b0b0b0;
}
//---------------------------------------------------------------------------
int CToolButton::load(CImage *img,int idx)
{
	if(CImageWindow::load(img))
		return -1;
	int w = pImage->get_Width();
	int h = pImage->get_Height();
	if(w>h){
		rcImage.left = idx*h;
		rcImage.top = 0;
		rcImage.right = rcImage.left+h;
		rcImage.bottom = rcImage.top+h;
	}
	else{
		rcImage.left = 0;
		rcImage.top = idx*w;
		rcImage.right = rcImage.left+w;
		rcImage.bottom = rcImage.top+w;
	}
	return 0;
}
//---------------------------------------------------------------------------
int CToolButton::draw(u8 *screen)
{
	int x,y,w,h;
	
	if(CBaseWindow::draw(screen))
		return -1;	
	if(!pImage)
		return 0;
	w = rcImage.right-rcImage.left;
	h = rcImage.bottom-rcImage.top;
	x = rcWin.left + (((rcWin.right - rcWin.left) - w) >> 1);
	y = rcWin.top + (((rcWin.bottom - rcWin.top) - h) >> 1);
	return pImage->draw(screen,x,y,w,h,rcImage.left,rcImage.top);
}
//---------------------------------------------------------------------------
CScrollBar::CScrollBar(int m) : CBaseWindow()
{
	bkcolor = 0xff909090;
	status |= m ? 0x100 : 0;
	pos = min = 0;
	max = 0;
	page = 0;
	recalc_layout();
}
//---------------------------------------------------------------------------
int CScrollBar::set_ScrollInfo(u32 mn,u32 mx,u32 pg)
{
	min = mn;
	max = mx;
	page = page;	
	recalc_layout();
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CScrollBar::recalc_layout()
{
	float f,ff;
	RECT rc;
	
	rcThumb.right = rcThumb.bottom = 0;
	f = max - min;
	get_ClientRect(&rc);
	if(status & 0x100)
		ff = rc.right - rc.left;
	else
		ff = rc.bottom - rc.top;
	return 0;
}
//---------------------------------------------------------------------------
int CScrollBar::set_ScrollPos(u32 p)
{
	pos = p;
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CScrollBar::get_ScrollPos(u32 *p)
{
	if(!p)
		return -1;
	*p = pos;
	return 0;
}
//---------------------------------------------------------------------------
int CScrollBar::get_ScrollInfo(u32 *mn,u32 *mx,u32 *pg)
{
	if(mn)
		*mn = min;
	if(mx)
		*mx = max;
	if(pg)
		*pg = page;
	return 0;
}
//---------------------------------------------------------------------------
int CScrollBar::draw(u8 *screen)
{
	float f;
	
	if(is_invalidate())
		return -1;
	gfxFillRoundRect(&rcWin,2,bkcolor,bkcolor,screen);
	if(rcThumb.right || rcThumb.bottom){
		gfxFillRoundRect(&rcThumb,0x20002,0xffb0b0b0,0xffb0b0b0,screen);
	}
	return 0;
}
//---------------------------------------------------------------------------
int CScrollBar::create(u32 x,u32 y,u32 w,u32 h,u32 id)
{
	if(status & 0x100)
		h = 5;
	else
		w = 5;
	return CBaseWindow::create(x,y,w,h,id);
}
//---------------------------------------------------------------------------
CBaseWindow *CScrollBar::onTouchEvent(touchPosition *p,u32 flags)
{
	if(p->px < rcThumb.left)
		goto fail;
	if(p->px > rcThumb.right)
		goto fail;
	if(p->py < rcThumb.top)
		goto fail;
	if(p->py > rcThumb.bottom)
		goto fail;
	fire_event("clicked");
	return this;
fail:	
	if(!CBaseWindow::onTouchEvent(p,flags))
		return 0;
	return this;
}
//---------------------------------------------------------------------------
CListBox::CListBox() : CContainerWindow()
{
	SIZE sz;
	
	first_visible_item=0;
	vbar = NULL;
	gfxGetTextExtent(font,"X",&sz);
	item_height = sz.cy;	
}
//---------------------------------------------------------------------------
int CListBox::create(u32 x,u32 y,u32 w,u32 h,u32 id)
{
	RECT rc;
	
	CContainerWindow::create(x,y,w,h,id);
	vbar = new CScrollBar();
	get_ClientRect(&rc);
	vbar->create(rc.right-6,rc.top,0,rc.bottom,1);
	vbar->set_Events("clicked",CListBox::onScroll);
	set_Events("clicked",CListBox::onClicked);
	return add(vbar);	
}
//---------------------------------------------------------------------------
int CListBox::onScroll(CBaseWindow *sb,u32 param)
{
	return ((CListBox *)sb)->onScroll(param);
}
//---------------------------------------------------------------------------
int CListBox::onClicked(CBaseWindow *sb,u32 param)
{
	return ((CListBox *)sb)->onClicked(param);
}
//---------------------------------------------------------------------------
int CListBox::onClicked(u32 param)
{
	POINT pt;
	u32 item,i;
	
	getCursorPos(&pt);
	pt.x -= rcWin.left;
	pt.y -= rcWin.top;
	i=0;
	item = first_visible_item + (pt.y / item_height);
	for (std::vector<std::string>::iterator t = items.begin(); t != items.end(); ++t,i++){
		if(i < first_visible_item)
			continue;
		if(i == item){
			fire_event("clickitem",(u32)(*t).c_str());
			break;
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
int CListBox::onScroll(u32 param)
{
	return 0;
}	
//---------------------------------------------------------------------------
int CListBox::remove_item(u32 idx)
{
	if(idx == 0xffffffff)
		items.clear();
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CListBox::add_item(char *val)
{
	if(!val || !*val)
		return -1;
	items.push_back(val);
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CListBox::set_ItemHeight(u32 val)
{
	item_height=val;
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CListBox::draw(u8 *screen)
{
	DRAWITEM dwItem;
	int y;
	RECT rc,rcItem;
	u32 i,res;	
	LPEVENTFUNC pfn;
	
	if(CContainerWindow::draw(screen))
		return -1;
	get_ClientRect(&rc,1);
	rc.right -= 6;
	y = rc.top;
	CopyRect(rcItem,rc);
	i = 0;
	pfn = NULL;
	if(!has_event("drawitem")){
		if((pfn = (LPEVENTFUNC)events["drawitem"]) != NULL){
			dwItem.screen = screen;
			dwItem.prcItem = &rcItem;
		}
	}	
	gfxSetTextColor(color);
	for (std::vector<std::string>::iterator t = items.begin(); t != items.end(); ++t,i++){
		if(i < first_visible_item)
			continue;
		rcItem.top = y;
		rcItem.bottom = y + item_height;
		if(rcItem.bottom >= rc.bottom)
			break;
		res = -1;
		if(pfn){
			dwItem.index = i;
			dwItem.value = (char *)(*t).c_str();
			res = pfn(this,(u32)&dwItem);
		}
		if(res<0){
			gfxDrawText(screen,NULL,(char *)(*t).c_str(),&rcItem,DT_VCENTER);
			res = item_height;
		}
		y += res + 1;
	}
	return 0;
}
//---------------------------------------------------------------------------
CEditBox::CEditBox() : CContainerWindow()
{
}
//---------------------------------------------------------------------------
int CEditBox::create(u32 x,u32 y,u32 w,u32 h,u32 id)
{
	RECT rc;
	
	CContainerWindow::create(x,y,w,h,id);
	vbar = new CScrollBar();
	get_ClientRect(&rc);
	vbar->create(rc.right-7,rc.top+2,0,rc.bottom-4,1);
	//vbar->set_Events("clicked",CListBox::onScroll);
	//set_Events("clicked",CListBox::onClicked);
	return add(vbar);	
}
//---------------------------------------------------------------------------
int CEditBox::EraseBkgnd(u8 *screen)
{
	if(is_invalidate())
		return -1;
	gfxFillRoundRect(&rcWin,3,color,bkcolor,screen);
	return 0;
}
//---------------------------------------------------------------------------
int CEditBox::onActivate(int v)
{
	CDesktop *d;
	
	d = (CDesktop *)get_Desktop();
	CContainerWindow::onActivate(v);
	if(v){
		ptCursor.x = rcWin.left+3;
		ptCursor.y = rcWin.top+3;
		d->ShowCursor(this,ptCursor.x,ptCursor.y);
	}
	else
		d->HideCursor();
	return 0;
}