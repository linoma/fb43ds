#include <vector>
#include <string>
#include <map>
#include <3ds.h>
#include "images.h"
#include "font.h"

#ifndef __WIDGETSH__
#define __WIDGETSH__

class CBaseWindow {
public:
	CBaseWindow();
	CBaseWindow(gfxScreen_t s);
	virtual ~CBaseWindow();
	virtual int draw(u8 *screen);
	int set_BkColor(u32 c);
	int set_TextColor(u32 c);
	gfxScreen_t get_Screen(){return scr;}
	virtual int onTouchEvent(touchPosition *p,u32 flags=0);
	virtual int onKeysPressEvent(u32 press);
	virtual int onKeysUpEvent(u32 press);
	virtual int onCharEvent(u8 c);
	virtual int onActivate(int v);	
	virtual int create(u32 x,u32 y,u32 w,u32 h,u32 id);
	int set_Parent(CBaseWindow *w){parent = w;return 0;};
	CBaseWindow *get_Parent(){return parent;};
	virtual int Invalidate(int flags=0);
	virtual int set_Pos(int x, int y);
	int get_WindowRect(LPRECT prc);
	virtual int set_Text(char *s);	
	int get_Text(char *s,u32 len);
	u32 get_ID(){return ID;};
	int set_Events(char *key,void *value);
	virtual int Show();
	virtual int Hide();
protected:
	virtual int is_invalidate();
	virtual int destroy();
	virtual int fire_event(const char *key);
	CBaseWindow *get_Desktop();
	
	u32 color,bkcolor,status,ID,redraw,text_len;
	gfxScreen_t scr;
	RECT rcWin;
	CBaseWindow *parent;
	char *text;
	font_s *font;
	std::map<std::string,void *>events;
};

class CContainerWindow : public CBaseWindow{
public:
	CContainerWindow();
	CContainerWindow(gfxScreen_t s);
	virtual ~CContainerWindow(){};
	int add(CBaseWindow *w);
	int remove(CBaseWindow *w);
	int remove(u32 id);
	virtual int draw(u8 *screen);
	virtual int Invalidate(int flags=0);
	CBaseWindow *get_Window(u32 id);
protected:
	virtual int EraseBkgnd(u8 *screen);
	std::vector<CBaseWindow *>wins;
	u32 bktcolor;
};

class CCursor : public CAnimation {
public:
	CCursor(CContainerWindow *w);
	virtual ~CCursor(){};
	int Show(CBaseWindow *w,int x,int y);
	int Hide();
	int onTimer();
	int set_Pos(int x,int y);	
protected:
	int draw();
	CBaseWindow *win;
	CContainerWindow *desk;
	POINT pos;
	SIZE sz;
};

class CDialog : public CContainerWindow {
public:
	CDialog();
	virtual ~CDialog();
};

class CDesktop : public CContainerWindow {
public:
	CDesktop(gfxScreen_t s);
	virtual ~CDesktop(){};
	virtual int onTouchEvent(touchPosition *p,u32 flags=0);
	int onKeysPressEvent(u32 press);
	int onKeysUpEvent(u32 press);	
	virtual int ShowCursor(CBaseWindow *w,int x,int y);
	virtual int HideCursor();
	virtual int SetCursorPos(int x,int y);
	u8 *get_Buffer();
	int ShowDialog(CDialog *w);
	int HideDialog();
	virtual int draw(u8 *screen);
	virtual int Invalidate(int flags=0);
	virtual int init(){return -1;};
protected:
	int onActivateWindow(CBaseWindow *win);
	CBaseWindow *a_win,*dlg_win;
	CCursor *cursor;
};
//---------------------------------------------------------------------------
class CWindow : public CBaseWindow {
public:	
	CWindow();
	virtual ~CWindow();
	virtual int draw(u8 *screen);
};
//---------------------------------------------------------------------------
class CLabel : public CBaseWindow{	
public:
	CLabel(char *c);
	virtual ~CLabel(){};
	int draw(u8 *screen);
};
//---------------------------------------------------------------------------
class CImageWindow : public CBaseWindow{
public:
	CImageWindow();
	virtual int draw(u8 *screen);
	virtual int load(u8 *src,int w=-1,int h=-1);
	virtual int load(CImage *img);
protected:
	virtual int destroy();
	CImage *pImage;
};
//---------------------------------------------------------------------------
class CButton : public CBaseWindow{
public:
	CButton(char *c);
	CButton();
	virtual ~CButton();
	virtual int draw(u8 *screen);
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
protected:	
	int EraseBkgnd(u8 *screen);
};
//---------------------------------------------------------------------------
class CMenuBar : public CContainerWindow{
public:
	CMenuBar();
	virtual ~CMenuBar(){};
	int draw(u8 *screen);
};
//---------------------------------------------------------------------------
class CScrollBar : public CBaseWindow{
public:
	CScrollBar();
	virtual ~CScrollBar(){};
	int draw(u8 *screen){return 0;};
protected:
	u32 pos,min,max;
};
//---------------------------------------------------------------------------
class CEditText : public CBaseWindow{
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
	u32 char_pos;
	POINT ptCursor;
};
//---------------------------------------------------------------------------
class CEditBox : public CEditText
{
public:
	CEditBox();
};
//---------------------------------------------------------------------------
class CListBox : public CBaseWindow{
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