#ifndef __plugin_h
#define __plugin_h

#include <config.h>
#include <time.h>

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#include <spocp.h>

/* backend functions */
typedef spocp_result_t (befunc)( octet_t *arg, struct _be_cpool *bcp, octet_t *b ) ;
typedef spocp_result_t (beinitfn)( confgetfn *cgf, void *conf, struct _be_cpool *bcp ) ;

/* and then plugin specific configuration */

typedef struct _cachetime {
  time_t             limit ;
  octet_t            pattern ;
  struct _cachetime *next ;
} cachetime_t ;

typedef struct _pconf {
  char          *key ;
  octnode_t     *val ;
  struct _pconf *next ;
} pconf_t ;

/* plugin info */

typedef struct _plugin {

#ifdef HAVE_LIBPTHREAD
  pthread_mutex_t mutex ;
#endif

  void            *handle ; /* the dynamic library handle */
  char            *name ;
  cachetime_t     *ct ;
  pconf_t         *conf ;
  befunc          *test ;
  beinitfn        *init ;

  struct _plugin *next ;
  struct _plugin *prev ;
} plugin_t ;

time_t       cachetime_set( octet_t *str, plugin_t *pi ) ;
cachetime_t *cachetime_new( octet_t *s ) ;
void         cachetime_free( cachetime_t *s ) ;

/* ---------------------------------------------------------------------- */

octnode_t *pconf_get_keyval( plugin_t *pl, char *key ) ;
octnode_t *pconf_get_keyval_by_plugin( plugin_t *top, char *pname, char *key ) ;

plugin_t  *plugin_match( plugin_t *top, octet_t *oct ) ;
plugin_t  *plugin_add_conf( plugin_t *top, char *pname, char *key, char *val ) ;
plugin_t  *plugin_add_cachedef( plugin_t *top, char *pname, char *s ) ;

plugin_t  *init_plugin( plugin_t *top ) ;
void       plugin_display( plugin_t *pl ) ;

/* ---------------------------------------------------------------------- */

#endif
