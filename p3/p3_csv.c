#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "p3msg.h"
#include "os_wrapper.h"
#include "debug.h"

#if 0
#define DEBUG (debug)
#else
#define DEBUG
#endif

#define msgbuf_flush (debug("\n"))
#define empty_data   ("-")

/* structures */
typedef struct csv_template {
    char 	fdate[11];
    char 	fid[20];
    char 	fin[6];
    char 	fout[6];
    char 	fworking[8];
    int		ferr;
} csv_tmp;


typedef void (*p3_error_fnc)(FILE* fp, const P3msg* pmsg, csv_tmp* pcsv);

static int		p3msqid;
static P3msg	msgP3;

static int p3_csv_column_compare(const char*, const char*, int);
static void p3_split_csv_column(csv_tmp*, char*, int, char*);
static void p3_csv_set_modify_row(FILE*, int*, int*);
static void p3_csv_update_error(FILE* fp, const P3msg* pmsg, csv_tmp* pcsv);
static void p3_csv_error0_evt_cmd0(FILE* fp, const P3msg* pmsg, csv_tmp* pcsv);
static void p3_csv_error_success(FILE* fp, const P3msg* pmsg, csv_tmp* pcsv);
static void p3_csv_set_working(const P3msg* pmsg, csv_tmp* pcsv);

static const p3_error_fnc const p3_error_matrix[3][2] = {
/*	err/cmd	*/		/*	0	*/		/*	1	*/
/*	0	*/	{	p3_csv_error0_evt_cmd0,		p3_csv_error_success	},
/*	1	*/	{	NULL,				p3_csv_error_success	},
/*	2	*/	{	p3_csv_error_success,		NULL			},
};

/*	csv file initialization	*/
int p3_csv_init(void)
{
	FILE*	fp;
	char	filename[32]={0};
	char 	line_data[256];
	char	err[6];

	time_t	now = time(NULL);
	struct	tm tim = *localtime(&now);
	int	wflg=1;
	csv_tmp	csv = {0};

	sprintf(filename, "./csv/%04d%02d.csv", tim.tm_year+1900, tim.tm_mon+1);

	/* Open the file for updating (reading and writing) */
	if ((fp = fopen(filename, "r+")) == NULL) {
		if ((fp = fopen(filename, "w"))== NULL){
			printf("p3_csv_init():invaild file\n");
			return 0;
		}
	}

	if(fscanf(fp, "%s\n",line_data) != EOF)
	{

		p3_split_csv_column(&csv, line_data, 0, err);

		/* Some data is written in the first line	*/
		/* Check all query format strings			*/
		if((strcmp((const char*)csv.fdate, "date")==0) &&
		(strcmp((const char*)csv.fid, "id")==0) &&
		(strcmp((const char*)csv.fin, "in")==0) &&
		(strcmp((const char*)csv.fout, "out")==0) &&
		(strcmp((const char*)csv.fworking, "working")==0) &&
		(strcmp(err, "error")==0))
			wflg=0;	/* If all match, writing is unnecessary	*/
		else
			wflg=1;	/* It is necessary to write because it does not match	*/
	}
	else
		wflg=1;	/* Write required to create a new	*/

	/* Need to write	*/
	if(wflg){
		fseek(fp, 0, SEEK_SET);
		/* Write first line in specified format	*/
		fprintf(fp, "date,id,in,out,working,error\n");
	}

	fclose(fp);
	return 1;
}

/*	csv file update		*/
void p3_csv_update(void* pdata)
{
	char	c;
	FILE*	fp;
	char	filename[32]={0};
	time_t	now = time(NULL);
	struct	tm tim = *localtime(&now);
	volatile int	new_record;
	int	i;
	csv_tmp	csv = {0};
	char 	line_data[256];
	P3msg* pmsg = (P3msg*)pdata

	sprintf(filename, "./csv/%04d%02d.csv", tim.tm_year+1900, tim.tm_mon+1);

	new_record=1;	/* Initialize as new registration	*/
	/* Open the file for updating (reading and writing) */
	if ((fp = fopen(filename, "r+")) == NULL) {
		printf("p3_csv_update(): error\n");
		return;
	}

	/* The first line is skipped for formatting	*/
	while ((c = fgetc(fp)) != '\n');

	while(fscanf(fp, "%s\n",line_data) != EOF)
	{
		p3_split_csv_column(&csv, line_data, 1, NULL);

		/* Date and ID match	*/
		if((p3_csv_column_compare(csv.fdate, (const char*)pmsg->date.day, strlen(pmsg->date.day))==0)
		&&(p3_csv_column_compare(csv.fid, (const char*)pmsg->mtext, strlen(pmsg->mtext))==0))
		{
			DEBUG("dupulicated\n");
			/* Cancel new registration	*/
			new_record=0;
			/* Error update process	*/
			p3_csv_update_error(fp, pmsg, &csv);
			break;
		}
	}

	/* sign up	*/
	if(new_record){
		if(pmsg->cmd==0){
			csv.ferr=0;
			fprintf(fp, "%s,%s,%s,%s,%s,%d\n",
					pmsg->date.day,
					pmsg->mtext,
					pmsg->date.tim,
					empty_data,
					empty_data,
					1);
		}
		else
			csv.ferr=2;
	}

	/* Set error code to message	*/
	pmsg->err=csv.ferr;
	DEBUG("err:%d\n",pmsg->err);

	fclose(fp);

}

void p3_csv_exit(void)
{
	remove("./csv/temp.csv");
}

static int p3_csv_column_compare(const char* dst, const char* src, int src_len)
{
	int ret;

	ret = (strlen(dst) != src_len) ? -1 : strncmp(dst, src, src_len);

	return ret;
}

static void p3_split_csv_column(csv_tmp* pcsv, char* pdata, int init, char* opt)
{
	char*	ptr;

	ptr = strtok(pdata, ",");
	strcpy(pcsv->fdate, ptr );
	DEBUG("%s\n", pcsv->fdate);
	ptr = strtok(NULL, ",");
	strcpy(pcsv->fid, ptr);
	DEBUG("%s\n", pcsv->fid);
	ptr = strtok(NULL, ",");
	strcpy(pcsv->fin, ptr);
	DEBUG("%s\n", pcsv->fin);
	ptr = strtok(NULL, ",");
	strcpy(pcsv->fout, ptr);
	DEBUG("%s\n", pcsv->fout);
	ptr = strtok(NULL, ",");
	strcpy(pcsv->fworking, ptr);
	DEBUG("%s\n", pcsv->fworking);

	if(init){
		ptr = strtok(NULL, ",");
		pcsv->ferr = atoi(ptr);
		DEBUG("ferr:%d\n",pcsv->ferr);
	}
	else{
		if(opt){
			ptr = strtok(NULL, ",");
			strcpy(opt, ptr);
			DEBUG("ferr:%s\n",opt);
		}
	}

}

static void p3_csv_update_error(FILE* fp, const P3msg* pmsg, csv_tmp* pcsv)
{
	if(pcsv->ferr < 3 && pmsg->cmd < 2){
		if(p3_error_matrix[pcsv->ferr][pmsg->cmd])
			p3_error_matrix[pcsv->ferr][pmsg->cmd](fp, pmsg, pcsv);
	}
}

static void p3_csv_error0_evt_cmd0(FILE* fp, const P3msg* pmsg, csv_tmp* pcsv)
{
	DEBUG("err/cmd %d,%d\n", pcsv->ferr, pmsg->cmd);
	pcsv->ferr=1;
	/* If no leave time	*/
	if(strcmp(pcsv->fout, empty_data)==0) {
		/* Go back to current line home	*/
		p3_csv_set_modify_row(fp, NULL, NULL);
		/* Set error to 1 and update data	*/
		/* ¦The temp file is not used because the total number of bytes of */
		/*	the file does not increase or decrease	*/
		fprintf(fp, "%s,%s,%s,%s,%s,%d\n",
				pcsv->fdate,
				pcsv->fid,
				pcsv->fin,
				empty_data,
				empty_data,
				pcsv->ferr);
	}

}

static void p3_csv_error_success(FILE* fp, const P3msg* pmsg, csv_tmp* pcsv)
{
	int pos1, pos2;
	char	data[256]={0};
	FILE*	tmp;

	DEBUG("err/cmd %d,%d\n", pcsv->ferr, pmsg->cmd);

	/* Go back to current line home	*/
	p3_csv_set_modify_row(fp, &pos1, &pos2);

	/* Open work csv by writing */
	if ((tmp = fopen("./csv/temp.csv", "w")) == NULL) {
		perror("fopen");
		return ;
	}

	fseek(fp, pos2, SEEK_SET);
	while(fscanf(fp, "%s\n", data) != EOF){
		DEBUG("tmp=%s\n",data);
		fprintf(tmp, "%s\n", data);
	}
	/* Close once */
	fclose(tmp);
	fseek(fp, pos1, SEEK_SET);

	/* Set working time	*/
	p3_csv_set_working(pmsg, pcsv);

	/* Set error to 0 and update data	*/
	pcsv->ferr=0;
	fprintf(fp, "%s,%s,%s,%s,%s,%d\n",
			pcsv->fdate,
			pcsv->fid,
			pcsv->ferr==2 ? pcsv->fout : pcsv->fin,
			pmsg->date.tim,
			pcsv->fworking,	/*	working	*/
			pcsv->ferr);

	/* Open csv for work by reading only */
	if ((tmp = fopen("./csv/temp.csv", "r")) == NULL) {
		perror("fopen");
		return ;
	}
	/* Write to master file	*/
	while(fscanf(tmp, "%s\n", data) != EOF){
		fprintf(fp, "%s\n", data);
	}
	/* close the csv file */
	fclose(tmp);

}

static void p3_csv_set_working(const P3msg* pmsg, csv_tmp* pcsv)
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

	strcpy(fsrcin, pcsv->ferr==2 ? pcsv->fout : pcsv->fin);
	p1 = strtok(fsrcin, ":");
	strcpy(fin1_1,p1);
	while(p1 != NULL) {
		p1= strtok(NULL, ":");
		if(p1 != NULL){
			strcpy(fin1_2, p1);
		}
	}

	strcpy(fsrcout, pmsg->date.tim);
	p2 = strtok(fsrcout, ":");
	strcpy(fout1_1,p2);
	while(p2 != NULL) {
		p2= strtok(NULL, ":");
		if(p2 != NULL){
			strcpy(fout1_2, p2);
		}
	}

	fin1_1_1 = atoi(fin1_1);
	fin1_2_2 = atoi(fin1_2);
	fout1_1_1 = atoi(fout1_1);
	fout1_2_2 = atoi(fout1_2);

	timin = fin1_1_1 * 60 + fin1_2_2;
	tomin = fout1_1_1 * 60 + fout1_2_2;

	sprintf(pcsv->fworking, "%d:%02d", (tomin-timin)/60, (tomin-timin)%60);
}

static void p3_csv_set_modify_row(FILE* fp, int* pbefore, int* pafter )
{
	if(pafter)
		*pafter = ftell(fp);

	do {
		fseek(fp, -1, SEEK_CUR);
	} while(*(fp->_IO_read_ptr)!='\n');

	fseek(fp, 1, SEEK_CUR);

	if(pbefore)
		*pbefore = ftell(fp);
}
