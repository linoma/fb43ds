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

#define RGB(a,b,c) 		((((u8)a)<<16)|(((u8)b)<<8)|((u8)c))
#define MAKELONG(a,b) 	((a << 16)|((u16)b))
#define MAKESHORT(a,b) 	((((u8)a)<<8)((u8)b))
#define InflateRect(rc,a,b) rc.left += a;\
	rc.right -= a;\
	rc.top += b;\
	rc.bottom -= b;

#define CopyRect(b,a) b.left = a.left;\
	b.top=a.top;\
	b.right=a.right;\
	b.bottom=a.bottom;
	
#endif
