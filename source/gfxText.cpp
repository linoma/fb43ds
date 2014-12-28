#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gfxtext.h"

static u32 color = 0xFFFFFFFF;
//---------------------------------------------------------------------------
static int drawCharacter(u8* fb, font_s* f, char c, s16 x, s16 y, u16 w, u16 h)
{	
	charDesc_s* cd=&f->desc[(int)c];
	if(!cd->data)
		return 0;
	x += cd->xo; 
	y += -cd->yo-cd->h;
	if(x<0 || x+cd->w>=w || y<-cd->h || y>=h+cd->h)
		return 0;
	u8* charData=cd->data;
	int i, j;
	s16 cy=y, ch=cd->h, cyo=0;
	if(y<0){
		cy=0;
		cyo=-y;
		ch=cd->h-cyo;
	}
	else if(y+ch>h)
		ch=h-y;
	fb+=(x*h+cy)*3;
	u8 r=color>>16, g=color>>8, b=color;
	for(i=0;i<cd->w;i++){
		charData+=cyo;
		for(j=0;j<ch;j++){
			u8 v=*(charData++);
			if(v){
				fb[0]=(fb[0]*(0xFF-v)+(b*v))>>8;
				fb[1]=(fb[1]*(0xFF-v)+(g*v))>>8;
				fb[2]=(fb[2]*(0xFF-v)+(r*v))>>8;
			}
			fb+=3;
		}
		charData+=(cd->h-(cyo+ch));
		fb+=(h-ch)*3;
	}
	return cd->xa;
}
//---------------------------------------------------------------------------
void drawString(u8* fb, font_s* f, char* str, s16 x, s16 y, u16 w, u16 h)
{
	int k,dx,dy,length;
	
	if(!fb || !str)
		return;
	if(!f)
		f = &fontDefault;
	y = 239-y;
	length = strlen(str);
	for(k=dx=dy=0;k<length;k++){
		dx += drawCharacter(fb,f,str[k],x+dx,y+dy,w,h);
		if(str[k] == '\n'){
			dx = 0;
			dy -= f->height;
		}
	}
}
//---------------------------------------------------------------------------
void gfxSetTextColor(u32 col)
{
	color = col;
}
//---------------------------------------------------------------------------
void gfxDrawText(gfxScreen_t screen, gfx3dSide_t side, font_s* f, char* str, s16 x, s16 y)
{
	if(!str)
		return;
	if(!f)
		f=&fontDefault;
	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);
	drawString(fbAdr, f, str, y, x, fbHeight, fbWidth);
}
//---------------------------------------------------------------------------
int gfxGetTextExtent(font_s* f,const char *str, LPSIZE psz)
{
	if(!str || !str[0])
		return -1;
	if(!psz)
		return -2;
	if(!f)
		f=&fontDefault;
	int k; int dx=0, dy=0,mdx=0;
	int length=strlen(str);
	for(k=0;k<length;k++){
		u8 c = str[k];
		charDesc_s* cd=&f->desc[(int)c];
		if(!cd->data)
			continue;
		dx += cd->xa;
		if(c=='\n'){
			if(dx>mdx)
				mdx = dx;
			dx = 0;
			dy += f->height;
		}
	}
	if(mdx>dx)
		dx = mdx;
	psz->cx = dx;
	psz->cy = dy + f->height;
	return 0;
}
