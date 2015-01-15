#include "gif-image.h"
#include "widgets.h"

#ifndef __KEYBOARDH__
#define __KEYBOARDH__

#define F_1	0x0
#define F_2	0x0
#define F_3	0x0
#define F_4	0x0
#define F_5	0x0
#define F_6	0x0
#define F_7	0x0
#define F_8	0x0
#define F_9	0x0
#define F10	0x0
#define F11	0x0
#define F12	0x0

#define TAB	0x0

#define ESC	0x0 // Escape
#define BSP	0x8 // Backspace
#define CAP	0x2 // Caps
#define RET	'\n' // Enter
#define SHF	0x4 // Shift
#define	CTR	0x0 // Ctrl
#define SPC	0x20 // Space
#define ALT	0x0 // Alt
#define NDS	0x0 // DS
#define SCN	0x0 // Screen

#define CUP	0x0 // Cursor up
#define CDW	0x0 // Cursor down


class CKeyboard : public CImageGif{
public:
	CKeyboard();
	int init(CDesktop *d);
	int show(CBaseWindow *w);
	int hide();
	int draw(u8 *dst);
	int onTouchEvent(touchPosition *p,u32 flags=0);
protected:
	int status;
	CDesktop *desk;
	CBaseWindow *win;
	POINT pt;
};

#endif