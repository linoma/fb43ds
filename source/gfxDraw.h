#ifndef __GFXDRAWH__
#define __GFXDRAWH__

#ifdef __cplusplus
extern "C" {
#endif

void gfxPixel(int x, int y, char r, char g, char b, u8* screen);
void gfxFillRect(int x1, int y1, int x2, int y2, char r, char g, char b, u8* screen);
void gfxRect(int x1, int y1, int x2, int y2, char r, char g, char b, u8* screen);
void gfxRoundRect(int x1, int y1, int x2, int y2, int radius,char r, char g, char b, u8* screen);
void gfxFillRoundRect(int x1, int y1, int x2, int y2, int radius,u32 b_col,u32 f_col, u8* screen);

#ifdef __cplusplus
}
#endif

#endif