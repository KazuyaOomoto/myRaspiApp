#define UTF32SIZE	4

/* プロトタイプ宣言 */
//UTF-8コードの文字列を　UTF-32LEに変換する
size_t utf8to32(unsigned char *in, unsigned long *out,size_t len);
