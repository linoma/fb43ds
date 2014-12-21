#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "gui.h"

extern "C" int (*pfn_State)();

//---------------------------------------------------------------------------
static int fb_login()
{
	return 0;
}
//---------------------------------------------------------------------------
extern "C" int fb_init()
{
	CWindow *b;
	
	if(gui_init())
		return -1;
	
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
	b->create(105,50,110,20,3);	
	bottom->add(b);
	
	b = new CEditText();
	b->create(105,80,110,20,4);	
	bottom->add(b);
	
	b = new CButton();
	b->create(110,120,100,15,5);
	bottom->add(b);
		
	pfn_State = fb_login;
	return 0;
}
//---------------------------------------------------------------------------
int fb_destroy()
{
	return 0;
}
