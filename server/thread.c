/***************************************************************************
                          thread.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include "locl.h"

void print_io_buffer( char *s, spocp_iobuf_t *io ) ;

void *thread_main(void *);

socklen_t      addrlen ;

void thread_make(srv_t *srv, int i)
{
  thread_t *threadp ;

  threadp = &srv->worker[i] ;
  threadp->conn = ( conn_t * ) Calloc (1, sizeof( conn_t )) ;
  threadp->conn->srv = srv ;
  threadp->id = i ;

  pthread_create( &(threadp->thread), NULL, &thread_main, (void *) threadp );
  return;         /* main thread returns */
}

void *thread_main(void *arg)
{
  socklen_t               clilen ;
  struct sockaddr_un      cliaddrun ;
  struct sockaddr_storage cliaddrin_storage ;
  thread_t                *local ; 
  conn_t                  *con ;    
  srv_t                   *srv ;
  char                    name[NI_MAXHOST], numname[NI_MAXHOST] ;
  int                     ret ;

  local = ( thread_t *) arg ;

  con = local->conn ;
  srv = con->srv ;

  init_connection( con ) ;

  con->rs = srv->root ;
  con->tls = 0 ;

  /*LOG(SPOCP_DEBUG)*/ traceLog("thread %d starting", local->id);

  for ( ; ; ) {
    pthread_mutex_lock(&srv->mlock);

    switch( srv->type ) {
      case  AF_INET : 
      case  AF_INET6 : 
        clilen = sizeof( cliaddrin_storage ) ; 

        /* traceLog("[%d] Waiting for accept", local->id) ; */

        con->fd = accept( srv->listen_fd,
                          (struct sockaddr *) &cliaddrin_storage, &clilen);
        if( con->fd < 0 ) {
          LOG( SPOCP_ERR ) 
            traceLog( "connection attempt failed to accept") ;
          pthread_mutex_unlock( &srv->mlock ) ;
          continue ;
        } 

        /* traceLog( "Got accept, getnameinfo") ; */

        ret = getnameinfo((struct sockaddr *)&cliaddrin_storage, clilen,
                          name, sizeof(name), NULL, 0, 0);

        if (ret != 0) strncpy(name, "unknown", sizeof(name));

        con->sri.hostname = Strdup(name);
 
        /* traceLog("Got name: [%s]", name ) ; */

        ret = getnameinfo((struct sockaddr *)&cliaddrin_storage, clilen,
                          numname, sizeof(numname), NULL, 0, NI_NUMERICHOST);

        /* traceLog("Got numeric name: [%s]", numname ) ; */

        if (ret != 0) {
            free(con->sri.hostname);
            LOG( SPOCP_ERR )
                traceLog( "getnameinfo (NUMERICHOST) failed");
            pthread_mutex_unlock(&srv->mlock);
            continue;
        }
 
        con->sri.hostaddr = Strdup(numname);

        /* limitations as to who can speak to this server ? */

        if( server_access( con ) != SPOCP_SUCCESS ) {
          traceLog( "connection attempt on %d from %s(%s) DISALLOWED", con->fd, name,
                         con->sri.hostaddr ) ; 

          pthread_mutex_unlock(&srv->mlock);
          continue;
        }
      
        traceLog( "connection on %d from %s(%s)", con->fd, name, con->sri.hostaddr ) ; 
        break ;
  
      case AF_LOCAL : 
        clilen = sizeof( cliaddrun ) ; 

        LOG( SPOCP_DEBUG ) traceLog("[%d] Waiting for accept", local->id) ; 

        con->fd = accept(srv->listen_fd, (struct sockaddr *) &cliaddrun, &clilen);
  
        LOG( SPOCP_DEBUG ) traceLog( "Got accept") ;

        /* If con type could change then this wouldn't work */
        con->sri.hostname = myname.nodename ;
        con->sri.hostaddr = "127.0.0.1" ;
        break ; 
    }

    pthread_mutex_unlock(&srv->mlock);
    local->count++;
  
    LOG( SPOCP_DEBUG) traceLog( "thread %d handling connection", local->id ) ;
    spocp_server(con);              /* process the request */

    LOG( SPOCP_INFO ) traceLog( "server done" ) ;

    if( srv->type == AF_INET ) {
      free( con->sri.hostname ) ;
      free( con->sri.hostaddr ) ;
    }

    con->sri.hostname = 0 ;
    con->sri.hostaddr = 0 ;

    LOG( SPOCP_DEBUG ) traceLog( "con->close") ;

    con->close( con );
    clear_buffers( con ) ;
    con_reset( con ) ;
    /*
    print_io_buffer( "IN", con->in ) ;
    print_io_buffer( "OUT", con->out ) ;
     */
  
    /* if ever it was, TLS is not active anymore */
    con->tls = 0 ;
  }
}

