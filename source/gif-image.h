#include "images.h"
#include <gif_lib.h>

#ifndef __GIFIMAGEH__
#define __GIFIMAGEH__

class CImageGif : public CImage{
public:
	CImageGif();
	int load(u8 *src,int w=-1,int h=-1);
	int get_pixel(u32 *ret,int f=0,int flags=1);
	int begin_draw(int x=0,int y=0);
protected:
	static int readFunc(GifFileType* GifFile, GifByteType* buf, int count);
	int destroy();
	u32 *palette;
	int bk_idx;
};

#endif