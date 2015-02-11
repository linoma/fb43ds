#include <3ds.h>
#include "types.h"
#include <queue>

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
	int set_Worker(u32 command,u32 size,...); 
	int set_Result(u32 command,u32 size,...); 
	int get_Result(u32 *buf,u32 size);
protected:
	Handle ev[2],thread,mutex;
	u8 stack[0x8000] __attribute__((aligned(8)));
	u32 status;
	int result;
	std::queue<u32> workers,results;
};
#endif