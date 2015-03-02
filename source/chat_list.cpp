#include "chat_list.h"

//---------------------------------------------------------------------------
CChatList::CChatList() : CWindow()
{
	Hide();
	set_Text("Chat");
}
//---------------------------------------------------------------------------
CChatList::~CChatList()
{
}
//---------------------------------------------------------------------------
int CChatList::create(u32 x,u32 y,u32 w,u32 h,u32 id)
{
	CBaseWindow *b;
	RECT rc;
	
	CWindow::create(x,y,w,h,id);
	b = new CScrollBar();
	get_ClientRect(&rc);
	b->create(rc.right-12,rc.top,0,rc.bottom,1);
	return add(b);
}
//---------------------------------------------------------------------------
int CChatList::draw(u8 *screen)
{
	if(CWindow::draw(screen))
		return -1;
	return 0;
}
//---------------------------------------------------------------------------
CUser *CChatList::add_user(const char *cid)
{
	if(!cid || !*cid)
		return NULL;
	if(users.count(cid))
		return users[cid];
	CUser *p = new CUser(cid);
	if(!p)
		return NULL;
	users[cid]=p;
	return p;
}
	