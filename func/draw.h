#include <string.h>

#define SCREENWIDTH	480
#define SCREENHEIGHT	272
#define SCREENSIZE	(SCREENWIDTH * SCREENHEIGHT * 2)

#define RGB(r,g,b) (((r) << 11) | ((g) << 5) | (b))

/******************************************
	指定した座標値を指定した色にする
******************************************/
void lcd_put_pnt(unsigned short *pfb,int x,int y,unsigned short color);


/************************************************
	線分の描写
	(x1,y1),(x2,x2)：直線の座標を指定
	color:色
************************************************/
void lcd_drw_lin(unsigned short *pfb,int x1,int y1,int x2,int y2,unsigned short color);


/************************************************
	矩形の描写　
	(x1,y1),(x2,x2):対角上の座標を指定
	color:色
	fil: 0:塗りつぶしなし , 1:塗りつぶし
************************************************/
void lcd_drw_rec(unsigned short *pfb,int x1,int y1,int x2, int y2, unsigned short color,int fil);


/*********************************************************
	円の描写　
	(x0,y0):円原点座標
	r:半径
	color:色
	fil: 0:塗りつぶしなし , 1:塗りつぶし
*********************************************************/
void lcd_drw_cir(unsigned short *pfb,int x0,int y0,int r,unsigned short color,int fil);


/*********************************************************
	楕円の描写
	(x0,y0):円原点座標
	a:x方向の半径
	b:y方向の半径
	color:色
	fil: 0:塗りつぶしなし , 1:塗りつぶし
*********************************************************/
void lcd_drw_ell(unsigned short *pfb,int x0,int y0,int a,int b,unsigned short color,int fil);


/******************************************
	画面をクリアーする
******************************************/
int  lcd_clr(unsigned short *pfb);

