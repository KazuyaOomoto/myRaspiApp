#include <ft2build.h>
#include FT_FREETYPE_H
#include "font.h"

#define SCREENWIDTH		800
#define SCREENHEIGHT	480

/*	Draw Dot	*/
static void pset(unsigned long *pfb,int x,int y,unsigned long color)
{
	if(y*SCREENWIDTH+x < SCREENWIDTH*SCREENHEIGHT)
		*(pfb+y*SCREENWIDTH+x) = color;
	return;
}

/*	Display Character	*/
int put_char(unsigned long *pfb, char fontfile[],
	unsigned long moji,unsigned long size,int x,int y,
	unsigned long color, unsigned long bkcolor){
	FT_Library library;
	FT_Face    face;

	FT_GlyphSlot slot;
	FT_UInt glyph_index;
	FT_Bitmap bitmap;
	int xx,yy,i;
	unsigned char r, g, b;
	int error;
	int ret;

	// FreeType initialization and loading TrueType fonts
	FT_Init_FreeType( &library );
	error=FT_New_Face( library, fontfile, 0, &face);
	if(error !=0){
		printf("ERROR : FT_New_Face (%d) \n",error);
		FT_Done_FreeType(library);
		return(error);
	}
	slot = face->glyph;

	FT_Set_Pixel_Sizes( face, 0, size);

	// Bitmap the character
	FT_Load_Char( face, moji, FT_LOAD_RENDER);
	// Render Glyph
	FT_Render_Glyph(slot, ft_render_mode_normal);
	bitmap = slot->bitmap;
	//adjust baseline
	y=y+size-slot->bitmap_top;
	i=0;
	for(yy = 0; yy < bitmap.rows; yy++){
		for(xx = 0; xx < bitmap.width; xx++){
			if(bitmap.buffer[i])
				pset(pfb, xx + x, yy + y, color);
			else if(color !=bkcolor)
				pset(pfb, xx + x, yy + y, bkcolor);
			i++;
		}
	}
	/*printf("bitmap.width = %d, slop->bitmap_left=%d, advance.x=%d\n", 
		bitmap.width,
		slot->bitmap_left,
		slot->advance.x>>6
		);
	ret = bitmap.width + slot->bitmap_left;*/
	ret = slot->advance.x>>6;
	FT_Done_Face(face);
	FT_Done_FreeType(library);
	return ret;
}

