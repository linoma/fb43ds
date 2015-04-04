#include "images.h"
#include "jpegdecoder.h"

#ifndef __JPEGIMAGEH__
#define __JPEGIMAGEH__

class jpeg_memory_stream : public jpeg_decoder_stream{	
public:
	jpeg_memory_stream(u8 *p,u32 sz);
	virtual int read(u8 *Pbuf, int max_bytes_to_read, bool *Peof_flag);
protected:
	u8 *mem;
	u32 size,ofs;
};

class CImageJpeg : public CImage{
public:
	CImageJpeg();
	int load(u8 *src,u32 sz,int w=-1,int h=-1);
protected:
	virtual ~CImageJpeg();
};

#endif