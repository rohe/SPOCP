/***************************************************************************
                          difftime.c  -  description
                             -------------------
    begin                : Thur Jul 10 2003
    copyright            : (C) 2003 by Stockholm University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#define _XOPEN_SOURCE 500 /* glibc2 needs this */
/* #define _POSIX_C_SOURCE 200112 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <spocp.h>

#define MAXPARTS 4 

/* ======================================================== */

spocp_result_t difftime_test( octet_t *arg, becpool_t *b, octet_t *blob ) ;

/* ======================================================== */

/* format of time definition
 diff = YYMMDDTHH:MM:SS;YYMMDD_HH:MM:SS; ('+' / '-' ) ('+' / '-' )

 000007_00:00:00;2003-09-21T08:00:00

 7 days 

 1 year = 365 days
 1 month = 30 days
 1 day = 86400 seconds

 The cases:
 pt = present time (+)
 gt = given time   (o)
 delta = time difference

[--]
 pt < gt && pt + delta <  gt
 +------>+      o

[-+]
 pt < gt && pt + delta >  gt
 +---o-->+      

[+-]
 pt > gt && pt < gt + delta
 +<--o-------+
 
[++]
pt > gt && pt > gt + delta
 o  <--------+
 
 */

time_t diff2seconds( octet_t *arg ) 
{
  time_t n, days = 0, sec = 0 ;
  char   *s = arg->val ;
  
  n = *s++ - '0' ; 
  if( n ) days += n * 3650 ;

  n = *s++ - '0' ;
  if( n ) days += n * 365 ;

  n = *s++ - '0' ;
  if( n ) days += n * 300 ;

  n = *s++ - '0' ;
  if( n ) days += n * 30 ;

  n = *s++ - '0' ;
  if( n ) days += n * 10 ;

  n = *s++ - '0' ;
  if( n ) days += n ;

  s++ ;

  if( days ) sec = days * 86400 ;
 
  n = *s++ - '0' ;
  if( n ) sec += n * 36000 ;

  n = *s++ - '0' ;
  if( n ) sec += n * 3600 ;

  n = *s++ - '0' ;
  if( n ) sec += n * 600 ;

  n = *s++ - '0' ;
  if( n ) sec += n * 60 ;

  n = *s++ - '0' ;
  if( n ) sec += n * 10 ;

  n = *s - '0' ;
  if( n ) sec += n ;

  return sec ;
}

spocp_result_t difftime_test( octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  spocp_result_t res = SPOCP_DENIED ;

  int         n ;
  time_t      pt, gt, sec = 0 ;
  struct tm   ttm ;
  char        tmp[30] ;
  octet_t     **argv ;

  if( arg == 0 ) return SPOCP_MISSING_ARG ;

  /* get the time */
  time(&pt);

  argv = oct_split( arg, ';', 0, 0, 0, &n ) ;

  if( n < 2 || argv[0]->len != 15 ) return res ;

  sec = diff2seconds( argv[0] ) ;

  if( argv[1]->len > 30 ) return SPOCP_SYNTAXERROR;

  strncpy(tmp, argv[1]->val, argv[1]->len ) ;
  tmp[ argv[1]->len ] = 0 ;
  strptime( tmp, "%Y-%m-%dT%H:%M:%S", &ttm );

  gt = mktime( &ttm ) ;

  if( argv[2]->val[0] == '+' ) {
    if( pt > gt ) {
      if( argv[2]->val[1] == '+' ) {
        if(( pt - gt ) > sec ) res = SPOCP_SUCCESS ;
      }
      else if( argv[2]->val[1] == '-' ) {
        if(( pt - gt ) < sec ) res = SPOCP_SUCCESS ;
      }
    } 
  }
  else if( argv[2]->val[0] == '-' ) {
    if( pt < gt ) {
      if( argv[2]->val[1] == '+' ) {
        if(( gt - pt ) > sec ) res = SPOCP_SUCCESS ;
      }
      else if( argv[2]->val[1] == '-' ) {
        if( (gt - pt ) < sec ) res = SPOCP_SUCCESS ;
      }
    } 
  }

  oct_freearr( argv ) ;
  return res ;
}
