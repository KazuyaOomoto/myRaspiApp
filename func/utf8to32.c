#include <string.h>
#include <stdio.h>

#include <iconv.h>
#include "utf8to32.h"

//UTF-8コードの文字列を　UTF-32LEに変換する
size_t utf8to32(unsigned char *in, unsigned long *out,size_t len)
{
	char *pin;
	char *pout;
	iconv_t cd;
	size_t ilen,olen,rlen;

	memset(out, 0, len);

	// iconv_open - 文字セット変換のためのディスクリプタを割り当てる 
	// from UTF-8 to UTF-32LE
	cd = iconv_open("UTF-32LE","UTF-8");
	if(cd == (iconv_t)-1){
		perror("iconv open");
		return -1;
	}

	ilen = strlen(in);
	olen = len;
	pin  = in; 
	pout = (unsigned char *)out;
	while(ilen!=0){
		//iconv - 文字セット変換を行う
		rlen = iconv(cd,&pin, &ilen, &pout, &olen);
		if (rlen == -1) {
			perror("iconv");
			return -1;
		}
	}
	//iconv_close - 文字セット変換のためのディスクリプタを解放する 
	iconv_close(cd);
	return (len-olen)/UTF32SIZE;
}

