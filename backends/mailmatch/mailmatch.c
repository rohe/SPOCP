/***************************************************************************
                          addrmatch.c  -  description
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

#include <spocp.h>


spocp_result_t addrmatch_test( octet_t *arg, becpool_t *bcp, octet_t *blob ) ;

/*
   The format of arg is:
  
   <filename>:<mailaddress>
*/

/* 
   The format of the file must be

   comment = '#' whatever CR
   line    = addr-spec / xdomain CR
   xdomain = ( "." / "@" ) domain

   addr-spec and domain as defined by RFC 822
*/

spocp_result_t addrmatch_test( octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  spocp_result_t r = SPOCP_DENIED ;

  char     line[512] ;
  char     *filename = 0 ;
  FILE     *fp = 0 ;
  int      len, n ;
  octet_t  **argv ;
  becon_t *bc = 0 ;

  argv = oct_split( arg, ':', '\\', 0, 0, &n ) ;

  filename = oct2strdup( argv[0], 0 ) ;

  if( bcp == 0 || ( bc = becon_get( "file", filename, bcp )) == 0 ) {
    if(( fp = fopen(filename, "r")) == 0 ) {
      r = SPOCP_UNAVAILABLE ;
      goto done ;
    }
    else if( bcp ) bc = becon_push( "file", filename, &P_fclose, (void *) fp, bcp ) ;
  }
  else {
    fp = (FILE *) bc->con ;
    if( fseek( fp, 0L, SEEK_SET) == -1 ) r = SPOCP_UNAVAILABLE ;
  }

  while( r == SPOCP_DENIED && fgets( line, 512, fp )) {
    if( *line == '#' ) continue ;

    rmcrlf( line ) ;
    len = strlen( line ) ;
   
    if(( n =  argv[1]->len - len) <= 0  ) continue ;
     
    if( strncasecmp( &argv[1]->val[n], line, len ) == 0 ) r = SPOCP_SUCCESS ;
  }

done:
  if( filename ) free( filename ) ;

  if( bc ) becon_return( bc ) ; 
  else if( fp ) fclose( fp );

  oct_freearr( argv ) ;

  return r ;
}
