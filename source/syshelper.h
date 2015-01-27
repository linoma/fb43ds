#include <3ds.h>
#include "types.h"

#ifndef __CSYSHELPER__
#define __CSYSHELPER__

class CSysHelper{
public:
	CSysHelper();
	virtual ~CSysHelper();
	static void main(u32 arg0);
	void onMain();
	int Initialize();
	int Destroy();
	int is_Busy();
	int set_Worker(LPDEFFUNC fn,u32 arg0 = 0); 
	int get_Result(){return result;};
protected:
	Handle ev[2],thread;
	u8 stack[0x8000] __attribute__((aligned(8)));
	u32 status,param;
	int result;
	LPDEFFUNC pfn_SysFunc;
};
#endif