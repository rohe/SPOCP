/***************************************************************************
                          wrappers.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#define _GNU_SOURCE
#include <string.h>

#include <wrappers.h>
#include <func.h>

void *Malloc( size_t size )
{
  void *vp = malloc( size ) ;

  if( vp == 0 ) FatalError("Out of memory",0,0) ;

  return vp ;
}


void *Calloc( size_t n, size_t size )
{
  void *vp = calloc ( n, size ) ;

  if( vp == 0 ) FatalError("Out of memory",0,0) ;

  return vp ;
}


void *Recalloc( void *vp, size_t n, size_t size )
{
  void *nvp = realloc ( vp, n*size ) ;

  if( nvp == 0 ) FatalError("Out of memory",0,0) ;

  return nvp ;
}


void *Realloc( void *vp, size_t n )
{
  void *nvp = realloc ( vp, n ) ;

  if( nvp == 0 ) FatalError("Out of memory",0,0) ;

  return nvp ;
}

char *Strdup( char *s )
{
  char *sp = strdup( s ) ;

  if( s == 0 ) return 0 ;

  sp = strdup( s ) ;

  if( sp == 0 ) FatalError("Out of memory",0,0) ;

  return sp ;
}

char *Strcat( char *dest, char *src, int *size )
{
  char *tmp ;
  int  dl, sl ;

  if( src == 0 || *size <= 0 ) return 0 ;

  dl = strlen(dest) ;
  sl = strlen(src) ;

  if( sl + dl  > *size ) {
    *size = sl + dl + 32 ;
    tmp = Realloc( dest, *size ) ;
    dest = tmp ;
  }

  strcat( dest, src ) ;

  return dest ;
}

#ifndef HAVE_STRNLEN
size_t strnlen( const char *s, size_t len )
{
  size_t i ;

  for( i = 0 ; i < len && s[i] ; i++ ) ;

  return i ;
}
#endif

#ifndef HAVE_STRNDUP
char *strndup( const char *old, size_t sz ) 
{
  size_t len = strnlen( old, sz ) ;
  char   *t  = malloc( len + 1 ) ;

  if( t != NULL ) {
    memcpy( t, old, len ) ;
    t[len] = '\0' ;
  }

  return t ;
}
#endif

char *Strndup( char *s, size_t n )
{
  char *sp = strndup( s, n ) ;

  if( sp == 0 ) FatalError("Out of memory",0,0) ;

  return sp ;
}

/* -------------- special debugging helps ------------ */

void xFree( void *vp )
{
  traceLog("%p-free\n", vp ) ;
  free( vp ) ;
}

void *xMalloc( size_t size )
{
  void *vp = malloc( size ) ;

  traceLog("%p(%d)-malloc\n", vp, size ) ;

  if( vp == 0 ) FatalError("Out of memory",0,0) ;

  return vp ;
}


void *xCalloc( size_t n, size_t size )
{
  void *vp = calloc ( n, size ) ;

  traceLog("%p(%d,%d)-calloc\n", vp, n, size ) ;

  if( vp == 0 ) FatalError("Out of memory",0,0) ;

  return vp ;
}


void *xRecalloc( void *vp, size_t n, size_t size )
{
  void *nvp = realloc ( vp, n*size ) ;

  traceLog("%p->%p(%d)-recalloc\n", vp, nvp, n*size ) ;

  if( nvp == 0 ) FatalError("Out of memory",0,0) ;

  return nvp ;
}


void *xRealloc( void *vp, size_t n )
{
  void *nvp = realloc ( vp, n ) ;

  traceLog("%p->%p(%d)-realloc\n", vp, nvp, n) ;

  if( nvp == 0 ) FatalError("Out of memory",0,0) ;

  return nvp ;
}

char *xStrdup( char *s )
{
  char *sp = strdup( s ) ;

  traceLog("%p(%d)-strdup\n", sp, strlen(s) ) ;

  if( sp == 0 ) FatalError("Out of memory",0,0) ;

  return sp ;
}

char *xStrcat( char *dest, char *src, int *size )
{
  char *tmp ;
  int  dl, sl ;

  dl = strlen(dest) ;
  sl = strlen(src) ;

  if( sl + dl  > *size ) {
    *size = sl + dl + 32 ;
    tmp = xRealloc( dest, *size ) ;
    dest = tmp ;
  }

  strcat( dest, src ) ;

  return dest ;
}

char *xStrndup( char *s, size_t n )
{
  char *sp = strndup( s, n ) ;

  traceLog("%p(%d)-strndup\n", sp, n ) ;

  if( sp == 0 ) FatalError("Out of memory",0,0) ;

  return sp ;
}

int P_fclose( void *vp )
{
  return fclose( (FILE *) vp ) ;
}

