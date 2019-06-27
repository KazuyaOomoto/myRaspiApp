typedef struct tagBITMAPFILEHEADER{
	unsigned short	bfType;
	unsigned long	bfSize;
	unsigned short	bfReserved1;
	unsigned short	bfReserved2;
	unsigned long	bfOffBits;
} __attribute__((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
	unsigned long	biSize;
	long		biWidth;
	long		biHeight;
	unsigned short	biPlanes;
	unsigned short	biBitCount;
	unsigned long	biCompression;
	unsigned long	biSizeImage;
	long		biXPixPerMeter;
	long		biYPixPerMeter;
	unsigned long	biClrUsed;
	unsigned long	biClrImporant;
} __attribute__((packed)) BITMAPINFOHEADER;

/*プロトタイプ宣言*/
int bmp2RGB565(FILE * fpbmp, unsigned short *pfb, int max_x, int max_y);
int load_bmp(char *filename, unsigned short *pfb, int max_x, int max_y);
