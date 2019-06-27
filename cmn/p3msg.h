typedef struct {
	unsigned char day[12];	/* YYYY-MM-DD	*/
	unsigned char tim[10];	/* HH:MM:SS	*/
}datetime;

typedef struct{
	long		mtype;
	int 		err;	/* �G���[�R�[�h	*/
	int 		cmd;	/* =0 :�o�� =1 :�ދ�	*/
	unsigned char mtext[20];	/* UUID	*/
	//struct tm	time;	/* ���ԑ�	*/
	datetime	date;
}P3msg;

/*  struct tm {  
   int tm_sec;  // �b (0-59)  
   int tm_min;  // �� (0-59)  
   int tm_hour;  // �� (0-23)  
   int tm_mday;  // �� (1-31)  
   int tm_mon;  // �� - 1 (0-11)  
   int tm_year;  // �N - 1900  
   int tm_wday;  // �j�� (0-6)  
   int tm_yday;  // �N�ʎZ�� (0-365)  
   int tm_isdst;  // �G�ߎ��ԃt���O (-1)  
 };  */
