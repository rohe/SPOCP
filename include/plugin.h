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

 
struct _cmd_param ;

/* backend functions */
typedef spocp_result_t (befunc)( struct _cmd_param *cdp, octet_t *blob ) ;
typedef spocp_result_t (beinitfn)( void **conf, struct _pdyn **d ) ;


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

/** function to call for a raw-args */
typedef spocp_result_t (conf_args) (void **conf, void *cmd_data, int argc, char **argv);

typedef struct conf_com_struct {
    /** Name of this command */
    const char *name;
    /** The function to be called when this directive is to be parsed */
    conf_args *func;
    /** Extra data, for functions which implement multiple commands... */
    void *cmd_data;		
    /** 'usage' message, in case of syntax errors */
    const char *errmsg;
} conf_com_t ;

typedef struct _plugin plugin_t ;

struct _plugin {
  int                   version ;
  int                   minor_version ;
  const char           *filename ;
  struct _plugin       *next ;
  unsigned long         magic ;
  pdyn_t               *dyn ;  
  void                 *handle ; /* the dynamic library handle */

  void                 *conf ;   /* where the plugin can keep its data */
  char                 *name ;

  befunc               *test ;
  beinitfn             *init ;
  conf_com_t           *ccmds ;
} ;

#define MODULE_MAGIC_NUMBER_MAJOR 20040321
#define MODULE_MAGIC_NUMBER_MINOR 0                     /* 0...n */
#define MODULE_MAGIC_COOKIE 0x53503230UL /* "SPO20" */

#define SPOCP20_PLUGIN_STUFF	MODULE_MAGIC_NUMBER_MAJOR, \
				MODULE_MAGIC_NUMBER_MINOR, \
				__FILE__, \
				NULL, \
				MODULE_MAGIC_COOKIE, \
                                NULL, \
                                NULL, \
                                NULL, \
                                NULL      

#define SPOCP20_DBACK_STUFF	MODULE_MAGIC_NUMBER_MAJOR, \
				MODULE_MAGIC_NUMBER_MINOR, \
				__FILE__, \
				MODULE_MAGIC_COOKIE, \
                                NULL, \
                                NULL, \
                                NULL      

typedef struct _cmd_param {
  element_t *q ;     /* The query S-expression */
  element_t *r ;     /* The rule S-expression */
  element_t *x ;     /* the result of the 'XPath' parsing of the query S-expression */
  octet_t   *arg ;   /* The plugin specific part of a boundary condition */
  pdyn_t    *pd ;
  void      *conf ;   /* where the plugin can keep its data */
} cmd_param_t ;

/* ---------------------------------------------------------------------- */

time_t       cachetime_set( octet_t *str, cachetime_t *ct ) ;
cachetime_t *cachetime_new( octet_t *s ) ;
void         cachetime_free( cachetime_t *s ) ;

/* ---------------------------------------------------------------------- */

pdyn_t *pdyn_new( int size ) ;
void    pdyn_free( pdyn_t *pdp ) ;

/* ---------------------------------------------------------------------- */

octarr_t *pconf_get_keyval( plugin_t *pl, char *key ) ;
octarr_t *pconf_get_keyval_by_plugin( plugin_t *top, char *pname, char *key ) ;
octarr_t *pconf_get_keys_by_plugin( plugin_t *top, char *pname ) ;

plugin_t  *plugin_match( plugin_t *top, octet_t *oct ) ;
plugin_t  *plugin_add_conf( plugin_t *top, char *pname, char *key, char *val ) ;
int        plugin_add_cachedef( plugin_t *top, char *s ) ;

plugin_t  *init_plugin( plugin_t *top ) ;
void       plugin_display( plugin_t *pl ) ;

plugin_t  *plugin_get( plugin_t *top, char *name ) ;

plugin_t *plugin_load( plugin_t *top, char *name, char *load ) ;

/* ---   cache.c ---- */

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
