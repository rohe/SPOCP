#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>

#ifdef HAVE_LIBWRAP
#include <tcpd.h>
#endif

#include "srv.h"
#include <spocp.h>
#include <func.h>
#include <macros.h>
#include <wrappers.h>

/* Todo: 
 * o Timeouts and limits on how long connections should be kept when not
 *   actively written to or read from. Also for half-done reads and writes.
 * o Check exception handling
 */


/* ---------------------------------------------------------------------- */

/* !!! This is borrowed from OpenLDAP !!!! */

/* Return a pair of socket descriptors that are connected to each other.
 * The returned descriptors are suitable for use with select(). The two
 * descriptors may or may not be identical; the function may return
 * the same descriptor number in both slots. It is guaranteed that
 * data written on sds[1] will be readable on sds[0]. The returned
 * descriptors may be datagram oriented, so data should be written
 * in reasonably small pieces and read all at once. On Unix systems
 * this function is best implemented using a single pipe() call.
 */

int lutil_pair( int sds[2] )
{
#ifdef HAVE_PIPE
        return pipe( sds );
#else
        struct sockaddr_in si;
        int rc, len = sizeof(si);
        int sd;

        sd = socket( AF_INET, SOCK_DGRAM, 0 );
        if ( sd == SPOCP_SOCKET_INVALID ) {
                return sd;
        }

        (void) memset( (void*) &si, '\0', len );
        si.sin_family = AF_INET;
        si.sin_port = 0;
        si.sin_addr.s_addr = htonl( INADDR_LOOPBACK );

        rc = bind( sd, (struct sockaddr *)&si, len );
        if ( rc == SPOCP_SOCKET_ERROR ) {
                close(sd);
                return rc;
        }

        rc = getsockname( sd, (struct sockaddr *)&si, (socklen_t *) &len );
        if ( rc == SPOCP_SOCKET_ERROR ) {
                close(sd);
                return rc;
        }

        rc = connect( sd, (struct sockaddr *)&si, len );
        if ( rc == SPOCP_SOCKET_ERROR ) {
                close(sd);
                return rc;
        }

        sds[0] = sd;
#if !HAVE_WINSOCK
        sds[1] = dup( sds[0] );
#else
        sds[1] = sds[0];
#endif
        return 0;
#endif
}


void wake_listener(int w) 
{
  if(w) write( wake_sds[1], "0", 1 );
} 

/* ---------------------------------------------------------------------- */


void spocp_srv_run( srv_t *srv )
{

#ifdef HAVE_LIBWRAP
  int        allow;
  extern int allow_severity;
  extern int deny_severity;
  struct request_info req;
#endif /* HAVE_LIBWRAP */

  int                 val, err, maxfd, client, nready, stop_loop = 0, n;
  socklen_t           len ;
  conn_t              *conn ;
  pool_item_t         *pi, *next ;
  struct sockaddr_in  client_addr;
  fd_set              rfds,wfds;
  struct timeval      noto;
  char                hostbuf[NI_MAXHOST+1];
  int                 pe = 1 ;
  
  signal(SIGPIPE,SIG_IGN);
  
  traceLog( "Running server!!" ) ;

  while (!stop_loop) {

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
      
    FD_SET( srv->listen_fd, &rfds );
    FD_SET( wake_sds[0], &rfds );
    maxfd = MAX( wake_sds[0], srv->listen_fd ) ;

    /* Loop through connection list and prune away closed connections, 
       the active are added to the fd_sets 
       Since this routine is the only one that adds and delets connections
       and there is only one thread that runs this routine I don't have to
       lock the connection pool 
    */
    for( pi = afpool_first( srv->connections ) ; pi ; pi = next ) {
      conn = ( conn_t * ) pi->info ;
 
      if(( err = pthread_mutex_lock(&conn->clock)) != 0)
        traceLog("Panic: unable to obtain lock on connection: %.100s",strerror(err));
    
      /*
      DEBUG( SPOCP_DSRV ) traceLog("fd=%d [status: %d, ops_pending:%d]\n",
                                  conn->fd, conn->status, conn->ops_pending);
      */

      /* this since pi might disapear before the next turn of this loop is done */
      next = pi->next ;

      if( conn->stop && conn->ops_pending <= 0 ) {
        if( 0 ) timestamp( "removing connection" ) ;
        conn->close( conn ) ;
        conn_reset( conn ) ;
        /* unlocks the conn struct as a side effect */
        item_return( srv->connections, pi );

        /* release lock */
        if(( err = pthread_mutex_unlock( &conn->clock )) != 0 )
          traceLog("Panic: unable to release lock on connection: %s",strerror(err));

        pe-- ;
        if( 0 ) traceLog("Next is %p", next ) ;
      }
      else { 
        if(!conn->stop) {
          maxfd = MAX( maxfd, conn->fd );
          FD_SET(conn->fd,&rfds);
          FD_SET(conn->fd,&wfds);
        }
    
        if(( err = pthread_mutex_unlock( &conn->clock )) != 0 )
          traceLog("Panic: unable to release lock on connection: %s",strerror(err));

      }
    }

    /* 0.001 seconds timeout */
    noto.tv_sec = 0;
    noto.tv_usec = 1000 ;

    if(0) timestamp( "before select" ) ;
    /* Select on all file descriptors, don't wait forever */
    nready = select(maxfd+1,&rfds,&wfds,NULL, 0 ); 
    if(0) timestamp( "after select" ) ;
      
    /* Check for errors */
    if(nready < 0) {
      if(errno != EINTR) traceLog( "Unable to select: %s\n",strerror(errno));
      continue;
    }
      
    /* Timeout... or nothing there */
    if(nready == 0) continue;
      
    if(0) timestamp( "Readable/Writeable" ) ;

    /* traceLog( "maxfd: %d, nready: %d", maxfd, nready ) ;  */

    if( FD_ISSET( wake_sds[0], &rfds ) ) {
       char c[BUFSIZ];
       read( wake_sds[0], c, sizeof(c) );
    }

    /* 
     *  Check for new connections 
     */
      
    if(FD_ISSET(srv->listen_fd,&rfds)) {

      if(1) timestamp( "New connection" ) ;

      len = sizeof(client_addr);
      client = accept( srv->listen_fd, (struct sockaddr *)&client_addr, &len);
      
      if( client < 0) {
        if(errno != EINTR) traceLog( "Accept error: %.100s",strerror(errno));
        continue;
      }

      DEBUG( SPOCP_DSRV ) traceLog( "Accepted connection on fd=%d\n", client);
      if(0) timestamp( "Accepted" ) ;
   
      val = fcntl(client,F_GETFL,0);
      if(val == -1) {
        traceLog( "Unable to get file descriptor flags on fd=%d: %s", client,strerror(errno));
        close(client);
        goto fdloop;
      }

      if( fcntl( client, F_SETFL, val|O_NONBLOCK ) == -1) {
        traceLog("Unable to set socket nonblock on fd=%d: %s",client,strerror(errno));
        close(client);
        goto fdloop;
      }
    
      /* Get address not hostname of the connection */
      if( (err = getnameinfo((struct sockaddr *)&client_addr, len, hostbuf, NI_MAXHOST,
                            NULL, 0, NI_NUMERICHOST)) ) {
        traceLog("Unable to getnameinfo for fd %d:%.100s",client,strerror(err));
        close(client);
        goto fdloop;
      }

      traceLog( "Accepted connection from %s on fd=%d",hostbuf,client);

      /* determine if libwrap allows the connection */
#ifdef HAVE_LIBWRAP
      /* traceLog( "Is access allowed ?" ) ; */

      allow = hosts_access(request_init(&req,
                                      RQ_DAEMON, srv->id,
                                      RQ_CLIENT_NAME, hostbuf,
                                      RQ_CLIENT_SIN, &client_addr,
                                      RQ_USER, "",
                                      0)) ;

      /* traceLog( "The answer is %d", allow ) ; */

      if( allow == 0 ) {
        close( client ) ;

        traceLog( "connection attempt on %d from %s(%s) DISALLOWED", client, hostbuf) ; 

        continue;
      }
#endif /* HAVE_LIBWRAP */
      
      /* get a connection object */
      pi = afpool_get_empty( srv->connections ) ; 

      if( pi ) conn = (conn_t *) pi->info ;
      else conn = 0 ;

      if( conn == 0 ) {
        traceLog("Unable to get connection for fd=%d",client);
        close(client);
      }
      else {
        /* initialize all the conn values */

        DEBUG( SPOCP_DSRV )traceLog( "Initializing connection to %s", hostbuf ) ;
        conn_setup( conn, srv, client, hostbuf ) ;

        afpool_push_item( srv->connections, pi ) ;

        pe++ ;
      }
      if(1) timestamp( "Done with new connection setup" ) ;
    }

    /* 
     * Main I/O loop: Check all filedescriptors and read and write
     * nonblocking as needed.
     */
      
fdloop:
    for( pi = afpool_first( srv->connections ) ; pi ; pi = pi->next ) {
      spocp_result_t res;
      proto_op       *operation = 0 ;
      /* int            f = 0 ; */
    
      conn = ( conn_t *) pi->info ;

      /* wasn't around when the select was done */
      if( conn->fd > maxfd ) {
        if( 0 ) traceLog( "Not this time ! Too young") ;
        continue ;
      }

      /*
       Don't have to lock the connection, locking the io bufferts are
       quite sufficient 
      if(( err = pthread_mutex_lock(&conn->clock)) != 0)
        traceLog("Panic: unable to obtain lock on connection %d: %s",
                  conn->fd,strerror(err));
      */
    
      if( FD_ISSET( conn->fd, &rfds ) && !conn->stop) {
        if(0) timestamp( "input readable") ; 

        /* read returns number of bytes read */
        n = spocp_conn_read( conn );
        if( 0 ) traceLog( "Read returned %d from %d", n, conn->fd ) ;  
        /* traceLog( "[%s]", conn->in->buf ) ; */
        if( n == 0 ) { /* connection probably terminated by other side */
          conn->stop = 1 ;
          continue ;
        }
        if(0) timestamp( "read con" ) ; 

        res = get_operation( conn, &operation ) ;
        if( 0 ) traceLog( "Getops returned %d", res ) ;
        if(res != SPOCP_SUCCESS) continue ;

        if(0) timestamp( "add work item" ) ; 
        /* this will not happen until the for loop is done */
        tpool_add_work( srv->work, operation, (void *) conn ) ;
      }

      if(FD_ISSET(conn->fd,&wfds)) { 
        if(0) timestamp( "output writeable") ; 

        n = iobuf_content( conn->out ) ;

        if( n == 0 ) iobuf_flush( conn->out ) ;
        else {
          if(0) timestamp( "**writing con" ) ; 
        
          DEBUG( SPOCP_DSRV ) traceLog("Outgoing data on fd %d (%d)\n",conn->fd, n);

          n = spocp_conn_write( conn ) ;

          if( n == -1 ) traceLog("Error in writing to %d", conn->fd );
        } 
      }
    
      /*
      if(( err = pthread_mutex_unlock(&conn->clock)) != 0)
        traceLog("Panic: unable to release lock on connection %d: %s",
                 conn->fd,strerror(err));
       */
    
    }
    if( 0 ) timestamp( "one loop done" ) ;
  }
}
