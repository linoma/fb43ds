#include <3ds.h>
#include "types.h"
#include "gfxdraw.h"

#ifndef __IMAGESH__
#define __IMAGESH__

//---------------------------------------------------------------------------
class CTimer {
public:
	CTimer(LPDEFFUNC f,u64 i,u32 p);
	virtual ~CTimer(){};
	int onCounter();
	int set_Param(u32 p);
	int set_Enabled(int v);
protected:
	u64 elapsed,interval;
	LPDEFFUNC fnc;
	u32 param,status;
};
//---------------------------------------------------------------------------
class CAnimation : public CTimer{
public:
	CAnimation();
	CAnimation(u64 v);
	virtual ~CAnimation(){};
	virtual int Start();
	virtual int Stop();
protected:
	virtual int onTimer()=0;
	static int onTimer(u32 param);
	int frame;
};
//---------------------------------------------------------------------------
class CImage{
public:
	CImage();
	virtual int load(u8 *src,int w=-1,int h=-1);
	virtual int draw(u8 *dst,int x,int y,int w=-1,int h=-1,int x0=0,int y0=0);
	virtual int begin_draw(int x=0,int y=0);
	virtual int get_pixel(u32 *ret,int f=0,int flags=1);
	int get_Width(){return width;};
	int get_Height(){return height;};
	void set_Alpha(int v){alpha=v;};
	u32 add_ref(u32 v=1);
	void release();	
protected:	
	virtual ~CImage();
	virtual int destroy();
	u8 *buf,*bd;
	int width,height,format,alpha;
	u32 status,refs;
};

#endif