/***************************************************************************
                          socket.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/tcp.h>

#include <spocp.h>
#include <struct.h>
#include <macros.h>
#include <func.h>
#include <srv.h>

int  spocp_stream_socket( int lport ) ;
int  spocp_unix_domain_socket( char *uds ) ;
void daemon_init( char *procname, int facility ) ;

typedef struct sockaddr SA ;

#define LISTENQ 256

int lutil_pair( int sds[2] ) ;

/* ---------------------------------------------------------------------- */

int spocp_stream_socket( int lport )
{
  int                lfd ;
  struct sockaddr_in sin ;

  memset(&sin, 0, sizeof(struct sockaddr_in));
  sin.sin_family = AF_INET ;
  sin.sin_addr.s_addr = htonl(INADDR_ANY) ;
  sin.sin_port = htons(lport) ;

  if(( lfd = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) {
    LOG( SPOCP_DEBUG )
      traceLog( "Unable to create socket: %s\n", strerror( errno )) ;
    return -1 ;
  }

#ifdef SO_LINGER
  {
    struct linger ling;

    ling.l_onoff = 1;
    ling.l_linger = 30; /* Just to pick something */

    if (setsockopt(lfd,SOL_SOCKET, SO_LINGER,(char *)&ling,sizeof(struct linger)) < 0) {
       LOG( SPOCP_DEBUG ) traceLog("Unable to enable reuse: %.100s",strerror(errno));
       return -1 ;
    }
  }
#endif
#ifdef SO_REUSEADDR
  {
    int one = 1;

    if (setsockopt(lfd,SOL_SOCKET, SO_REUSEADDR,(char *)&one,sizeof(int)) < 0) {
      LOG( SPOCP_DEBUG ) traceLog("Unable to enable reuse: %.100s",strerror(errno));
      return -1 ;
    }
  }
#endif
/* #ifdef TCP_NODELAY */
  {
    int one = 1;

    if (setsockopt(lfd, IPPROTO_TCP, TCP_NODELAY, (char*)&one,sizeof(int)) < 0) {
      LOG( SPOCP_DEBUG ) traceLog("Unable to disable nagle: %.100s",strerror(errno));
      return -1;
    }
  }
/* #endif */
#ifdef SO_KEEPALIVE
  {
    int one = 1;

    if (setsockopt(lfd,SOL_SOCKET, SO_KEEPALIVE,(char *)&one,sizeof(int)) < 0) {
      LOG( SPOCP_DEBUG ) traceLog("Unable to enable keepalive: %.100s",strerror(errno));
      return -1;
    }
  }
#endif

  if( bind( lfd, (SA *) &sin, sizeof(sin)) == -1 ) {
    perror("Bind") ;
    return -1 ;
  }

  if( listen( lfd, LISTENQ) == -1 ) {
    perror("Listen") ;
    return -1 ;
  }

  return lfd ;
}

int spocp_unix_domain_socket( char *uds )
{
  int                fd ;
  struct sockaddr_un sun ;

  if(( fd = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0 ) {
    LOG( SPOCP_DEBUG )
      traceLog( "Unable to create socket: %s\n", strerror( errno )) ;
    return -1 ;
  }

  unlink( uds ) ;

  memset( &sun, 0, sizeof( struct sockaddr_un )) ;
  sun.sun_family = AF_LOCAL ;
  strcpy( sun.sun_path, uds ) ;

  if( bind( fd, (SA *) &sun, sizeof(sun)) == -1 ) {
    LOG( SPOCP_ERR ) traceLog( "Unable to bind to socket: %s", strerror(errno) ) ;
    return -1 ;
  }

  if( listen( fd, LISTENQ) == -1 ) {
    LOG( SPOCP_ERR ) traceLog( "Unable to start listening on socket: %s", strerror(errno)) ;
    return -1 ;
  }

  return fd ;
}

/* #define MAXFD   64 */

void daemon_init( char *procname, int facility )
{
  /* int i ; */
  pid_t pid ;

  if(( pid = fork()) != 0 )
    exit(0) ;                 /* exit parent */

  if(0) fprintf(stderr, "1st generation\n" ) ;
  setsid() ;
  signal(SIGHUP, SIG_IGN) ; /* ignore SIGHUP */

  if(( pid = fork()) != 0 )
    exit(0) ;                 /* exit 1st child */

  if(0) fprintf(stderr, "2nd generation\n" ) ;

  umask(0) ;

  if(0) fprintf(stderr, "closing filedescriptors\n" ) ;
  /*
  for( i = 0 ; i < MAXFD ; i++ ) close(i) ;
  */
  if(0) fprintf(stderr,"Done daemoning\n" ) ;

  if( lutil_pair( wake_sds ) != 0 ) {
    fprintf( stderr, "Problem in creating wakeup pair" ) ;
    exit( 0 ) ;
  }
}

