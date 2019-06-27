#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#include "p1.h"
#include "p2.h"
#include "p3.h"
#include "p4.h"
#include "os_wrapper.h"

static int				p2shmid;
static int				p2msqid;
static int				p3msqid;
static int				p4msqid;

static int create_resource(void);
static int destroy_resource(void);
static void sighdlr(int signum);

int main(int argc,char *argv[])
{
	int p1_id, p2_id, p3_id, p4_id, st;
	struct	sigaction	sig={0};

	sig.sa_handler=sighdlr;
	sigaction(SIGINT,&sig,NULL);

	if(create_resource())exit(1);

	if((p1_id = fork())==0)
	{
		p1_main();
	}

	if((p2_id = fork())==0)
	{
		p2_main();
	}

	if((p3_id = fork())==0)
	{
		p3_main();
	}

	if((p4_id = fork())==0)
	{
		p4_main();
	}

	wait(&st);
	destroy_resource();
	kill(p1_id, SIGTERM);
	kill(p2_id, SIGTERM);
	kill(p3_id, SIGTERM);
	kill(p4_id, SIGTERM);

	return 0;
}

static int create_resource(void)
{
	if((p2msqid=msg_create("./resource/p2msg",'a'))==-1)
		return 1;

	if((p3msqid=msg_create("./resource/p3msg",'a'))==-1)
		return 1;

	if((p4msqid=msg_create("./resource/p4msg",'a'))==-1)
		return 1;

	if((p2shmid=shm_create("./resource/p2shm",'a',1))==-1)
		return 1;

	return 0;
}

static int destroy_resource(void)
{
	if(msg_destroy(p2msqid)==-1)
		return 1;

	if(msg_destroy(p3msqid)==-1)
		return 1;

	if(msg_destroy(p4msqid)==-1)
		return 1;

	if(shm_destroy(p2shmid)==-1)
		return 1;

	return 0;
}

static void sighdlr(int signum)
{
	printf("Terminated +1[%d]\n", getpid());
	destroy_resource();
	exit(1);
}
