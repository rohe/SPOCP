#include <string.h>

#include <func.h>
#include "srv.h"
#include "wrappers.h"

afpool_t *new_afpool( void )
{
  afpool_t *afp ;
  pool_t   *p ;

  afp = ( afpool_t *) Malloc( sizeof( afpool_t )) ;

  p = ( pool_t * ) Calloc ( 2, sizeof( pool_t )) ;
  afp->free = &p[0] ;
  afp->active = &p[1] ;
 
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

void rm_from_pool( pool_t *pool, pool_item_t *item )
{
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
}
 
pool_item_t *pop_from_pool( pool_t *pool )
{
  pool_item_t *item ;

  if( pool->size == 0 ) return 0 ;

  item = pool->tail ;

  if( item != pool->head ) {
    item->prev->next = item->next ;
    pool->tail = item->prev ;
  }
  else 
    pool->head = pool->tail = 0 ;
  
  item->next = item->prev = 0 ;

  pool->size-- ;

  return item ;
}

void add_to_pool( pool_t *pool, pool_item_t *item )
{
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
}

/* remove it from the active pool and place it in the free pool 
   only done from spocp_server_run(), of which there is only one running 
 */
int return_item( afpool_t *afp, pool_item_t *item ) 
{
  if( afp == 0 ) return 0 ;

  rm_from_pool( afp->active, item ) ;

  add_to_pool( afp->free, item ) ;

  return 1 ;
}

/* get a item from from the free pool, place it in the active pool
   only done from spocp_server_run(), of which there is only one running 
 */
pool_item_t *get_item( afpool_t *afp ) 
{
  pool_item_t *item ;

  if( afp == 0 ) return 0 ;

  if( afp->free == 0 || afp->free->head == 0 ) return 0 ;

  /* get it from the free pool */
  item = pop_from_pool( afp->free ) ;

  /* place it in the active pool */

  if( item ) add_to_pool( afp->active, item ) ;

  return item ;
}

pool_item_t *first_active( afpool_t *afp )
{
  if( afp && afp->active && afp->active->size ) return afp->active->head ;
  else return 0 ;
}

