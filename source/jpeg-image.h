#include "images.h"

#ifndef __JPEGIMAGEH__
#define __JPEGIMAGEH__

class CImageJpeg : public CImage{
public:
	CImageJpeg();
	int load(u8 *src,int w=-1,int h=-1);
protected:
	virtual ~CImageJpeg();
};

#endif