#include <stdlib.h>
#include <gdbm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <dback.h>
#include "../../server/srvconf.h"

extern gdbm_error gdbm_errno ;
extern char *gdbm_version ;

/* function prototypes */

conf_args db_gdbm_set_file ;

dbackfn  db_gdbm_get ;
dbackfn  db_gdbm_put ;
dbackfn  db_gdbm_replace ;
dbackfn  db_gdbm_delete ;
dbackfn  db_gdbm_allkeys;
dbackfn  db_gdbm_begin;

/*  */

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

  if( d->dptr ) o->len = o->size = d->dsize ;
  else o->len = o->size = 0 ;

  return o ; 
}

static void octet2datum( datum *d, octet_t *o )
{
  d->dptr = o->val ;
  d->dsize = o->len ;
}

static void char2datum( datum *d, char *str )
{
  d->dptr = str ;
  if( str ) d->dsize = strlen(str) ;
  else d->dsize = 0 ;
}

/* ---------------------------------------------------------------------- */

spocp_result_t db_gdbm_set_file( void **conf, void *cd, int argc, char **argv ) 
{
  *conf = ( void * ) strdup( argv[0] ) ;

  if( *conf == 0 ) return SPOCP_NO_MEMORY ;
  else return SPOCP_SUCCESS ;
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_put( dbcmd_t *dbc, void *vkey, void *vdat, spocp_result_t *rc ) 
{
  datum     dkey, dcontent ;
  char      *file = (char *) dbc->dback->conf ;
  GDBM_FILE dbf ;
  char      *key = ( char * ) vkey ;
  octet_t   *dat = ( octet_t * ) vdat ;

  if( dbc->handle ) dbf = ( GDBM_FILE ) dbc->handle ;
  else {
    dbf = gdbm_open( file, 0, GDBM_WRCREAT|GDBM_SYNC, S_IRUSR|S_IWUSR, 0 ) ;

    if( dbf == 0 ) {
      *rc = SPOCP_OPERATIONSERROR ;
      return 0 ;
    } 
    else *rc = SPOCP_SUCCESS ;
  }

  char2datum( &dkey, key ) ;
  octet2datum( &dcontent, dat ) ;

  if( gdbm_store( dbf, dkey, dcontent, GDBM_INSERT ) == 1 ) 
    *rc = SPOCP_EXISTS ;

  /* If I got it from someone else I shouldn't close it */
  if( !dbc->handle ) gdbm_close( dbf ) ;

  return 0 ;
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_replace( dbcmd_t *dbc, void *vkey, void *vdat, spocp_result_t *rc ) 
{
  datum     dkey, dcontent ;
  char      *file = (char *) dbc->dback->conf ;
  GDBM_FILE dbf ;
  char      *key = ( char * ) vkey ;
  octet_t   *dat = ( octet_t * ) vdat ;

  if( dbc->handle ) dbf = ( GDBM_FILE ) dbc->handle ;
  else {
    dbf = gdbm_open( file, 0, GDBM_WRCREAT|GDBM_SYNC, S_IRUSR|S_IWUSR, 0 ) ;

    if( dbf == 0 ) {
      *rc = SPOCP_OPERATIONSERROR ;
      return 0 ;
    } 
    else *rc = SPOCP_SUCCESS ;
  }

  char2datum( &dkey, key ) ;
  octet2datum( &dcontent, dat ) ;

  if( gdbm_store( dbf, dkey, dcontent, GDBM_REPLACE ) == 1 ) 
    *rc = SPOCP_EXISTS ;

  gdbm_close( dbf ) ;

  return 0 ;
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_get( dbcmd_t *dbc, void *vkey, void *null, spocp_result_t *rc ) 
{
  datum     dkey, dcontent ;
  char      *file = (char *) dbc->dback->conf ;
  GDBM_FILE dbf ;
  char      *key  = ( char * ) vkey ;
  octet_t   *content = 0 ;

  if( dbc->handle ) dbf = ( GDBM_FILE ) dbc->handle ;
  else {
    dbf = gdbm_open( file, 0, GDBM_READER, 0, 0 ) ;
  
    if( dbf == 0 ) {
      *rc = SPOCP_OPERATIONSERROR ;
      return 0 ;
    } 
    else *rc = SPOCP_SUCCESS ;
  }

  char2datum( &dkey, key ) ;

  dcontent = gdbm_fetch( dbf, dkey  ) ;

  gdbm_close( dbf ) ;

  content = datum2octet( content, &dcontent ) ;

  return (void *) content ;
}

/* ---------------------------------------------------------------------- */

void *db_gdbm_delete( dbcmd_t *dbc, void *v0, void *v1, spocp_result_t *rc ) 
{
  datum     dkey ;
  int       res ;
  char      *gdbmfile = (char *) dbc->dback->conf ;
  GDBM_FILE dbf ;
  char      *key = ( char * ) v0 ;

  if( dbc->handle ) dbf = ( GDBM_FILE ) dbc->handle ;
  else {
    dbf = gdbm_open( gdbmfile, 0, GDBM_WRITER|GDBM_SYNC, 0, 0 ) ;
  
    if( dbf == 0 ) {
      *rc = SPOCP_OPERATIONSERROR ;
      return 0 ;
    } 
    else *rc = SPOCP_SUCCESS ;
  }

  char2datum( &dkey, key ) ;

  res = gdbm_delete( dbf, dkey  ) ;

  if( res == -1 ) *rc = SPOCP_UNAVAILABLE ;

  gdbm_close( dbf ) ;

  return 0 ;
}

/* ---------------------------------------------------------------------- *

void *db_gdbm_open( dbcmd_t *dbc, void *v1, void *v2, spocp_result_t *rc )
{
  char      *gdbmfile = (char *) dbc->conf ;
  GDBM_FILE dbf ;

  *rc = SPOCP_SUCCESS ;

  if( dbc->handle == 0 ) {

    dbf = gdbm_open( gdbmfile, 0, GDBM_READER, 0, 0 ) ;

    dbc->handle = ( void * ) dbf ;

    if( dbf == 0 )  *rc = SPOCP_OPERATIONSERROR ;
  }
  
  return dbc->oonhande ;
}

* ---------------------------------------------------------------------- *

void *db_gdbm_close( dbcmd_t *dbc, void *v1, void *v2, spocp_result_t *rc )
{
  GDBM_FILE dbf ;

  *rc = SPOCP_SUCCESS ;

  if( dbc->handle ) {
    dbf = ( GDBM_FILE ) dbc->handle ;

    gdbm_close( dbf ) ;
  } 

  return 0 ;
}

* ---------------------------------------------------------------------- *

void *db_gdbm_firstkey( dbcmd_t *dbc, void *v1, void *v2, spocp_result_t *rc )
{
  GDBM_FILE dbf = ( GDBM_FILE ) dbc->handle ;
  datum     key ;
  octet_t  *res = 0 ;

  if( dbf == 0 ) {
    *rc = SPOCP_OPERATIONSERROR ;
  } 
  else {
    key = gdbm_firstkey( dbf ) ;
    res = datum2octet( res, &key ) ;
    *rc = SPOCP_SUCCESS ;
  }

  return ( void * ) res ;
}

* ---------------------------------------------------------------------- *

void *db_gdbm_nextkey( dbcmd_t *dbc, void *v1, void *v2, spocp_result_t *rc )
{
  GDBM_FILE dbf = ( GDBM_FILE ) dbc->handle ;
  datum     key, nextkey ;
  octet_t  *res = 0 ;

  if( def == 0 ) {
    *rc = SPOCP_OPERATIONSERROR ;
  }
  else {
    octet2datum( &key, (octet_t *) v1 ) ;
    nextkey = gdbm_nextkey( dbf, key ) ;
    res = datum2octet( res, &nextkey ) ;
    *rc = SPOCP_SUCCESS ;
  }

  return ( void *) res ;
}
  
 * ---------------------------------------------------------------------- */

void *db_gdbm_allkeys( dbcmd_t *dbc, void *v1, void *v2, spocp_result_t *rc )
{
  GDBM_FILE dbf ;
  datum     key, nextkey ;
  octet_t   *oct ;
  octarr_t  *oa = 0 ;
  char      *file = (char *) dbc->dback->conf ;

  if( dbc->handle ) dbf = ( GDBM_FILE ) dbc->handle ;
  else {
    dbf = gdbm_open( file, 0, GDBM_READER, 0, 0 ) ;
  
    if( dbf == 0 ) {
      *rc = SPOCP_OPERATIONSERROR ;
      return 0 ;
    } 
    else *rc = SPOCP_SUCCESS ;
  }

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

void *db_gdbm_begin( dbcmd_t *dbc, void *v1, void *v2, spocp_result_t *rc )
{
  *rc = SPOCP_NOT_SUPPORTED ;

  return 0 ;
}

/* ---------------------------------------------------------------------- */

static conf_com_t gdbm_confcmd[] = {
  { "gdbmfile", db_gdbm_set_file, NULL, "The GDBM data store" },
  {NULL} 
} ;

/* ---------------------------------------------------------------------- */

struct dback_struct gdbm_dback = {
  SPOCP20_DBACK_STUFF,
  db_gdbm_get,
  db_gdbm_put,
  db_gdbm_replace,
  db_gdbm_delete,
  db_gdbm_allkeys,

  db_gdbm_begin,
  NULL,
  NULL,
  NULL,

  NULL, /* No init function */

  gdbm_confcmd 
} ;
