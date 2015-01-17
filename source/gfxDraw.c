#include "gfxDraw.h"

//---------------------------------------------------------------------------
void gfxPixel(int x, int y, char r, char g, char b, u8* screen)
{
	screen += (239-y+x*240)*3;
	screen[0]=b;
	screen[1]=g;
	screen[2]=r;
}
//---------------------------------------------------------------------------
void gfxDrawImage(u8 *dst,LPRECT pdrc,u8 *src,LPRECT psrc)
{
	if(!dst || !src || !pdrc || !psrc)
		return;
	dst += (240-pdrc->top+pdrc->left*240)*3;
}
//---------------------------------------------------------------------------
void gfxGradientFillRect(LPRECT prc,int radius,int mode,u32 s_col,u32 e_col,u8 *screen)
{
	s32 X1,X2,Y1,Y2;
	u32 rf,gf,bf,rs,gs,bs,re,ge,be,x,y,af,as,ae;
	s32 ri,gi,bi,ai;
	
	rs = (u32)(u8)(s_col >> 16);
	gs = (u32)(u8)(s_col >> 8);
	bs = (u32)(u8)s_col;
	as = (u32)(u8)(s_col >> 24);
	
	re = (u32)(u8)(e_col >> 16);
	ge = (u32)(u8)(e_col >> 8);
	be = (u32)(u8)e_col;
	ae = (u32)(u8)(e_col >> 24);
	
	if(prc->left < prc->right){ 
		X1 = prc->left;
		X2 = prc->right;
	} 
	else{ 
		X1 = prc->right;
		X2 = prc->left;
	}
	if(prc->top < prc->bottom){ 
		Y1 = prc->top;
		Y2 = prc->bottom;
	} 
	else{ 
		Y1 = prc->bottom;
		Y2 = prc->top;
	}		
	{
		s32 i = (mode ? (Y2-Y1) : (X2-X1));
	
		ri = ((re-rs) << 12);
		gi = ((ge-gs) << 12);
		bi = ((be-bs) << 12);
		ai = ((ae-as) << 12);
		
		ri /= i;	
		gi /= i;
		bi /= i;
		ai /= i;
	}
	
	screen += (239 - Y1 + X1 * 240) * 3;
	
	rf = rs << 12;
	gf = gs << 12;
	bf = bs << 12;
	af = as << 12;
	
	if(mode){
		for(x=X1;x<=X2;x++){
			u8 *p = screen;
			u32 r,g,b,a;
			
			r = rf;
			g = gf;
			b = bf;
			a = af;
			for(y=Y1;y<=Y2;y++,p-=3){
				u8 da = a >> 12;
				u8 sa = (255 - da);
				p[0] = (p[0] * sa + (b >> 12) * da) >> 8;
				p[1] = (p[1] * sa + (g >> 12) * da) >> 8;
				p[2] = (p[2] * sa + (r >> 12) * da) >> 8;
				r += ri;
				g += gi;
				b += bi;
				a += ai;
			}
			screen += 240*3;
		}	
	}
	else{
		for(x=X1;x<=X2;x++){
			u8 *p = screen;
			u8 da = af >> 12;
			u8 sa = 255-da;
			for(y=Y1;y<=Y2;y++,p-=3){
				p[0] = (p[0] * sa + (bf >> 12) * da) >> 8;
				p[1] = (p[1] * sa + (gf >> 12) * da) >> 8;
				p[2] = (p[2] * sa + (rf >> 12) * da) >> 8;
			}
			rf += ri;
			gf += gi;
			bf += bi;
			af += ai;
			screen += 240*3;
		}
	}
}
//---------------------------------------------------------------------------
void gfxFillRoundRect(int x1, int y1, int x2, int y2, int radius,u32 b_col,u32 f_col, u8* screen)
{
	int X1,X2,Y1,Y2,x,y,r2,i,xe,ye,x0,y0;
	int rf,gf,bf,rb,gb,bb;
	u8 r,g,b;
	
	rf = (int)(f_col >> 16) & 0xFF;
	gf = (int)(f_col >> 8) & 0xFF;
	bf = (int)(f_col & 0xFF);
	
	rb = (int)(b_col >> 16) & 0xFF;
	gb = (int)(b_col >> 8) & 0xFF;
	bb = (int)(b_col & 0xFF);

	if(x1 < x2){ 
		X1 = x1;
		X2 = x2;
	} 
	else{ 
		X1 = x2;
		X2 = x1;
	}
	if(y1 < y2){ 
		Y1 = y1;
		Y2 = y2;
	} 
	else{ 
		Y1 = y2;
		Y2 = y1;
	}
	r2 = radius*radius;
	xe = X2-X1;
	ye = Y2-Y1;
	screen += (239 - Y1 + X1 * 240) * 3;
	for(x=0,i=X1;i<=X2;i++,x++){
		u8 *p1 = screen;
		for(y=0;y<=radius;p1-=3,y++){
			r = rf;
			g = gf;
			b = bf;
			x0 = y0 = 0;
			if(x <= radius){
				x0 = radius - x;
				y0 = radius - y;
			}
			else if(x >= (xe-radius)){
				x0 = radius - (xe - x);
				y0 = radius - y;
			}
			x0 = (x0*x0+y0*y0);
			if(x0 > r2)
				continue;
			else if(x0 == r2 || y == 0){
				r = rb;
				g = gb;
				b = bb;
			}
			p1[0] = b;
			p1[1] = g;
			p1[2] = r;			
		}
		for(;y < (ye-radius);p1-=3,y++){
			r = rf;
			g = gf;
			b = bf;
			if(x == 0 || x == xe){
				r = rb;
				g = gb;
				b = bb;
			}
			p1[0] = b;
			p1[1] = g;
			p1[2] = r;			
		}
		for(;y<=ye;p1-=3,y++){
			r = rf;
			g = gf;
			b = bf;
		
			x0 = y0 = 0;
			if(x <= radius){
				x0 = radius - x;
				y0 = radius - (ye-y);					
			}
			else if(x >= (xe-radius)){
				x0 = radius - (xe - x);
				y0 = radius - (ye-y);					
			}
			x0 = (x0*x0+y0*y0);
			if(x0 > r2)
				continue;
			else if(x0 == r2 || y == ye){
				r = rb;
				g = gb;
				b = bb;
			}
			p1[0] = b;
			p1[1] = g;
			p1[2] = r;		
		}

		screen += 240*3;
	}
}
//---------------------------------------------------------------------------
void gfxRect(LPRECT prc, u32 col, u8* screen)
{
	int X1,X2,Y1,Y2,i;
	u8 *p,*p1,r,g,b,ad,as;
	
	ad = (u8)(col >> 24);
	as=255-ad;
	r = (u8)(col >> 16);
	g = (u8)(col >> 8);
	b = (u8)col;
	if(prc->left < prc->right){ 
		X1 = prc->left;
		X2 = prc->right;
	} 
	else{ 
		X1 = prc->right;
		X2 = prc->left;
	}
	if(prc->top < prc->bottom){ 
		Y1 = prc->top;
		Y2 = prc->bottom;
	} 
	else{ 
		Y1 = prc->bottom;
		Y2 = prc->top;
	}
	p = screen + (239 - Y1 + X1 * 240)*3;
	p1 = screen + (239 - Y1 + X2 * 240)*3;
	for(i=Y1;i<=Y2;i++){
		p[0] = ((p[0]*as)+(b*ad))>>8;
		p[1] = ((p[1]*as)+(g*ad))>>8;
		p[2] = ((p[2]*as)+(r*ad))>>8;
		p -= 3;
		p1[0] = ((p1[0]*as)+(b*ad))>>8;
		p1[1] = ((p1[1]*as)+(g*ad))>>8;
		p1[2] = ((p1[2]*as)+(r*ad))>>8;
		p1 -= 3;		
	}
	p = screen + (239 - Y1 + X1 * 240)*3;
	p1 = screen + (239 - Y2 + X1 * 240)*3;
	for(i=X1;i<=X2;i++){
		p[0] = ((p[0]*as)+(b*ad))>>8;
		p[1] = ((p[1]*as)+(g*ad))>>8;
		p[2] = ((p[2]*as)+(r*ad))>>8;
		p += 240*3;
		p1[0] = ((p1[0]*as)+(b*ad))>>8;
		p1[1] = ((p1[1]*as)+(g*ad))>>8;
		p1[2] = ((p1[2]*as)+(r*ad))>>8;
		p1 += 240*3;
	}	
}
//---------------------------------------------------------------------------
void gfxFillRect(LPRECT prc, u32 col, u8* screen)
{
	int X1,X2,Y1,Y2,i,j;
	u8 r,g,b,ad,as;
	
	ad = (u8)(col >> 24);
	as = 255-ad;
	r = (u8)(col >> 16);
	g = (u8)(col >> 8);
	b = (u8)col;
	if(prc->left < prc->right){ 
		X1 = prc->left;
		X2 = prc->right;
	} 
	else{ 
		X1 = prc->right;
		X2 = prc->left;
	}
	if(prc->top < prc->bottom){ 
		Y1 = prc->top;
		Y2 = prc->bottom;
	} 
	else{ 
		Y1 = prc->bottom;
		Y2 = prc->top;
	}

	screen += (239 - Y1 + X1 * 240)*3;
	for(i=X1;i<=X2;i++){
		u8 *p1 = screen;
		for(j=Y1;j<=Y2;j++,p1-=3){
			p1[0]=(p1[0] * as + b*ad) >> 8;
			p1[1]=(p1[1] * as + g*ad) >> 8;
			p1[2]=(p1[2] * as + r*ad) >> 8;
		}
		screen += 240*3;
	}
}
