/* ------------------Include Files------------------	*/
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/shm.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tslib.h>
#include <errno.h>

#include "jpeg.h"
#include "p2msg.h"
#include "p3msg.h"
#include "p4msg.h"
#include "debug.h"
#include "font.h"
#include "utf8to32.h"
#include "os_wrapper.h"

/* ------------------Defines------------------	*/
#define DEVFB			"/dev/fb0"
#define TSNAME			"/dev/input/event0"
#define SCREENWIDTH		800
#define SCREENHEIGHT	480
#define SCREENSIZE		SCREENWIDTH * SCREENHEIGHT * 4

#define SERIAL_DEV		"/dev/ttyUSB0"
#define SERIAL_BAUDRATE	B19200
#define BUF_SIZE		64
#define READ_SIZE		(BUF_SIZE / 2)

#define MSG_SND_SUCCESS	(1)
#define MSG_SND_FAILURE	(0)
#define SESSION_ERR		(-255)

#define FAILURE_TIMEOUT	(3)
#define SESSION_TIMEOUT	(10)

#define RGB(r,g,b)		( ((r) << 16) | ((g) << 8) | (b) )
#define FONTFILE		"./resource/font/aozoramincho/AozoraMinchoMedium.ttf"

#define P1_CMD_NON		(-1)
#define P1_CMD_SHUKKIN	(0)
#define P1_CMD_TAIKIN	(1)

/* for Debug (modify #if 0 to #if 1)	*/
#if 0
#define DEBUG (debug)
#else
#define DEBUG
#endif

/* ------------------Type Definition------------------	*/
/*	Time Update Trigger	*/
typedef struct {
	struct	tm	tim;
	int			update;
}P1_TIMUPDATE;

/*	State Transition ElementNumber	*/
typedef enum {
	top=0,
	send_uuid_req,
	wait_uuid_resp,
	error_session_err,
	error_syukkin_ok,
	error_taikin_ok,
	error_syukkin_zumi,
	error_syukkin_minyuuryoku,
}state;

/*	State Transition Argments	*/
#define P1_STATE_TRANS_ARGMENTS		(state before, const P1_TIMUPDATE* const ptim, int* pcmd)

typedef state (*pfunc) P1_STATE_TRANS_ARGMENTS;

/*	State Transition Controller	*/
typedef struct {
	const pfunc	const	pstat;		/*	State Transition Trigger Function	*/
	const char* const	pimgfile;	/*	Image File to Display				*/
	int					guideid;	/*	Guidance ID in the State			*/
}P1_CTL;

/* ------------------Private Function------------------	*/
/*	State Transition Trigger Function	*/
static state p1_top P1_STATE_TRANS_ARGMENTS;
static state p1_send_uuid_req P1_STATE_TRANS_ARGMENTS;
static state p1_wait_uuid_resp P1_STATE_TRANS_ARGMENTS;
static state p1_error P1_STATE_TRANS_ARGMENTS;
/*	Utility Function	*/
static void p1_sig(int);
static void p1_tmh(int);
static void p1_set_timer_sec(unsigned long);
static void p1_set_timer_usec(unsigned long);
static void p1_stop_timer(void);
static struct tm p1_get_tim(void);
static void p1_tim_update(struct tm*, P1_TIMUPDATE*);
static void p1_contents_init(const struct tm* const);
static int p1_sndmsgto_p3(int, long, const struct tm* const);
static void p1_img_update(state);
static void p1_font_update(state, const struct tm* const);
static void p1_play_guidance(state);
static void p1_stop_guidance(void);
static void p1_wait_guidance_stop(state);
static void p1_contents_update(state, const struct tm* const);
static void p1_state_update(state, state, const struct tm* const);

/* ------------------Variables Definition------------------	*/
static int				fb_fd;		/*	Frame Buffer File Descriptor			*/
static unsigned long	*pfb;		/*	Frame Buffer							*/
static unsigned long	*pwork;		/*	Back Ground Working Buffer				*/
static int				p2shmid;	/*	Shared Memory ID for Process2			*/
static int				p2msqid;	/*	Message Queue ID for Process2			*/
static int				p3msqid;	/*	Message Queue ID for Process3			*/
static int				p4msqid;	/*	Message Queue ID for Process4			*/
static P2msg			msgP2;		/*	Message Buffer Instance for Process2	*/
static P3msg			msgP3;		/*	Message Buffer Instance for Process3	*/
static P4msg			msgP4;		/*	Message Buffer Instance for Process4	*/

/* ------------------Constant Variables Definition------------------	*/
static const P1_CTL p1_ctl[] = {
	{	p1_top,				"./resource/jpeg/top.jpg",							0	},
	{	p1_send_uuid_req,	"./resource/jpeg/input.jpg",						1	},
	{	p1_wait_uuid_resp,	"./resource/jpeg/input.jpg",						1	},
	{	p1_error,			"./resource/jpeg/error_session_err.jpg",			2	},
	{	p1_error,			"./resource/jpeg/error_syukkinsimasita.jpg",		3	},
	{	p1_error,			"./resource/jpeg/error_taikinsimasita.jpg",			4	},
	{	p1_error,			"./resource/jpeg/error_syukkin_zumi.jpg",			5	},
	{	p1_error,			"./resource/jpeg/error_syukkin_minyuuryoku.jpg",	6	}
};

/*	Process1 main function	*/
int p1_main(void)
{
	struct	sigaction	sig={0};
	struct	sigaction	alm={0};

	P1_TIMUPDATE		new;
	struct	tm			old;
	int					isupdate;
	state				stat;
	int					cmd;

	sig.sa_handler=p1_sig;
	sigaction(SIGINT,&sig,NULL);

	alm.sa_handler=p1_tmh;
	sigaction(SIGALRM,&alm,NULL);

	fb_fd=open(DEVFB,O_RDWR);
	if (fb_fd<0) {
		perror("open(fb)");
		return 1;
	}

	pfb = mmap(0, SCREENSIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fb_fd, 0);
	if (pfb == MAP_FAILED){
		perror("mmap");
		return 1;
	}

	pwork = (unsigned long*)malloc(SCREENSIZE);
	if (pwork == NULL){
		perror("malloc");
		return 1;
	}

	if((p2msqid=msg_create("./resource/p2msg",'a'))==-1){
		return 1;
	}

	if((p3msqid=msg_create("./resource/p3msg",'a'))==-1){
		return 1;
	}

	if((p4msqid=msg_create("./resource/p4msg",'a'))==-1){
		return 1;
	}

	if((p2shmid=shm_create("./resource/p2shm",'a',1))==-1){
		return 1;
	}
	DEBUG("p2msqid=%lu\n", p2msqid);
	DEBUG("p3msqid=%lu\n", p3msqid);
	DEBUG("p4msqid=%lu\n", p4msqid);
	DEBUG("p2shmid=%lu\n", p2shmid);

	new.tim = p1_get_tim();
	p1_contents_init(&new.tim);
	stat = top;
	cmd = P1_CMD_NON;

	while(1)
	{
		old = new.tim;
		new.tim = p1_get_tim();
		p1_tim_update(&old, &new);

		stat = p1_ctl[stat].pstat(stat, &new, &cmd);
	}

	free(pwork);
	close(fb_fd);
	return 0;
}

/*	Process1 Initialize Display Contents	*/
static void p1_contents_init(const struct tm* const ptim)
{
	p1_contents_update(top, ptim);
	p1_play_guidance(top);
}

/*	Setting Timer(in seconds)	*/
static void p1_set_timer_sec(unsigned long sec)
{
	struct itimerval val;
	val.it_interval.tv_sec=val.it_value.tv_sec = sec;
	val.it_interval.tv_usec=val.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &val, NULL);
}

/*	Setting Timer(in micro seconds)	*/
static void p1_set_timer_usec(unsigned long usec)
{
	struct itimerval val;
	val.it_interval.tv_sec=val.it_value.tv_sec = 0;
	val.it_interval.tv_usec=val.it_value.tv_usec = usec;
	setitimer(ITIMER_REAL, &val, NULL);
}

/*	Stop Timer	*/
static void p1_stop_timer(void)
{
	struct itimerval val;
	val.it_interval.tv_sec=val.it_value.tv_sec = 0;
	val.it_interval.tv_usec=val.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &val, NULL);
}

/*	Process1 Signal Handler	*/
static void p1_sig(int signum)
{
	DEBUG("p1_sig\n");
	if(pwork){
		free(pwork);
		/*	prevent double free	*/
		pwork=NULL;
	}
	close(fb_fd);
	_exit(1);
}

/*	Process1 Timer(Alarm) Handler	*/
static void p1_tmh(int signum)
{
	p1_stop_timer();
	return;
}

/*	Get Current Time	*/
static struct tm p1_get_tim(void)
{
	time_t		tm;
	struct	tm	tim;

	tm = time(NULL);
	tim = *localtime( &tm );
	tim.tm_year += 1900;
	tim.tm_mon += 1;

	return tim;
}

/*	Update Time Flag	*/
static void p1_tim_update(struct tm* pold, P1_TIMUPDATE* pnew)
{
	/*	In every minites, Contents Refresh	*/
	pnew->update = (pold->tm_min != pnew->tim.tm_min) ?  1 : 0;
}

/*	Update display Image	*/
static void p1_img_update(state after)
{
	load_jpeg((char*)p1_ctl[after].pimgfile, pwork, SCREENWIDTH, SCREENHEIGHT);
}

/*	Update display phrase using by Freetype2	*/
static void p1_font_update(state after, const struct tm* const ptim)
{
	int len,i,w,x,z=0;
	unsigned char str[100];
//	unsigned long str[]={0x3042, 0x3044, 0x3046, 0x3048, 0x304A};	/* test */

	if(after == top)
	{
		const char* days[] = {
			"Sun",
			"Mon",
			"Tue",
			"Wed",
			"Thu",
			"Fri",
			"Sat"
		};
		sprintf(str,"%02d:%02d", ptim->tm_hour, ptim->tm_min);
		x = 240;
		len=strlen(str);
//		len=sizeof(str)/sizeof(*str);
		for(i=0;i<len;i++){
			w = put_char(pwork,FONTFILE,str[i],120,x,-10,
				RGB(0,0,0),RGB(0,0,0));
			x += w;
		}
		sprintf(str,"%04d/%02d/%02d(%s)", 
				ptim->tm_year, 
				ptim->tm_mon, 
				ptim->tm_mday,
				days[ptim->tm_wday]
				);
		x = 130;
		len=strlen(str);
		for(i=0;i<len;i++){
			w = put_char(pwork,FONTFILE,str[i],60,x,110,
				RGB(0,0,0),RGB(0,0,0));
			x += w;
		}
	}
	else
	{
		sprintf(str,"%02d:%02d", ptim->tm_hour, ptim->tm_min);
		x = 15;
		len=strlen(str);
		for(i=0;i<len;i++){
			w = put_char(pwork,FONTFILE,str[i],45,x,0,
				RGB(0,0,0),RGB(0,0,0));
			x += w;
		}
	}
}

/*	Play guidance	*/
static void p1_play_guidance(state no)
{
	char* pos;

	msgP2.mtype = P2_SOUND_PLAY_REQ;
	msgP2.sound_id = p1_ctl[no].guideid;
	DEBUG("msgP2.guideid = %d, p2shmid=%d\n", msgP2.sound_id, p2shmid);
	pos = (char*)shmat(p2shmid, 0, 0);
	DEBUG("p1_play_guidance:p2shmid(attach)=%p(%d), errno=%d\n", pos, p2shmid, errno);
	*pos = P2_RUNNING;
	DEBUG("p1(play):p2shmid=%d\n", *pos);
	shmdt((void*)pos);
	DEBUG("p1_play_guidance:p2shmid(detach)=%p(%d), errno=%d\n", pos, p2shmid,errno);

	if(msgsnd(p2msqid,&msgP2,sizeof(msgP2)-sizeof(msgP2.mtype),0)==-1){
		perror("p1:msgsnd");
	}
	DEBUG("play:msgP2.mtype=%d, msgP2.guideid=%d\n", msgP2.mtype, msgP2.sound_id);
}

/*	Stop guidance	*/
static void p1_stop_guidance(void)
{
	char* pos;
	DEBUG("p2shmid=%d\n", p2shmid);
	pos = (char*)shmat(p2shmid, 0, 0);
	DEBUG("p1_stop_guidance:p2shmid(attach)=%p(%d), errno=%d\n", pos, p2shmid, errno);
	*pos = P2_IDLE;
	shmdt((void*)pos);
	DEBUG("p1_stop_guidance:p2shmid(detach)=%p(%d), errno=%d\n", pos, p2shmid,errno);
	DEBUG("p1(stop):p2shmid=%d\n", *pos);
}

/*	Wait guidance stop	*/
static void p1_wait_guidance_stop(state now)
{
	/* Start guidance waiting timer(15sec)	*/
	p1_set_timer_sec(15);
	while(msgrcv(p2msqid,&msgP2,sizeof(msgP2)-sizeof(msgP2.mtype), P2_SOUND_STOP_NOTIFY, 0)!=-1){
		if(msgP2.sound_id==p1_ctl[now].guideid)break;
	}
	DEBUG("stop:msgP2.mtype=%d, msgP2.guideid=%d\n", msgP2.mtype, msgP2.sound_id);
	p1_stop_timer();

}

/*	Update Display Contents	*/
static void p1_contents_update(state after, const struct tm* const ptim)
{
	/*	Update display Image	*/
	p1_img_update(after);
	/*	Update display phrase	*/
	p1_font_update(after, ptim);
	/*	Reflected in the frame buffer here (screen flicker prevention measures)	*/
	memcpy(pfb, pwork, SCREENSIZE);
}

/*	Process1 state update	*/
static void p1_state_update(state now, state next, const struct tm* const ptim)
{
	/*	Stop current state guidance	*/
	p1_stop_guidance();
	/*	Wait for stopping guidance	*/
	p1_wait_guidance_stop(now);
	/*	Display next state contents	*/
	p1_contents_update(next, ptim);
	/*	Play next state guidance	*/
	p1_play_guidance(next);
}

/*	Process1 Top	*/
static state p1_top P1_STATE_TRANS_ARGMENTS
{
	state				after;
	struct ts_sample	ts_result;
	struct tsdev		*ts;

	after = top;
	memset(&ts_result, 0, sizeof(ts_result));
	ts = ts_open(TSNAME, 0);
	ts_config(ts);
	p1_set_timer_sec(1);
	ts_read(ts, &ts_result, 1);
	p1_stop_timer();
	DEBUG("x = %d, y = %d\n", ts_result.x, ts_result.y);
	if((ts_result.x>=90) && (ts_result.x<=315) && 
	(ts_result.y >= 370) && (ts_result.y <=450))
	{
		after = send_uuid_req;
		*pcmd = P1_CMD_SHUKKIN;
	}
	else if((ts_result.x>=480) && (ts_result.x<=710) && 
	(ts_result.y >= 370) && (ts_result.y <=450))
	{
		after = send_uuid_req;
		*pcmd = P1_CMD_TAIKIN;
	}
	ts_close(ts);

	if(after != top){
		DEBUG("--- top exit ---\n");
		p1_state_update(top,after, &ptim->tim);
	}
	else{
		if(ptim->update)
			p1_contents_update(top, &ptim->tim);
	}
	return after;
}

/*	Send UUID Request to Process4	*/
static state p1_send_uuid_req P1_STATE_TRANS_ARGMENTS
{
	msgP4.mtype=P4_MSG_UUID_REQ;
	if(msgsnd(p4msqid,&msgP4,sizeof(msgP4)-sizeof(msgP4.mtype),0)==-1){
		perror("p1:msgsnd");
	}

	DEBUG("--- send_uuid_req exit ---\n");

	return wait_uuid_resp;
}

/*	Wait UUID Response from Process4	*/
static state p1_wait_uuid_resp P1_STATE_TRANS_ARGMENTS
{
	int		rcv;
	int		result;
	state	after;

	DEBUG("--- wait_uuid_resp enter ---\n");

	after = before;

	rcv=msgrcv(p4msqid,&msgP4,sizeof(msgP4)-sizeof(msgP4.mtype), P4_MSG_UUID_RESP, IPC_NOWAIT);

	if(rcv != -1)
	{
		if(msgP4.err == P4_ERR_SUCESS)
		{
			DEBUG("success\n");
			result=p1_sndmsgto_p3(*pcmd, msgP4.uuid, &ptim->tim);
			if(result==MSG_SND_FAILURE)
				after = (*pcmd==P1_CMD_SHUKKIN) ? error_syukkin_zumi :error_syukkin_minyuuryoku;
			else if (result<0) after = error_session_err;
			else after = (*pcmd==P1_CMD_SHUKKIN) ? error_syukkin_ok : error_taikin_ok;
		}
		else{
			DEBUG("msgP4.mtype=%d, msgP4.uuid=%d, msgP4.err=%d\n",
				msgP4.mtype, msgP4.uuid, msgP4.err);
			DEBUG("session_err\n");
			after = error_session_err;
		}

		*pcmd = P1_CMD_NON;
		DEBUG("--- wait_uuid_resp exit ---\n");
		p1_state_update(before, after, &ptim->tim);
	}
	else
	{
		if(ptim->update)
			p1_contents_update(before, &ptim->tim);
	}

	return after;
}

/*	Process1 Error function	from other Process	*/
static state p1_error P1_STATE_TRANS_ARGMENTS
{
	DEBUG("--- error exit ---\n");

	p1_wait_guidance_stop(before);
	p1_contents_update(top, &ptim->tim);
	p1_play_guidance(top);

	return top;
}

static int p1_sndmsgto_p3( int cmd, long uuid, const struct tm* const ptim )
{
	int	ret;

	DEBUG("p1_sndmsgto_p3\n");

	sprintf(msgP3.mtext, "%d", uuid);
	msgP3.mtext[strlen(msgP3.mtext)]='\0';
	DEBUG("%s", msgP3.mtext);
	msgP3.mtype=1;
	msgP3.cmd = cmd;
	msgP3.err = 0;
	DEBUG("UUID=%s\n", msgP3.mtext);
	sprintf(msgP3.date.day, "%04d-%02d-%02d", ptim->tm_year, ptim->tm_mon, ptim->tm_mday);
	sprintf(msgP3.date.tim, "%02d:%02d:%02d", ptim->tm_hour, ptim->tm_min, ptim->tm_sec);
	DEBUG("strlen(msgP3.date.day)=%d\n", strlen(msgP3.date.day));
	DEBUG("strlen(msgP3.date.tim)=%d\n", strlen(msgP3.date.tim));
	msgP3.date.day[strlen(msgP3.date.day)] = '\0';
	msgP3.date.tim[strlen(msgP3.date.tim)] = '\0';
	DEBUG("day=%s\n", msgP3.date.day);
	DEBUG("tim=%s\n", msgP3.date.tim);

	DEBUG("wait\n");
	p1_set_timer_sec(15);
	if(msgsnd(p3msqid,&msgP3,sizeof(msgP3)-sizeof(msgP3.mtype),0)==-1){
		perror("p1:msgsnd");
		p1_stop_timer();
		return SESSION_ERR;
	}
	DEBUG("p1 snd\n");

	if(msgrcv(p3msqid,&msgP3,sizeof(msgP3)-sizeof(msgP3.mtype), 1, 0)==-1){
		perror("p1:msgrcv");
		p1_stop_timer();
		return SESSION_ERR;
	}

	DEBUG("p1 rcv\n");

	DEBUG("msgP3.err=%d\n",msgP3.err);

	if(msgP3.err == 0)
		ret = MSG_SND_SUCCESS;
	else
		ret = MSG_SND_FAILURE;

	DEBUG("ret=%d\n",ret);
	p1_stop_timer();

	return ret;
}
