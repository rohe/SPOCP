/*! \file lib/be_cpool.c
 * \author Roland Hedberg <roland@catalogix.se>
 */
#include <string.h>
#include <pthread.h>

#include <spocp.h>
#include <be.h>
#include <wrappers.h>
#include <macros.h>

/* ====================================================================== *
                    LOCAL FUNCTIONS
 * ====================================================================== */

static becon_t *becon_new( int copy )
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

static void becon_free( becon_t *bc, int close )
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

    if( bc->arg ) oct_free( bc->arg ) ;


    free( bc ) ;
  }
}

/* ------------------------------------------------------------------- */

static void becpool_rm( becpool_t *bcp, int close )
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

static int becpool_full( becpool_t *bcp )
{
  if( bcp->size == bcp->max ) return 1 ;
  else return 0 ;
}

/* ====================================================================== * 
                    PUBLIC FUNCTIONS 
 * ====================================================================== */

/*!
 * \brief Creates a new backend connection pool
 * \param max The maximum size of the connection pool, the connections held in this
 *        should never be allowed to exceed this number.
 * \return A pointer to the newly created pool
 */
becpool_t *becpool_new( size_t max )
{
  becpool_t *bcp ;

  bcp = ( becpool_t * ) Malloc ( sizeof( becpool_t )) ;
  bcp->size = 0 ;
  bcp->max = max ;
  bcp->head = bcp->tail = 0 ;

  bcp->lock = ( pthread_mutex_t * ) Malloc ( sizeof( pthread_mutex_t )) ;
  pthread_mutex_init( bcp->lock, 0 ) ;

  return bcp ;
}

/* ------------------------------------------------------------------- */
/*!
 * \fn becon_t *becon_push( octet_t *, closefn *, void *, becpool_t * )
 * \brief Pushes a connection onto the connection pool
 * \param arg The name of the connection
 * \param close The function that should be used to close a connection of this type
 * \param con A handle to the connection
 * \param bcp A pointer to the connection pool
 * \return A becon_t struct representing an active connection
 */
becon_t *becon_push( octet_t *arg, closefn *close, void *con, becpool_t *bcp )
{
  becon_t *bc, *old = 0 ;
 
  if( becpool_full( bcp ) ) return 0 ;

  bc = becon_new( 0 ) ;
  
  bc->arg = octdup( arg ) ;
  bc->close = close ;
  bc->con = con ;
  bc->status = CON_ACTIVE ;

  pthread_mutex_lock( bc->c_lock ) ;

  pthread_mutex_lock( bcp->lock ) ;

  bc->last = time( (time_t*) 0 );

  /* place the newest at the end */

  if( bcp->tail ) {
    bcp->tail->next = bc ;
    bc->prev = bcp->tail ;
    bc->next = 0 ;
    bcp->tail = bc ;
  }
  else bcp->head = bcp->tail = bc ;

  bcp->size++ ;

  pthread_mutex_unlock( bcp->lock ) ;

  return bc ;
}

/* ------------------------------------------------------------------- */

/*!
 * \fn becon_t *becon_get( octet_t *arg, becpool_t *bcp ) 
 * \brief Get a old unused, hopefully still active connection
 * \param arg The name of the connection type
 * \param bcp A pointer to this backends connection pool
 * \return A pointer to a struct representing the connection if there is
 *         a usefull connection, otherwise NULL. It will also return NULL 
 *         if there is no connection pool available, something that might mean
 *         that this backend should not use connection pools for some reason.
 */

becon_t *becon_get( octet_t *arg, becpool_t *bcp ) 
{
  becon_t *bc ;

  if( bcp == 0 ) return 0 ;

  pthread_mutex_lock( bcp->lock ) ;

  for( bc = bcp->head ; bc ; bc = bc->next ) {
    if( octcmp( bc->arg, arg ) == 0 ) {

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
        bc->next = 0 ;
        bcp->tail = bc ;
      }

      break ;
    }
  }

  pthread_mutex_unlock( bcp->lock ) ;

  return bc ;
}

/* ------------------------------------------------------------------- */
/*!
 * \fn void becon_return( becon_t *bc )
 * \brief Return a connection to the pool
 * \param bc The connection
 */
void becon_return( becon_t *bc )
{
  bc->last = time( (time_t *) 0 ); /* update last usage */
  bc->status = CON_FREE ;
  pthread_mutex_unlock( bc->c_lock ) ;
}

/* ------------------------------------------------------------------- */

/*!
 * \fn void becon_rm( becpool_t *bcp, becon_t *bc )
 * \brief Removes the representation of a connection from the pool,
 *    the connection itself should be closed before the representation is removed.
 * \param bcp The connection pool
 * \param bc The connection representation
 */
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

/* ------------------------------------------------------------------- */
/*!
 * \fn void becon_update( becon_t *bc, void *con )
 * \brief Used to replaces a defunct connecction with a working
 * \param bc A pointer to the connection representation
 * \param con The connection handle
 *
 */
void becon_update( becon_t *bc, void *con )
{
  bc->con = con ;
}

