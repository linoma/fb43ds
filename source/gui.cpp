#include "gui.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <algorithm>
#include "widgets.h"
#include "gfxdraw.h"
#include "gfxtext.h"
#include "fb4_logo_bin.h"
#include "fb.h"
#include "utils.h"

CTopDesktop *top;
CBottomDesktop *bottom;
CConsoleWindow *console;
CImage *loader_img,*toolbar_img;
CLoaderWindow *loaderDlg;
//---------------------------------------------------------------------------
int gui_init()
{
	top = new CTopDesktop();
	bottom = new CBottomDesktop();
	console = new CConsoleWindow();
	loader_img = new CImageGif();
	toolbar_img = new CImageGif();
	loaderDlg = new CLoaderWindow();
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
CTopDesktop::CTopDesktop() : CDesktop(GFX_TOP)
{
	bkcolor = 0xFfffffff;//0xFFd3d8e8
	
	logo = new CImageGif();
	
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
}
//---------------------------------------------------------------------------
CTopDesktop::~CTopDesktop()
{
	if(logo != NULL)
		logo->release();
	logo = NULL;
}
//---------------------------------------------------------------------------
int CTopDesktop::init()
{
	ShowDialog(loaderDlg);
	logo->load((u8 *)fb4_logo_bin,fb4_logo_bin_size);
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CTopDesktop::EraseBkgnd(u8 *screen)
{
	if(!CDesktop::EraseBkgnd(screen)){
		if(logo != NULL && !(fb->get_Status() & 1))
			logo->draw(screen,10,30);
		return 0;
	}
	return -1;
}
//---------------------------------------------------------------------------
CBaseWindow *CTopDesktop::onKeysPressEvent(u32 press,u32 flags)
{
	if(press & (KEY_L|KEY_R)){
		if(wins.empty())
			return NULL;
		if(a_win == NULL){
			if(press & KEY_L)
				a_win=wins.back();
			else
				a_win=wins.front();
		}
		else{
			for (std::vector<CBaseWindow *>::iterator win = wins.begin(); win != wins.end(); ++win){
				if(*win != a_win)
					continue;
				if(press & KEY_L){
					if(win==wins.begin())
						a_win=wins.back();
					else{
						win--;
						a_win=*win;
					}
				}
				else{
					if(++win == wins.end())
						a_win=wins.front();
					else
						a_win=*win;
				}
				break;
			}
		}
		printd("awin 0x%X",(u32)a_win);
		return a_win;
	}	
	return CDesktop::onKeysPressEvent(press,flags);
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
	bkcolor = 0xFFe9eaed;//0xFFe9eaed;
	keyboard = new CKeyboard();
}
//---------------------------------------------------------------------------
CBottomDesktop::~CBottomDesktop()
{
	if(keyboard){
		keyboard->release();
		keyboard=NULL;
	}
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
CBaseWindow *CBottomDesktop::onTouchEvent(touchPosition *p,u32 flags)
{
	if(dlg_win)
		return dlg_win->onTouchEvent(p,flags);
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
CConsoleWindow::CConsoleWindow() : CDialog()
{
	bkcolor = 0xff000000;
	color = 0xffffffff;
}
//---------------------------------------------------------------------------
int CConsoleWindow::EraseBkgnd(u8 *screen)
{
	if(!CDialog::EraseBkgnd(screen)){
		if(text){
			gfxSetTextColor(color);
			gfxDrawText(screen,font,text,&rcWin,0);
		}
		return 0;
	}
	return -1;
}
//---------------------------------------------------------------------------
int CConsoleWindow::set_Text(char *s)
{
	int res,len,dy,top,right,dx;	
	font_s *f;
	char *p;
	
	res = CDialog::set_Text(s);
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
CLoader::CLoader() : CImageWindow(),CAnimation(250)
{
}
//---------------------------------------------------------------------------
int CLoader::onTimer()
{
	frame++;
	if(pImage != NULL){
		if(frame*66 >= pImage->get_Width())
			frame = 0;
		Invalidate();
	}
	return 0;
}
//---------------------------------------------------------------------------
int CLoader::Start()
{
	CAnimation::Start();
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CLoader::Stop()
{
	CAnimation::Stop();
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CLoader::draw(u8 *screen)
{
	int w,h;
	
	if(is_invalidate() || pImage == NULL)
		return -1;
	CBaseWindow::draw(screen);
	w = rcWin.right - rcWin.left;
	h = rcWin.bottom - rcWin.top;
	return pImage->draw(screen,rcWin.left,rcWin.top,w,h,frame*66,0);
}
//---------------------------------------------------------------------------
CClock::CClock() : CLabel("clock"),CAnimation(900)
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
//---------------------------------------------------------------------------
CLoaderWindow::CLoaderWindow() : CDialog()
{
	loader = NULL;
}
//---------------------------------------------------------------------------
CLoaderWindow::~CLoaderWindow()
{
}
//---------------------------------------------------------------------------
int CLoaderWindow::Hide()
{
	if(!CDialog::Hide()){
		if(loader)
			loader->set_Enabled(0);
	}
	return -1;
}
//---------------------------------------------------------------------------
int CLoaderWindow::Show()
{
	if(!CDialog::Show()){
		if(loader == NULL){
			loader = new CLoader();
			loader->create(162,72,66,66,-1);
			loader->load(loader_img);
			add(loader);
			fb->SetTimer(loader);
		}
		loader->Start();
	}
	return -1;
}
//---------------------------------------------------------------------------
int CLoaderWindow::destroy()
{
	int res = CDialog::destroy();
	if(loader!= NULL){
		delete loader;
		loader=NULL;
	}
	return res;
}