/***************************************************************************
                          be_lastlogin.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#define _XOPEN_SOURCE 500 /* glibc2 needs this */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <spocp.h>

/* ============================================================= */

spocp_result_t lastlogin_test( octet_t *arg, becpool_t *bcp, octet_t *blob ) ;
spocp_result_t lastlogin_syntax( char *arg ) ;

/* ============================================================= */

/*

  typespec = file ";" since ";" userid ";" ipnum
 
  first argument is filename

  second argument is the graceperiod
  third argument is the key ( user name )
  and the fourth argument is an ipaddress

   The check is whether the ip address is the same
   as the one used at the last login.
   The last login shouldn't be more than half an hour
   away.
*/

/* The format of the file is

Jul 25 13:16:42 catalin pop3d: LOGIN, user=aron, ip=[::ffff:213.114.116.145]
Jul 25 13:16:44 catalin pop3d: LOGOUT, user=aron, ip=[::ffff:213.114.116.145], top=0, retr=0
Jul 25 19:40:44 catalin pop3d: Connection, ip=[::ffff:213.114.116.142]
Jul 25 19:40:44 catalin pop3d: LOGIN, user=aron, ip=[::ffff:213.114.116.142]
Jul 25 19:40:44 catalin pop3d: LOGOUT, user=aron, ip=[::ffff:213.114.116.142], top=0, retr=0
Jul 25 21:53:25 catalin pop3d: Connection, ip=[::ffff:213.67.231.206]
Jul 25 21:53:25 catalin pop3d: LOGIN, user=anna, ip=[::ffff:213.67.231.206]
Jul 25 21:53:27 catalin pop3d: LOGOUT, user=anna, ip=[::ffff:213.67.231.206], top=0, retr=6188
Jul 25 22:14:47 catalin pop3d: Connection, ip=[::ffff:213.67.231.206]
Jul 25 22:14:47 catalin pop3d: LOGIN, user=lars, ip=[::ffff:213.67.231.206]
Jul 25 22:14:47 catalin pop3d: LOGOUT, user=lars, ip=[::ffff:213.67.231.206], top=0, retr=0

*/

spocp_result_t lastlogin_test( octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  spocp_result_t  r = SPOCP_DENIED ;

  char      line[256] ;
  char      *cp, test[128], date[64], *sp, *str = 0 ;
  FILE      *fp = 0 ;
  time_t    t, pt ;
  struct tm tm ;
  char      **hms ;
  int       n, since = 0 ;
  octet_t   **argv ;
  becon_t  *bc = 0 ;

  time(&t);

  strcpy( date, "2002 " ) ;

  argv = oct_split( arg, ';', '\\', 0, 0, &n ) ;

  if( n < 3 ) goto done ;

  str = oct2strdup( argv[0], 0 ) ;

  if( bcp == 0 || ( bc = becon_get( "file", str, bcp )) == 0 ) {
    if(( fp = fopen( str, "r")) == 0 ) {
      r = SPOCP_UNAVAILABLE ;
      goto done ;
    }

    if( bcp ) bc = becon_push( "file", str, &P_fclose, (void *) fp, bcp ) ;
  }
  else {
    fp = (FILE *) bc->con ;
    if( fseek( fp,  0L, SEEK_SET) == -1 ) return SPOCP_UNAVAILABLE ;
  }

  free( str ) ;

  str = oct2strdup( argv[1], 0 ) ;
  hms = line_split( str, ':', 0, 0, 0, &n ) ;
  free( str ) ;

  if( n > 3 ) goto done ;

  if( hms[2] ) { /* hours, minutes and seconds are defined */
    since += 3600 * atoi( hms[0] ) ;
    since += 60 * atoi( hms[1] ) ;
    since += atoi( hms[2] ) ;
  }
  else if( hms[1] ) { /* minutes and seconds are defined */
    since += 60 * atoi( hms[0] ) ;
    since += atoi( hms[1] ) ;
  }
  else { /* only seconds are defined */
    since += atoi( hms[0] ) ;
  }

  charmatrix_free( hms ) ;

  if( since == 0 ) goto done ;

  str = oct2strdup( argv[2], 0 ) ;
  sprintf( test, "LOGIN, user=%s, ", str ) ;
  free( str ) ;

  str = oct2strdup( argv[3], 0 ) ;
  while( fgets( line, 256, fp )) {
    if( strstr( line, test) == 0 ) continue ;

    strncpy( date+5, line, 16 ) ;
    date[21] = 0 ;
    strptime( date, "%Y %b %d %H:%M:%S", &tm );
    pt = mktime(&tm);

    if( t - pt > since ) continue ;

    cp = strstr(line, "ip=[" ) ;
    cp += 4 ;
    if( strncmp( cp, "::ffff:", 7) == 0 ) cp += 7 ;

    sp = find_balancing( cp, '[', ']' ) ;
    *sp = 0 ;

    if( strcmp( cp, str ) == 0 ) {
      r = SPOCP_SUCCESS ;
      break ;
    }
  }

done:
  if( str ) free( str ) ;

  if( bc ) becon_return( bc ) ; 
  else if( fp ) fclose( fp );

  oct_freearr( argv ) ;

  return r ;
}

