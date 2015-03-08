#include "widgets.h"
#include "fb_user.h"

#ifndef __FBCHATLISTH__
#define __FBCHATLISTH__

class CChatList : public CWindow{
public:
	CChatList();
	virtual ~CChatList();
	CUser *add_user(const char *cid);
	virtual int draw(u8 *screen);
	virtual int create(u32 x,u32 y,u32 w,u32 h,u32 id);
	static int onScroll(CBaseWindow *sb);
	static int onClick(CBaseWindow *w);
	int onClickItem();
	int onVScroll(CScrollBar *sb);
	int Update();
	CBaseWindow *onKeysPressEvent(u32 press,u32 flags = 0);
protected:
	std::map<std::string,CUser *>users;
};

#endif
