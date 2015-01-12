#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "gui.h"
#include "webrequest.h"
#include "syshelper.h"

LPDEFFUNC pfn_State = NULL;
CSysHelper *sys_helper = NULL;
u8 *linear_buffer = NULL;

int widgets_draws();
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
/*CContainerWindow *c = new CStatusBar();	
	c->create(0,220,400,20,2);	
	top->add(c);
	
	c = new CMenuBar();
	c->create(0,0,320,20,1);
	bottom->add(c);
	
	c = new CStatusBar();	
	c->create(0,220,320,20,3);	
	bottom->add(c);*/
	
	b = new CEditText();
	b->create(105,20,110,20,3);	
	bottom->add(b);
	
	b = new CEditText();
	b->create(105,45,110,20,4);	
	bottom->add(b);
	
	b = new CButton("Connect");
	b->create(110,70,100,20,5);
	bottom->add(b);
	
	//top->HideDialog();
	//bottom->HideDialog();
	pfn_State = NULL;
	return 0;
}
//---------------------------------------------------------------------------
static int sys_init(u32 arg0)
{	
	int ret;
	
	top->init();
	bottom->init();
	print("\nrequesting...");
	CWebRequest *req = new CWebRequest();	
	if(req == NULL)
		return -1;
	ret = req->begin("http://www.facebook.com/index.php");
	print("%d\n",ret);
	ret = req->send();
	print("%d\n",ret);
	return ret;
}
//---------------------------------------------------------------------------
int fb_init(u32 arg0)
{
	Result rc;
	int ret;
	
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
int fb_destroy(u32 arg0)
{
	if(sys_helper){
		delete sys_helper;
		sys_helper=NULL;
	}
	if(linear_buffer){
		linearFree(linear_buffer);
		linear_buffer=NULL;
	}	
	return 0;
}
