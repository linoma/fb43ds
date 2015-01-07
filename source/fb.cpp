#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "gui.h"
#include "loader_bin.h"
#include "ssl_cert_bin.h"

extern "C" int (*pfn_State)();
static Handle sysEvent[2]={0},systhread = 0;
static int sysRunEvents = 0;
static u8 systhreadstack[0x4000] __attribute__((aligned(8)));
int (*pfn_SysFunc)();
extern "C" int widgets_draws();
Result HTTPC_AddTrustedRootCA(Handle handle, Handle contextHandle, u8* cert, int size);

//---------------------------------------------------------------------------
int sys_is_busy()
{
	if(svcWaitSynchronization(sysEvent[1],0))
		return -1;
	svcClearEvent(sysEvent[1]);
	return 0;
}
//---------------------------------------------------------------------------
static int fb_login()
{
	CBaseWindow *b;

	if(sys_is_busy())
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
static int sys_init()
{	
	Result ret;
	httpcContext context;		
	u32 statuscode,contentsize;
	
	statuscode = contentsize = 0;
	loader->load((u8 *)loader_bin);
	loader->Start();
	keyboard->init(bottom);
	print("\nrequesting...");
	ret = httpcOpenContext(&context, "https://www.sslshopper.com/", 0);
	if(!ret){
		statuscode = 1;
		httpcAddRequestHeaderField(&context,"User-Agent","Opera/9.50 (Windows NT 5.1; U; it)");
		ret = HTTPC_AddTrustedRootCA(context.servhandle,context.httphandle,(u8 *)ssl_cert_bin,ssl_cert_bin_size);
		statuscode = 2;
		if(!ret){
			ret = httpcBeginRequest(&context);
			if(!ret){
				statuscode = 3;
				ret = httpcGetResponseStatusCode(&context,&statuscode,0);
				if(statuscode == 200){				
					if(!httpcGetDownloadSizeState(&context, NULL, &contentsize)){
					//ret = httpcDownloadData(context, buf, contentsize, NULL);
					}
				
				}
			}
		}
		httpcCloseContext(&context);
	}
	{
		char s[100];
		
		sprintf(s,"status %08x %08x %08x",statuscode,contentsize,ret);
		print(s);
	}
	pfn_SysFunc = pfn_State=NULL;
	svcSignalEvent(sysEvent[1]);
	return 0;
}
//---------------------------------------------------------------------------
static void SYSThread(u32 arg0)
{
	while (sysRunEvents){
		svcWaitSynchronization(sysEvent[0],U64_MAX);
		svcClearEvent(sysEvent[0]);
		if(pfn_SysFunc)
			pfn_SysFunc();			
	}
	svcExitThread();
}
extern "C" u8 *linear_buffer;

extern "C" Result ACU_cmd26(Handle* servhandle, u32 *ptr, u8 val);
extern "C" Result ACU_CreateDefaultConfig(Handle* servhandle, u32 *ptr);
//---------------------------------------------------------------------------
extern "C" int fb_init()
{
	Result rc;
	
	if(gui_init())
		return -1;
	print("Initializing...");
	acInit();
	rc = ACU_CreateDefaultConfig(0,(u32 *)linear_buffer);
	print("\n->%d",rc);
	rc=ACU_cmd26(0,(u32 *)linear_buffer,0);
	print("\n->%d",rc);
	httpcInit();	
		
	for(int i = 0;i<sizeof(sysEvent)/sizeof(Handle);i++){
		rc = svcCreateEvent(&sysEvent[i],0);	
	}
	sysRunEvents = 1;
	//if(svcCreateThread(&systhread,SYSThread,0,(u32*)(systhreadstack+sizeof(systhreadstack)),0x30,0xfffffffe))
	//	return -2;
	print("ok\n");	
	svcSignalEvent(sysEvent[0]);
	pfn_SysFunc = sys_init;
	pfn_State = sys_init;
	//pfn_State = NULL;
	return 0;
}
//---------------------------------------------------------------------------
extern "C" int fb_destroy()
{
	sysRunEvents = 0;
	svcSignalEvent(sysEvent[0]);
	return 0;
}
Result HTTPC_AddTrustedRootCA(Handle handle, Handle contextHandle, u8* cert, int size)
{
	//return 0;
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x240082; //request header code
	cmdbuf[1]=contextHandle;
	cmdbuf[2]=size;
	cmdbuf[3]=(size<<4)|0xa;
	cmdbuf[4]=(u32)cert;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}