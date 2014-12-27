#include <vector>
#include <string>
#include <3ds.h>
#include "types.h"

#ifndef __WIDGETSH__
#define __WIDGETSH__

//---------------------------------------------------------------------------
class CTimer {
public:
	CTimer(LPDEFFUNC f,u64 i,u32 p);
	virtual ~CTimer(){};
	int onCounter();
	int set_Param(u32 p);
	int set_Enabled(int v);
protected:
	u64 elapsed,interval;
	LPDEFFUNC fnc;
	u32 param,status;
};

class CBaseWindow {
public:
	CBaseWindow();
	CBaseWindow(gfxScreen_t s);
	virtual ~CBaseWindow();
	virtual int draw(u8 *screen);
	int set_BkColor(u32 c);
	int set_TextColor(u32 c);
	gfxScreen_t get_Screen(){return scr;}
	virtual int onTouchEvent(touchPosition *p);
	virtual int onKeysPressEvent(u32 press);
	virtual int onKeysUpEvent(u32 press);
	virtual int onCharEvent(u8 c);
	virtual int onActivate(int v);	
	virtual int create(u32 x,u32 y,u32 w,u32 h,u32 id);
	int set_Parent(CBaseWindow *w){parent = w;return 0;};
	CBaseWindow *get_Parent(){return parent;};
	virtual int Invalidate();
	virtual int set_Pos(int x, int y);
	int get_WindowRect(LPRECT prc){*prc = *(&sz);return 0;};
	int set_Text(char *s);	
protected:
	virtual int isInvalidate();
	virtual int destroy();
	CBaseWindow *get_Desktop();
	
	u32 color,bkcolor,status,ID,redraw;
	gfxScreen_t scr;
	RECT sz;
	CBaseWindow *parent;
	char *text;
};

class CContainerWindow : public CBaseWindow{
public:
	CContainerWindow();
	CContainerWindow(gfxScreen_t s);
	virtual ~CContainerWindow(){};
	int add(CBaseWindow *w);
	virtual int draw(u8 *screen);
	virtual int Invalidate();
protected:
	std::vector<CBaseWindow *>wins;	
};

class CCursor : public CTimer {
public:
	CCursor(CContainerWindow *w);
	virtual ~CCursor(){};
	int Show(CBaseWindow *w,int x,int y);
	int Hide();
	int onTimer();
	int set_Pos(int x,int y);
protected:
	int draw();
	static int onTimer(u32 param);
	CBaseWindow *win;
	CContainerWindow *desk;
	POINT pos;
	SIZE sz;
};

class CDesktop : public CContainerWindow {
public:
	CDesktop(gfxScreen_t s);
	virtual ~CDesktop(){};
	int onTouchEvent(touchPosition *p);
	int onKeysPressEvent(u32 press);
	int onKeysUpEvent(u32 press);	
	int SetTimer(LPDEFFUNC f,u64 val,u32 p);
	int IncrementTimers();
	int ShowCursor(CBaseWindow *w,int x,int y);
	int HideCursor();
	u8 *get_Buffer();
protected:
	int onActivateWindow(CBaseWindow *win);
	CBaseWindow *a_win;
	CCursor *cursor;
	std::vector<CTimer *>timers;	
};

class CWindow : public CBaseWindow {
public:	
	CWindow();
	virtual ~CWindow();
	virtual int draw(u8 *screen);
	virtual int onTouchEvent(touchPosition *p);
protected:
	u32 styles;
	int (*cb)(CWindow *);
};
//---------------------------------------------------------------------------
class CLabel : public CWindow{	
public:
	CLabel(char *c);
	virtual ~CLabel(){};
	int draw(u8 *screen);
};
//---------------------------------------------------------------------------
class CButton : public CWindow{
public:
	CButton();
	virtual ~CButton();
	int draw(u8 *screen);
	int onKeysPressEvent(u32 press);
	int set_Accelerator(int v){accel = v;return 0;};
protected:
	int accel;
};
//---------------------------------------------------------------------------
class CStatusBar  : public CContainerWindow{
public:
	CStatusBar();
	virtual ~CStatusBar();
	int draw(u8 *screen);
};
//---------------------------------------------------------------------------
class CMenuBar : public CContainerWindow{
public:
	CMenuBar();
	virtual ~CMenuBar(){};
	int draw(u8 *screen);
};
//---------------------------------------------------------------------------
class CScrollBar : public CWindow{
public:
	CScrollBar();
	virtual ~CScrollBar(){};
	int draw(u8 *screen){return 0;};
protected:
	u32 pos,min,max;
};
//---------------------------------------------------------------------------
class CEditText : public CWindow{
public:
	CEditText();
	virtual ~CEditText();
	int draw(u8 *screen);
	int onActivate(int v);
	virtual int create(u32 x,u32 y,u32 w,u32 h,u32 id);
	virtual int onCharEvent(u8 c);
protected:
	int HideCursor();
	int ShowCursor(int x,int y);
};
//---------------------------------------------------------------------------
class CEditBox : public CEditText
{
public:
	CEditBox();
};
//---------------------------------------------------------------------------
class CListBox : public CWindow{
public:
	CListBox();
	virtual ~CListBox(){};
	int draw(u8 *screen){return 0;};
};
//---------------------------------------------------------------------------
class CToolBar : public CContainerWindow{
public:
	CToolBar();
	virtual ~CToolBar(){};
};

#endif