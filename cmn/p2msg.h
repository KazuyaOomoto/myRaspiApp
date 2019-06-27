#ifndef _p2msg_h_
#define _p2msg_h_

typedef struct {
	long	mtype;
	long	sound_id;
}P2msg;

#define P2_SOUND_PLAY_REQ		(1)
#define P2_SOUND_STOP_NOTIFY	(2)

#define P2_RUNNING				(0)
#define P2_IDLE					(1)

#endif

