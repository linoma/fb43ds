#include "webrequest.h"

static u32 client;
//---------------------------------------------------------------------------
CWebRequest::CWebRequest()
{
	sk=-1;
	_buf=NULL;
}
//---------------------------------------------------------------------------
CWebRequest::~CWebRequest()
{
	destroy();
}
//---------------------------------------------------------------------------
int CWebRequest::destroy()
{
	return 0;
}	
//---------------------------------------------------------------------------
int CWebRequest::get_StatusCode()
{
	return -1;
}
//---------------------------------------------------------------------------
int CWebRequest::IntializeClient()
{
	if(!(client&1)){
	//SOC_Initialize((u32*)memalign(0x1000, 0x100000), 0x100000);
		client |= 1;
	}
	if(!(client & 2)){
		CyaSSL_Init();
		client |= 2;
	}
	return 0;
}
//---------------------------------------------------------------------------
int CWebRequest::DestroyClient()
{
	if(client&2){
	}
	if((client&1)){
		SOC_Shutdown();
	}
	return 0;
}