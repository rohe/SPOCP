#ifndef __plugin_h
#define __plugin_h

#include <config.h>

#include <time.h>

#include <struct.h>
#include <be.h>

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#include <spocp.h>

struct _pdyn ;
struct _varr ;

/* backend functions */
typedef spocp_result_t (befunc)(
  element_t *q, element_t *r, element_t *x, octet_t *, struct _pdyn *, octet_t * ) ;
typedef spocp_result_t (beinitfn)( confgetfn *cgf, void *conf, struct _pdyn *d ) ;


/* ---------------------------------------------------------------------- */

#ifdef HAVE_LIBPTHREAD
typedef struct _rdwr_var {
  int readers_reading ;
  int writer_writing ;
  pthread_mutex_t mutex ;
  pthread_cond_t lock_free ;
} pthread_rdwr_t ;
#endif

/* ---------------------------------------------------------------------- */

/****** caching **********************/

typedef struct {
  unsigned int  hash ;
  octet_t       blob ;
  unsigned int  timeout ;
  int           res ;
} cacheval_t ;

typedef struct _cachetime {
  time_t             limit ;
  octet_t            pattern ;
  struct _cachetime *next ;
} cachetime_t ;

/* ------------------------------------ */

typedef struct _cache {
  struct _varr   *va ;
#ifdef HAVE_LIBPTHREAD
  pthread_rdwr_t rw_lock ;
#endif
} cache_t ;

/* ------------------------------------ */

typedef struct _pconf {
  char          *key ;
  octarr_t      *val ;
  struct _pconf *next ;
} pconf_t ;

/* ------------------------------------ */

typedef struct _pdyn {
  cache_t         *cv ;
  cachetime_t     *ct ;
  becpool_t       *bcp ;
  int             size ;
} pdyn_t ;

/* ---------------------------------------------------------------------- */
/* plugin info */

typedef struct _plugin {

#ifdef HAVE_LIBPTHREAD
  pthread_mutex_t mutex ;
#endif

  void            *handle ; /* the dynamic library handle */
  char            *name ;
  pconf_t         *conf ;
  befunc          *test ;
  beinitfn        *init ;
  pdyn_t          dyn ;  

  struct _plugin *next ;
  struct _plugin *prev ;
} plugin_t ;

time_t       cachetime_set( octet_t *str, cachetime_t *ct ) ;
cachetime_t *cachetime_new( octet_t *s ) ;
void         cachetime_free( cachetime_t *s ) ;

/* ---------------------------------------------------------------------- */

octarr_t *pconf_get_keyval( plugin_t *pl, char *key ) ;
octarr_t *pconf_get_keyval_by_plugin( plugin_t *top, char *pname, char *key ) ;
octarr_t *pconf_get_keys_by_plugin( plugin_t *top, char *pname ) ;

plugin_t  *plugin_match( plugin_t *top, octet_t *oct ) ;
plugin_t  *plugin_add_conf( plugin_t *top, char *pname, char *key, char *val ) ;
plugin_t  *plugin_add_cachedef( plugin_t *top, char *pname, char *s ) ;

plugin_t  *init_plugin( plugin_t *top ) ;
void       plugin_display( plugin_t *pl ) ;

plugin_t  *plugin_get( plugin_t *top, char *name ) ;

/* cache.c */

int      cached( cache_t *v, octet_t *arg, octet_t *blob ) ;
void     cached_rm( cache_t *v, octet_t *arg ) ;
cache_t *cache_value( cache_t *, octet_t *, unsigned int , int , octet_t * ) ;

/* void  cacheval_free( cacheval_t * ) ; */

/* ---------------------------------------------------------------------- */
/* rdwr.c */

int pthread_rdwr_init( pthread_rdwr_t *rdwrp ) ;
int pthread_rdwr_rlock( pthread_rdwr_t *rdwrp ) ;
int pthread_rdwr_wlock( pthread_rdwr_t *rdwrp ) ;
int pthread_rdwr_runlock( pthread_rdwr_t *rdwrp ) ;
int pthread_rdwr_wunlock( pthread_rdwr_t *rdwrp ) ;
int pthread_rdwr_destroy( pthread_rdwr_t *p ) ;

/* ---------------------------------------------------------------------- */

#endif
