#include "locl.h"
RCSID("$Id$");

static pool_t *pool_new( void )
{
  pool_t *p ;

  p = ( pool_t * ) Calloc ( 1, sizeof( pool_t )) ;

  pthread_mutex_init( &p->lock, NULL ) ;

  return p ;
}

afpool_t *afpool_new( void )
{
  afpool_t *afp ;

  afp = ( afpool_t *) Malloc( sizeof( afpool_t )) ;

  afp->free = pool_new() ;
  afp->active = pool_new() ;
 
  pthread_mutex_init( &afp->aflock, NULL ) ;

  return afp ;
}

int afpool_lock( afpool_t *afp ) 
{
  return pthread_mutex_lock( &afp->aflock ) ;
}

int afpool_unlock( afpool_t *afp ) 
{
  return pthread_mutex_unlock( &afp->aflock ) ;
}

int afpool_free_item( afpool_t *afp ) 
{
  if( afp && afp->free && afp->free->size ) return 1 ;
  else return 0 ;
}

int afpool_active_item( afpool_t *afp ) 
{
  if( afp && afp->active && afp->active->size ) return 1 ;
  else return 0 ;
}

void pool_rm( pool_t *pool, pool_item_t *item )
{
  pthread_mutex_lock( &pool->lock ) ;

  if( pool->head == item ) {
    if( pool->head == pool->tail ) pool->head = pool->tail = 0 ;
    else {
      item->next->prev = 0 ;
      pool->head = item->next ;
    }
  }
  else if( pool->tail == item ) {
    item->prev->next = 0 ;
    pool->tail = item->prev ;
  }
  else {
    if( item->prev ) item->prev->next = item->next ;
    if( item->next ) item->next->prev = item->prev ;
  }

  item->prev = item->next = 0 ;

  pool->size-- ;

  pthread_mutex_unlock( &pool->lock ) ;
}
 
pool_item_t *pool_pop( pool_t *pool )
{
  pool_item_t *item ;

  if( pool->size == 0 ) return 0 ;

  pthread_mutex_lock( &pool->lock ) ;

  item = pool->tail ;

  if( item != pool->head ) {
    item->prev->next = item->next ;
    pool->tail = item->prev ;
  }
  else 
    pool->head = pool->tail = 0 ;
  
  item->next = item->prev = 0 ;

  pool->size-- ;

  pthread_mutex_unlock( &pool->lock ) ;

  return item ;
}

void pool_add( pool_t *pool, pool_item_t *item )
{
  pthread_mutex_lock( &pool->lock ) ;

  if( pool->size == 0 ) {
    pool->tail = pool->head = item ;
    item->next = item->prev = 0 ;
  }
  else {
    pool->tail->next = item ;

    item->prev = pool->tail ;
    item->next = 0 ;

    pool->tail = item ;
  }

  pool->size++ ;

  pthread_mutex_unlock( &pool->lock ) ;
}

int pool_size( pool_t *pool )
{
  int          i ;
  pool_item_t *pi ;

  if( pool == 0 ) return 0 ;
 
  if( pool->head == 0 ) return 0 ;

  pthread_mutex_lock( &pool->lock ) ;
  
  for( pi = pool->head, i = 0 ; pi ; pi = pi->next ) i++ ;

  pthread_mutex_unlock( &pool->lock ) ;

  return i ;
}

/* remove it from the active pool and place it in the free pool 
   only done from spocp_server_run(), of which there is only one running 
 */
int item_return( afpool_t *afp, pool_item_t *item ) 
{
  if( afp == 0 ) return 0 ;

  pool_rm( afp->active, item ) ;

  pool_add( afp->free, item ) ;

  return 1 ;
}

/* get a item from from the free pool, place it in the active pool
   only done from spocp_server_run(), of which there is only one running 
 */
pool_item_t *item_get( afpool_t *afp ) 
{
  pool_item_t *item ;

  if( afp == 0 ) return 0 ;

  if( afp->free == 0 || afp->free->head == 0 ) return 0 ;

  /* get it from the free pool */
  item = pool_pop( afp->free ) ;

  /* place it in the active pool */

  if( item ) pool_add( afp->active, item ) ;

  return item ;
}

void afpool_push_item( afpool_t *afp, pool_item_t *pi )
{
  if( afp && afp->active ) pool_add( afp->active, pi ) ;
}

void afpool_push_empty( afpool_t *afp, pool_item_t *pi )
{
  if( afp && afp->free ) pool_add( afp->free, pi ) ;
}

pool_item_t *afpool_get_item( afpool_t *afp )
{
  if( afp && afp->active && afp->active->size ) return pool_pop( afp->active ) ;
  else return 0 ;
}

pool_item_t *afpool_get_empty( afpool_t *afp )
{
  if( afp && afp->free && afp->free->size ) return pool_pop( afp->free) ;
  else return 0 ;
}

pool_item_t *afpool_first( afpool_t *afp )
{
  if( afp && afp->active ) return afp->active->head ;
  return 0 ;
}

int number_of_active( afpool_t *afp ) 
{
  return pool_size( afp->active ) ;
}

int number_of_free( afpool_t *afp ) 
{
  return pool_size( afp->free ) ;
}

