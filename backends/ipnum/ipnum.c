/***************************************************************************
                          ipnum.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <spocp.h>

/* ============================================================== */

spocp_result_t ipnum_test( octet_t *arg, becpool_t *bcp, octet_t *blob ) ;

int            ipnumcmp( octet_t *ipnum, char *ipser ) ;
spocp_result_t ipnum_syntax( octet_t *arg ) ;

/* ============================================================== */

/* first argument is filename
   second argument is the key
   and if there is any the third key is a ipnumber
*/

/* The format of the file should be

   comment = '#' whatever
   line    = keyword ':' value *( ',' value )
   value   = ipnumber "/" netpart

   for instance

   staff:130.239.0.0/16,193.193.7.13

*/

int ipnumcmp( octet_t *ipnum, char *ipser )
{
  char          *np, h1[16], h2[19] ;
  int            netpart ;
  struct in_addr ia1, ia2 ;

  strncpy( h1, ipnum->val, ipnum->len ) ;
  if( inet_aton( h1, &ia1 ) == 0 ) return -1 ;

  if( strlen( ipser ) > 18 ) return -1 ;

  strcpy( h2, ipser ) ;
  if(( np = index( h2, '/' ))) *np++ = 0 ; 

  if( inet_aton( h2, &ia2 ) == 0 ) return -1 ;

  /* converts it to network byte order before comparing */
  ia1.s_addr = htonl( ia1.s_addr ) ;
  ia2.s_addr = htonl( ia2.s_addr ) ;

  if( np ) {
    netpart = atoi( np ) ;
    if( netpart > 31 || netpart < 0 ) return 1 ;

    ia1.s_addr >>= netpart ;
    ia2.s_addr >>= netpart ;

  }

  if( ia1.s_addr == ia2.s_addr ) return 0 ;
  else return 1 ;
}

/* returns evaluation */

spocp_result_t ipnum_test( octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  spocp_result_t r = SPOCP_DENIED ;

  octet_t  **argv ;
  char     line[256] ;
  char     *cp, **arr, *file, *sp ;
  FILE     *fp ;
  int      j, n, k, ne ;
  becon_t *bc = 0 ;

  argv = oct_split( arg, ':', '\\', 0, 0, &n ) ;

  file = oct2strdup( argv[0], 0 ) ;

  if( bcp == 0 || ( bc = becon_get( "file", file, bcp )) == 0 ) {

    if(( fp = fopen( file, "r")) == 0 ) {
      r = SPOCP_UNAVAILABLE ;
      goto done ;
    }

    if( bcp ) bc = becon_push( "file", file, &P_fclose, (void *) fp, bcp ) ;
  }
  else {
    fp = (FILE *) bc->con ;
    if( fseek( fp, 0L, SEEK_SET ) == -1 ) return SPOCP_UNAVAILABLE ;
  }

  if( n == 0 ) {
    free( file ) ;
    oct_freearr( argv ) ;
    return r ;
  }

  while( r == SPOCP_DENIED && fgets( line, 256, fp )) {
    if( *line == '#' ) continue ;
    if(( cp = index( line, ':')) == 0 ) continue ;

    rmcrlf( cp ) ;

    *cp++ = 0 ;

    if( oct2strcmp( argv[1], line ) == 0 ) {
      if( n == 1 ) {
        r = SPOCP_SUCCESS ;
        break ;
      }
      else {
        arr = line_split( cp, ',', '\\', 0, 0, &ne ) ;

        for( j = 0 ; j <= ne ; j++ ) {
          sp = rmlt( arr[j] ) ;
          k = ipnumcmp( argv[2], sp ) ;
          if( k == 0 ) break ;
          else if( k < 0 ) {
            r = -1 ;
            break ;
          }
        }

        if( r >= 0 && arr[j] ) {
          r = SPOCP_SUCCESS ;
        }

        charmatrix_free( arr ) ;
      }
    }
  }

done:
  if( bc ) becon_return( bc ) ; 
  else if( fp ) fclose( fp );

  free( file ) ;
  oct_freearr( argv ) ;

  return r ;
}

