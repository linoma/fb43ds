#include "gui.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include "widgets.h"
#include "gfxdraw.h"
#include "gfxtext.h"
#include "loader_bin.h"
#include "fb4_logo_bin.h"

CDesktop *top,*bottom;
CConsoleWindow *console;
//---------------------------------------------------------------------------
int gui_init()
{
	top = new CTopDesktop();
	bottom = new CBottomDesktop();
	console = new CConsoleWindow();
	console->create(20,20,280,200,-1);	
	bottom->ShowDialog(console);
	return 0;
}
//---------------------------------------------------------------------------
int gui_destroy()
{
	return 0;
}
//---------------------------------------------------------------------------
int widgets_draws()
{
	u8 *fb;
	u16 w,h;
	
	fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &w, &h);
	top->draw(fb);
	fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &w, &h);
	bottom->draw(fb);
	
	top->IncrementTimers();
	bottom->IncrementTimers();
	
	return 0;
}
//---------------------------------------------------------------------------
CTopDesktop::CTopDesktop() : CDesktop(GFX_TOP)
{
	bkcolor = 0xFFFFFFFF;//0xFFd3d8e8
	
	logo=new CImageGif();
	
	CContainerWindow *c = new CStatusBar();	
	c->create(0,0,rcWin.right,20,2);	
	add(c);
	
	CBaseWindow *w = new CLabel("fb43ds");
	w->create(2,2,50,10,3);
	c->add(w);

	/*CClock *p = new CClock();
	p->create(rcWin.right-50,2,50,10,4);
	c->add(p);
	
	SetTimer((CTimer *)p);
	p->Start();*/
	
	loader = new CLoaderWindow();
	loader->create(194,104,32,32,-1);
}
//---------------------------------------------------------------------------
CTopDesktop::~CTopDesktop()
{
	if(logo != NULL)
		delete logo;
	logo = NULL;
}
//---------------------------------------------------------------------------
int CTopDesktop::init()
{
	loader->load((u8 *)loader_bin);
	show_loader();
	logo->load((u8 *)fb4_logo_bin);
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CTopDesktop::EraseBkgnd(u8 *screen)
{
	if(!CDesktop::EraseBkgnd(screen)){
		if(logo != NULL)
			logo->draw(screen,10,30);
		return 0;
	}
	return -1;
}
//---------------------------------------------------------------------------
int CTopDesktop::show_loader()
{
	SetTimer(loader);
	loader->Start();
	return ShowDialog(loader);
}
//---------------------------------------------------------------------------
int CBottomDesktop::EraseBkgnd(u8 *screen)
{
	if(!is_invalidate()){
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
	keyboard = new CKeyboard();
	CContainerWindow *c = new CStatusBar();	
	c->create(0,rcWin.bottom-20,rcWin.right,20,2);	
	add(c);
}
//---------------------------------------------------------------------------
int CBottomDesktop::ShowCursor(CBaseWindow *w,int x,int y)
{
	if(!CDesktop::ShowCursor(w,x,y)){
		keyboard->show(w);
		return 0;
	}
	return -1;
}
//---------------------------------------------------------------------------
int CBottomDesktop::HideCursor()
{
	if(!CDesktop::HideCursor()){
		if(keyboard != NULL)
			keyboard->hide();
		return 0;
	}
	return -1;
}
//---------------------------------------------------------------------------
int CBottomDesktop::onTouchEvent(touchPosition *p,u32 flags)
{
	if(keyboard && !keyboard->onTouchEvent(p,flags))
		return 0;
	return CDesktop::onTouchEvent(p,flags);
}
//---------------------------------------------------------------------------
int CBottomDesktop::init()
{
	keyboard->init(bottom);
	return 0;
}
//---------------------------------------------------------------------------
CConsoleWindow::CConsoleWindow() : CWindow()
{
	bkcolor = 0xff000000;
	color = 0xffffffff;
}
//---------------------------------------------------------------------------
int CConsoleWindow::set_Text(char *s)
{
	int res,len,dy,top,right,dx;	
	font_s *f;
	char *p;
	
	res = CWindow::set_Text(s);
	if(!text || !text_len)
		return res;
	if(!(f = font))
		f = &fontDefault;
	len = text_len;
	dy = rcWin.bottom;
	top = rcWin.top + f->height;
	right = rcWin.right - rcWin.left;

	p = &text[--len];
	for(dx=0;len >= 0 && dy >= top;len--){
		char c = *p--;
		if(c == '\n'){
			dy -= f->height;
			continue;
		}
		charDesc_s* cd = &f->desc[(int)c];
		if(!cd->data)
			continue;
		dx += cd->xa;
		if(dx > right){
			dx = cd->xa;
			dy -= f->height;
		}
	}
	return res;	
}
//---------------------------------------------------------------------------
int CConsoleWindow::printf(char *fmt,...)
{
	va_list argptr;
	int len;
	char *s;

	len = text ? text_len : 0;
	s = (char *)malloc(len + 1024);
	if(!s)
		return -1;
	memset(s,0,len+1024);
	if(text)
		strcpy(s,text);
	va_start(argptr, fmt);
	vsprintf(&s[len],fmt, argptr);
	va_end(argptr);
	set_Text(s);
	free(s);
	return 0;
}
//---------------------------------------------------------------------------
CLoaderWindow::CLoaderWindow() : CImageWindow(),CAnimation(400000000)
{
}
//---------------------------------------------------------------------------
int CLoaderWindow::onTimer()
{
	frame++;
	if(pImage != NULL){
		if(frame*32 >= pImage->get_Height())
			frame = 0;
		Invalidate();
	}
	return 0;
}
//---------------------------------------------------------------------------
int CLoaderWindow::Start()
{
	CAnimation::Start();
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CLoaderWindow::Stop()
{
	CAnimation::Stop();
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CLoaderWindow::draw(u8 *screen)
{
	int w,h;
	
	if(is_invalidate() || pImage == NULL)
		return -1;
	CBaseWindow::draw(screen);
	w = rcWin.right - rcWin.left;
	h = rcWin.bottom - rcWin.top;
	return pImage->draw(screen,rcWin.left,rcWin.top,w,h,0,frame*32);
}
//---------------------------------------------------------------------------
CClock::CClock() : CLabel("clock"),CAnimation(900000000)
{
}
//---------------------------------------------------------------------------
int CClock::onTimer()
{
	char c[20];
	u64 t;
	int h,m,s;
	
	t = osGetTime() / 1000;
	s = t % 86400;
	h = s / 3600;
	s -= h * 3600;
	m = s / 60;
	s -= m * 60;	
	sprintf(c,"%02d:%02d.%02d",h,m,s);
	set_Text(c);
	top->Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CClock::Start()
{
	CAnimation::Start();
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CClock::Stop()
{
	CAnimation::Stop();
	Invalidate();
	return 0;
}
