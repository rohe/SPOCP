/* spocp backend */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <spocp.h>
#include <spocpcli.h>

/*
   arg is composed of the following parts
   spocphost:path:rule

   The native Spocp protocol is used 
 */ 

extern int debug ;

int P_spocp_close( void *vp )
{
  SPOCP *spocp ;

  spocp = (SPOCP *) vp ;

  spocpc_send_logout( spocp ) ;
  spocpc_close( spocp ) ;
  free_spocp( spocp ) ;

  return 1 ;
}

int spocp_test( octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  spocp_result_t r = SPOCP_DENIED ;

  octet_t   **argv ;
  octnode_t   on ;
  char       *path, *server, *query ;
  int         n ;
  becon_t    *bc = 0 ;
  SPOCP      *spocp ;

  argv = oct_split( arg, ';', '\\', 0, 0, &n ) ;

  debug = 0 ;
  server = oct2strdup( argv[0], 0 ) ;

  traceLog("Spocp query to %s", server ) ;

  if( bcp == 0 || ( bc = becon_get( "spocp", server, bcp )) == 0 ) {
    if(( spocp = spocpc_open( server )) == 0 ) {
      traceLog("Couldn't open connection") ;
      r = SPOCP_UNAVAILABLE ;
      goto done ;
    }

    if( bcp ) bc = becon_push( "spocp", server, &P_spocp_close, (void *) spocp, bcp ) ;
  }
  else {
    spocp = (SPOCP *) bc->con ;
    /* how to check that the server is still there ?  Except sending a dummy query ? */
    if(( r = spocpc_send_query( spocp, "", "(5:XxXxX)", 0 )) == SPOCP_UNAVAILABLE ) {
      if( spocpc_reopen( spocp  ) == FALSE ) {
        becon_rm( bcp, bc ) ;
        free( server ) ;
        oct_freearr( argv ) ;
        return r ;
      }
    }
  }

  if( n == 0 ) {
    r = SPOCP_SUCCESS ;
    goto done ;
  }

  traceLog( "filedesc: %d", spocp->fd ) ;

  path = oct2strdup( argv[1], 0 ) ;
  query = oct2strdup( argv[2], 0 ) ;
  memset( &on, 0, sizeof( octnode_t )) ;

  if(( r = spocpc_send_query( spocp, path, query, &on )) == SPOCP_SUCCESS ) {
    if( on.oct.size ) {
      octmove( blob, &on.oct ) ;
      if( on.next ) octnode_free( on.next ) ;
    }
  }
  traceLog("Spocp returned:%d", (int) r ) ;
 
  free( path ) ;
  free( query ) ;

done:
  if( bc ) becon_return( bc ) ; 
  else if( spocp ) P_spocp_close( (void *) spocp );

  free( server ) ;
  oct_freearr( argv ) ;

  return r ;
}

