/***************************************************************************
                          log.c  -  description
                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/


#include <config.h>

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <struct.h>
#include <spocp.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

FILE *spocp_logf = 0 ;
int   spocp_loglevel = 0 ;
int   spocp_debug = 0 ;
int   procid = 0 ;

#ifdef HAVE_LIBPTHREAD
pthread_mutex_t loglock ;
#endif

void traceLog( const char *fmt, ...) ;

spocp_result_t spocp_open_log( char *file, int level )
{
  spocp_loglevel = (level&SPOCP_LEVELMASK) ;
  spocp_debug    = level ;

  if( !procid ) procid = getpid() ;

#ifdef HAVE_LIBPTHREAD
  if( spocp_logf == 0 ) pthread_mutex_init( &loglock, NULL ) ;
  pthread_mutex_lock( &loglock ) ;
#endif

  /* Close the present logfile if there is one */
  if( spocp_logf != 0 && spocp_logf != stderr )
    fclose( spocp_logf ) ;

  if( file ) spocp_logf = fopen( file, "a" ) ;
  else spocp_logf = stderr ;
  
#ifdef HAVE_LIBPTHREAD
  pthread_mutex_unlock( &loglock ) ;
#endif

  if( spocp_logf == 0 ) return SPOCP_OPERATIONSERROR ;
  else {
    traceLog( "Using loglevel %d", spocp_loglevel ) ;
    return SPOCP_SUCCESS ;
  }
}

static void tracelog_doit( const char *fmt, va_list ap)
{
  char      buf[SPOCP_MAXLINE], date[24], *sp = 0 ;
  int       r, len ;
  time_t    t ;
  struct tm tm ;

  if( spocp_logf == 0 ) return ;

  memset( buf, 0, SPOCP_MAXLINE ) ;

  time( &t ) ;
  localtime_r( &t, &tm ) ;
  strftime( date, 24, "%Y-%m-%d %H:%M:%S", &tm ) ;

  /* place the timestamp up front */
  sprintf( buf, "%s [%d]: ", date, procid ) ;
  len = strlen( buf ) ;
  sp = buf + len ;

  r = vsnprintf(sp, SPOCP_MAXLINE - len, fmt, ap);  /* this is safe */

#ifdef HAVE_LIBPTHREAD
  pthread_mutex_lock(&loglock) ;
#endif

  fprintf(spocp_logf, "%s\n", buf ) ;

#ifdef HAVE_LIBPTHREAD
  pthread_mutex_unlock(&loglock) ;
#endif

  /* do I care ?? */
  if( r == -1 || r > SPOCP_MAXLINE - 22 ) {
    ;
  }

  fflush( spocp_logf ) ;

  return;
}

void traceLog( const char *fmt, ...)
{
  va_list    ap;

  va_start(ap, fmt);

  tracelog_doit( fmt, ap);

  va_end(ap);

  return;
}

void FatalError( char *msg, char *s, int i )
{
  traceLog("*** Fatal Error ***") ;
  traceLog(msg,s,i);
  traceLog("*** Shutting down ***") ;
  exit(1);
}

void print_elapsed( char *s, struct timeval start, struct timeval end )
{

  if( end.tv_usec > start.tv_usec ) end.tv_usec -= start.tv_usec ;
  else {
    end.tv_sec-- ;
    end.tv_usec += 1000000 - start.tv_usec ;
  }
  end.tv_sec -= start.tv_sec ;

  if( 0 ) traceLog("%s: %.3ld.%.6ld\n",s,end.tv_sec,end.tv_usec) ;
  else fprintf(stderr, "%s: %.3ld.%.6ld\n",s,end.tv_sec,end.tv_usec) ;
}

void timestamp( char *txt ) 
{
  struct timeval tv ;

  gettimeofday( &tv, 0 ) ;

  traceLog("%s: %ld.%ld",txt, tv.tv_sec, tv.tv_usec) ;
}
