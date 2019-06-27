#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<jpeglib.h>
#include "jpeg.h"

int jpeg2RGB(char *filename, unsigned long *pfb, int max_x, int max_y) {
	int x0 ,y0 ,xx ,yy;
	int p;
	unsigned char r, g, b;

	FILE  *fs;
	JSAMPARRAY  img;
	struct  jpeg_decompress_struct  jds;
	struct  jpeg_error_mgr  jerr;
	int i;
	char buf[256];

	jds.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&jds);
	if((fs = fopen(filename, "rb"))==NULL){
		sprintf(buf,"fopen(%s)",filename);
		perror(buf);
		return -1;
	}
	jpeg_stdio_src(&jds,fs);
	jpeg_read_header(&jds,TRUE);
	jpeg_start_decompress(&jds);

	img = (JSAMPARRAY)malloc(sizeof(JSAMPROW) *jds.output_height);
	for(i=0;i < jds.output_height; i++)
		img[i] = (JSAMPROW)calloc(sizeof(JSAMPLE), 3 * 
							jds.output_width);

	while(jds.output_scanline <  jds.output_height){
		jpeg_read_scanlines(&jds, img + jds.output_scanline,
			jds.output_height - jds.output_scanline);
	}

	memset(pfb, 0, max_x * max_y * 2);

	x0 = (max_x - jds.output_width) / 2;
	y0 = (max_y - jds.output_height) / 2;

	for(yy = 0; yy < jds.output_height; yy++){
		p=0;
		for(xx = 0; xx < jds.output_width; xx++){
			r = img[yy][p++];
			g = img[yy][p++];
			b = img[yy][p++];
			pfb[(yy + y0) * max_x + xx + x0] = 
			((r & 0x000000ff) << 16 | (g & 0x000000ff) << 8 | (b & 0x000000ff));
		}
	}
	for(i=0;i< jds.output_height;i++)
		free(img[i]);
	free(img);
	jpeg_finish_decompress(&jds);
	jpeg_destroy_decompress(&jds);
	fclose(fs);
	return 0;
}

int load_jpeg(char *filename, unsigned long *pfb, int max_x, int max_y){
	jpeg2RGB(filename, pfb, max_x, max_y);
	return 0;
}

int RGBtojpeg(char *filename, unsigned long *pfb, int max_x, int max_y) {
	int x0 ,y0 ,xx ,yy;
	int p;
	unsigned char r, g, b;

	FILE  *fs;
	JSAMPARRAY  img;
	struct  jpeg_compress_struct  jcs;
	struct  jpeg_error_mgr  jerr;
	int i;

	jcs.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&jcs);
	if((fs = fopen(filename, "wb"))==NULL){
		perror("fopen(file)");
		return -1;
	}
	jpeg_stdio_dest(&jcs,fs);
	jcs.image_width=max_x; //画像の横幅
	jcs.image_height=max_y; //画像の縦幅
	jcs.input_components = 3; //１ピクセルのバイト数
	jcs.in_color_space = JCS_RGB; //カラー画像の場合はJSC_RGB;
	jpeg_set_defaults( &jcs );//デフォルト値をセット(jcs.in_color_spaceに値をセット後に呼び出す)
	jpeg_set_quality( &jcs, 75, TRUE ); //品質: 0 - 100

	jpeg_start_compress(&jcs , TRUE);//圧縮を開始する

	img = (JSAMPARRAY)malloc(sizeof(JSAMPROW) * max_y );
	for(i=0;i < jcs.image_height; i++)
		img[i] = (JSAMPROW)calloc(sizeof(JSAMPLE), 3 * 
							max_x );

	//fbの値をimgにコピー
	for(yy = 0; yy < max_y; yy++){
		p=0;
		for(xx = 0; xx < max_x; xx++){
			img[yy][p++] = (JSAMPLE)((pfb[yy * max_x + xx] & 0x00FF0000) >> 16);
			img[yy][p++] = (JSAMPLE)((pfb[yy * max_x + xx] & 0x0000FF00) >> 8);
			img[yy][p++] = (JSAMPLE)((pfb[yy * max_x + xx] & 0x000000FF));
		}
	}


	while(jcs.next_scanline <  max_y){
		jpeg_write_scanlines(&jcs, img + jcs.next_scanline,
			max_y - jcs.next_scanline);
	}

	jpeg_finish_compress(&jcs);

	for(i=0;i< max_y;i++)
		free(img[i]);
	free(img);
	jpeg_destroy_compress(&jcs);
	fclose(fs);
	return 0;
}

int save_jpeg(char *filename, unsigned long *pfb, int max_x, int max_y){
	RGBtojpeg(filename, pfb, max_x, max_y);
	return 0;
}
