#include "fb_user.h"
#include <string.h>
#include "webrequest.h"
#include "fb.h"

//---------------------------------------------------------------------------
CUser::CUser(const char *cid)
{
   status=0;
   strcpy(id,cid);
}
//---------------------------------------------------------------------------
CUser::~CUser()
{
}
//---------------------------------------------------------------------------
int CUser::set_Active(int val)
{
   if(val)
       status |=1;
   else
       status &= ~1;
   return 0;
}
//---------------------------------------------------------------------------
int CUser::get_info()
{
	CWebRequest *req;
	int res;
	std::string s;
	
	res=-1;
	if((req = new CWebRequest()) == NULL)
		goto fail;
	res--;
	s="https://www.facebook.com/chat/user_info/?__user=";
	s += fb->get_UserId();
	s += "&__a=1";
	req->begin(s.c_str());
fail:
	if(req)
		delete req;
	return res;
}
//---------------------------------------------------------------------------
int CUser::get_info_user(u32 arg0)
{
	return ((CUser *)arg0)->get_info();
}
