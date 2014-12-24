#include <vector>
#include <3ds.h>
#include "widgets.h"

#ifndef __GUIH__
#define __GUIH__

class CTopDesktop : public CDesktop{
public:
	CTopDesktop();
	virtual ~CTopDesktop(){};
	virtual int draw(u8 *screen);
};

class CBottomDesktop : public CDesktop{
public:
	CBottomDesktop();
	virtual ~CBottomDesktop(){};
	virtual int draw(u8 *screen);
};

int gui_init();
int gui_destroy();

extern CDesktop *top,*bottom;

#endif