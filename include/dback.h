#ifndef __DBACK_H
#define __DBACK_H

#include <plugin.h>

struct __dback ;
struct _dback_cmd_param ;

typedef void *(dbackfn)( struct _dback_cmd_param *, void *, void *, spocp_result_t * ) ;

typedef struct dback_struct dback_t ;

struct dback_struct {
  int               version ;
  int               minor_version ;
  const char       *filename ;
  unsigned long     magic ;  /* So its not at the same place as in plugin_t */
  char             *name ;
  void             *handle ; /* the dynamic library handle */
  void             *conf ;   /* where the plugin can keep its data */

  /* These are absolute necessities */
  dbackfn          *get ;
  dbackfn          *put ;
  dbackfn          *replace ;
  dbackfn          *delete ;
  dbackfn          *allkeys;

  /* These are optional */
  dbackfn          *begin ;
  dbackfn          *end ;
  dbackfn          *commit ;
  dbackfn          *rollback ;

  dbackfn          *init ;

  const conf_com_t *ccmds ;
} ;

typedef struct _dback_cmd_param {
  dback_t *dback ;
  void    *handle ;
} dbcmd_t ;

/* ---------------------------------------------------------------------- */
  
dback_t        *dback_load( char *name, char *load ) ;

/* dback_t        *init_dback( plugin_t *pl ) ; */
spocp_result_t  dback_init( dbcmd_t *dbc ) ;

spocp_result_t  dback_save( dbcmd_t *d, char *, octet_t *, octet_t *, char * ) ; 
spocp_result_t  dback_replace( dbcmd_t *d, char *, octet_t *, octet_t *, char * ) ; 
spocp_result_t  dback_read( dbcmd_t *d, char *, octet_t *, octet_t *, char ** ) ;
spocp_result_t  dback_delete( dbcmd_t *d, char *key ) ;
octarr_t       *dback_all_keys( dbcmd_t *dbc, spocp_result_t *r ) ;

/*
void           *dback_open( dback_t *dback, spocp_result_t *r ) ;
spocp_result_t  dback_close( dback_t *dback, void *vp ) ;
octet_t        *dback_first_key( dback_t *, void *h, spocp_result_t * ) ;
octet_t        *dback_next_key( dback_t *, void *h, octet_t *, spocp_result_t * ) ;
*/

spocp_result_t  dback_begin( dback_t *dback, dbcmd_t * ) ;
spocp_result_t  dback_end( dback_t *dback, dbcmd_t * ) ;
spocp_result_t  dback_commit( dback_t *dback, dbcmd_t * ) ;
spocp_result_t  dback_rollback( dback_t *dback, dbcmd_t * ) ;
#endif

