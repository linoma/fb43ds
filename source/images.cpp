#include "images.h"
#include "utils.h"

//---------------------------------------------------------------------------
CImage::CImage()
{
	buf = NULL;
	width = height = 0;
	alpha = 255;
	bd = NULL;
	refs = 1;
}
//---------------------------------------------------------------------------
CImage::~CImage()
{
	destroy();
}
//---------------------------------------------------------------------------
void CImage::release()
{
	if(--refs)
		return;
	delete this;
}
//---------------------------------------------------------------------------
u32 CImage::add_ref(u32 v)
{
	refs += v;
	return refs;
}
//---------------------------------------------------------------------------
int CImage::destroy()
{
	if(buf){
		linearFree(buf);
		buf = NULL;
	}
	bd = NULL;
	status = 0;
	return 0;
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
	*ret = (alpha << 24);
	*ret |= RGB(bd[0],bd[1],bd[2]);	
	bd += width * 3;
	return 0;
}
//---------------------------------------------------------------------------
int CImage::draw(u8 *dst,int x,int y,int w,int h,int x0,int y0)
{
	if(buf == NULL || dst == NULL || !(status & 1))
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
			p[1] = col >> 8;
			p[2] = col >> 16;
		}
		dst += 240*3;
	}
	return 0;
}
//---------------------------------------------------------------------------
CTimer::CTimer(LPDEFFUNC f,u64 i,u32 p)
{
	elapsed = 0;
	interval = 268123.480 * i;
	fnc = f;
	param = p;
	status = 0;
	set_Enabled(1);
}
//---------------------------------------------------------------------------
int CTimer::onCounter()
{
	u64 val;
	
	if((status & 1) == 0)
		return -1;
	val = svcGetSystemTick();
	u64 dt = val - elapsed;
	if(dt >= interval){	
		elapsed = val;
		if(fnc != NULL)
			fnc(param);
	}
	return 0;
}
//---------------------------------------------------------------------------
int CTimer::set_Param(u32 p)
{
	param = p;
	return 0;
}
//---------------------------------------------------------------------------
int CTimer::set_Enabled(int v)
{
	if(v){
		status |= 1;
		elapsed = svcGetSystemTick();
	}
	else
		status &= ~1;
	return 0;
}
//---------------------------------------------------------------------------
CAnimation::CAnimation() : CTimer(onTimer,200,(u32)this)
{
	set_Enabled(0);
	frame = 0;
}
//---------------------------------------------------------------------------
CAnimation::CAnimation(u64 v) : CTimer(onTimer,v,(u32)this)
{
	set_Enabled(0);
	frame = 0;
}
//---------------------------------------------------------------------------
int CAnimation::Start()
{
	set_Enabled(1);
	frame = 0;
	return 0;
}
//---------------------------------------------------------------------------
int CAnimation::Stop()
{
	set_Enabled(0);
	return 0;
}
//---------------------------------------------------------------------------
int CAnimation::onTimer(u32 param)
{
	return ((CAnimation *)param)->onTimer();
}