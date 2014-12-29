#include "types.h"
#include "types.h"
#include "font.h"

#ifndef __GFXTEXTH__
#define __GFXTEXTH__

void gfxDrawText(u8* fb, font_s* f, char* str, LPRECT prc,u32 flags);
int gfxGetTextExtent(font_s* f,const char *str, LPSIZE psz);
void gfxSetTextColor(u32 col);
#endif