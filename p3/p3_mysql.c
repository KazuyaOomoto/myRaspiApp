#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mysql.h>

#include "p3msg.h"
#include "debug.h"
#include "p3_cmn.h"

#if 0
#define DEBUG (debug)
#else
#define DEBUG
#endif

#define DB_SERVER		"192.168.101.101"
#define DB_USER			"pi"
#define DB_PASS			"n4p42032E"

#define DB_NAME			"working_db"
#define DB_PORT			3306
#define TBL_WORKING		"tb_working"
#define TBL_EMPLOYEE	"tb_employee"

#define P3_MYSQL_ARGMENTS	(P3msg* pmsg, MYSQL_ROW row)

typedef void (*p3_error_fnc) P3_MYSQL_ARGMENTS;

static int	p3_mysql_column_compare(const char*, const char*, int);
static void	p3_mysql_update_row P3_MYSQL_ARGMENTS;
static void	p3_mysql_error0_evt_cmd0 P3_MYSQL_ARGMENTS;
static void	p3_mysql_error_success P3_MYSQL_ARGMENTS;
static void p3_mysql_free_result(MYSQL_RES**);

static MYSQL*		conn;
static MYSQL_RES*	res;
static char			sql_query_string[512];

static const p3_error_fnc const p3_error_matrix[3][2] = {
/*	err/cmd	*/		/*	0	*/				/*	1	*/
/*	0	*/	{	p3_mysql_error0_evt_cmd0,	p3_mysql_error_success	},
/*	1	*/	{	NULL,						p3_mysql_error_success	},
/*	2	*/	{	p3_mysql_error_success,		NULL					},
};

/*	mysql initialization	*/
int p3_mysql_open(void)
{
	int			ret;
	char		db_name[256]={0};
	time_t		tm;
	struct	tm	tim;

	ret = 1;
	tm = time(NULL);
	tim = *localtime( &tm );
	tim.tm_year += 1900;
	tim.tm_mon += 1;

	sprintf(db_name, "%s_%d%02d", DB_NAME, tim.tm_year, tim.tm_mon);

	conn = mysql_init(NULL);
	if(!mysql_real_connect(conn, DB_SERVER, DB_USER, DB_PASS, db_name, DB_PORT, NULL, 0)){
		perror("mysql_connect:");
		ret = 0;
	}
	
	return ret;
}

/*	mysql initialization	*/
void p3_mysql_close(void)
{
	p3_mysql_free_result(&res);
	if(conn){
		mysql_close(conn);
		/*	prevent double free	*/
		conn = (MYSQL*)NULL;
	}
}

/*	set data to mysql	*/
void p3_mysql_set_data(void* pdata)
{
	int			new_record;
	P3msg*		pmsg = (P3msg*)pdata;
	MYSQL_ROW	row=NULL;

	new_record=1;	/*	Initialize as new registration	*/

	sprintf(sql_query_string, "SELECT * FROM %s;", TBL_WORKING);

	if(mysql_query(conn, sql_query_string)){
		printf("%s\n", mysql_error(conn));
		return;
	}

	res = mysql_store_result(conn);
	while((row = mysql_fetch_row(res))){
		DEBUG("date: %s, uuid : %s, iw : %s, lw : %s, working_hour : %s, extra_hour : %s, error : %d\n",
			row[0],row[1],row[2],row[3],row[4], row[5], atoi(row[6]));

		/*	Date and ID match	*/
		if((p3_mysql_column_compare(row[0], (const char*)pmsg->date.day, strlen(pmsg->date.day))==0)
		&&(p3_mysql_column_compare(row[1], (const char*)pmsg->mtext, strlen(pmsg->mtext))==0)){
			/*	Cancel new registration	*/
			new_record=0;
			/*	Error update process	*/
			p3_mysql_update_row(pmsg, row);
			break;
		}
	}
	p3_mysql_free_result(&res);

	/*	sign up	*/
	if(new_record){
		if(pmsg->cmd==0){
			pmsg->err=0;
			sprintf(sql_query_string, 
					"INSERT INTO %s VALUE ('%s','%s','%s','0','0','0',1);", 
					TBL_WORKING,
					pmsg->date.day,
					pmsg->mtext,
					pmsg->date.tim);
			printf("%s\n", sql_query_string);
			if(mysql_query(conn, sql_query_string)){
				printf("%s\n", mysql_error(conn));
			}
		}
		else
			pmsg->err=2;
	}

	DEBUG("err:%d\n",pmsg->err);

}

static void p3_mysql_free_result(MYSQL_RES** pres)
{
	if(*pres){
		mysql_free_result(*pres);
		/*	prevent double free	*/
		*pres=(MYSQL_RES*)NULL;
	}
}

static int p3_mysql_column_compare(const char* dst, const char* src, int src_len)
{
	int ret;

	ret = (strlen(dst) != src_len) ? -1 : strncmp(dst, src, src_len);

	return ret;
}

static void p3_mysql_update_row P3_MYSQL_ARGMENTS
{
	int drow = atoi((const char*)row[6]);

	if(drow < 3 && pmsg->cmd < 2){
		if(p3_error_matrix[drow][pmsg->cmd])
			p3_error_matrix[drow][pmsg->cmd](pmsg, row);
	}
}

static void p3_mysql_error0_evt_cmd0 P3_MYSQL_ARGMENTS
{
	DEBUG("err/cmd %d,%d\n", atoi((const char*)row[6]), pmsg->cmd);
	pmsg->err=1;
}

static void p3_mysql_error_success P3_MYSQL_ARGMENTS
{
	P3_TIM_CAL	cal;

	DEBUG("err/cmd %d,%d\n", atoi((const char*)row[6]), pmsg->cmd);

	/* Set working time	*/
	p3_cal_working_tim((const char*)row[2], pmsg, &cal);
	
	/* Set error to 0 and update data	*/
	pmsg->err=0;

	sprintf(sql_query_string, 
			"UPDATE %s SET lw='%s', working_hour='%s', extra_hour='%s', error=0 WHERE dt='%s' AND uuid='%s';", 
			TBL_WORKING,
			pmsg->date.tim,
			cal.working_hour,
			cal.extra_hour,
			pmsg->date.day,
			pmsg->mtext
			);

	if(mysql_query(conn, sql_query_string)){
		printf("%s\n", mysql_error(conn));
	}

}
