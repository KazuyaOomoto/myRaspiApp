/* プロトタイプ宣言 */
int jpeg2RGB(char *filename, unsigned long *pfb, int max_x, int max_y);
int load_jpeg(char *filename, unsigned long *pfb, int max_x, int max_y);
int RGBtojpeg(char *filename, unsigned long *pfb, int max_x, int max_y);
int save_jpeg(char *filename, unsigned long *pfb, int max_x, int max_y);
