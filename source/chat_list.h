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
protected:
	std::map<std::string,CUser *>users;
};

#endif
