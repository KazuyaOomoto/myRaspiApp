#ifndef os_wrapper_h_
#define os_wrapper_h_

extern int sem_create(const char*, int, int);
extern int sem_lock(int);
extern int sem_unlock(int);
extern int sem_destroy(int);
extern int msg_create(const char*, int);
extern int msg_send(int, void*, size_t);
extern int msg_rcv(int, void*, size_t, long);
extern int msg_destroy(int);
extern int shm_create(const char*, int, size_t);
extern void* shm_attach(int, const void*, int);
extern int shm_detach(void*);
extern int shm_destroy(int);

#endif

