/***************************************************************************
                           motd.c  -  description
                             -------------------
    begin                : Thu Oct 30 2003
    copyright            : (C) 2003 by Stockholm University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>

#define DIGITS(n) ( (n) >= 100000 ? 6 : (n) >= 10000 ? 5 : (n) >= 1000 ? 4 : (n) >= 100 ? 3 : ( (n) >= 10 ? 2 : 1 ) )

befunc motd_test ;

char *motd = "/etc/motd" ;
char *type = "Text/plain" ;

spocp_result_t motd_test(
  element_t *e, element_t *r, element_t *x, octet_t *arg, pdyn_t *b, octet_t *blob )
{
  FILE *fp ;
  char  buf[256], *lp ;
  int   len, lt, tot ;

  fp = fopen( motd, "r" ) ;
  if( fp == 0 ) return SPOCP_UNAVAILABLE ;

  lp = fgets( buf, 256, fp ) ;

  if( lp == 0 ) {
    fclose( fp ) ;
    return SPOCP_UNAVAILABLE ;
  }

  lp = &buf[strlen( buf ) - 1] ;

  while( lp != buf && ( *lp == '\r' || *lp == '\n' )) *lp-- = '\0' ;

  len = strlen( buf ) ;
  lt = strlen( type ) ;

  if( len == 0 ) return SPOCP_UNAVAILABLE ;
  tot = len + DIGITS(len) + 2 + lt + DIGITS(lt) ;

  blob->len = blob->size = tot ;
  blob->val = (char *) malloc( tot * sizeof( char )) ;
  sprintf( blob->val, "%d:%s%d:%s", lt, type, len, buf ) ;

  return SPOCP_SUCCESS ;
}

