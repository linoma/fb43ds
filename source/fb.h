#include <3ds.h>
#include "types.h"

#ifndef __FBH__
#define __FBH__

int fb_init(u32 arg0);

class CFBClient{
public:
	CFBClient();
	virtual ~CFBClient();
	static int Initialize();
	static int Destroy();
	static int onTouchEvent(touchPosition *p,u32 flags);
	
};
extern CFBClient *fb;
#endif