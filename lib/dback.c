#include <config.h>

#include <sys/types.h>
#include <string.h>
#include <dlfcn.h>

#include <spocp.h>
#include <dback.h>
#include <wrappers.h>
#include <func.h>
#include <proto.h>

static char *necessary_function[] = { "get", "put", "delete", "allkeys",  0 } ;
static char *optional_function[] = { "init", "firstkey", "nextkey", "open", "close", 0 } ;

dback_t *dback_new( void ) 
{
  return ( dback_t * ) Calloc (1, sizeof( dback_t )) ;
}

dback_t *init_dback( plugin_t *pl )
{
  dback_t   *dback = 0 ;
  dbackfn   *dbf ;
  octarr_t  *oa ;
  char      *error ;
  int        i, j ;
  
  oa = pconf_get_keys_by_plugin( pl, "dback" ) ;
  if( !oa ) {
    traceLog( "No dback definition" ) ;
    return 0 ;
  }
   
  for( j = 0 ; necessary_function[j] ; j++ ) 
    for( i = 0 ; i < oa->n ; i++ ) {
      if( strcmp( necessary_function[j], oa->arr[i]->val ) == 0 ) break ;

    if( i == oa->n ) {
      traceLog( "Necessary dback function \"%s\" missing", necessary_function[j] ) ;
      octarr_free( oa ) ;
      return 0 ; 
    }
  }

  octarr_free( oa ) ;
  oa = 0 ;

  oa = pconf_get_keyval_by_plugin( pl, "dback", "filename" ) ;

  if( !oa ) {
    traceLog( "No link to the db backend library") ;
    return 0 ;
  }

  dback = dback_new() ;

  /* should only be one, disregard the other */
  dback->handle = dlopen( oa->arr[0]->val, RTLD_LAZY ) ;
  if( !dback->handle ) {
    traceLog( "Unable to open %s library: [%s]", oa->arr[0]->val,  dlerror() ) ;
    free( dback ) ;
    return 0 ;
  }  

  octarr_free( oa ) ;
  oa = 0 ;

  /* ------------------------------------------------------------ *
     The necessary functions 
   * ------------------------------------------------------------ */

  for( i = 0 ; necessary_function[i] ; i++ ) {
    oa = pconf_get_keyval_by_plugin( pl, "dback", necessary_function[i] ) ;

    if( oa->n > 1 ) { /* */
      traceLog( "Should only be one %s db backend function defined", necessary_function[i] ) ;
    }

    dbf = ( dbackfn * ) dlsym( dback->handle, oa->arr[0]->val ) ;
    if(( error = dlerror()) != NULL ) {
      traceLog("Couldn't link the %s db backend function", oa->arr[0]->val ) ;
      free( dback ) ;
      return 0 ;
    }

    octarr_free( oa ) ;
    oa = 0 ;

    switch( i ) {
      case 0:
        dback->get = dbf ;
        break ;

      case 1:
        dback->put = dbf ;
        break ;

      case 2:
        dback->delete = dbf ;
        break ;

      case 3:
        dback->allkeys = dbf ;
        break ;
    }
  }

  /* ------------------------------------------------------------ *
     The optional functions 
   * ------------------------------------------------------------ */

  for( i = 0 ; optional_function[i] ; i++ ) {
    oa = pconf_get_keyval_by_plugin( pl, "dback", optional_function[i] ) ;

    if( !oa ) continue ;

    if( oa->n > 1 ) { /* */
      traceLog( "Should only be one %st dback function defined", optional_function[i] ) ;
    }

    dbf = ( dbackfn * ) dlsym( dback->handle, oa->arr[0]->val ) ;
    if(( error = dlerror()) != NULL ) {
      traceLog("Couldn't link the %s dback function", oa->arr[0]->val ) ;
      continue ;
    }

    octarr_free( oa ) ;
    oa = 0 ;

    switch( i ) {
      case 0:
        dback->init = dbf ;
        break ;
    }
  }

  return dback ;
}

octet_t *datum_make( octet_t *rule, octet_t *blob, char *bcname )
{
  octet_t *oct = 0 ;
  int      len = 0 ;
  size_t   size ;
  char    *str ;

  if( !blob && !bcname ) return octdup( rule ) ;

  len = rule->len + 1 + DIGIT( rule->len ) ;
  if( blob ) len += blob->len + 1 + DIGIT( blob->len ) ;
  if( bcname ) len += strlen( bcname ) + 1 + DIGIT( strlen( bcname )) ;

  len += 4 ;

  size = len ;

  str = ( char * ) Malloc ( size ) ;

  sexp_printv( str, &size, "olola", rule, ":", blob, ":", bcname ) ;

  oct = (octet_t *) Malloc ( sizeof( octet_t )) ;

  oct->val = str ;
  oct->size = len ;
  oct->len = len - size ;

  return oct ;
}

/*
 The arg follows the following format
   <rulelen> ':' rule ':' <bloblen> ':' blob ':' <bcnamelen> ':' bcname 
 */
spocp_result_t datum_parse( octet_t *arg, octet_t *rule, octet_t *blob, char **bcname ) 
{
  spocp_result_t r ;
  octet_t        oct ;

  /* the rule spec */
  if(( r = get_str( arg, rule )) != SPOCP_SUCCESS ) return r ;

  if( *arg->val != ':' ) return SPOCP_SYNTAXERROR ;

  arg->val++ ;
  arg->len-- ;

  /* the blob spec */
  if( *arg->val == ':' ) { /* no blob */
    blob->len = 0 ; 
  }
  else {
    if(( r = get_str( arg, blob )) != SPOCP_SUCCESS ) return r ;
  }

  if( *arg->val != ':' ) return SPOCP_SYNTAXERROR ;

  arg->val++ ;
  arg->len-- ;

  /* the bcname spec */

  if( arg->len == 0 ) *bcname = 0 ;
  else {
    if(( r = get_str( arg, &oct )) != SPOCP_SUCCESS ) return r ;

    *bcname = oct.val ;
  }

  return SPOCP_SUCCESS ;
}

spocp_result_t dback_init( dback_t *dback, void *cfg, void *conf )
{
  spocp_result_t r = SPOCP_UNAVAILABLE ;

  if( dback && dback->init ) dback->init( cfg, conf, &r ) ;

  return r ;
}

spocp_result_t dback_save( dback_t *dback, char *uid, octet_t *rule, octet_t *blob, char *bcname ) 
{
  octet_t        *datum ;
  spocp_result_t  r ;
  octet_t         oct ;

  if( dback == 0 ) return SPOCP_SUCCESS ;

  datum = datum_make( rule, blob, bcname ) ;

  oct_assign( &oct, uid ) ;

  dback->put( (void *) &oct, (void *) datum, &r ) ; 

  oct_free( datum ) ;

  return r ;
}

spocp_result_t
dback_read( dback_t *dback, char *uid, octet_t *rule, octet_t *blob, char **bcname ) 
{
  octet_t        *datum = 0 ;
  spocp_result_t  r ;
  octet_t         oct ;

  if( dback == 0 ) {
    return SPOCP_UNAVAILABLE;
  }

  oct_assign( &oct, uid ) ;

  if(( datum = dback->get( (void *) &oct, 0, &r )) == 0 ) {
    return SPOCP_UNAVAILABLE ; 
  }
  else 
    return datum_parse( datum, rule, blob, bcname ) ;
}

void *dback_open( dback_t *dback, spocp_result_t r )
{
  return dback->open( 0, 0, &r ) ;
}

spocp_result_t dback_close( dback_t *dback, void *vp )
{
  spocp_result_t r ;

  dback->close( vp, 0, &r ) ;

  return r ;
}

octet_t *dback_first_key( dback_t *dback, void *vp, spocp_result_t r )
{
  return (octet_t *) dback->firstkey( vp, 0, &r ) ;
}

octet_t *dback_next_key( dback_t *dback, void *vp, octet_t *key, spocp_result_t r )
{
  return (octet_t *) dback->nextkey( vp, (void *) key, &r ) ; 
}

octarr_t *dback_all_keys( dback_t *dback, spocp_result_t r )
{
  if( dback == 0 ) return 0 ;

  return (octarr_t *) dback->allkeys( 0, 0, &r ) ; 
}

spocp_result_t dback_delete( dback_t *dback, char *key )
{
  spocp_result_t r ;
  octet_t oct ;

  if( dback == 0 ) return SPOCP_SUCCESS ;

  oct_assign( &oct, key ) ;

  dback->delete( &oct, 0, &r ) ;

  return r ;
}

