/* */

#include <string.h>
#include <pthread.h>

#include <spocp.h>
#include <wrappers.h>
#include <macros.h>

becon_t *becon_new( int copy )
{
  becon_t *bc ;

  bc = ( becon_t * ) Calloc ( 1, sizeof( becon_t )) ;

  if( !copy ) {
    bc->con = 0 ;
    bc->status = CON_FREE ;
    bc->c_lock = ( pthread_mutex_t *) Malloc ( sizeof( pthread_mutex_t )) ;
    pthread_mutex_init( bc->c_lock, NULL ) ; 
  }

  return bc ;
}

becon_t *becon_dup( becon_t *old )
{
  becon_t *new = becon_new( 1 ) ;

  new->type    = Strdup( old->type ) ;
  new->arg     = Strdup( old->arg ) ;
  new->close   = old->close ;
  new->last    = old->last ;
  new->con     = old->con ;
  new->c_lock  = old->c_lock ;
  new->status  = old->status ;

  return new ;
}

void becon_free( becon_t *bc, int close )
{
  if( bc ) {
    if( close ) { 
      pthread_mutex_lock( bc->c_lock ) ; 

      if( bc->con != 0 ) bc->close( bc->con ) ;
      bc->con = 0 ;

      pthread_mutex_unlock( bc->c_lock ) ; 

      /* can't be locked when destroyed */
      pthread_mutex_destroy( bc->c_lock ) ; 

      free( bc->c_lock ) ;
    }

    if( bc->type ) free( bc->type ) ;
    if( bc->arg ) free( bc->arg ) ;


    free( bc ) ;
  }
}

/* ------------------------------------------------------------------- */

becpool_t *becpool_new( size_t max, int copy )
{
  becpool_t *bcp ;

  bcp = ( becpool_t * ) Malloc ( sizeof( becpool_t )) ;
  bcp->size = 0 ;
  bcp->max = max ;
  bcp->head = bcp->tail = 0 ;
  if( !copy ) {
    bcp->lock = ( pthread_mutex_t * ) Malloc ( sizeof( pthread_mutex_t )) ;
    pthread_mutex_init( bcp->lock, 0 ) ;
  }

  return bcp ;
}

becpool_t *becpool_dup( becpool_t *bcp )
{
  becpool_t *new ;
  becon_t   *bc, *cpy ;

  new = becpool_new( bcp->max, 1 ) ;

  for( bc = bcp->head ; bc ; bc = bc->next ) {
    cpy = becon_dup( bc ) ;
    if( new->head == 0 ) new->head = new->tail = cpy ;
    else {
      new->tail->next = cpy ;
      cpy->prev = new->tail ;
      new->tail = cpy ;
    } 
  }

  pthread_mutex_unlock( bcp->lock ) ;

  return new ;
}

void becpool_rm( becpool_t *bcp, int close )
{
  becon_t *pres, *next ;

  if( bcp ) {
    pthread_mutex_lock( bcp->lock ) ;

    for( pres = bcp->head ; pres ; pres = next ) {
      next = pres->next ;
      becon_free( pres, close ) ;
    }

    pthread_mutex_unlock( bcp->lock ) ;

    if( close ) {
      pthread_mutex_destroy( bcp->lock ) ;
      free( bcp->lock ) ;
    }

    free( bcp ) ;
  }
}

/* ------------------------------------------------------------------- */

becon_t *
becon_push( char *type, char *arg, closefn *close, void *con, becpool_t *bcp )
{
  becon_t *bc, *old = 0 ;
 
  bc = becon_new( 0 ) ;
  
  bc->type = Strdup( type ) ;
  bc->arg = Strdup( arg ) ;
  bc->close = close ;
  bc->con = con ;
  bc->status = CON_ACTIVE ;

  pthread_mutex_lock( bc->c_lock ) ;

  pthread_mutex_lock( bcp->lock ) ;

  bc->last = time( (time_t*) 0 );

  /* If I have reached the maximum number of open file descriptors 
     release the oldest unused
   */
  if( bcp->size == bcp->max ) {
    if( old->status == CON_ACTIVE ) {
       becon_free( bc, 0 ) ;  /* free but don't close */
       return 0 ;
    }

    old = bcp->head ;
    bcp->head = old->next ;
    bcp->head->prev = 0 ;
    becon_free( old, 1 ) ;
    bcp->size-- ;
  }

  /* place the newest at the end */

  if( bcp->tail ) {
    bcp->tail->next = bc ;
    bc->prev = bcp->tail ;
    bcp->tail = bc ;
  }
  else bcp->head = bcp->tail = bc ;

  bcp->size++ ;

  pthread_mutex_unlock( bcp->lock ) ;

  return bc ;
}

/* returns bc if there is a usefull connection, otherwise 0 */

becon_t *becon_get( char *type, char *arg, becpool_t *bcp ) 
{
  becon_t *bc ;

  pthread_mutex_lock( bcp->lock ) ;

  for( bc = bcp->head ; bc ; bc = bc->next ) {
    if( strcmp( bc->type, type ) == 0 && 
        strcmp( bc->arg, arg ) == 0 ) {

      if( bc->status == CON_ACTIVE ) continue ;

      /* make sure I own this connection */
      pthread_mutex_lock( bc->c_lock ) ;

      bc->status = CON_ACTIVE ;
      bc->last = time( (time_t*) 0 ); /* update last usage */

      /* move to end */
      if( bcp->tail == bc ) ; /* already tail */ 
      else {
        if( bcp->head == bc ) { /* head */
          bcp->head = bc->next ;
          bc->next->prev = 0 ;
        }
        else { /* somewhere in the middle */
          bc->prev->next = bc->next ;
          bc->next->prev = bc->prev ;
        }
        
        bcp->tail->next = bc ;
        bc->prev = bcp->tail ;
        bcp->tail = bc ;
      }

      break ;
    }
  }

  pthread_mutex_unlock( bcp->lock ) ;

  return bc ;
}

void becon_return( becon_t *bc )
{
  bc->last = time( (time_t*) 0 ); /* update last usage */
  bc->status = CON_FREE ;
  pthread_mutex_unlock( bc->c_lock ) ;
}

void becon_rm( becpool_t *bcp, becon_t *bc )
{
  becon_t *next, *prev ;

  if( bc == 0 ) return ;

  pthread_mutex_lock( bcp->lock ) ;


  if( bc == bcp->head ) {
    if( bc == bcp->tail ) {
      bcp->head = bcp->tail = 0 ;
    }
    else {
      next = bc->next ;
      bcp->head = next ;
      if( next ) next->prev = 0 ;
    }
  }
  else if( bc == bcp->tail ) {
    prev = bc->prev ;
    bcp->tail = prev ;
    if( prev ) prev->next = 0 ;    
  }
  else {
    bc->prev->next = bc->next ;
    bc->next->prev = bc->prev ;
  }

  becon_free( bc, 1 ) ;

  bcp->size-- ;
  
  pthread_mutex_unlock( bcp->lock ) ;

}

