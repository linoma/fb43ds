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

LPDEFFUNC pfn_State = NULL;
CSysHelper *sys_helper = NULL;
u8 *linear_buffer = NULL;
FS_archive sdmcArchive;
CFBClient *fb;
static unsigned char fb_char_test[] = {"€,´,€,´,水,Д,Є"};
//---------------------------------------------------------------------------
static int on_show_console(CBaseWindow *w)
{
	bottom->ShowDialog(console);
	return 0;
}
//---------------------------------------------------------------------------
static int fb_authenticate(u32 arg0)
{
	if(!sys_helper || sys_helper->is_Busy())
		return -1;
	top->HideDialog();
	if(sys_helper->get_Result())
		return -2;		
	for(int i=0;i<6;i++)
		bottom->remove(i);
	CToolBar *b = new CToolBar();
	b->create(0,0,320,30,1);		
	b->set_BkColor(0x00c0c0c0);
	
	CToolButton *c = new CToolButton();
	c->create(0,0,22,22,1);
	c->load(toolbar_img,12);
	b->add(c);
	b->set_Events("clicked",(void *)on_show_console);
	bottom->add(b);
	bottom->HideDialog();
	return 0;
}
//---------------------------------------------------------------------------
static int sys_login(u32 arg0)
{
	char s[90],s1[90];
	int ret;
	
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
	ret = req->send(CWebRequest::RQ_POST);
	
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
			if(len)
				ret = 0;
		}
	}
	if(!ret)
		print("ok");
	else
		print("error %d",ret);
	delete req;
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
	pfn_State = fb_authenticate;
	top->ShowDialog(loaderDlg);	
	sys_helper->set_Worker(sys_login);
	return 0;
}
//---------------------------------------------------------------------------
static int fb_login(u32 arg0)
{
	CBaseWindow *b;
	
	if(!sys_helper || sys_helper->is_Busy())
		return -1;
	top->HideDialog();
	if(sys_helper->get_Result())
		return -2;		
	bottom->HideDialog();
	
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
	pfn_State = NULL;
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
	return ret;
}
//---------------------------------------------------------------------------
int fb_init(u32 arg0)
{
	if(gui_init())
		return -1;
	print("Initializing...");			
	linear_buffer = (u8 *)linearAlloc(0x80000);	
	if(!linear_buffer)
		return -2;
	sys_helper = new CSysHelper();
	if(!sys_helper || sys_helper->Initialize())
		return -3;
	sys_helper->set_Worker(sys_init);
	pfn_State = fb_login;
	print("ok\n");	
	return 0;
}
//---------------------------------------------------------------------------
CFBClient::CFBClient()
{
	email = "";
	pass = "";
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
	sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);	
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
int CFBClient::onTouchEvent(touchPosition *p,u32 flags)
{
	return bottom->onTouchEvent(p,flags);
}
//---------------------------------------------------------------------------	
int CFBClient::onTimersLoop()
{
	for (std::vector<CTimer *>::iterator t = timers.begin(); t != timers.end(); ++t)
		(*t)->onCounter();
	return 0;
}
//---------------------------------------------------------------------------	
int CFBClient::onMainLoop()
{
	u8 *screen;
	
	screen = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL,NULL);
	top->draw(screen);
	screen = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL,NULL);
	bottom->draw(screen);	
	fb->onTimersLoop();
	return 0;
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
	cookies[key]=value;
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
		if((flags&1) == 0 && t->second == "deleted")
			continue;
		s += " "+t->first + "="+t->second;		
	}
	if(!str || size < s.length())
		return s.length();
	strcpy(str,s.c_str());
	return 0;
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
