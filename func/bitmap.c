#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitmap.h"

int bmp2RGB565(FILE * fpbmp, unsigned short *pfb, int max_x, int max_y) {
	int fd_in;
	int datasize;
	int x0, x1 ,y0 ,y1 ,xx, yy;
	int padding;
	int c = 0;
	unsigned char *bmpdata;
	unsigned char r, g, b;

	struct bmpheader_t{
		BITMAPFILEHEADER fh;
		BITMAPINFOHEADER ih;
	} bmp;

	if (fread(&bmp, sizeof(bmp), 1, fpbmp) != 1){
		perror("fread(file)");
		return -1;
	}

	if (bmp.fh.bfType != 0x4d42 || bmp.ih.biBitCount != 24
		|| bmp.ih.biCompression != 0 || bmp.ih.biHeight < 0){
		fprintf(stderr, "unsupported bitmap format\n");
		return -1;
	}

	if (bmp.ih.biWidth > max_x || bmp.ih.biHeight > max_y){
		fprintf(stderr, "image size too big\n");
		return -1;
	}

	datasize = bmp.fh.bfSize - sizeof(bmp);
	if (!(bmpdata = malloc(datasize))){
		perror("malloc");
		return -1;
	}

	if (fread(bmpdata, datasize, 1, fpbmp) != 1){
		perror("fread(file)");
		free(bmpdata);
		return -1;
	}

	memset(pfb, 0, max_x * max_y * 2);

	x0 = (max_x - bmp.ih.biWidth) / 2;
	y0 = (max_y - bmp.ih.biHeight) / 2;
	x1 = x0 + bmp.ih.biWidth;
	y1 = y0 + bmp.ih.biHeight;
	padding = bmp.ih.biWidth % 4;

	for(yy = y1 - 1; yy >= y0; yy--){
		for(xx = x0; xx < x1; xx++){
			b = bmpdata[c++];
			g = bmpdata[c++];
			r = bmpdata[c++];
			pfb[yy * max_x + xx] = 
			((r & 0xf8) << 8 | (g & 0xfc) << 3 | (b & 0xf8) >> 3);
		}
		c += padding;
	}
	free(bmpdata);
	return 0;
}

int load_bmp(char *filename, unsigned short *pfb, int max_x, int max_y){
	FILE * fpbmp;

	if ((fpbmp=fopen(filename, "rb")) < 0) {
		perror("fopen(file)");
		return -1;
	}
	bmp2RGB565(fpbmp, pfb, max_x, max_y);
	fclose(fpbmp);
	return 0;
}
