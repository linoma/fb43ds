#include "chat_list.h"
#include "utils.h"

//---------------------------------------------------------------------------
CChatList::CChatList() : CWindow()
{
	Hide();
	set_Events("clicked",CChatList::onClick);	
	first_visible_item=0;
	item_height=32;
}
//---------------------------------------------------------------------------
CChatList::~CChatList()
{
}
//---------------------------------------------------------------------------
int CChatList::onActivate(int v)
{
	if(!v)
		Hide();
	return CWindow::onActivate(v);
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
int CChatList::onClick(CBaseWindow *w,u32 param)
{
	return ((CChatList *)w)->onClick();
}
//---------------------------------------------------------------------------
int CChatList::onClickItem(CBaseWindow *w,u32 param)
{
	w = w->get_Parent();
	if(!w)
		return -1;
	return ((CChatList *)w)->onClickItem((char *)param);
}
//---------------------------------------------------------------------------
int CChatList::onClick()
{
	POINT pt;
	u32 item;
	CUser *u;
	
	getCursorPos(&pt);
	if(pt.x > rcHandle.left && pt.x < rcHandle.right && pt.y > rcHandle.top && pt.y < rcHandle.bottom){
		Hide();
		return 0;
	}		
	return 0;
}
//---------------------------------------------------------------------------
int CChatList::onClickItem(char *item)
{
	CUser *u;
	
	if(!item || !users.count(item))
		return -1;
	u = users[item];
	u->show_chat();
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
	RECT rc;
	
	CWindow::create(x,y,w,h,id);
	lb = new CListBox();
	get_ClientRect(&rc);
	lb->create(rc.left,rc.top,rc.right-4,rc.bottom,1);
	lb->set_ItemHeight(32);
	lb->set_Events("drawitem",CChatList::onDrawItem);
	lb->set_Events("clickitem",CChatList::onClickItem);
	lb->set_BkColor(0);
	rcHandle.right = rcWin.right;
	rcHandle.left = rcHandle.right - 4;
	rcHandle.top = rcWin.top + (((rcWin.bottom - rcWin.top) - 16) >> 1);
	rcHandle.bottom = rcHandle.top + 24;
	return add(lb);
}
//---------------------------------------------------------------------------
int CChatList::Update()
{
	Invalidate();
	return 0;
}
//---------------------------------------------------------------------------
int CChatList::onDrawItem(CBaseWindow *w,u32 param)
{
	w = w->get_Parent();
	if(!w)
		return -1;
	return ((CChatList *)w)->onDrawItem((LPDRAWITEM)param);
}
//---------------------------------------------------------------------------
int CChatList::onDrawItem(LPDRAWITEM p)
{
	CUser *u;
	
	if(!p)
		return -1;	
	if(users.count(p->value) == 0)
		return -2;
	u = (CUser *)users[p->value];
	u->draw(p->prcItem,p->screen);
	return p->prcItem->bottom - p->prcItem->top;
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
	lb->add_item((char *)cid);
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
		