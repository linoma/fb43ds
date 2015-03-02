#include "gfxDraw.h"

#define BLEND_PIXEL(p,sa,da,r,g,b)\
	p[0] = (p[0] * sa + b * da) >> 8;\
	p[1] = (p[1] * sa + g * da) >> 8;\
	p[2] = (p[2] * sa + r * da) >> 8;

//---------------------------------------------------------------------------
void gfxPixel(int x, int y, char r, char g, char b, u8* screen)
{
	screen += (239-y+x*240)*3;
	screen[0] = b;
	screen[1] = g;
	screen[2] = r;
}
//---------------------------------------------------------------------------
void gfxGradientFillRect(LPRECT prc,int radius,int mode,u32 s_col,u32 e_col,u8 *screen)
{
	s32 X1,X2,Y1,Y2;
	u32 rf,gf,bf,rs,gs,bs,re,ge,be,x,y,af,as,ae;
	s32 ri,gi,bi,ai,r2,ye,xe,y0,x0,x1,y1,r3;
	
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
		s32 i = ((mode&1) ? (Y2-Y1) : (X2-X1));
	
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

	r2 = radius*radius;
	if(r2){
		r3=radius-1;
		r3*=r3;
	}
	else
		r3=0;
	xe = X2-X1;
	ye = Y2-Y1;
	
	if(mode&1){//vertical
		for(x=0,x1=X1;x1<=X2;x1++,x++){
			u8 *p = screen;
			u32 r,g,b,a;
			int yy;
			
			r = rf;
			g = gf;
			b = bf;
			a = af;
			yy = (mode & 4) == 0 ? radius : -1;
			for(y=0;y<=yy;p-=3,y++){
				x0 = y0 = 0;
				if(x <= radius){
					x0 = radius - x;
					y0 = radius - y;
				}
				else if(x > (xe-radius)){
					x0 = radius - (xe - x);
					y0 = radius - y;
				}
				x0 = (x0*x0+y0*y0);
				if(x0 > r2){
				}
				else{
					u8 da = a >> 12;
					u8 sa = (255 - da);
					BLEND_PIXEL(p,sa,da,(r>>12),(g>>12),(b>>12))
				}
				r += ri;
				g += gi;
				b += bi;
				a += ai;				
			}
			yy = ye;
			if((mode & 2) == 0)
				yy -= radius;
			else
				yy++;
			for(;y<yy;p-=3,y++){
				u8 da = a >> 12;
				u8 sa = (255 - da);
				BLEND_PIXEL(p,sa,da,(r>>12),(g>>12),(b>>12))
				r += ri;
				g += gi;
				b += bi;
				a += ai;
			}			
			for(;y<=ye;p-=3,y++){
				x0 = y0 = 0;
				if(x <= radius){
					x0 = radius - x;
					y0 = radius - (ye-y);					
				}
				else if(x >= (xe-radius)){
					x0 = radius - (xe - x);
					y0 = radius - (ye - y);					
				}
				x0 = (x0*x0+y0*y0);	
				if(x0>r2){
				}
				else{
					u8 da = a >> 12;
					u8 sa = (255 - da);
					BLEND_PIXEL(p,sa,da,(r>>12),(g>>12),(b>>12))
				}
				r += ri;
				g += gi;
				b += bi;
				a += ai;				
			}			
			screen += 240*3;
		}	
	}
	else{
		for(x1=X1;x1<=X2;x1++){
			u8 *p = screen;
			u8 da = af >> 12;
			u8 sa = 255 - da;
			
			for(y=0;y<=radius;p-=3,y++){
				x0 = y0 = 0;
				if(x <= radius){
					x0 = radius - x;
					y0 = radius - y;
				}
				else if(x > (xe-radius)){
					x0 = radius - (xe - x);
					y0 = radius - y;
				}
				x0 = (x0*x0+y0*y0);
				if(x0>r2){
				}
				else{
					BLEND_PIXEL(p,sa,da,(rf>>12),(gf>>12),(bf>>12))
				}
			}					
			for(;y<(ye-radius);p-=3,y++){
				BLEND_PIXEL(p,sa,da,(rf>>12),(gf>>12),(bf>>12))
			}						
			for(;y<=ye;p-=3,y++){
				x0 = y0 = 0;
				if(x <= radius){
					x0 = radius - x;
					y0 = radius - (ye-y);					
				}
				else if(x >= (xe-radius)){
					x0 = radius - (xe - x);
					y0 = radius - (ye - y);					
				}
				x0 = (x0*x0+y0*y0);
				if(x0>r2){
				}
				else {
					BLEND_PIXEL(p,sa,da,(rf>>12),(gf>>12),(bf>>12))
				}
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
void gfxFillRoundRect(LPRECT prc, int radius,u32 b_col,u32 f_col, u8* screen)
{
	int X1,X2,Y1,Y2,x,y,r2,i,xe,ye,x0,y0,r3;
	int rf,gf,bf,rb,gb,bb,af,ab,mode;
	u8 r,g,b,a,as;
	
	mode = (int)(u16)(radius>>16);
	radius &= 0xffff;
	
	af = (int)(f_col >> 24) & 0xFF;
	rf = (int)(f_col >> 16) & 0xFF;
	gf = (int)(f_col >> 8) & 0xFF;
	bf = (int)(f_col & 0xFF);
	
	ab = (int)(b_col >> 24) & 0xFF;
	rb = (int)(b_col >> 16) & 0xFF;
	gb = (int)(b_col >> 8) & 0xFF;
	bb = (int)(b_col & 0xFF);

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

	r2 = radius*radius;
	if(radius){
		r3 =radius-1;
		r3 *=r3;
	}
	else
		r3=0;
	xe = X2-X1;
	ye = Y2-Y1;
	screen += (239 - Y1 + X1 * 240) * 3;
	for(x=0,i=X1;i<=X2;i++,x++){
		u8 *p1 = screen;
		for(y=0;y<=radius;p1-=3,y++){
			r = rf;
			g = gf;
			b = bf;
			a = af;
			x0 = y0 = 0;
			if(x <= radius){
				x0 = radius - x;
				y0 = radius - y;
			}
			else if(x > (xe-radius)){
				x0 = radius - (xe - x);
				y0 = radius - y;
			}
			x0 = (x0*x0+y0*y0);
			if(x0 > r2)
				continue;
			if(x0>r3 || !y){
				r = rb;
				g = gb;
				b = bb;
				a=ab;
			}
			as = 255 - a;
			BLEND_PIXEL(p1,as,a,r,g,b)
		}
		for(;y < (ye-radius);p1-=3,y++){
			r = rf;
			g = gf;
			b = bf;
			a=af;
			if(x == 0 || x == xe){
				r = rb;
				g = gb;
				b = bb;
				a=ab;
			}
			as=255-a;
			BLEND_PIXEL(p1,as,a,r,g,b)			
		}
		for(;y<=ye;p1-=3,y++){
			r = rf;
			g = gf;
			b = bf;	
			a=af;
			x0 = y0 = 0;
			if(x <= radius){
				x0 = radius - x;
				y0 = radius - (ye-y);					
			}
			else if(x >= (xe-radius)){
				x0 = radius - (xe - x);
				y0 = radius - (ye - y);					
			}
			x0 = (x0*x0+y0*y0);
			if(x0 > r2)
				continue;
			if(x0>r3 || y == ye){
				r = rb;
				g = gb;
				b = bb;
				a=ab;
			}
			as=255-a;
			BLEND_PIXEL(p1,as,a,r,g,b)
		}
		screen += 240*3;
	}
}
//---------------------------------------------------------------------------
void gfxRoundRectShadow(LPRECT prc, int radius,u32 col,int length, u8* screen)
{
	int X1,X2,Y1,Y2,x,y,r2,i,xe,ye,x0,y0,r3;
	int rf,gf,bf,rb,gb,bb,af,ab,ai;
	u8 r,g,b,a,as;
	
	af = (int)(col >> 24) & 0xFF;
	rf = (int)(col >> 16) & 0xFF;
	gf = (int)(col >> 8) & 0xFF;
	bf = (int)(col & 0xFF);
	
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

	r2 = radius*radius;
	if(r2){
		r3 = radius-1;
		r3 *= r3;
	}
	else
		r3=0;
	xe = X2-X1;
	ye = Y2-Y1;
	
	ai = ((af-0) << 12);
	ai /= length;
	
	screen += (239 - Y1 + X1 * 240) * 3;
	for(x=0,i=X1;i<=X2;i++,x++){
		u8 *p1 = screen;
		for(y=0;y<=radius;p1-=3,y++){
			r = rf;
			g = gf;
			b = bf;
			a = af;
			x0 = y0 = 0;
			if(x <= radius){
				x0 = radius - x;
				y0 = radius - y;
			}
			else if(x > (xe-radius)){
				x0 = radius - (xe - x);
				y0 = radius - y;
			}
			x0 = (x0*x0+y0*y0);
			if(x0 > r2)
				continue;
			if(x0>r3 || !y){
				r = rb;
				g = gb;
				b = bb;
				a=ab;
			}
			as=255-a;
			BLEND_PIXEL(p1,as,a,r,g,b)
		}
		for(;y < (ye-radius);p1-=3,y++){
			r = rf;
			g = gf;
			b = bf;
			a=af;
			if(x == 0 || x == xe){
				r = rb;
				g = gb;
				b = bb;
				a=ab;
			}
			as=255-a;
			BLEND_PIXEL(p1,as,a,r,g,b)			
		}
		for(;y<=ye;p1-=3,y++){
			r = rf;
			g = gf;
			b = bf;	
			a=af;
			x0 = y0 = 0;
			if(x <= radius){
				x0 = radius - x;
				y0 = radius - (ye-y);					
			}
			else if(x >= (xe-radius)){
				x0 = radius - (xe - x);
				y0 = radius - (ye - y);					
			}
			x0 = (x0*x0+y0*y0);
			if(x0 > r2)
				continue;
			else if(x0>r3 || y == ye){
				r = rb;
				g = gb;
				b = bb;
				a=ab;
			}
			as=255-a;
			BLEND_PIXEL(p1,as,a,r,g,b)
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
		BLEND_PIXEL(p,as,ad,r,g,b)			
		p -= 3;
		BLEND_PIXEL(p1,as,ad,r,g,b)			
		p1 -= 3;		
	}
	p = screen + (239 - Y1 + X1 * 240)*3;
	p1 = screen + (239 - Y2 + X1 * 240)*3;
	for(i=X1;i<=X2;i++){
		BLEND_PIXEL(p,as,ad,r,g,b)			
		p += 240*3;
		BLEND_PIXEL(p1,as,ad,r,g,b)
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
			BLEND_PIXEL(p1,as,ad,r,g,b)
		}
		screen += 240*3;
	}
}
//---------------------------------------------------------------------------
void gfxLine(int x0, int y0, int x1, int y1,u32 col,u8 *screen) 
{
	int e2,ix,iy,dx,dy,err,sx,sy;
	u8 r,g,b,a,as;
	
	dx = abs(x1-x0);
	dy = abs(y1-y0);
	err = (dx > dy ? dx : -dy) >> 1;
	sx = x0 < x1 ? 1 : -1;
	sy = y0 < y1 ? 1 : -1;
	
	a = (u8)(col>>24);
	r = (u8)(col>>16);
	g = (u8)(col>>8);
	b = (u8)col;
	as = 255-a;
	
	u8 *p=screen + (239 - y0 + x0 * 240)*3;
	ix = sx*240*3;
	iy = sy*-3;
	
	while(1){
		BLEND_PIXEL(p,as,a,r,g,b)
		if (x0==x1 && y0==y1) 
			break;
		e2 = err;
		if (e2 >-dx) { 
			err -= dy; 
			x0 += sx; 
			p += ix;
		}
		if (e2 < dy) { 
			err += dx; 
			y0 += sy; 
			p += iy;
		}
	}
}
//---------------------------------------------------------------------------
void gfxFloodFill(int x0, int y0, u32 fillColor, u32 interiorColor,u8 *screen)
{
	u32 col;
	u8 *p;
	
	interiorColor &= 0xFFFFFF;
	p = screen + (239 - y0 + x0 * 240)*3;
	col = p[0];
	col |= p[1] << 8;
	col |= p[2] << 16;	
	if(col == interiorColor){
		u8 r,g,b,a,as;
		
		a = (u8)(fillColor>>24);
		r = (u8)(fillColor>>16);
		g = (u8)(fillColor>>8);
		b = (u8)fillColor;
		as = 255 - a;	
		BLEND_PIXEL(p,as,a,r,g,b)
		gfxFloodFill(x0 + 1, y0, fillColor, interiorColor,screen);
		gfxFloodFill(x0 - 1, y0, fillColor, interiorColor,screen);
		gfxFloodFill(x0, y0 + 1, fillColor, interiorColor,screen);
		gfxFloodFill(x0, y0 - 1, fillColor, interiorColor,screen);
	}
}