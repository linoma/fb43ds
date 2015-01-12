#include <string.h>
#include "gif-image.h"
#include <stdio.h>
#include <stdlib.h>

extern "C" u8 *linear_buffer;
static const short InterlacedOffset[] = {0,4,2,1};
static const short InterlacedJumps[] = {8,8,4,2}; 
//---------------------------------------------------------------------------
int CImageGif::readFunc(GifFileType* GifFile, GifByteType* buf, int count)
{
   char* ptr = (char*)GifFile->UserData;
   memcpy(buf,ptr,count);
   GifFile->UserData = ptr + count;
   return count;
}
//---------------------------------------------------------------------------
CImageGif::CImageGif() : CImage()
{
	palette = NULL;
	bk_idx=-1;
}
//---------------------------------------------------------------------------
int CImageGif::destroy()
{
	palette = NULL;
	bk_idx = -1;
	return CImage::destroy();	
}
//---------------------------------------------------------------------------
int CImageGif::get_pixel(u32 *ret,int f,int flags)
{
	int idx;
	
	idx = *bd;
	*ret = idx == bk_idx ? 0 : 0xff000000;
	*ret |= palette[*bd];
	bd+= width;
	return 0;
}
//---------------------------------------------------------------------------
int CImageGif::begin_draw(int x,int y)
{
	bd = NULL;
	if(buf == NULL)
		return -1;
	bd = &buf[x+y*width];
	return 0;
}
//---------------------------------------------------------------------------
int CImageGif::load(u8 *src,int w,int h)
{
	GifFileType *GifFile;
	GifRecordType RecordType;
	GifByteType *Extension;
	ColorMapObject *ColorMap;
	GifPixelType *LineBuf;
	int ExtCode,f_wstep,f_ihstep,i,j,x1,x2,i1,y,res;
	
	if(!src)
		return -1;
	GifFile = DGifOpen((void *)src,readFunc);
	if(GifFile == NULL)
		return -2;
	destroy();
	palette = NULL;
	res = -3;
	LineBuf = (GifPixelType *)linear_buffer;
	do{
		DGifGetRecordType(GifFile, &RecordType);	
		switch(RecordType){
			case IMAGE_DESC_RECORD_TYPE:
				DGifGetImageDesc(GifFile);
				width = GifFile->Image.Width;
				height = GifFile->Image.Height;
				if(w == -1)
					w = width;
				if(h == -1)
					h = height;	
				f_wstep = (width << 12) / w;
				f_ihstep = (h << 12) / height;
				if(buf == NULL){
					ColorMap = GifFile->Image.ColorMap != NULL ? GifFile->Image.ColorMap : GifFile->SColorMap;
					buf = (u8 *)linearAlloc(w*h + ColorMap->ColorCount * sizeof(u32));
					if(buf == NULL)
						goto ex_load;
					palette = (u32 *)&buf[w*h];		
					i = ColorMap->ColorCount;
					while(--i >= 0){
						GifColorType* pColor = &ColorMap->Colors[i];
						palette[i] = RGB(pColor->Red,pColor->Green, pColor->Blue);
					}					
				}
				
				if(GifFile->Image.Interlace) {
					for(i = 0;i < 4;i++){
						j = InterlacedOffset[i];
						y = j * f_ihstep;
						for(;j < h;j += InterlacedJumps[i]){						
							DGifGetLine(GifFile,LineBuf,width);						
							for(x1 = x2 = 0;x2 < w;x2++){
								buf[(y >> 12) * w + x2] = LineBuf[x1 >> 12];
								x1 += f_wstep;
							}
							x1 = y;
							x2 = y + f_ihstep;
							x2 = (x2 - x1) >> 12;
                            for(i1=1;x2 > 1;x2--,i1++)
                               memcpy(&buf[((x1 >> 12) + i1) * w],&buf[(x1 >> 12) * w],w);
							y += InterlacedJumps[i] * f_ihstep;
						}
					}
				}
				else{
					for(y = i = 0; i < h;i++) {
						DGifGetLine(GifFile,LineBuf,width);
						for(x1=x2=0;x2 < w;x2++){
							buf[(y >> 12) * w + x2] = LineBuf[x1 >> 12];
							x1 += f_wstep;
						}
						x1 = y;
						y += f_ihstep;
						x2 = (y - x1) >> 12;
						for(j=1;x2 > 1;x2--,j++)
							memcpy(&buf[((x1 >> 12) + j) * w],&buf[(x1 >> 12) * w],w);
					}
				}
			break;
			case EXTENSION_RECORD_TYPE:
				DGifGetExtension(GifFile, &ExtCode, &Extension);
				if(ExtCode == GRAPHICS_EXT_FUNC_CODE && Extension[0] > 3){
					if ((Extension[1] & 1) != 0)
						bk_idx = Extension[4];
				}
				while (Extension != NULL) {
					if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR)
						break;
				}				
			break;
			case TERMINATE_RECORD_TYPE:
				res = 0;
			break;
			default:
			break;
		}
	}while(RecordType != TERMINATE_RECORD_TYPE);
ex_load:
	if(res)
		destroy();
	DGifCloseFile(GifFile);
	return res;	
}
