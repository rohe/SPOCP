/* spocp backend */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>
#include "spocpcli.h"

/*
   arg is composed of the following parts
   spocphost:path:rule

   The native Spocp protocol is used 
 */ 

extern int debug ;
befunc spocp_test ;

int P_spocp_close( void *vp )
{
  SPOCP *spocp ;

  spocp = (SPOCP *) vp ;

  spocpc_send_logout( spocp ) ;
  spocpc_close( spocp ) ;
  free_spocp( spocp ) ;

  return 1 ;
}

spocp_result_t spocp_test(
  element_t *qp, element_t *rp, element_t *xp,  octet_t *arg, pdyn_t *dyn, octet_t *blob )
{
  spocp_result_t r = SPOCP_DENIED ;

  octarr_t   *argv ;
  octet_t    *oct ;
  octnode_t  *on = 0 ;
  char       *path, *server, *query ;
  becon_t    *bc = 0 ;
  SPOCP      *spocp ;

  if( arg == 0 ) return SPOCP_MISSING_ARG ;

  if(( oct = element_atom_sub( arg, xp )) == 0 ) return SPOCP_SYNTAXERROR ;

  argv = oct_split( oct, ';', 0, 0, 0 ) ;

  if( oct != arg ) oct_free( oct ) ;

  debug = 0 ;
  oct = argv->arr[0] ;

  if(( bc = becon_get( oct, dyn->bcp )) == 0 ) {
    server = oct2strdup( oct, 0 ) ;
    traceLog("Spocp query to %s", server ) ;

    spocp = spocpc_open( server ) ;
    free( server ) ;

    if( spocp == 0 ) {
      traceLog("Couldn't open connection") ;
      r = SPOCP_UNAVAILABLE ;
    }
    else if( dyn->size ) {
      if( !dyn->bcp ) dyn->bcp = becpool_new( dyn->size ) ;
      bc = becon_push( oct, &P_spocp_close, (void *) spocp, dyn->bcp ) ;
    }
  }
  else {
    spocp = (SPOCP *) bc->con ;
    /* how to check that the server is still there ?  Except sending a dummy query ? */
    if(( r = spocpc_send_query( spocp, "", "(5:XxXxX)", 0 )) == SPOCP_UNAVAILABLE ) {
      if( spocpc_reopen( spocp  ) == FALSE ) r = SPOCP_UNAVAILABLE ;
    }
  }

  if( r == SPOCP_DENIED ) {
    /* means there is not actual query, just checking if the LDAP server
       is up and running */
    if( argv->n == 1 ) r = SPOCP_SUCCESS ;
    else {
      traceLog( "filedesc: %d", spocp->fd ) ;

      path = oct2strdup( argv->arr[1], 0 ) ;
      query = oct2strdup( argv->arr[2], 0 ) ;

      if(( r = spocpc_send_query( spocp, path, query, &on )) == SPOCP_SUCCESS ) {
        if( on ) {
          octmove( blob, on->oct ) ; /* just pick the first */
          spocpc_octnode_free( on ) ;
        }
      }

      traceLog("Spocp returned:%d", (int) r ) ;
 
      free( path ) ;
      free( query ) ;
    }
  }

  if( bc ) becon_return( bc ) ; 
  else if( spocp ) P_spocp_close( (void *) spocp );

  octarr_free( argv ) ;

  return r ;
}

