#include "syshelper.h"

//---------------------------------------------------------------------------
CSysHelper::CSysHelper()
{
	ev[0] = ev[1] = 0;
	thread = 0;
	status = 0;
	pfn_SysFunc = NULL;
	result = -1;
}
//---------------------------------------------------------------------------
CSysHelper::~CSysHelper()
{
	Destroy();
}
//---------------------------------------------------------------------------
void CSysHelper::main(u32 arg0)
{
	((CSysHelper *)arg0)->onMain();
	svcExitThread();
}
//---------------------------------------------------------------------------
void CSysHelper::onMain()
{
	while((status&1)==0){
		svcWaitSynchronization(ev[0],U64_MAX);
		svcClearEvent(ev[0]);
		if(pfn_SysFunc){
			result = pfn_SysFunc(0);
			pfn_SysFunc = NULL;
		}
		svcSignalEvent(ev[1]);
	}
}
//---------------------------------------------------------------------------
int CSysHelper::set_Worker(LPDEFFUNC fn)
{
	pfn_SysFunc=fn;
	svcSignalEvent(ev[0]);
	return 0;
}
//---------------------------------------------------------------------------
int CSysHelper::Initialize()
{
	int res;
	Result rc;
	
	res = -1;
	for(int i = 0;i<sizeof(ev)/sizeof(Handle);i++){
		rc = svcCreateEvent(&ev[i],0);	
		if(rc)
			goto err_init;
	}
	res--;
	if(svcCreateThread(&thread,main,(u32)this,(u32*)(stack+sizeof(stack)),0x30,0xfffffffe))
		goto err_init;
	return 0;
err_init:
	Destroy();
	return res;
}
//---------------------------------------------------------------------------
int CSysHelper::is_Busy()
{
	if(!ev[1])
		return -1;
	if(svcWaitSynchronization(ev[1],0))
		return -2;
	svcClearEvent(ev[1]);
	return 0;
}
//---------------------------------------------------------------------------
int CSysHelper::Destroy()
{
	status |= 1;
	if(ev[0])
		svcSignalEvent(ev[0]);
	if(thread){
		svcWaitSynchronization(thread,U64_MAX);
		
	}
	for(int i = 0;i<sizeof(ev)/sizeof(Handle);i++){
		if(!ev[i])
			continue;
		svcCloseHandle(ev[i]);
		ev[i] = 0;
	}
	return 0;
}
