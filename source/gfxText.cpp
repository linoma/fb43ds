#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gfxtext.h"

static u32 color = 0xFFFFFFFF;
//---------------------------------------------------------------------------
static int drawCharacter(u8* fb, font_s* f, char c, s16 x, s16 y)
{	
	int i, j;
	
	charDesc_s* cd = &f->desc[(int)c];
	if(!cd->data)
		return 0;
	u8* charData = cd->data;
	x += cd->xo; 
	y += -cd->yo-cd->h;
	s16 cy=y, ch=cd->h, cyo=0;
	if(y<0){
		cy=0;
		cyo=-y;
		ch=cd->h-cyo;
	}
	else if(y+ch>240)
		ch = 240-y;
	fb += (x*240+cy)*3;
	u8 r=color >>16, g = color>>8, b = color;
	for(i=0;i<cd->w;i++){
		charData += cyo;
		for(j=0;j<ch;j++){
			u8 v = *charData++;
			if(v){
				u8 as = 255-v;
				fb[0] = (fb[0]*as+(b*v))>>8;
				fb[1] = (fb[1]*as+(g*v))>>8;
				fb[2] = (fb[2]*as+(r*v))>>8;
			}
			fb += 3;
		}
		charData += cd->h - (cyo+ch);
		fb += (240 - ch) * 3;
	}
	return cd->xa;
}

//---------------------------------------------------------------------------
void gfxSetTextColor(u32 col)
{
	color = col;
}
//---------------------------------------------------------------------------
void gfxDrawText(u8* fb, font_s* f, char* str, LPRECT prc,u32 flags)
{
	int k,dx,dy,length,x,y;
	
	if(!fb || !str || !str[0] || !prc)
		return;
	if(!f)
		f = &fontDefault;
	y = prc->top;
	x = prc->left;
	length = strlen(str);
	if(flags & 1){
		SIZE sz;
		
		gfxGetTextExtent(f,str,&sz);
		k = ((prc->bottom-y) - sz.cy)>>1;
		y += k;
		k = ((prc->right-x) - sz.cx)>>1;
		x += k;
	}
	y = 239 - y;
	for(k=dx=dy=0;k<length;k++){
		char c = str[k];
		if(c == '\n'){
			if(!(flags&2)){
				dx = 0;
				dy -= f->height;
			}
			continue;
		}
		charDesc_s* cd = &f->desc[(int)c];		
		if(!cd->data)
			continue;
		if((x + cd->xa + dx) >= prc->right){
			if(flags&2)
				return;
			dx = 0;
			dy -= f->height;
		}		
		dx += drawCharacter(fb,f,c,x+dx,y+dy);
	}
}
//---------------------------------------------------------------------------
int gfxGetTextExtent(font_s* f,const char *str, LPSIZE psz)
{
	if(!str || !str[0])
		return -1;
	if(!psz)
		return -2;
	if(!f)
		f = &fontDefault;
	int k; int dx=0, dy=0,mdx=0;
	int length=strlen(str);
	for(k=0;k<length;k++){
		u8 c = str[k];
		charDesc_s* cd = &f->desc[(int)c];
		if(!cd->data)
			continue;
		dx += cd->xa;
		if(c == '\n'){
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
