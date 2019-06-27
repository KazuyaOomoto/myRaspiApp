#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "p3msg.h"
#include "debug.h"
#include "p3_cmn.h"

#define P3_SPLIT_DATE_STR	("-")
#define P3_CAL_MIN_UNIT		(15)
#define P3_STD_WK_HOUR		(7)
#define P3_STD_WK_MIN		(45)

static int zeller(int y,int m,int d);
static int hantei(int y,int m,int d,int youbi);
static int p3_IsHoliday(const P3msg* const);

void p3_cal_working_tim(const char* const iw, const void* const pdata, P3_TIM_CAL* pcal)
{
	char	fsrcin[16]={0};
	char	fsrcout[16]={0};
	char	fin1_1[16]={0};
	char	fin1_2[16]={0};
	char	fout1_1[16]={0};
	char	fout1_2[16]={0};
	char	*p1;
	char	*p2;
	int		fin1_1_1;
	int		fin1_2_2;
	int		fout1_1_1;
	int		fout1_2_2;
	int		timin;
	int		tomin;
	int		wk_h;
	int		wk_m;
	int		ex_h;
	int		ex_m;
	int		isholiday;
	P3msg*	pmsg;

	if(!iw || !pdata || !pcal)	return;

	pmsg = (P3msg*)pdata;
	isholiday = p3_IsHoliday(pmsg);
	memset(pcal, 0, sizeof(P3_TIM_CAL));

	strcpy(fsrcin, iw);
	p1 = strtok(fsrcin, ":");
	strcpy(fin1_1,p1);
	while(p1 != NULL) {
		p1= strtok(NULL, ":");
		if(p1 != NULL){
			strcpy(fin1_2, p1);
			break;
		}
	}

	strcpy(fsrcout, pmsg->date.tim);
	p2 = strtok(fsrcout, ":");
	strcpy(fout1_1,p2);
	while(p2 != NULL) {
		p2= strtok(NULL, ":");
		if(p2 != NULL){
			strcpy(fout1_2, p2);
			break;
		}
	}

	fin1_1_1 = atoi(fin1_1);
	fin1_2_2 = atoi(fin1_2);
	fout1_1_1 = atoi(fout1_1);
	fout1_2_2 = atoi(fout1_2);

	timin = fin1_1_1 * 60 + fin1_2_2;
	tomin = fout1_1_1 * 60 + fout1_2_2;

	if(tomin -timin > 0 )
	{
		wk_h = (tomin-timin)/60;
		wk_m = (((tomin-timin)%60)/P3_CAL_MIN_UNIT)*P3_CAL_MIN_UNIT;
		if(isholiday){
			sprintf(pcal->extra_hour, "%d:%02d", wk_h, wk_m);
		}
		else{
			ex_h = wk_h - P3_STD_WK_HOUR;
			ex_m = wk_m - P3_STD_WK_MIN;

			sprintf(pcal->working_hour, "%d:%02d", wk_h, wk_m);

			if((ex_h>=0) && (ex_m >= 0))
				sprintf(pcal->extra_hour, "%d:%02d", ex_h, ex_m);
		}
	}

}

static int p3_IsHoliday(const P3msg* const pmsg)
{
	int		i,day,youbi,y,m;
	int		flag=0;
	int		ret;
	char	date[16]={0};

	strncpy(date, pmsg->date.day, strlen(pmsg->date.day));
	y = atoi(strtok(date, P3_SPLIT_DATE_STR));
	m = atoi(strtok(NULL, P3_SPLIT_DATE_STR));
	day = atoi(strtok(NULL, P3_SPLIT_DATE_STR));

	if(m == 1 || m == 2){
		youbi = zeller(y-1,m+12,1);
	}
	else{
		youbi = zeller(y,m,1);
	}

	for(i=1;i<=day+youbi;i++){
		if(hantei(y,m,i-youbi,(i-1)%7) == 2){
			flag = 1;
			ret=1;
		}
		else if(hantei(y,m,i-youbi,(i-1)%7) == 1 || flag == 1){
			ret=1;
		}
		else if((i-1)%7 == 6){
			ret=1;
		}
		else{
			flag = 0;
			ret=0;
		}
	}
	return ret;
}

static int zeller(int y,int m,int d)
{
	return ((y + (int)(y/4) - (int)(y/100) + (int)(y/400) + (int)(2.6*m + 1.6) + d) % 7);
}

static int hantei(int y,int m,int d,int youbi)
{
	if((m==3 && d== (int)(20.8431 + 0.242194*(y-1980) - (y-1980)/4)) // t•ª‚Ì“ú
	 || (m==9 && d== (int)(23.2488 + 0.242194*(y-1980) - (y-1980)/4)) // H•ª‚Ì“ú
	 || (m==1 && d==1) // Œ³’U
	 || (m==1 && d>7 && d<15 && youbi==1) // ¬l‚Ì“ú
	 || (m==2 && d==11) // Œš‘‹L”O“ú
	 || (m==4 && d==29) // ‚Ý‚Ç‚è‚Ì“ú
	 || (m==5 && d==3) // Œ›–@‹L”O“ú
	 || (m==5 && d==4) // ‘–¯‚Ìj“ú
	 || (m==5 && d==5) // Žq‹Ÿ‚Ì“ú
	 || (m==7 && d>14 && d<22 && youbi==1) // ŠC‚Ì“ú
	 || (m==9 && d>14 && d<22 && youbi==1) // Œh˜V‚Ì“ú
	 || (m==10 && d>7 && d<15 && youbi==1) // ‘Ìˆç‚Ì“ú
	 || (m==11 && d==3) // •¶‰»‚Ì“ú
	 || (m==11 && d==23) // ‹Î˜JŠ´ŽÓ‚Ì“ú
	 || (m==12 && d==23))// “Vc’a¶“ú
	{
		if(youbi == 0){// “ú—j‚Æ‹x“ú‚ªd‚È‚Á‚½ˆ—
			return 2;
		}
		else{
			return 1;
		}
	}
	if(youbi == 0){ // “ú—j“ú
		return 1;
	}
	return 0;
}
