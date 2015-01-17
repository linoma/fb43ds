#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "gui.h"
#include "webrequest.h"
#include "syshelper.h"
#include "fb.h"
#include "loader_bin.h"

LPDEFFUNC pfn_State = NULL;
CSysHelper *sys_helper = NULL;
u8 *linear_buffer = NULL;
FS_archive sdmcArchive;
CFBClient *fb;
//---------------------------------------------------------------------------
static int sys_login(u32 arg0)
{
	return -1;
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
	if(!email[0] || !pass[0]){
		bottom->ShowDialog(console);		
		print("Error !!!");
		return -1;
	}
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
	bottom->add(b);
	
	b = new CLabel("Password");
	b->create(60,45,50,20,3);
	b->set_TextColor(0xff404040);	
	bottom->add(b);
	
	b = new CEditText();
	b->create(115,45,110,20,4);	
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
	top->init();
	bottom->init();
	print("\nrequesting...");
	CWebRequest *req = new CWebRequest();	
	if(req == NULL)
		return -1;
	ret = req->begin("http://www.facebook.com/index.php");
	ret = req->send();
	print("%d\n",ret);
	delete req;
	ret=0;
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
		sys_helper=NULL;
	}
	if(linear_buffer){
		linearFree(linear_buffer);
		linear_buffer=NULL;
	}
	if(fb){
		delete fb;
		fb=0;
	}
	return 0;
}
//---------------------------------------------------------------------------
int CFBClient::onTouchEvent(touchPosition *p,u32 flags)
{
	return bottom->onTouchEvent(p,flags);
}