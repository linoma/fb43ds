#include "types.h"
#include "types.h"
#include "font.h"

#ifndef __GFXTEXTH__
#define __GFXTEXTH__

#define DT_CENTER		2
#define DT_VCENTER		1
#define DT_SINGLELINE	4

void gfxDrawText(u8* fb, font_s* f, char* str, LPRECT prc,u32 flags);
int gfxGetTextExtent(font_s* f,const char *str, LPSIZE psz,u32 flags=0);
void gfxSetTextColor(u32 col);

#endif