#ifndef __DBACK_H
#define __DBACK_H

#include <plugin.h>

struct __dback ;

typedef void *(dbackfn)( void *h, void *a, void *b, spocp_result_t *rc ) ;

typedef struct __dback {
  void     *vp ;        /* for the use of the dback backend */
  void     *conhandle ; /* transaction/connection handle */
  void     *handle ;    /* the library handle */
  dbackfn  *get ;
  dbackfn  *put ;
  dbackfn  *replace ;
  dbackfn  *delete ;
  dbackfn  *allkeys;

  dbackfn  *begin ;
  dbackfn  *end ;
  dbackfn  *commit ;
  dbackfn  *rollback ;

  dbackfn  *init ;
  dbackfn  *firstkey ;
  dbackfn  *nextkey ;
  dbackfn  *open ;
  dbackfn  *close ;
} dback_t ;
  
dback_t        *init_dback( plugin_t *pl ) ;
spocp_result_t  dback_init( dback_t *dback, void *cfg, void *conf ) ;

spocp_result_t  dback_save( dback_t *, void *,  char *, octet_t *, octet_t *, char * ) ; 
spocp_result_t  dback_replace( dback_t *, void *,  char *, octet_t *, octet_t *, char * ) ; 
spocp_result_t  dback_read( dback_t *, void *, char *, octet_t *, octet_t *, char ** ) ;
spocp_result_t  dback_delete( dback_t *dback, void *h, char *key ) ;

void           *dback_open( dback_t *dback, spocp_result_t *r ) ;
spocp_result_t  dback_close( dback_t *dback, void *vp ) ;
octet_t        *dback_first_key( dback_t *, void *h, spocp_result_t * ) ;
octet_t        *dback_next_key( dback_t *, void *h, octet_t *, spocp_result_t * ) ;
octarr_t       *dback_all_keys( dback_t *dback, void *h, spocp_result_t *r ) ;

void           *dback_begin( dback_t *dback, void *h, spocp_result_t *r ) ;
spocp_result_t  dback_end( dback_t *dback, void *h ) ;
spocp_result_t  dback_commit( dback_t *dback, void *h ) ;
spocp_result_t  dback_rollback( dback_t *dback, void *h ) ;
#endif

