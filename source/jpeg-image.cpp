#include "jpeg-image.h"
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

extern "C" u8 *linear_buffer;
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
int CImageJpeg::load(u8 *src,u32 sz,int w,int h)
{	
	jpeg_memory_stream *stream;
	jpeg_decoder_ptr_t decoder;
	int res,f_wstep,f_ihstep,y,x1,x2,j,bpp,nc;
	
	res=-1;
	if(!src)
		return res;
	decoder=0;
	res--;
    stream = new jpeg_memory_stream(src,sz);
	if(!stream)
		goto esci;
	res--;
	decoder = new jpeg_decoder(stream);
	if(!decoder)
		goto esci;
	res--;
    width = decoder->get_width();
	height = decoder->get_height();
	nc = decoder->get_num_components();
	bpp = decoder->get_bytes_per_pixel();
	if(w == -1)
		w = width;
	if(h == -1)
		h = height;	
	f_wstep = (width << 12) / w;
	f_ihstep = (h << 12) / height;
	if(decoder->begin())
		goto esci;
	res--;
	buf = (u8 *)linearAlloc(w*h*3);
	if(!buf)
		goto esci;
	res--;
	y=0;
    for(;;){
		const void *pScan_line;
		u32 scan_line_len;
		u8 *sp,*p,*pp;
		
		if (decoder->decode(&pScan_line, &scan_line_len))
			break;
		sp = (u8 *)pScan_line;
		for(x1=x2=0;x2 < w;x2++){
			p=&buf[((y >> 12) * w + x2)*3];
			pp=&sp[(x1 >> 12)*4];
			p[0]=pp[0];p[1]=pp[1];p[2]=pp[2];
			x1 += f_wstep;
		}
		x1 = y;
		y += f_ihstep;
		x2 = (y - x1) >> 12;
		for(j=1;x2 > 1;x2--,j++)
			memcpy(&buf[((x1 >> 12) + j) * w*3],&buf[(x1 >> 12) * w*3],w*3);
    }
	status |=1;
	res=0;
esci:	
	if(decoder)
		delete decoder;
	if(stream)
		delete stream;
	if(res)
		destroy();
	return res;
}
//---------------------------------------------------------------------------
jpeg_memory_stream::jpeg_memory_stream(u8 *p,u32 sz) : jpeg_decoder_stream()
{
	mem = p;
	size = sz;
	ofs = 0;
}
//---------------------------------------------------------------------------
int jpeg_memory_stream::read(u8 *Pbuf, int max_bytes_to_read, bool *Peof_flag)
{
	u32 n;
	
	n = size-ofs;
	if(n > max_bytes_to_read)
		n = max_bytes_to_read;
	if(!n)
		*Peof_flag=true;
	else{
		memcpy(Pbuf,&mem[ofs],n);
		ofs += n;
	}
	return n;
}
