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

void spocp_server_run( srv_t *srv )
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
    maxfd = srv->listen_fd;
      
    if( !first_active( srv->connections) ) {
      if( pe ) pe = 0, traceLog( "Empty queue" ) ; 
    }

    /* Loop through connection list and prune away closed connections */
    for( pi = first_active( srv->connections ) ; pi ; pi = next ) {
      conn = ( conn_t * ) pi->info ;
 
      if(( err = pthread_mutex_lock(&conn->clock)) != 0)
        traceLog("Panic: unable to obtain lock on connection: %.100s",strerror(err));
    
      /*
      DEBUG( SPOCP_DSRV ) traceLog("fd=%d [status: %d, ops_pending:%d]\n",
                                  conn->fd, conn->status, conn->ops_pending);
      */

      if( conn->stop && conn->ops_pending <= 0 ) {
        next = pi->next ;
        conn->close( conn ) ;
        reset_conn( conn ) ;
        /* unlocks the conn struct as a side effect */
        return_item( srv->connections, pi );

        /* remove from the set of file descriptors */
        FD_CLR( conn->fd, &rfds ) ;
        FD_CLR( conn->fd, &wfds ) ;
 
        continue ;
      }
    
      if(!conn->stop) {
        maxfd = MAX( maxfd, conn->fd );
        FD_SET(conn->fd,&rfds);
        FD_SET(conn->fd,&wfds);
      }
    
      if(( err = pthread_mutex_unlock( &conn->clock )) != 0 )
        traceLog("Panic: unable to release lock on connection: %s",strerror(err));

       next = pi->next ;
    }

    /* 0.001 seconds timeout */
    noto.tv_sec = 0;
    noto.tv_usec = 1000 ;

    if(0) timestamp( "before select" ) ;
    /* Select on all file descriptors, don't wait forever */
    nready = select(maxfd+1,&rfds,&wfds,NULL,&noto); 
    if(0) timestamp( "after select" ) ;
      
    /* Check for errors */
    if(nready < 0) {
      if(errno != EINTR) traceLog( "Unable to select: %s\n",strerror(errno));
      continue;
    }
      
    /* Timeout... or nothing there */
    if(nready == 0) continue;
      

    if(0)timestamp( "Readable/Writeable" ) ;

    /* traceLog( "maxfd: %d, nready: %d", maxfd, nready ) ; */

    /* 
     *  Check for new connections 
     */
      
    if(FD_ISSET(srv->listen_fd,&rfds)) {

      if(0) timestamp( "New connection" ) ;

      nready--;
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

      if(fcntl( client, F_SETFL, val|O_NONBLOCK ) == -1) {
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
      
      /* get a connection object, mutex locked as a side effect */
      pi = get_item( srv->connections ) ; 

      if( pi ) conn = (conn_t *) pi->info ;
      else conn = 0 ;

      if( conn == 0 ) {
        traceLog("Unable to get connection for fd=%d",client);
        close(client);
      }
      else {
        /* initialize all the conn values */

        DEBUG( SPOCP_DSRV )traceLog( "Initializing connection to %s", hostbuf ) ;
        init_conn( conn, srv, client, hostbuf ) ;

        /* pthread_mutex_unlock( &conn->lock ) ; */
        pe = 1 ;
      }
      if(0) timestamp( "Done with new connection setup" ) ;
    }

    /* 
     * Main I/O loop: Check all filedescriptors and read and write
     * nonblocking as needed.
     */
      
fdloop:

    for( pi = first_active( srv->connections ) ; pi && ( nready > 0 ) ; pi = pi->next ) {
      spocp_result_t res;
      proto_op       *operation = 0 ;
    
      conn = ( conn_t *) pi->info ;

      /* wasn't around when the select was done */
      if( conn->fd > maxfd ) {
        traceLog( "Not this time ! Too young") ;
        continue ;
      }

      if(( err = pthread_mutex_lock(&conn->clock)) != 0)
        traceLog("Panic: unable to obtain lock on connection %d: %s",
                  conn->fd,strerror(err));
    
      if( FD_ISSET( conn->fd, &rfds ) && !conn->stop) {
        if(0) timestamp( "input readable") ; 
        nready--;

        /* read returns number of bytes read */
        n = conn->readn( conn, conn->in->w, conn->in->left );
        /* traceLog( "Read returned %d", n ) ;  */
        if( n == 0 ) { /* connection terminated by other side */
          conn->stop = 1 ;
          goto next_connection;
        }
        if(0) timestamp( "read con" ) ; 

        conn->in->w += n ;
        conn->in->left -= n ;
        
        res = get_operation( conn, &operation ) ;
        /* traceLog( "Getops returned %d", res ) ;*/
        if(res != SPOCP_SUCCESS) goto next_connection;

        if(0) timestamp( "add work item" ) ; 
        /* this will not happen until the for loop is done */
        tpool_add_work( srv->work, operation, (void *) conn ) ;
      }

      if(FD_ISSET(conn->fd,&wfds)) { /* will happen extremly seldom, since synchronous protocol */
        if(0) timestamp( "output writeable") ; 

        if(!FD_ISSET(conn->fd,&rfds)) nready--;

        n = conn->out->w - conn->out->r ;

        if( n == 0 ) flush_io_buffer( conn->out ) ;
        else {
          if(0) timestamp( "**writing con" ) ; 
        
          DEBUG( SPOCP_DSRV ) traceLog("main: Outgoing data on fd %d\n",conn->fd);
          n = conn->writen( conn, conn->out->r, n ) ;
          if(0) timestamp( "**wrote con" ) ; 

          /* traceLog( "Wrote %d bytes", n ) ; */

          if( n == -1 ) 
            goto next_connection;
        } 
      }
    
next_connection:
    
      if(( err = pthread_mutex_unlock(&conn->clock)) != 0)
        traceLog("Panic: unable to release lock on connection %d: %s",
                 conn->fd,strerror(err));
    
    }
  }
}
