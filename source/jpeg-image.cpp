#include "jpeg-image.h"
#include "gba-jpeg-decode.h"

//---------------------------------------------------------------------------
CImageJpeg::CImageJpeg() : CImage()
{
}
//---------------------------------------------------------------------------
CImageJpeg::~CImageJpeg()
{
	destroy();
}
//---------------------------------------------------------------------------
int CImageJpeg::load(u8 *src,int w,int h)
{
	JPEG_Decoder decoder;
	JPEG_FrameHeader *frame;
	u32 data[10];
	
    if(!JPEG_Decoder_ReadHeaders(&decoder,(const unsigned char **)&src))
        return -1;
	return -1;
}
