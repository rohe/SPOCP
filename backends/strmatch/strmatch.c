#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>

/* 
  There are two valid cases:

  1) string:string
  2) ${...}:${...}

  Example:

  "dc=se,dc=umu,cn=person,uid=benjoh01,-":"dc=se,dc=umu,cn=person,uid=rolhed01,-"
 
  ${0}:${1}

*/

befunc strmatch_test ;

#define CASE       0
#define CASEIGNORE 1

__attribute__((unused)) static element_t *gete( element_t *ep, octet_t *oct ) 
{
  int     rc ;
  octet_t spec ;

  spec.val = &oct->val[2] ;

  if( oct->val[ oct->len - 1 ] != '}' ) return 0 ;

  spec.len = oct->len - 3 ;

  return element_eval( &spec, ep, &rc ) ;
}

spocp_result_t strmatch_test(
  element_t *qp, element_t *rp, element_t *xp, octet_t *arg, pdyn_t *dyn, octet_t *blob )
{
  octarr_t  *argv = 0 ;
  element_t *ce[2], *cen[2] ;
  int        i, r = SPOCP_DENIED, n ;
  octet_t   *oct = 0, *var0 = 0, *var1 = 0 ;

  if( arg == 0 || arg->len == 0 ) return SPOCP_MISSING_ARG ;

  oct = element_atom_sub( arg, xp ) ;

  if( oct ) { /* simple substitutions was OK */

    argv = oct_split( oct, ':', 0, 0, 0 ) ;

    if( argv->n >= 2 ) {
      if( octcmp( argv->arr[0], argv->arr[1] ) == 0 ) r = SPOCP_SUCCESS ;
    }
    else r = SPOCP_MISSING_ARG ;

    octarr_free( argv ) ;

    if( oct != arg ) oct_free( oct ) ;

  }
  else { 
    ce[0] = element_nth( xp, 0 ) ;
    ce[1] = element_nth( xp, 1 ) ;

    if(( n = element_size( ce[0] )) == element_size( ce[1] ) ) {
      for( i = 0 ; i < n ; i++ ) {
        cen[0] = element_nth( ce[0], i ) ;
        cen[1] = element_nth( ce[1], i ) ;

        if( element_type(cen[0]) == SPOC_ATOM && element_type( cen[1] ) == SPOC_ATOM ) {
          var0 = element_data( cen[0] ) ;
          var1 = element_data( cen[1] ) ;
          if( octcmp( var0, var1 ) != 0 ) break ;
        }
        else break ; 
      } 
      if( i == n ) r = SPOCP_SUCCESS ;
    } 
  }

  return r ;
}

/*
spocp_result_t
strmatch_test( element_t *ep, octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  return( _strmatch_test( ep, arg, CASE )) ;
}

spocp_result_t
strcasematch_test( element_t *ep, octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  return( _strmatch_test( ep, arg, CASEIGNORE )) ;
}
*/

