/***************************************************************************
                          be_flatfile.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <spocp.h>

spocp_result_t flatfile_test( octet_t *arg, becpool_t *b, octet_t *blob ) ;

/* first argument is filename
   second argument is the key
   and if there are any the
   third and following arguments
   are permissible values
*/

/* The format of the file should be

   comment = '#' whatever
   line    = keyword [ ':' value *( ',' value ) ]

   for instance

   staff:roland,harald,leif

*/

spocp_result_t flatfile_test( octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  spocp_result_t r = SPOCP_SUCCESS ;

  char      line[256] ;
  char      *cp, **arr, *str, *sp ;
  octet_t   **argv ;
  FILE      *fp ;
  int       j, i, n, ne ;
  becon_t  *bc = 0 ;

  argv = oct_split( arg, ':', 0, 0, 0, &n ) ;

  argv[0]->val[argv[0]->len] = 0 ;

  str = argv[0]->val ;

  if( bcp == 0 || ( bc = becon_get( "file", str, bcp )) == 0 ) {
    if(( fp = fopen( str, "r")) == 0 ) {
      r = SPOCP_UNAVAILABLE ;
    }
    else if( bcp ) bc = becon_push( "file", str, &P_fclose, (void *) fp, bcp ) ;
  }
  else {
    fp = (FILE *) bc->con ;
    if( fseek( fp, 0L, SEEK_SET) == -1 ) r = SPOCP_UNAVAILABLE ;
  }

  if( n == 0 || r != SPOCP_SUCCESS ) goto done ;

  for( i = 1 ; i <= n ; i++ ) argv[i]->val[argv[i]->len] = 0 ;

  r = SPOCP_DENIED ;

  while( r == SPOCP_DENIED && fgets( line, 256, fp )) {
    if( *line == '#' ) continue ;

    rmcrlf( line ) ;

    if(( cp = index( line, ':'))) {
      *cp++ = 0 ;
    }

    if( oct2strcmp( argv[1], line ) == 0 ) {
      if( n == 1 ) {
        r = SPOCP_SUCCESS ;
        break ;
      }
      else if( cp ) {
        arr = line_split( cp, ',', '\\', 0, 0, &ne ) ;

        for( j = 0 ; j <= ne ; j++ ) {
          sp = rmlt( arr[j] ) ;
          for( i = 2 ; i <= n ; i++ ) {
            if( oct2strcmp( argv[i], sp ) == 0 ) break ;
          }

          if( i <= n ) {
            r = SPOCP_SUCCESS ;
            break ;
          }
        }

        charmatrix_free( arr ) ;
      }
    }
  }

done:
  if( bc ) becon_return( bc ) ; 
  else if( fp ) fclose( fp );

  oct_freearr( argv ) ;

  return r ;
}

