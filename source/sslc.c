#include <string.h>
#include <3ds.h>
#include "sslc.h"

Handle __sslc_servhandle = 0;


/*Result sslcInit()
{
	Result ret=0;

	if(__sslc_servhandle)
		return 0;
	if((ret=srvGetServiceHandle(__sslc_servhandle,"ssl:C")))
		return ret;

	//*((u32*)0x600) = __httpc_servhandle;
	ret = SSLC_Initialize(__sslc_servhandle);
	if(ret!=0)
		return ret;
	return 0;
}

void sslcExit()
{
	if(__sslc_servhandle)
		return;
	svcCloseHandle(__sslc_servhandle);
	__sslc_servhandle = 0;
}

Result SSLOpenContext(sslcContext *context, char* url, u32 use_defaultproxy)
{
	Result ret=0;
	
	if((ret = sslcInit()))
		return ret;	
	ret = SSLC_CreateContext(__sslc_servhandle, url, &context->sslhandle);
	if(ret!=0)
		return ret;
	ret = srvGetServiceHandle(&context->servhandle, "ssl:C");
	if(ret!=0)
		return ret;


	return 0;
}

Result sslcCloseContext(sslcContext *context)
{
/*	Result ret=0;

	ret = SSLC_CloseContext(context->servhandle, context->httphandle);

	svcCloseHandle(context->servhandle);

	return ret;*/
/*}

Result SSLC_CreateContext(Handle handle, int fd, char* url, Handle* contextHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	u32 l=strlen(url)+1;

	cmdbuf[0]=0x200C2; //request header code
	cmdbuf[1]=fd;
	cmdbuf[2]=0x01; //unk
	cmdbuf[3]=l;	
	cmdbuf[4]=(l<<4)|0xA;
	cmdbuf[5]=(u32)url;	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))
		return ret;	
	if(contextHandle)
		*contextHandle=cmdbuf[2];
	return cmdbuf[1];
}

Result SSLC_Initialize(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x10002; //request header code
	cmdbuf[1]=0x1000; //unk
	cmdbuf[2]=0x20;//processID header, following word is set to processID by the arm11kernel.
	cmdbuf[4]=0;
	cmdbuf[5]=0;//Some sort of handle.
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))
		return ret;

	return cmdbuf[1];
}*/