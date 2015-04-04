#include "chat_list.h"
#include "utils.h"

//---------------------------------------------------------------------------
CChatList::CChatList() : CWindow()
{
	Hide();
	set_Events("clicked",(void *)CChatList::onClick);
	first_visible_item=0;
	item_height=32;
}
//---------------------------------------------------------------------------
CChatList::~CChatList()
{
}
//---------------------------------------------------------------------------
int CChatList::EraseBkgnd(u8 *screen)
{
	if(is_invalidate())
		return -1;
	gfxGradientFillRect(&rcWin,0,0,0xffeaeaea,bkcolor,screen);	
	gfxGradientFillRect(&rcHandle,0,0,0xffeaeaea,bkcolor,screen);
	gfxRect(&rcHandle,0x80808080,screen);
	return 0;
}
//---------------------------------------------------------------------------
int CChatList::onClick(CBaseWindow *w)
{
	return ((CChatList *)w)->onClickItem();
}
//---------------------------------------------------------------------------
int CChatList::onClickItem()
{
	POINT pt;
	u32 item;
	CUser *u;
	
	getCursorPos(&pt);
	if(pt.x > rcHandle.left && pt.x < rcHandle.right && pt.y > rcHandle.top && pt.y < rcHandle.bottom){
		Hide();
		return 0;
	}		
	pt.x -= rcWin.left;
	pt.y -= rcWin.top;
	item = first_visible_item + (pt.y/item_height);
	u = get_UserFromIndex(item);
	if(u)
		printd((char *)u->get_Name());
	printd("x=%d y=%d",pt.x,pt.y);
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
	b->create(rc.right-9,rc.top,0,rc.bottom,1);
	b->set_Events("clicked",(void *)CChatList::onScroll);
	rcHandle.right = rcWin.right;
	rcHandle.left = rcHandle.right - 4;
	rcHandle.top = rcWin.top + (((rcWin.bottom - rcWin.top) - 16) >> 1);
	rcHandle.bottom = rcHandle.top + 24;
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
	u32 i;
	
	if(CWindow::draw(screen))
		return -1;
	get_ClientRect(&rc,1);
	y = rc.top;
	CopyRect(rcItem,rc);
	i=0;
	for (std::map<std::string,CUser *>::iterator t = users.begin(); t != users.end(); ++t,i++){
		if(i<first_visible_item)
			continue;
		rcItem.top = y;
		rcItem.bottom = y + item_height;
		if(rcItem.bottom>=rc.bottom)
			break;
		if(!t->second->draw(&rcItem,screen)){
			y = rcItem.bottom + 1;
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
//---------------------------------------------------------------------------
CUser *CChatList::get_UserFromIndex(u32 idx)	
{
	u32 i;
	
	if(idx >= users.size())
		return NULL;
	i=0;
	for (std::map<std::string,CUser *>::iterator t = users.begin(); t != users.end(); ++t,i++){
		if(i==idx)
			return t->second;
	}
	return NULL;
}
		