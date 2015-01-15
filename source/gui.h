#include <vector>
#include <3ds.h>
#include "widgets.h"
#include "keyboard.h"

#ifndef __GUIH__
#define __GUIH__

class CLoaderWindow : public CImageWindow, public CAnimation{
public:
	CLoaderWindow();
	virtual ~CLoaderWindow(){};
	int onTimer();
	int Start();
	int Stop();
	int draw(u8 *screen);
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
	int show_loader();
protected:
	int EraseBkgnd(u8 *screen);
	CImageGif *logo;
	CLoaderWindow *loader;
};

class CBottomDesktop : public CDesktop{
public:
	CBottomDesktop();
	virtual ~CBottomDesktop(){};
	virtual int draw(u8 *screen);
	int onTouchEvent(touchPosition *p,u32 flags=0);
	int init();
	int ShowCursor(CBaseWindow *w,int x,int y);
	int HideCursor();
protected:
	int EraseBkgnd(u8 *screen);
	CKeyboard *keyboard;
};

class CConsoleWindow : public CWindow{
public:
	CConsoleWindow();
	int printf(char *fmt,...);
	int set_Text(char *s);
};


int gui_init();
int gui_destroy();

extern CDesktop *top,*bottom;
extern CConsoleWindow *console;
extern CLoaderWindow *loader;

#define print console->printf

#endif