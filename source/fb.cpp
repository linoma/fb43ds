#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "gui.h"
#include "webrequest.h"
#include "syshelper.h"
#include "fb.h"
#include "loader_bin.h"
#include "toolbar_bin.h"
#include "utils.h"
#include "jsmn.h"

LPDEFFUNC pfn_State = NULL;
CSysHelper *sys_helper = NULL;
u8 *linear_buffer = NULL;
FS_archive sdmcArchive;
CFBClient *fb;
static unsigned char fb_char_test[] = {"€,´,€,´,水,Д,Є"};
//---------------------------------------------------------------------------
static int on_clicked_button(CBaseWindow *w)
{
	return fb->onClicked(w->get_ID());	
}
//---------------------------------------------------------------------------
static int fb_main(u32 arg0)
{
	return fb->Main(arg0);
}
//---------------------------------------------------------------------------
static int fb_authenticate(u32 arg0)
{
	u32 val,*p;

	top->HideDialog();
	
	for(int i=0;i<6;i++)
		bottom->remove(i);
	CToolBar *b = new CToolBar();
	b->create(0,0,320,28,1);		
	b->set_BkColor(0xFFEEEEEE);
	
	CToolButton *c = new CToolButton();
	c->create(0,0,22,22,1);
	c->load(toolbar_img,12);
	b->add(c);
	c->set_Events("clicked",(void *)on_clicked_button);
	
	c = new CToolButton();
	c->create(0,0,22,22,2);
	c->load(toolbar_img,10);
	b->add(c);
	c->set_Events("clicked",(void *)on_clicked_button);

	c = new CToolButton();
	c->create(0,0,22,22,3);
	c->load(toolbar_img,3);
	b->add(c);
	c->set_Events("clicked",(void *)on_clicked_button);
	
	c = new CToolButton();
	c->create(0,0,22,22,4);
	c->load(toolbar_img,4);
	b->add(c);
	c->set_Events("clicked",(void *)on_clicked_button);
	bottom->add(b);
	return 0;
}
//---------------------------------------------------------------------------
static int sys_login(u32 arg0)
{
	char s[90],s1[90],*_buf;
	int ret;
	
	_buf = NULL;
	print("Authenticating...");
	CWebRequest *req = new CWebRequest();	
	if(req == NULL)
		return -1;
	ret = req->begin("https://login.facebook.com/login.php?login_attempt=1&_fb_noscript=1");
	if(ret)
		return -2;
	fb->add_Cookie("test_cookie","1");
	fb->add_Cookie("lsd","abcde");
	ret = fb->get_Cookies(NULL,0);
	if(ret > 0){
		char *p = (char *)malloc(ret+1);
		fb->get_Cookies(p,ret);
		req->add_header("Cookie",p);
		free(p);
	}
	urlencode((char *)fb_char_test,s);

    req->add_postdata("charset_test",s);
    req->add_postdata("locale","it_IT");

	urlencode((char *)fb->get_Email(),s1);
	req->add_postdata("email",s1);
	urlencode((char *)fb->get_Password(),s1);
	req->add_postdata("pass",s1);

    req->add_postdata("pass_placeHolder","Password");
    req->add_postdata("version","1.0");
	req->add_postdata("persistent","1");
    req->add_postdata("login","Login");
	req->add_postdata("charset_test",s);
    req->add_postdata("lsd","abcde");
	ret = req->send(CWebRequest::RQ_POST|CWebRequest::RQ_DEBUG);
	
	if(!ret){
		int len;
		char *p;
		u32 code;
		
		ret = -100;		
		if(!req->get_statuscode(&code)){
			len = req->get_responseheader("Set-Cookie",NULL,0);
			if(len && (p = (char *)malloc(1+len)) != NULL){
				req->get_responseheader("Set-Cookie",p,len);
				fb->set_Cookies(p);
				free(p);
			}
			ret--;
			len = fb->get_Cookie("c_user",s1,50);
			if(len){
				_buf = (char *)linearAlloc(len+10);
				strcpy(_buf,s1);
				ret = 0;
			}
		}
	}
	if(!ret)
		print("ok");
	else
		print("error %d",ret);
	delete req;
	sys_helper->set_Result(2,2,ret,_buf);
	return ret;
}
//---------------------------------------------------------------------------
static int on_clicked_login(CBaseWindow *w)
{
	CBaseWindow *b;
	char email[30],pass[30];
	
	pfn_State = 0;
	email[0]=0;
	pass[0]=0;
	b = bottom->get_Window(2);
	b->get_Text(email,30);
	b = bottom->get_Window(4);
	b->get_Text(pass,30);
	bottom->ShowDialog(console);	
/*	if(!email[0] || !pass[0]){
		print("Error !!!");
		return -1;
	}*/
	fb->set_Email(email);
	fb->set_Password(pass);
	top->ShowDialog(loaderDlg);	
	sys_helper->set_Job(2,1,sys_login);
	return 0;
}
//---------------------------------------------------------------------------
static int fb_login(u32 arg0)
{
	CBaseWindow *b;
	
	b = new CLabel("EMail");
	b->create(60,20,50,20,1);
	b->set_TextColor(0xff404040);
	bottom->add(b);
	
	b = new CEditText();
	b->create(115,20,110,20,2);	
	b->set_Text((char *)fb->get_Email());
	bottom->add(b);
	
	b = new CLabel("Password");
	b->create(60,45,50,20,3);
	b->set_TextColor(0xff404040);	
	bottom->add(b);
	
	b = new CEditText();
	b->create(115,45,110,20,4);	
	b->set_Text((char *)fb->get_Password());	
	bottom->add(b);
	
	b = new CButton("Connect");
	b->create(110,70,100,25,5);
	b->set_TextColor(0xff404040);	
	bottom->add(b);
	
	b->set_Events("clicked",(void *)on_clicked_login);

	return 0;
}
//---------------------------------------------------------------------------
static int sys_init(u32 arg0)
{	
	int ret;
	
	loader_img->load((u8 *)loader_bin);	
	toolbar_img->load((u8 *)toolbar_bin);	
	top->init();
	bottom->init();
	print("\nrequesting...");
	CWebRequest *req = new CWebRequest();	
	if(req == NULL)
		return -1;
	ret = req->begin("https://www.facebook.com/index.php");
	ret = req->send();
	print("%d\n",ret);
	if(!ret){
		int len;
		char *p;
		u32 code;
		
		if(!req->get_statuscode(&code)){
			len = req->get_responseheader("Set-Cookie",NULL,0);
			if(len && (p = (char *)malloc(++len)) != NULL){
				req->get_responseheader("Set-Cookie",p,len);
				fb->set_Cookies(p);
				free(p);
			}
		}		
	}
	delete req;
	sys_helper->set_Result(1,1,ret);
	return ret;
}
//---------------------------------------------------------------------------
CFBClient::CFBClient()
{
	email = "";
	pass = "";
	mode = 0;
}
//---------------------------------------------------------------------------
CFBClient::~CFBClient()
{
}
//---------------------------------------------------------------------------
int CFBClient::Initialize()
{
	fb = new CFBClient();
	if(!fb)
		return -1;
	linear_buffer = (u8 *)linearAlloc(0x80000);	
	if(!linear_buffer)
		return -2;
	sys_helper = new CSysHelper();
	if(!sys_helper || sys_helper->Initialize())
		return -3;
	sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);
	if(gui_init())
		return -4;	
	return 0;
}
//---------------------------------------------------------------------------
int CFBClient::Destroy()
{
	if(sys_helper){
		delete sys_helper;
		sys_helper = NULL;
	}
	if(linear_buffer){
		linearFree(linear_buffer);
		linear_buffer=NULL;
	}
	if(fb){
		delete fb;
		fb = NULL;
	}
	return 0;
}
//---------------------------------------------------------------------------
int CFBClient::main(u32 arg0)
{
	int ret;
	u8 *screen;
	
	ret=0;
	if(fb)
		ret=fb->Main(arg0);
	screen = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL,NULL);
	top->draw(screen);
	screen = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL,NULL);
	bottom->draw(screen);	
	return ret;
}
//---------------------------------------------------------------------------
int CFBClient::onTouchEvent(touchPosition *p,u32 flags)
{
	return bottom->onTouchEvent(p,flags);
}
//---------------------------------------------------------------------------	
int CFBClient::SetTimer(CTimer *p)
{
	if(p == NULL)
		return -1;
	for (std::vector<CTimer *>::iterator t = timers.begin(); t != timers.end(); ++t){
		if((*t) == p)
			return -2;
	}		
	timers.push_back(p);
	return 0;
}
//---------------------------------------------------------------------------	
int CFBClient::SetTimer(LPDEFFUNC f,u64 val,u32 p)
{
	CTimer *t = new CTimer(f,val,p);	
	return SetTimer(t);
}
//---------------------------------------------------------------------------	
int CFBClient::set_Email(char *str)
{
	if(!str)
		return -1;
	email = str;
	return 0;
}
//---------------------------------------------------------------------------	
int CFBClient::set_Password(char *str)
{
	if(!str)
		return -1;
	pass = str;
	return 0;
}
//---------------------------------------------------------------------------	
int CFBClient::add_Cookie(char *key,char *value)
{
	if(!key || !*key || !value || !*value)
		return -1;
	cookies[key] = value;
	return 0;
}
//---------------------------------------------------------------------------	
int CFBClient::get_Cookie(char *key,char *str,u32 size)
{
	if(!key || !*key || cookies.count(key) == 0)
		return 0;
	std::string s = cookies[key];
	int len = s.length();
	if(!str || size < len)
		return len;
	strcpy(str,s.c_str());
	return len;
}
//---------------------------------------------------------------------------	
int CFBClient::get_Cookies(char *str,u32 size,u32 flags)
{
	std::string s;
	
	for (std::map<std::string,std::string>::iterator t = cookies.begin(); t != cookies.end(); ++t){
		if(s.length())
			s += ";";
		if((flags & 1) == 0 && t->second == "deleted")
			continue;
		s += " "+t->first + "="+t->second;		
	}
	if(!str || size < s.length())
		return s.length();
	strcpy(str,s.c_str());
	return 0;
}
//---------------------------------------------------------------------------	
int CFBClient::onTimers(u32 id)
{
	switch(id){
		case 1:
		break;
		case 2:
		break;
	}
	printd("ontimer %lu",id);
	return 0;
}
//---------------------------------------------------------------------------	
int CFBClient::onClicked(u32 id)
{
	switch(id){
	}
	printd("click %lu",id);
	return 0;
}
//---------------------------------------------------------------------------	
static int on_timer(u32 val)
{
	if(fb == NULL)
		return 0;
	return fb->onTimers(val);
}
//---------------------------------------------------------------------------	
int CFBClient::get_buddy_list(u32 arg0)
{
	CWebRequest *req;
	u32 code,sz;
	int res,ret;
	u8 *_buf;
		
	res=-1;
	_buf = NULL;
	req = new CWebRequest();
	if(!req)
		goto fail;
	res--;
	req->begin("https://www.facebook.com/ajax/chat/buddy_list.php");
	req->add_postdata("user",fb->get_UserId());
	req->add_postdata("popped_out","true");
	req->add_postdata("force_render","true");
	req->add_postdata("buddy_list","1");
	req->add_postdata("__a","1");
	req->add_postdata("fb_dtsg",fb->get_dtsg());
	req->add_postdata("notifications","1");
	ret = fb->get_Cookies(NULL,0);
	if(ret > 0){
		char *p = (char *)malloc(ret+1);
		fb->get_Cookies(p,ret);
		req->add_header("Cookie",p);
		free(p);
	}	
	res--;
	if(req->send(CWebRequest::RQ_DEBUG|CWebRequest::RQ_POST))
		goto fail;
	res--;
	if(req->get_statuscode(&code))
		goto fail;
	res--;
	if(code != 200)		
		goto fail;
	res--;
	_buf = (u8 *)linearAlloc(0x20000);
	if(req->download_data((char *)_buf,0x20000,&sz))
		goto fail;
	res = 0;
fail:	
	if(req != NULL)
		delete req;
	if(res && _buf != NULL){
		linearFree(_buf);
		_buf=0;
	}
	sys_helper->set_Result(4,3,res,_buf,sz);
	return res;
}
//---------------------------------------------------------------------------	
int CFBClient::go_to_home(u32 arg0)
{
	CWebRequest *req;
	u32 code,sz;
	int res,ret;
	u8 *_buf;
		
	res=-1;
	_buf = NULL;
	req = new CWebRequest();
	if(!req)
		goto fail;
	res--;
	req->begin("https://www.facebook.com/index.php");
	ret = fb->get_Cookies(NULL,0);
	if(ret > 0){
		char *p = (char *)malloc(ret+1);
		fb->get_Cookies(p,ret);
		req->add_header("Cookie",p);
		free(p);
	}	
	res--;
	if(req->send(CWebRequest::RQ_DEBUG|CWebRequest::RQ_GET))
		goto fail;
	res--;
	if(req->get_statuscode(&code))
		goto fail;
	res--;
	if(code != 200)		
		goto fail;
	res--;
	_buf = (u8 *)linearAlloc(0x40000);
	if(req->download_data((char *)_buf,0x40000,&sz))
		goto fail;		
	res = 0;
fail:	
	if(req != NULL)
		delete req;
	if(res && _buf != NULL){
		linearFree(_buf);
		_buf=0;
	}
	sys_helper->set_Result(3,3,res,_buf,sz);
	return res;
}
//---------------------------------------------------------------------------	
int CFBClient::Main(u32 arg0)
{
	u32 val,*p;
	
	switch(mode){
		case -1:
		break;
		case 0:
			print("Initializing...");			
			sys_helper->set_Job(mode+1,1,sys_init);
			mode=-2;
		break;
		case 1:
			fb_login(0);
			mode=-2;
		break;
		case 2:
			fb_authenticate(0);
			SetTimer(on_timer,30000,1);
			SetTimer(on_timer,180000,2);
			sys_helper->set_Job(3,1,go_to_home);
			mode=-2;
		break;
		case 3:
			sys_helper->set_Job(4,1,get_buddy_list);
		break;
	}
	for (std::vector<CTimer *>::iterator t = timers.begin(); t != timers.end(); ++t)
		(*t)->onCounter();
	if(sys_helper->is_Busy())
		return 0;	
	val = sys_helper->get_Result(NULL,0);
	if(!val)
		return 0;
	p = (u32 *)malloc(val*sizeof(u32));
	sys_helper->get_Result(p,val);
	printd("res %u %u %d",p[0],p[1],(int)p[2]);
	switch(p[1]){
		case 1:
			print("ok\n");
			top->HideDialog();
			if(p[2])
				mode = -1;
			else{
				bottom->HideDialog();
				mode = p[1];
			}
		break;
		case 2:
			print("ok\n");
			top->HideDialog();
			if(p[2])
				mode = -1;
			else{
				bottom->HideDialog();
				mode = p[1];
			}
			if(p[3]){
				strcpy(userid,(char *)p[3]);
				linearFree((void *)p[3]);
			}
		break;
		case 3:
			if(p[2]){
				bottom->ShowDialog(console);
				mode=-1;
			}
			if(p[3]){
				char *s = strstr((char *)p[3],"name=\"fb_dtsg\"");
				
				if(s != NULL){
					int i2;
					
					s += 15;
					for(i2=0;*s != '\"';i2++,s++);
					
					for(s++,i2=0;*s != '\"';i2++)
						dtsg[i2] = *s++;
					dtsg[i2] = 0;					
					mode=3;
				}
				linearFree((void *)p[3]);
			}
		break;
		case 4:
			if(p[3]){
				parse_buddy_list((char *)p[3],p[4]);
				linearFree((void *)p[3]);
			}
			mode=-1;
		break;
	}
	free(p);
	return 0;
}
//---------------------------------------------------------------------------	
int CFBClient::parse_buddy_list(char *js,u32 sz)
{
	int nnodes,res,i,ii;
	jsmn_parser parser;
	jsmntok_t *t,*list,*user;
	
	write_to_sdmc("/buddylist.txt",(u8 *)js,sz);
	jsmn_init(&parser);
	nnodes = jsmn_parse(&parser, js, sz, NULL,0);
	if(nnodes < 0)
		return -1;
	t = (jsmntok_t *)linearAlloc(nnodes * sizeof(jsmntok_t));
	if(!t)
		return -2;
	jsmn_init(&parser);
	res = jsmn_parse(&parser,js,sz,t,nnodes);
	for(i=0;i<res;i++){
       if(t[i].type != JSMN_STRING)
           continue;
       if(strncmp(&js[t[i].start],"nowAvailableList",16) == 0)
           break;
	}
	printd("nodes %d %d %d",nnodes,i,res);
	list = &t[++i];
	for(ii=1,i=0;i<list->size;i++){
		user = &list[ii];
		char *id = &js[user[0].start];
		{
			char s[30];
			int n,nn;
			
			for(nn=0,n = user[0].start;n<user[0].end;n++,nn++)
				s[nn] = id[nn];
			s[nn] = 0;
			printd(s);
		}
		ii += json_nodes_length(&user[1]) + 1;
	}
	linearFree(t);
	return 0;
}
//---------------------------------------------------------------------------	
int CFBClient::json_nodes_length(jsmntok_t *t)
{
   int res;
    
   res=1;
   for(int i =0;i<t->size;i++)
	res += json_nodes_length(&t[res]);
   return res;
}
//---------------------------------------------------------------------------	
int CFBClient::set_Cookies(char *str)
{
	char *p,*p1,*p2;
	int len,i;

	if(!str || !*str)
		return -1;
	p = str;
	while((p1 = strtok(p,"\n")) != NULL){
		len = strlen(p1);
		if(!len)
           break;
		if(p1[len-1] == '\r')
			p1[len-1] = 0;
		if(!p1[0])
           break;
		p = p1 + len + 1;
		p2 = p1;
		i = 0;
		while((p1 = strtok(p2,";")) != NULL){
			char *p3;
	
			if(i)
				break;
			p2 = p1 + strlen(p1) + 1;
			i++;
			while((p3 = strtok(p1,"=")) != NULL){
				p3 = trim(p1 + strlen(p1) + 1); 
				p1 = trim(p1);
				if(strcmp(p3,"deleted")==0)
					cookies.erase(p1);
				else
					cookies[p1] = p3;
				break;
			}
		}
	}
	return 0;
}
