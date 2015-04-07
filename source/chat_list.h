#include "widgets.h"
#include "fb_user.h"

#ifndef __FBCHATLISTH__
#define __FBCHATLISTH__

class CChatList : public CWindow{
public:
	CChatList();
	virtual ~CChatList();
	CUser *add_user(const char *cid);
	virtual int create(u32 x,u32 y,u32 w,u32 h,u32 id);
	static int onClick(CBaseWindow *w,u32 param);
	static int onDrawItem(CBaseWindow *w,u32 param);
	static int onClickItem(CBaseWindow *w,u32 param);
	int onDrawItem(LPDRAWITEM p);
	int onClickItem(char *item);
	int onClick();
	int Update();
	CBaseWindow *onKeysPressEvent(u32 press,u32 flags = 0);
	CUser *get_UserFromIndex(u32 idx);
	int onActivate(int v);
protected:
	virtual int EraseBkgnd(u8 *screen);
	std::map<std::string,CUser *>users;
	RECT rcHandle;
	u32 first_visible_item,item_height;
	CListBox *lb;
};

#endif
