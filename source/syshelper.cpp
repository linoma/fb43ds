#include "syshelper.h"
#include <stdlib.h>
#include <stdarg.h>

//---------------------------------------------------------------------------
CSysHelper::CSysHelper()
{
	ev[0] = ev[1] = 0;
	mutex=0;
	thread = 0;
	status = 0;
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
	u32 cmd,size,flags,*p;
	LPDEFFUNC fn;
	
	while((status & 1) == 0){
		svcWaitSynchronization(ev[0],U64_MAX);
		svcClearEvent(ev[0]);
		while(workers.size()){
			svcWaitSynchronization(mutex,U64_MAX);
			size = workers.front();			
			workers.pop();
			cmd = workers.front();
			workers.pop();
			flags = size >> 16;
			size = (u32)(u16)size;
			p = NULL;
			if(size){
				if((p = (u32 *)malloc(size*sizeof(u32)))){
					for(u32 i=0;i<size;i++){
						p[i] = workers.front();
						workers.pop();
					}
				}
			}
			svcReleaseMutex(mutex);
			switch(cmd){
				default:
					fn = (LPDEFFUNC)p[0];
					if(fn != NULL)
						fn(0);
				break;
			}
			if(p != NULL)
				free(p);
		}
		svcSignalEvent(ev[1]);
	}
}
//---------------------------------------------------------------------------
int CSysHelper::get_Result(u32 *buf,u32 size)
{
	u32 val,i;
	
	svcWaitSynchronization(mutex,U64_MAX);
	val = results.front() + 1;	
	if(val <= size && buf){
		*buf++ = val;
		results.pop();
		*buf++ = val;
		results.pop();	
		for(i=0;i<val;i++){
			*buf++ = results.front();
			results.pop();
		}
	}
	svcReleaseMutex(mutex);
	return val;	
}
//---------------------------------------------------------------------------
int CSysHelper::set_Result(u32 command,u32 size,...)
{
	u32 i,val;
	va_list vl;
    
	svcWaitSynchronization(mutex,U64_MAX);
	results.push(size);
	results.push(command);
	va_start(vl,size);
	size = (u32)(u16)size;
	for(i=0;i<size;i++)
       results.push(va_arg(vl,u32));
	va_end(vl); 
	svcReleaseMutex(mutex);
	return 0;
}
//---------------------------------------------------------------------------
int CSysHelper::set_Worker(u32 command,u32 size,...)
{
	u32 i,val;
	va_list vl;
    
	svcWaitSynchronization(mutex,U64_MAX);
	workers.push(size);
	workers.push(command);
	va_start(vl,size);
	size = (u32)(u16)size;
	for(i=0;i<size;i++)
       workers.push(va_arg(vl,u32));
	va_end(vl); 
	svcSignalEvent(ev[0]);
	svcReleaseMutex(mutex);
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
	if(svcCreateMutex(&mutex, false))
		goto err_init;
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
	if(mutex){
		svcCloseHandle(mutex);
		mutex=0;
	}
	return 0;
}
