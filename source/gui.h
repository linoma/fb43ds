#include <vector>
#include <3ds.h>
#include "widgets.h"
#include "gif-image.h"

#ifndef __GUIH__
#define __GUIH__

class CTopDesktop : public CDesktop{
public:
	CTopDesktop();
	virtual ~CTopDesktop(){};
	virtual int draw(u8 *screen);
};

class CKeyboard : public CImageGif{
public:
	CKeyboard();
	int init(CDesktop *d);
	int show();
	int draw(u8 *dst,int x,int y,int w=-1,int h=-1,int x0=0,int y0=0);
protected:
	int status;
	CDesktop *desk;
};

class CBottomDesktop : public CDesktop{
public:
	CBottomDesktop();
	virtual ~CBottomDesktop(){};
	virtual int draw(u8 *screen);
protected:
	CKeyboard *keyboard;
};

int gui_init();
int gui_destroy();

extern CDesktop *top,*bottom;

#endif