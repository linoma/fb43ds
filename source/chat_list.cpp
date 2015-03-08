#include "chat_list.h"
#include "utils.h"

//---------------------------------------------------------------------------
CChatList::CChatList() : CWindow()
{
	Hide();
	set_Text("Chat");
	set_Events("clicked",(void *)CChatList::onClick);
}
//---------------------------------------------------------------------------
CChatList::~CChatList()
{
}
//---------------------------------------------------------------------------
int CChatList::onClick(CBaseWindow *w)
{
	return ((CChatList *)w)->onClickItem();
}
//---------------------------------------------------------------------------
int CChatList::onClickItem()
{
	touchPosition lt;
	
	hidTouchRead(&lt);
	lt.px -= rcWin.left;
	lt.py -= rcWin.top;
	printd("x=%d y=%d",lt.px,lt.py);
	return 0;
}
//---------------------------------------------------------------------------
int CChatList::onScroll(CBaseWindow *sb)
{
	CBaseWindow *w;
	
	if(!w)
		return -1;
	w = sb->get_Parent();
	if(!w)
		return -2;
	return ((CChatList *)w)->onVScroll((CScrollBar *)sb);
}
//---------------------------------------------------------------------------
int CChatList::onVScroll(CScrollBar *sb)
{
	return 0;
}
//---------------------------------------------------------------------------
CBaseWindow *CChatList::onKeysPressEvent(u32 press,u32 flags)
{
	printd("key=>%u",press);
	return CWindow::onKeysPressEvent(press,flags);
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
	b->set_Events("clicked",(void *)CChatList::onScroll);
	return add(b);
}
//---------------------------------------------------------------------------
int CChatList::Update()
{
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CChatList::draw(u8 *screen)
{
	int y;
	RECT rc,rcItem;
	
	if(CWindow::draw(screen))
		return -1;
	get_ClientRect(&rc,1);
	y = rc.top;
	CopyRect(rcItem,rc);
	for (std::map<std::string,CUser *>::iterator t = users.begin(); t != users.end(); ++t){
		rcItem.top = y;
		rcItem.bottom = y + szCaption.cy;
		if(!t->second->draw(&rcItem,screen)){
			y = rcItem.bottom + 1;
			if(y>=rc.bottom)
				break;
		}
	}
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
	users[cid] = p;
	return p;
}
	