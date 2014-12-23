#include <3ds.h>

#ifndef __TYPESH__
#define __TYPESH__

typedef struct __RECT{
	s32 left,top,right,bottom;
} RECT,*LPRECT;

typedef struct __POINT{
	s32 x,y;
} POINT,*LPPOINT;

typedef struct __SIZE{
	s32 cx,cy;
} SIZE,*LPSIZE;

typedef int (*LPDEFFUNC)(u32);

#endif
