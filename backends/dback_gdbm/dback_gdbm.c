#include <stdlib.h>
#include <gdbm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <spocp.h>
#include <be.h>
#include "../../server/srvconf.h"

extern gdbm_error gdbm_errno ;
extern char *gdbm_version ;

static char *gdbmfile = 0 ;

/* function prototype
void *db_gdbm_X( void *v0. void *v1, spocp_result_t *rc ) 
*/

static octet_t *octnew( void ) 
{
  return (octet_t *) calloc( 1, sizeof( octet_t )) ;
}

static octet_t *datum2octet( octet_t *o, datum *d )
{
  if( d->dptr == 0 ) {
    if( o ) o->len = 0 ;
    return o ;
  }

  if( o == 0 ) o = octnew() ;

  o->val = d->dptr ;
  o->len = o->size = d->dsize ;

  return o ; 
}

static void octet2datum( datum *d, octet_t *o )
{
  d->dptr = o->val ;
  d->dsize = o->len ;
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_init( void *vcfg, void *conf, spocp_result_t *rc ) 
{
  confgetfn *cgf = ( confgetfn * ) vcfg ;
  void      *vp ;
  octarr_t  *oa ;

  if( cgf( conf, PLUGIN, "dback", "dbfile", &vp) == SPOCP_SUCCESS ) {
    oa = ( octarr_t * ) vp ;
    gdbmfile = oct2strdup( oa->arr[0], 0 ) ;
    *rc = SPOCP_SUCCESS ;
  }
  else {
    traceLog( "No gdbm file to work with, what ever are you thinking about !" ) ;
    *rc = SPOCP_OPERATIONSERROR ;
  }

  return 0 ;
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_put( void *vkey, void *vdat, spocp_result_t *rc ) 
{
  datum     dkey, dcontent ;
  GDBM_FILE dbf ;
  octet_t   *key = ( octet_t * ) vkey ;
  octet_t   *dat = ( octet_t * ) vdat ;

  dbf = gdbm_open( gdbmfile, 0, GDBM_WRCREAT|GDBM_SYNC, S_IRUSR|S_IWUSR, 0 ) ;

  if( dbf == 0 ) {
    *rc = SPOCP_OPERATIONSERROR ;
    return 0 ;
  } 
  else *rc = SPOCP_SUCCESS ;

  octet2datum( &dkey, key ) ;
  octet2datum( &dcontent, dat ) ;

  if( gdbm_store( dbf, dkey, dcontent, GDBM_INSERT ) == 1 ) 
    *rc = SPOCP_EXISTS ;

  gdbm_close( dbf ) ;

  return 0 ;
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_get( void *vkey, void *null, spocp_result_t *rc ) 
{
  datum     dkey, dcontent ;
  GDBM_FILE dbf ;
  octet_t   *key  = ( octet_t * ) vkey ;
  octet_t   *content = 0 ;

  dbf = gdbm_open( gdbmfile, 0, GDBM_READER, 0, 0 ) ;
  
  if( dbf == 0 ) {
    *rc = SPOCP_OPERATIONSERROR ;
    return 0 ;
  } 
  else *rc = SPOCP_SUCCESS ;

  octet2datum( &dkey, key ) ;

  dcontent = gdbm_fetch( dbf, dkey  ) ;

  gdbm_close( dbf ) ;

  content = datum2octet( content, &dcontent ) ;

  return (void *) content ;
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_delete( void *v0, void *v1, spocp_result_t *rc ) 
{
  datum     dkey ;
  int       res ;
  GDBM_FILE dbf ;
  octet_t   *key = ( octet_t * ) v0 ;

  dbf = gdbm_open( gdbmfile, 0, GDBM_WRITER|GDBM_SYNC, 0, 0 ) ;
  
  if( dbf == 0 ) {
    *rc = SPOCP_OPERATIONSERROR ;
    return 0 ;
  } 
  else *rc = SPOCP_SUCCESS ;

  octet2datum( &dkey, key ) ;

  res = gdbm_delete( dbf, dkey  ) ;

  if( res == -1 ) *rc = SPOCP_UNAVAILABLE ;

  gdbm_close( dbf ) ;

  return 0 ;
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_open( void *v0, void *v1, spocp_result_t *rc )
{
  GDBM_FILE dbf ;

  dbf = gdbm_open( gdbmfile, 0, GDBM_READER, 0, 0 ) ;

  if( dbf == 0 ) {
    *rc = SPOCP_OPERATIONSERROR ;
    return 0 ;
  } 
  else {
    *rc = SPOCP_SUCCESS ;
    return ( void * ) dbf ;
  }
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_close( void *v0, void *v1, spocp_result_t *rc )
{
  GDBM_FILE dbf = ( GDBM_FILE ) v0 ;

  gdbm_close( dbf ) ;

  *rc = SPOCP_SUCCESS ;

  return 0 ;
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_firstkey( void *v0, void *v1, spocp_result_t *rc )
{
  GDBM_FILE dbf = ( GDBM_FILE ) v0 ;
  datum     key ;
  octet_t  *res = 0 ;

  key = gdbm_firstkey( dbf ) ;

  res = datum2octet( res, &key ) ;

  return ( void * ) res ;
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_nextkey( void *v0, void *v1, spocp_result_t *rc )
{
  GDBM_FILE dbf = ( GDBM_FILE ) v0 ;
  datum     key, nextkey ;
  octet_t  *res = 0 ;

  octet2datum( &key, (octet_t *) v0 ) ;

  nextkey = gdbm_nextkey( dbf, key ) ;

  res = datum2octet( res, &nextkey ) ;

  return ( void *) res ;
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_allkeys( void *v0, void *v1, spocp_result_t *rc )
{
  GDBM_FILE dbf ;
  datum     key, nextkey ;
  octet_t  *oct ;
  octarr_t *oa = 0 ;

  dbf = gdbm_open( gdbmfile, 0, GDBM_READER, 0, 0 ) ;
  
  if( dbf == 0 ) {
    *rc = SPOCP_OPERATIONSERROR ;
    return 0 ;
  } 
  else *rc = SPOCP_SUCCESS ;

  key = gdbm_firstkey( dbf ) ;
  while( key.dptr )  {
    oct = datum2octet( 0, &key ) ;
    oa = octarr_add( oa, oct ) ;
    nextkey = gdbm_nextkey( dbf, key ) ;
    key = nextkey ;
  }

  return ( void * ) oa ;
}

/* ---------------------------------------------------------------------- */

