/*!
 * \file be.h
 * \author Roland Hedberg <roland@catalogix.se>
 * \brief Structs used for the backend connection pool
 */
#ifndef __BE_H_
#define __BE_H_

#include <config.h>

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif
#include <time.h>
#include <sys/time.h>

#include <spocp.h>

/*
 * ------------------------------------- 
 */

/*! \brief Function prototype for the connection close function */
typedef int     (closefn) (void *);
/*! \brief Wrapper for the close function */ 
int             P_fclose(void *vp);

/*! \brief Flag denoting that this connection is available */
#define CON_FREE   0
/*! \brief Flag stating that this connection is already in use by someone else */
#define CON_ACTIVE 1

/*! \brief Struct holding information about a backend connection */
typedef struct be_con_t {
	/*! The connection handle */
	void           *con;
	/*! Status of the connection (in use or not) */
	int             status;
	/*! Arguments */
	octet_t        *arg;
	/*! Pointer to a function that can close this connection */
	closefn        *close;
	/*! When last used */
	time_t          last;
	/*! Next */
	struct be_con_t *next;
	/*! Previous */
	struct be_con_t *prev;
#ifdef HAVE_LIBPTHREAD
	/*! Lock, so that two threads can not grab the same connection */
	pthread_mutex_t *c_lock;
#endif
} becon_t;

/*! \brief a pool of backend connections */
typedef struct {
	/*! The first connection in the pool */
	becon_t        *head;
	/*! The last connection in the pool */
	becon_t        *tail;
	/*! The number of connections open */
	size_t          size;
	/*! The maximum number of simultaneous connections open */
	size_t          max;
#ifdef HAVE_LIBPTHREAD
	/*! lock on the connection pool */
	pthread_mutex_t *lock;
#endif
} becpool_t;

/*
 * function prototypes 
 */

/*
becpool_t      *becpool_dup(becpool_t * bcp);
*/

becpool_t      *becpool_new(size_t max);
becon_t        *becon_push(octet_t * a, closefn * cf, void *con,
			   becpool_t * b);
becon_t        *becon_get(octet_t * arg, becpool_t * bcp);
void            becpool_rm(becpool_t * bcp, int close);
void            becon_return(becon_t * bc);
void            becon_rm(becpool_t * bcp, becon_t * bc);
void            becon_update(becon_t * bc, void *con);

/*
 * ------------------------------------- 
 */

#endif
