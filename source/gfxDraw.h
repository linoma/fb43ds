#include "types.h"

#ifndef __GFXDRAWH__
#define __GFXDRAWH__

#ifdef __cplusplus
extern "C" {
#endif

void gfxPixel(int x, int y, char r, char g, char b, u8* screen);
void gfxFillRect(LPRECT prc,u32 col, u8* screen);
void gfxRect(LPRECT prc, u32 col, u8* screen);
void gfxRoundRect(int x1, int y1, int x2, int y2, int radius,u32 col, u8* screen);
void gfxFillRoundRect(LPRECT prc, int radius,u32 b_col,u32 f_col, u8* screen);
void gfxGradientFillRect(LPRECT prc,int radius,int mode,u32 s_col,u32 e_col,u8 *screen);
void gfxRoundRectShadow(LPRECT prc, int radius,u32 col,int length, u8* screen);
void gfxLine(int x0, int y0, int x1, int y1,u32 col,u8 *screen);
#ifdef __cplusplus
}
#endif

#endif