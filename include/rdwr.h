#ifndef __RDWR_H
#define __RDWR_H

#ifdef HAVE_LIBPTHREAD

#include <pthread.h>

typedef struct _rdwr_var {
	int             readers_reading;
	int             writer_writing;
	pthread_mutex_t mutex;
	pthread_cond_t  lock_free;
} pthread_rdwr_t;

int             pthread_rdwr_init(pthread_rdwr_t * rdwrp);
int             pthread_rdwr_rlock(pthread_rdwr_t * rdwrp);
int             pthread_rdwr_wlock(pthread_rdwr_t * rdwrp);
int             pthread_rdwr_runlock(pthread_rdwr_t * rdwrp);
int             pthread_rdwr_wunlock(pthread_rdwr_t * rdwrp);
int             pthread_rdwr_destroy(pthread_rdwr_t * p);

#endif


#endif

