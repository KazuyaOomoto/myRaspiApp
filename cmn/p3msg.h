typedef struct {
	unsigned char day[12];	/* YYYY-MM-DD	*/
	unsigned char tim[10];	/* HH:MM:SS	*/
}datetime;

typedef struct{
	long		mtype;
	int 		err;	/* エラーコード	*/
	int 		cmd;	/* =0 :出勤 =1 :退勤	*/
	unsigned char mtext[20];	/* UUID	*/
	//struct tm	time;	/* 時間帯	*/
	datetime	date;
}P3msg;

/*  struct tm {  
   int tm_sec;  // 秒 (0-59)  
   int tm_min;  // 分 (0-59)  
   int tm_hour;  // 時 (0-23)  
   int tm_mday;  // 日 (1-31)  
   int tm_mon;  // 月 - 1 (0-11)  
   int tm_year;  // 年 - 1900  
   int tm_wday;  // 曜日 (0-6)  
   int tm_yday;  // 年通算日 (0-365)  
   int tm_isdst;  // 季節時間フラグ (-1)  
 };  */
