#ifndef __DBACK_H
#define __DBACK_H

#include <plugin.h>

struct __dback ;

typedef void *(dbackfn)( void *a, void *b, spocp_result_t *rc ) ;

typedef struct __dback {
  void     *vp ;     /* for the use of the dback backend */
  void     *handle ; /* the library handle */
  dbackfn  *get ;
  dbackfn  *put ;
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
spocp_result_t dback_save( dback_t *, octet_t *, octet_t *, octet_t *, char * ) ; 
spocp_result_t  dback_read( dback_t *, octet_t *, octet_t *, octet_t *, char ** ) ;
void           *dback_open( dback_t *dback, spocp_result_t *r ) ;
spocp_result_t  dback_close( dback_t *dback, void *vp ) ;
octet_t        *dback_first_key( dback_t *, void *, spocp_result_t * ) ;
octet_t        *dback_next_key( dback_t *, void *, octet_t *, spocp_result_t * ) ;
octarr_t       *dback_all_keys( dback_t *dback, spocp_result_t *r ) ;
spocp_result_t  dback_delete( dback_t *dback, char *key ) ;

#endif

