#include "gif-image.h"
#include "widgets.h"

#ifndef __KEYBOARDH__
#define __KEYBOARDH__

class CKeyboard : public CImageGif{
public:
	CKeyboard();
	int init(CDesktop *d);
	int show(CBaseWindow *w);
	int hide();
	int draw(u8 *dst);
	int onTouchEvent(touchPosition *p);
protected:
	int status;
	CDesktop *desk;
	CBaseWindow *win;
	POINT pt;
};

extern CKeyboard *keyboard;
#endif