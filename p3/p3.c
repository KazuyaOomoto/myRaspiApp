#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tslib.h>

#include "p3msg.h"
#include "os_wrapper.h"
#include "debug.h"
#if 1
#include "p3_mysql.h"
#else
#include "p3_csv.h"
#endif

#if 0
#define DEBUG (debug)
#else
#define DEBUG
#endif

static int		p3msqid;
static P3msg	msgP3;

static void inthandler(int signum);

/*	Process3 main Function	*/
int p3_main(void)
{
	struct	sigaction sig={0};

	sig.sa_handler=inthandler;
	sigaction(SIGINT, &sig, NULL);

	if((p3msqid=msg_create("./resource/p3msg",'a'))==-1){
		return 1;
	}

#if 1
	if(p3_mysql_open()==0){
		return 1;
	}
#else
	/*	csv file initialization	*/
	if(p3_csv_init()==0){
		return 1;
	}
#endif

	while(1)
	{
		if(msgrcv(p3msqid,&msgP3,sizeof(msgP3)-sizeof(msgP3.mtype), 1, 0)==-1){
			perror("p3:msgrcv");
		}

#if 1
		p3_mysql_set_data((void*)&msgP3);
#else
		p3_csv_update((void*)&msgP3);
#endif

		if(msgsnd(p3msqid,&msgP3,sizeof(msgP3)-sizeof(msgP3.mtype),0)==-1){
			perror("p3:msgsnd");
		}

	}

#if 1
	p3_mysql_close();
#else
	p3_csv_exit();
#endif

	return 0;
}

static void inthandler(int signum)
{
#if 1
	p3_mysql_close();
#else
	p3_csv_exit();
#endif
	_exit(1);
}
