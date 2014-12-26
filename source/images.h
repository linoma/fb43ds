#include <3ds.h>
#include "types.h"
#include "gfxdraw.h"

#ifndef __IMAGESH__
#define __IMAGESH__

class CImage{
public:
	CImage();
	virtual ~CImage();
	virtual int load(u8 *src,int w=-1,int h=-1);
	virtual int draw(u8 *dst,int x,int y,int w=-1,int h=-1,int x0=0,int y0=0);
	virtual int begin_draw(int x=0,int y=0);
	virtual int get_pixel(u32 *ret,int f=0,int flags=1);
	int get_Width(){return width;};
	int get_Height(){return height;};
protected:
	virtual int destroy();
	u8 *buf,*bd;
	int width,height,format;
};

#endif