#ifndef __BE_H_
#define __BE_H_

#include <config.h>

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif
#include <time.h>
#include <sys/time.h>

#include <spocp.h>

/* ------------------------------------- */

typedef int (closefn)( void * ) ; 
int P_fclose( void *vp ) ;

#define CON_FREE   0
#define CON_ACTIVE 1

typedef struct _be_con {
  void            *con ;
  int             status ;
  octet_t         *arg ;
  closefn         *close ;
  time_t          last ;
  struct _be_con  *next ;
  struct _be_con  *prev ;
#ifdef HAVE_LIBPTHREAD
  pthread_mutex_t *c_lock ;
#endif
} becon_t ;

typedef struct _be_cpool {
  becon_t        *head ;
  becon_t        *tail ;
  size_t          size ;
  size_t          max ;
#ifdef HAVE_LIBPTHREAD
  pthread_mutex_t *lock ;
#endif
} becpool_t ;
  
/* function prototypes */

becon_t   *becon_new( int copy ) ;
becon_t   *becon_dup( becon_t *old ) ;
void       becon_free( becon_t *bc, int close ) ;

becpool_t *becpool_new( size_t max, int copy ) ;
becpool_t *becpool_dup( becpool_t *bcp ) ;
void       becpool_rm( becpool_t *bcp, int close ) ;

becon_t   *becon_push( octet_t *a, closefn *cf, void *con, becpool_t *b ) ;
becon_t   *becon_get( octet_t *arg, becpool_t *bcp ) ;
void       becon_return( becon_t *bc ) ;
void       becon_rm( becpool_t *bcp, becon_t *bc ) ;
void       becon_update( becon_t *bc, void *con ) ;


/* ------------------------------------- */
/* configuration callback function */

typedef spocp_result_t (confgetfn)( void *conf, int arg, char *, void **value ) ;

#endif
