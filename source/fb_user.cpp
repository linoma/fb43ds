#include "fb_user.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "webrequest.h"
#include "fb.h"
#include "utils.h"
#include "syshelper.h"
#include "gfxtext.h"
#include "gui.h"

//---------------------------------------------------------------------------
CUser::CUser(const char *cid) : CImageJpeg()
{
   status = 0;
   name = thumbSrc = NULL;
   win = NULL;
   strcpy(id,cid);
}
//---------------------------------------------------------------------------
CUser::~CUser()
{
	if(name != NULL)
		free(name);
	name = thumbSrc = NULL;
	if(win){
		delete win;
		win=NULL;
	}
}
//---------------------------------------------------------------------------
int CUser::onDrawItem(CBaseWindow *w,u32 param)
{
	CUser *p  = (CUser *)w->get_Parent();
	if(!p)
		return -1;
	return p->onDrawItem((LPDRAWITEM)param,(CListBox *)w);
}
//---------------------------------------------------------------------------
int CUser::onDrawItem(LPDRAWITEM p,CListBox *w)
{
	RECT rc,rcw;
	SIZE sz;
	
	rc.left = p->prcItem->left;
	rc.top = p->prcItem->top;
	rc.right = p->prcItem->right;
	sz.cx = rc.right-rc.left;
	gfxGetTextExtent(0,p->value,&sz,1);
	rc.bottom = rc.top+sz.cy;
	w->get_ClientRect(&rcw,1);
	if(rc.bottom>rcw.bottom)
		return 0;
	gfxDrawText(p->screen,NULL,p->value,&rc,0);
	printd("item y %d",sz.cy);
	return sz.cy;
}
//---------------------------------------------------------------------------
int CUser::show_chat()
{
	if(!win){
		CListBox *b;
		RECT rc;
		
		win = new CWindow();
		win->create(20,20,250,180,-1);
		win->set_Text(name);
		b = new CListBox();
		win->get_ClientRect(&rc);
		b->create(rc.left,rc.top,rc.right,rc.bottom,1);
		b->set_Events("drawitem",CUser::onDrawItem);
		win->add(b);
		top->add(win);
		sys_helper->set_Job(200,2,CUser::get_chat_content,this);	
	}
	if(!win)
		return -3;
	top->ActiveWindow(win);	
	return 0;
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
int CUser::draw(LPRECT prc,u8 *screen)
{
	RECT rc;
	
	if(!(status & 2) || !name)
		return -1;
	rc.left=prc->left;
	rc.right=prc->right;
	rc.top=prc->top;
	rc.bottom=prc->bottom;
	if(CImageJpeg::status&1){
		CImageJpeg::draw(screen,prc->left,prc->top);
		rc.left+=32;
	}
	gfxSetTextColor(0xff000000);
	gfxDrawText(screen,NULL,name,&rc,DT_VCENTER);
	return 0;
}
//---------------------------------------------------------------------------
int CUser::get_info()
{
	CWebRequest *req;
	int res,ret,nnodes,i,ii;
	std::string s;
	char *_buf;
	u32 code,sz;
	jsmn_parser parser;
	jsmntok_t *t,*nname,*nthumb;
	
	t = NULL;
	_buf = NULL;
	res = -1;
	if((req = new CWebRequest()) == NULL)
		goto fail;
	res--;
	s = "https://www.facebook.com/chat/user_info/?__user=";
	s += fb->get_UserId();
	s += "&__a=1&ids[0]=";
	s += id;
	req->begin(s.c_str());	
	ret = fb->get_Cookies(NULL,0);
	if(ret > 0){
		char *p = (char *)malloc(ret+1);
		fb->get_Cookies(p,ret);
		req->add_header("Cookie",p);
		free(p);
	}	
	res--;
	if(req->send(CWebRequest::RQ_DEBUG))
		goto fail;
	res--;
	if(req->get_statuscode(&code))
		goto fail;
	res--;
	if(code != 200)		
		goto fail;
	res--;
	_buf = (char *)linearAlloc(0x2000);
	if(req->download_data(_buf,0x2000,&sz))
		goto fail;
	res--;
	jsmn_init(&parser);
	nnodes = jsmn_parse(&parser, _buf, sz, NULL,0);
	if(nnodes < 0)
		goto fail;
	res--;
	t = (jsmntok_t *)linearAlloc(nnodes * sizeof(jsmntok_t));
	if(!t)
		goto fail;
	res--;
	jsmn_init(&parser);
	ret = jsmn_parse(&parser,_buf,sz,t,nnodes);
	nname = nthumb = NULL;
	for(i=0;i<ret;i++){
		if(t[i].type != JSMN_STRING)
			continue;
		if(strncmp(&_buf[t[i].start],"name",4)==0){
			nname = &t[++i];
			break;
		}
	}
	for(i=0;i<ret;i++){
		if(t[i].type != JSMN_STRING)
			continue;
		if(strncmp(&_buf[t[i].start],"thumbSrc",8)==0){
			nthumb = &t[++i];
			break;
		}
	}
	if(nname == NULL)
		goto fail;
	res--;
	i = nname->end - nname->start + 1;
	ii = 0;
	if(nthumb)
		ii = nthumb->end - nthumb->start + 1;
	name = (char *)calloc(i+ii+2,1);
	if(name == NULL)
		goto fail;
	res--;
	thumbSrc = &name[i+1];
	strncpy(name,&_buf[nname->start],nname->end - nname->start);
	if(nthumb){
       strncpy(thumbSrc,&_buf[nthumb->start],nthumb->end - nthumb->start);
       stripslashes(thumbSrc);
       sys_helper->set_Job(101,2,CUser::get_avatar_user,this);
	}
	status |= 2;
	res = 0;
fail:	
	if(req != NULL)
		delete req;
	if(t)
		linearFree(t);
	if(_buf != NULL){
		linearFree(_buf);
		_buf=NULL;
	}
	sys_helper->set_Result(100,2,res,this);
	return res;	
}
//---------------------------------------------------------------------------
int CUser::get_chat()
{
	CWebRequest *req;
	int res,ret,nnodes,i,ii;
	std::string s;
	char *_buf;
	u32 code,sz;
	jsmn_parser parser;
	jsmntok_t *t,*tt;
	CListBox *lb;
	
	t = NULL;
	_buf = NULL;
	res = -1;
	if((req = new CWebRequest()) == NULL)
		goto fail;
	res--;
	req->begin("https://www.facebook.com/ajax/mercury/thread_info.php");	
	ret = fb->get_Cookies(NULL,0);
	if(ret > 0){
		char *p = (char *)malloc(ret+1);
		fb->get_Cookies(p,ret);
		req->add_header("Cookie",p);
		free(p);
	}
	req->add_postdata("client","mercury");
	req->add_postdata("__a","1");
	req->add_postdata("__user",fb->get_UserId());
	req->add_postdata("threads[user_ids][0]",(char *)get_ID());
	s = "messages[user_ids][";
	s += get_ID();
	s += "][offset]";
	req->add_postdata((char *)s.c_str(),"0");
	s = "messages[user_ids][";
	s += get_ID();
	s += "][limit]";
	req->add_postdata((char *)s.c_str(),"10");
	req->add_postdata("fb_dtsg",fb->get_dtsg());
	//ttstamp 26581711001075454981104998115
	res--;
	if(req->send(CWebRequest::RQ_POST|CWebRequest::RQ_DEBUG))
		goto fail;
	res--;
	if(req->get_statuscode(&code))
		goto fail;
	res--;
	if(code != 200)		
		goto fail;
	res--;
	_buf = (char *)linearAlloc(0x8000);
	if(req->download_data(_buf,0x8000,&sz))
		goto fail;
	//write_to_sdmc("/msg.txt",(u8 *)_buf,sz);
	res--;
	jsmn_init(&parser);
	nnodes = jsmn_parse(&parser, _buf, sz, NULL,0);
	if(nnodes < 0)
		goto fail;
	res--;
	t = (jsmntok_t *)linearAlloc(nnodes * sizeof(jsmntok_t));
	if(!t)
		goto fail;
	tt = NULL;
	res--;
	lb = (CListBox *)win->get_Window(1);
	jsmn_init(&parser);
	ret = jsmn_parse(&parser,_buf,sz,t,nnodes);
	for(i=0;i<ret;i++){
		if(t[i].type != JSMN_STRING)
			continue;
		if(strncmp(&_buf[t[i].start],"body",4)==0){
			tt = &t[++i];
			_buf[tt->end]=0;
			if(lb)
				lb->add_item(stripslashes(&_buf[tt->start]));
		}
	}
	
	res = 0;
fail:	
	if(req != NULL)
		delete req;
	if(t)
		linearFree(t);
	if(_buf != NULL){
		linearFree(_buf);
		_buf=NULL;
	}
	sys_helper->set_Result(200,2,res,this);
	return res;
}
//---------------------------------------------------------------------------
int CUser::get_chat_content(u32 arg0)
{
	return ((CUser *)arg0)->get_chat();
}
//---------------------------------------------------------------------------
int CUser::get_info_user(u32 arg0)
{
	return ((CUser *)arg0)->get_info();
}
//---------------------------------------------------------------------------
int CUser::get_avatar_user(u32 arg0)
{
	return ((CUser *)arg0)->get_avatar();
}
//---------------------------------------------------------------------------
int CUser::get_avatar()
{
	int res,ret;
	CWebRequest *req;
	char *_buf;
	u32 code,sz;
	
	res = -1;
	_buf = NULL;
	if((req = new CWebRequest()) == NULL)
		goto fail;
	res--;
	req->begin(thumbSrc);	
	ret = fb->get_Cookies(NULL,0);
	if(ret > 0){
		char *p = (char *)malloc(ret+1);
		fb->get_Cookies(p,ret);
		req->add_header("Cookie",p);
		free(p);
	}	
	res--;
	if(req->send(CWebRequest::RQ_DEBUG))
		goto fail;
	res--;
	if(req->get_statuscode(&code))
		goto fail;
	res--;
	if(code != 200)		
		goto fail;
	res--;
	_buf = (char *)linearAlloc(0x5000);
	if(req->download_data(_buf,0x5000,&sz))
		goto fail;
	res = load((u8 *)_buf,sz);
fail:
	if(req != NULL)
		delete req;
	if(_buf != NULL){
		linearFree(_buf);
		_buf = NULL;
	}
	sys_helper->set_Result(101,2,res,this);
	return res;
}
