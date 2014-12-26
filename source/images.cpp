#include "images.h"

//---------------------------------------------------------------------------
CImage::CImage()
{
	buf = NULL;
	width = height = 0;
	bd = NULL;
}
//---------------------------------------------------------------------------
CImage::~CImage()
{
	destroy();
}
//---------------------------------------------------------------------------
int CImage::destroy()
{
	if(buf){
		linearFree(buf);
		buf = NULL;
	}
	bd=NULL;
}
//---------------------------------------------------------------------------
int CImage::load(u8 *src,int w,int h)
{	
	return -1;
}
//---------------------------------------------------------------------------
int CImage::begin_draw(int x,int y)
{
	bd = NULL;
	if(buf == NULL)
		return -1;
	bd = &buf[(x+y*width)*3];
	return 0;
}
//---------------------------------------------------------------------------
int CImage::get_pixel(u32 *ret,int f,int flags)
{
	*ret = 0xFF000000;
	*ret |= RGB(bd[0],bd[1],bd[2]);	
	bd += width * 3;
	return 0;
}
//---------------------------------------------------------------------------
int CImage::draw(u8 *dst,int x,int y,int w,int h,int x0,int y0)
{
	if(buf == NULL || dst == NULL)
		return -1;
	dst += (239-y+x*240)*3;
	if(w == -1)
		w = width;
	if(h == -1)
		h = height;
	for(x=0;x<w;x++){
		u8 *p = dst;
		begin_draw(x0+x,y0);
		for(y=0;y<h;y++,p -= 3){
			u32 col;
			
			get_pixel(&col);
			if((col >> 24) == 0)
				continue;
			p[0] = col;
			p[1] = col >>8;
			p[2] = col >>16;
		}
		dst += 240*3;
	}
	return 0;
}
