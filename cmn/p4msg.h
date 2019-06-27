#ifndef _p4msg_h_
#define _p4msg_h_

typedef struct {
	long	mtype;
	long	uuid;
	int		err;
}P4msg;

#define P4_MSG_UUID_REQ		(1)
#define P4_MSG_UUID_RESP	(2)

#define P4_ERR_DEVICE	(-1)
#define P4_ERR_TIMEOUT	(0)
#define P4_ERR_SUCESS	(1)

#endif
