#include "types.h"
#include "types.h"
#include "font.h"

#ifndef __GFXTEXTH__
#define __GFXTEXTH__

void drawString(u8* fb, font_s* f, char* str, s16 x, s16 y, u16 w, u16 h);
void gfxDrawText(gfxScreen_t screen, gfx3dSide_t side, font_s* f, char* str, s16 x, s16 y);
int gfxGetTextExtent(font_s* f,const char *str, LPSIZE psz);
void gfxSetTextColor(u32 col);
#endif