#ifndef __CPOOL_H
#define __CPOOL_H

#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include <struct.h>

typedef int     (closefn) (void *);

#define CON_FREE   0
#define CON_ACTIVE 1

typedef struct _be_con {
	void           *con;
	int             status;
	char           *type;
	char           *arg;
	closefn        *close;
	time_t          last;
	pthread_mutex_t *c_lock;
	struct _be_con *next;
	struct _be_con *prev;
} becon_t;

typedef struct _be_cpool {
	becon_t        *head;
	becon_t        *tail;
	size_t          size;
	size_t          max;
	pthread_mutex_t *lock;
} becpool_t;

/*
 * function prototypes 
 */

becon_t        *becon_new(int copy);
becon_t        *becon_dup(becon_t * old);
void            becon_free(becon_t * bc, int close);

becpool_t      *becpool_new(size_t max, int copy);
becpool_t      *becpool_dup(becpool_t * bcp);
void            becpool_rm(becpool_t * bcp, int close);

becon_t        *becon_push(char *t, char *a, closefn * cf, void *con,
			   becpool_t * b);
becon_t        *becon_get(char *type, char *arg, becpool_t * bcp);
void            becon_return(becon_t * bc);

#endif
