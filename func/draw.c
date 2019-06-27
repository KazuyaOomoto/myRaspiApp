#include "draw.h"

/******************************************
	指定した座標値を指定した色にする
******************************************/
void lcd_put_pnt(unsigned short *pfb,int x,int y,unsigned short color)
{
	if(y*SCREENWIDTH+x < SCREENWIDTH*SCREENHEIGHT)
		*(pfb+y*SCREENWIDTH+x) = color;
	return;
}

/************************************************
	線分の描写
	(x1,y1),(x2,x2)：直線の座標を指定
	color:色
************************************************/
void lcd_drw_lin(unsigned short *pfb,int x1,int y1,int x2,int y2,unsigned short color)
{
	int i;
	int dx,dy;
	int sx,sy;

	/* 二点間の距離 */
	dx= ( x2 > x1 ) ? x2 - x1 : x1 - x2;
	dy = (y2 > y1 ) ? y2 - y1 : y1 - y2;

	/* 二点の方向 */
	sx = ( x2 > x1 ) ? 1 : -1;
	sy = ( y2 > y1 ) ? 1 : -1;

	if(dx==0)//縦線の時
	{
		dy++;
		while(dy--)
		{
			lcd_put_pnt( pfb, x1 , y1, color);
			y1+=sy;
		}
		return;
	}
	if(dy==0)//横線の時
	{
		dx++;
		while(dx--)
		{
			lcd_put_pnt( pfb, x1 , y1 , color);
			x1+=sx;
		}
		return;
	}


	/* 傾きが1より小さい場合 */
	if ( dx > dy ) {
		int E = -dx;
		for ( i = 0 ; i <= dx ; i++ ) {
			lcd_put_pnt( pfb, x1 , y1 , color);
			x1 += sx;
			E += 2 * dy;
			if ( E >= 0 ) {
				y1 += sy;
				E -= 2 * dx;
			}
		}
	}
	/* 傾きが1以上の場合 */
	else {
		int E = -dy;
		for ( i = 0 ; i <= dy ; i++ ) {
			lcd_put_pnt( pfb, x1 , y1 , color);
			y1 += sy;
			E += 2 * dx;
			if ( E >= 0 ) {
				x1 += sx;
				E -= 2 * dy;
			}
		}
	}
	return;
}

/************************************************
	矩形の描写　
	(x1,y1),(x2,x2):対角上の座標を指定
	color:色
	fil: 0:塗りつぶしなし , 1:塗りつぶし
************************************************/
void lcd_drw_rec(unsigned short *pfb,int x1,int y1,int x2, int y2, unsigned short color,int fil)
{

	if(fil){
		int dy;
		int sy;
		/* 二点間の高さ */
		dy = (y2 > y1 ) ? y2 - y1+1 : y1 - y2+1;
	
		/* 向き */
		sy = ( y2 > y1 ) ? 1 : -1;

		while(dy--)
		{
			lcd_drw_lin( pfb, x1 , y1, x2, y1, color);
			y1+=sy;
		}
	}
	else
	{
			lcd_drw_lin( pfb, x1 , y1, x2, y1, color);
			lcd_drw_lin( pfb, x1 , y1, x1, y2, color);
			lcd_drw_lin( pfb, x1 , y2, x2, y2, color);
			lcd_drw_lin( pfb, x2 , y1, x2, y2, color);
	}
	return;
}


/*********************************************************
	円の描写　
	(x0,y0):円原点座標
	r:半径
	color:色
	fil: 0:塗りつぶしなし , 1:塗りつぶし
*********************************************************/
void lcd_drw_cir(unsigned short *pfb,int x0,int y0,int r,unsigned short color,int fil)
{
	int x = r;
	int y = 0;
	int F = -2 * r + 3;
	
	if(r==0)
		return;

	if(fil){
		while ( x >= y ) {
			lcd_drw_lin( pfb, x0 + x, y0 + y, x0 - x, y0 + y, color);
			lcd_drw_lin( pfb, x0 + x, y0 - y, x0 - x, y0 - y, color);
			lcd_drw_lin( pfb, x0 + y, y0 + x, x0 - y, y0 + x, color);
			lcd_drw_lin( pfb, x0 + y, y0 - x, x0 - y, y0 - x, color);
			if ( F >= 0 ) {
				x--;
				F -= 4 * x;
			}
	 		y++;
			F += 4 * y + 2;
		}
	}
	else
	{
		while ( x >= y ) {
			lcd_put_pnt( pfb,  x0 + x, y0 + y, color );
			lcd_put_pnt( pfb,  x0 - x, y0 + y, color );
			lcd_put_pnt( pfb,  x0 + x, y0 - y, color );
			lcd_put_pnt( pfb,  x0 - x, y0 - y, color );
			lcd_put_pnt( pfb,  x0 + y, y0 + x, color );
			lcd_put_pnt( pfb,  x0 - y, y0 + x, color );
			lcd_put_pnt( pfb,  x0 + y, y0 - x, color );
			lcd_put_pnt( pfb,  x0 - y, y0 - x, color );
			if ( F >= 0 ) {
				x--;
				F -= 4 * x;
			}
			y++;
			F += 4 * y + 2;
		}
	}
	return;
}

/*********************************************************
	楕円の描写
	(x0,y0):円原点座標
	a:x方向の半径
	b:y方向の半径
	color:色
	fil: 0:塗りつぶしなし , 1:塗りつぶし
*********************************************************/
void lcd_drw_ell(unsigned short *pfb,int x0,int y0,int a,int b,unsigned short color,int fil)
{
	int x,y,d,tmp;
	int r = a*b;
	tmp=a;
	a=b;
	b=tmp;

	x = r /a;
	y = 0;
	d = a*r;
	int F;
	int H;

	a = a*a;
	b = b*b;
	F = (int)(-2.0*d) + a+2*b;
	H = (int)(-4.0*d)+2*a + b;
	
	if(r==0)
		return;

	if(fil){
		while ( x >= 0 ) {
			lcd_drw_lin( pfb, x0 + x, y0 + y, x0 - x, y0 + y, color);
			lcd_drw_lin( pfb, x0 + x, y0 - y, x0 - x, y0 - y, color);
			if ( F >= 0 ) {
				x--;
				F -= 4*a*x;
				H -= 4*a*x - 2*a;
			}
			if(H < 0){
	 			y++;
				F += 4*b*y + 2*b;
				H += 4*b*y;
			}
		}
	}
	else
	{
		while ( x >= 0 ) {
			lcd_put_pnt( pfb,  x0 + x, y0 + y, color );
			lcd_put_pnt( pfb,  x0 - x, y0 + y, color );
			lcd_put_pnt( pfb,  x0 + x, y0 - y, color );
			lcd_put_pnt( pfb,  x0 - x, y0 - y, color );
			if ( F >= 0 ) {
				x--;
				F -= 4*a*x;
				H -= 4*a*x - 2*a;
			}
			if(H < 0){
	 			y++;
				F += 4*b*y + 2*b;
				H += 4*b*y;
			}
		}
	}
	return;
}


/******************************************
	画面をクリアーする
******************************************/
int  lcd_clr(unsigned short *pfb)
{
	memset(pfb,0,SCREENSIZE);
	return 0;
}
