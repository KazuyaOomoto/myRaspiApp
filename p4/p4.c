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

#include "debug.h"
#include "os_wrapper.h"
#include "libpafe.h"

#include "p4msg.h"

#if 0
#define DEBUG (debug)
#else
#define DEBUG
#endif

static pasori		*p;
static felica		*f;
static int			p4msqid;
static	P4msg		msgP4;

static void p4_sig(int signum);
static void p4_destroy_resource(void);

/* Process4 main function */
int p4_main(void)
{
	int		cnt;
	struct	sigaction	sig={0};

	sig.sa_handler=p4_sig;
	sigaction(SIGINT,&sig,NULL);

	if((p4msqid=msg_create("./resource/p4msg",'a'))==-1){
		return 1;
	}

	while(1)
	{
		if(msgrcv(p4msqid,&msgP4,sizeof(msgP4)-sizeof(msgP4.mtype), P4_MSG_UUID_REQ, 0)==-1){
			perror("p4:msgrcv");
		}

		msgP4.mtype = P4_MSG_UUID_RESP;

		p = pasori_open();
		if (!p) {
			DEBUG("p1:usb error\n");
			msgP4.uuid=0;
			msgP4.err=P4_ERR_DEVICE;
			goto End;
		}
		pasori_init(p);
		pasori_set_timeout(p, 1000);
		msgP4.uuid=0;
		msgP4.err=P4_ERR_TIMEOUT;
		cnt=0;
		while((cnt < 10) && (msgP4.err==P4_ERR_TIMEOUT))
		{
			f = felica_polling(p, FELICA_POLLING_ANY, 0, 0);
			if (f) {
				msgP4.uuid = f->IDm[6] + f->IDm[7] * 0x100;
				msgP4.err=P4_ERR_SUCESS;
				DEBUG("#               SN = %d\n", msgP4.uuid);
			}
			cnt++;
		}
		p4_destroy_resource();
End:
		if(msgsnd(p4msqid,&msgP4,sizeof(msgP4)-sizeof(msgP4.mtype),0)==-1){
			perror("p4:msgsnd");
		}

	}

	return 0;
}

static void p4_sig(int signum)
{
	DEBUG("p4_sig\n");
	p4_destroy_resource();
	_exit(1);
}

static void p4_destroy_resource(void)
{
	/* free felica heap	*/
	if(f){
		free(f);
		/* prevent double free */
		f = NULL;
	}
	/* free pasori heap	*/
	if(p){
		pasori_close(p);
		/* prevent double free */
		p = NULL;
	}
}


