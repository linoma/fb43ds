#include "types.h"

#ifndef __GFXDRAWH__
#define __GFXDRAWH__

#ifdef __cplusplus
extern "C" {
#endif

void gfxPixel(int x, int y, char r, char g, char b, u8* screen);
void gfxFillRect(int x1, int y1, int x2, int y2,u32 col, u8* screen);
void gfxRect(int x1, int y1, int x2, int y2, u32 col, u8* screen);
void gfxRoundRect(int x1, int y1, int x2, int y2, int radius,u32 col, u8* screen);
void gfxFillRoundRect(int x1, int y1, int x2, int y2, int radius,u32 b_col,u32 f_col, u8* screen);
void gfxGradientFillRect(LPRECT prc,int radius,int mode,u32 s_col,u32 e_col,u8 *screen);
void gfxDrawImage(u8 *dst,LPRECT pdrc,u8 *src,LPRECT psrc);

#ifdef __cplusplus
}
#endif

#endif