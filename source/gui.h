#include <vector>
#include <3ds.h>
#include "widgets.h"
#include "keyboard.h"

#ifndef __GUIH__
#define __GUIH__

class CLoader : public CImageWindow, public CAnimation{
public:
	CLoader();
	virtual ~CLoader(){};
	int onTimer();
	int Start();
	int Stop();
	int draw(u8 *screen);
};

class CLoaderWindow : public CDialog{
public:
	CLoaderWindow();
	virtual ~CLoaderWindow();
	int Show();
	int Hide();
protected:
	int destroy();
	CLoader *loader;
};

class CClock : public CLabel, public CAnimation{
public:
	CClock();
	virtual ~CClock(){};
	int onTimer();
	int Start();
	int Stop();
};

class CTopDesktop : public CDesktop{
public:
	CTopDesktop();
	virtual ~CTopDesktop();
	int init();
protected:
	int EraseBkgnd(u8 *screen);
	CImageGif *logo;
};

class CBottomDesktop : public CDesktop{
public:
	CBottomDesktop();
	virtual ~CBottomDesktop();
	virtual int draw(u8 *screen);
	int onTouchEvent(touchPosition *p,u32 flags=0);
	int init();
	int ShowCursor(CBaseWindow *w,int x,int y);
	int HideCursor();
protected:
	int EraseBkgnd(u8 *screen);
	CKeyboard *keyboard;
};

class CConsoleWindow : public CDialog{
public:
	CConsoleWindow();
	int printf(char *fmt,...);
	int set_Text(char *s);
protected:
	virtual int EraseBkgnd(u8 *screen);
};


int gui_init();
int gui_destroy();

extern CTopDesktop *top;
extern CBottomDesktop *bottom;
extern CConsoleWindow *console;
extern CImage *loader_img;
extern CLoaderWindow *loaderDlg;

#define print console->printf

#endif