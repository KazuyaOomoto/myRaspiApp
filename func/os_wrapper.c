#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>

int sem_create(const char* path, int id, int sem_num)
{
	int semid;
	union semun {
		int val;
		struct semid_ds *buf;
		unsigned short *array;
		struct seminfo *__buf;
	}arg;


	if((semid=semget(ftok(path, id),1,0666|IPC_CREAT))== -1){//キー生成し、セマフォ集合を取得
		perror("semget");
		return -1;
	}

	arg.val=sem_num;
	if(semctl(semid,0,SETVAL,arg)==-1)
	{
		perror("semctl");
		return -1;
	}
	return semid;
}

int sem_lock(int semid)
{
	struct sembuf sb;
	sb.sem_num=0;	//操作するセマフォの番号
	sb.sem_op=-1;	//ロック操作は負の値
	sb.sem_flg=0;
	if(semop(semid,&sb,1)== -1){ //0番目のセマフォの値を-1する
		perror("semop");
		return -1;
	}
	return 1;
}

int sem_unlock(int semid)
{
	struct sembuf sb;
	sb.sem_num=0;	//操作するセマフォの番号
	sb.sem_op=1;	//アンロック操作は正の値
	sb.sem_flg=0;
	if(semop(semid,&sb,1)== -1){ //0番目のセマフォの値を+1する
		perror("semop");
		return -1;
	}
	return 1;
}

int sem_destroy(int semid)
{
	int ret;
	ret = semctl(semid,IPC_RMID,0);
	if(ret==-1){
		perror("semctl");
	}
	return ret;
}

int msg_create(const char* path, int id)
{
	int msgid;
	key_t msgkey;

	if((msgkey=ftok(path, id))==-1){
		perror("ftok");
		return -1;
	}
	
	if((msgid=msgget(msgkey, 0666 | IPC_CREAT))==-1){
		perror("msgget");
		return -1;
	}
	return msgid;
}

int msg_send(int msgid, void* pmsg, size_t msgsize)
{
	if(msgsnd(msgid, (struct msgbuf*)pmsg, msgsize, 0)==-1){
		perror("msgsnd");
		return -1;
	}
	return 1;
}

int msg_rcv(int msgid, void* pmsg, size_t msgsize, long mtype)
{	
	if(msgrcv(msgid, (struct msgbuf*)pmsg, msgsize, mtype, 0)==-1){
		perror("msgrcv");
		return -1;
	}
	return 1;
}

int msg_destroy(int msgid)
{
	int ret;
	ret = msgctl(msgid,IPC_RMID,0);
	if(ret==-1){
		perror("msgctl");
	}
	return ret;
}

int shm_create(const char* path, int id, size_t mem_size)
{
	key_t	shmkey;
	int	shmid;

	if((shmkey=ftok(path, id))==-1){
		perror("ftok");
		return -1;
	}

	if((shmid=shmget(shmkey,mem_size,IPC_CREAT | 0666))==-1)
	{
		perror("shmget");
		return -1;
	}
	return shmid;
}

void* shm_attach(int shmid, const void* addr, int shmflg)
{
	return shmat(shmid,addr,shmflg);	//共有メモリにアタッチ	
}

int shm_detach(void* shm)
{
	return shmdt(shm);		 	//共有メモリにデタッチ
}

int shm_destroy(int shmid)
{
	int ret;
	ret = shmctl(shmid,IPC_RMID,0);
	if(ret==-1){
		perror("shmctl");
	}
	return ret;
}
