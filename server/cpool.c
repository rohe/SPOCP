#include <string.h>

#include "func.h"
#include "srv.h"
#include "wrappers.h"

#define MAX_NUMBER_OF_CONNECTIONS 64

int init_conn( conn_t *conn, srv_t *srv, int fd, char *host )
{
  conn->srv = srv ;
  conn->fd = fd ;
  conn->status = CNST_LINGERING ;
  conn->con_type = NATIVE ;
  conn->sri.hostaddr = Strdup(host);

  conn->rs = srv->root ;
  conn->tls = -1 ;
  conn->stop = 0 ;

  time( &conn->last_event ) ;

  return 0 ;
}

int reset_conn( conn_t *conn )
{
  conn->status = CNST_FREE ;
  conn->fd = 0 ;
  if( conn->sri.hostname ) free( conn->sri.hostname ) ;
  if( conn->sri.hostaddr ) free( conn->sri.hostaddr ) ;

  return 0 ;
}

conn_t *new_conn( void )
{
  conn_t *con = 0 ;

  con = ( conn_t * ) Calloc (1, sizeof( conn_t )) ;

  init_connection( con ) ;

  return con ;
}

void free_con( conn_t *con )
{
  if( con ) {
    free_connection( con ) ;
    free( con ) ;
  }
}

afpool_t *new_cpool( int nc )
{
  afpool_t    *afpool = 0 ;
  pool_item_t *pi ;
  pool_t      *pool ;
  conn_t      *con ;
  int         i ;

  fprintf( stderr, "new_cpool\n" ) ;

  afpool = new_afpool() ;

  fprintf( stderr, "new_afpool\n" ) ;

  pool = afpool->free ;

  fprintf( stderr, "nc: %d\n", nc ) ;

  if( nc ) {
    for( i = 0 ; i < nc ; i++ ) {
      con = new_conn() ;

      pi = ( pool_item_t *) Calloc( 1, sizeof( pool_item_t )) ;
      pi->info = ( void *) con ;

      add_to_pool( pool, pi ) ;
    }
  }

  pthread_mutex_init( &(afpool->aflock), NULL ) ;

  return afpool ;
}

void free_cpool( afpool_t *afp )
{
  conn_t *con ;
  pool_t *pool ;
  pool_item_t *pi ;

  if( afp ) {
    if( afp->active ) { /* shouldn't be any actives */
      free( afp->active ) ;
    }
    if( afp->free ) {
      pool = afp->free ;

      while(( pi = pop_from_pool( pool ))) {
        con = ( conn_t * ) pi->info ;
        free_con( con ) ;
        free( pi ) ;
      }

      free( pool ) ;
    }

    free( afp ) ;
  }
}

int init_server( srv_t *srv, char *cnfg )
{
  fprintf( stderr, "init_server\n" ) ;

  /* make sure nothing unwanted is present */
  memset( (void *) srv, 0, sizeof( srv_t )) ;

  fprintf( stderr, "Read config\n" ) ;

  if( read_config( cnfg, srv ) == 0 ) return -1 ;

  /* fprintf( stderr, "New ruleset\n" ) ; */

  if( srv->root == 0 ) srv->root = ruleset_new( 0 ) ;
  
  pthread_mutex_init( &(srv->mutex), NULL ) ;

  /* max number of simultaneously open connections */
  if( srv->nconn == 0 ) srv->nconn = MAX_NUMBER_OF_CONNECTIONS ;

  srv->connections = new_cpool( srv->nconn ) ;

  return 0 ;
}

