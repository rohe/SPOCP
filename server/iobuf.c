/***************************************************************************
                           iobuf.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 **************************************************************************/

#include "locl.h"
RCSID("$Id$");

spocp_iobuf_t *iobuf_new( size_t size ) 
{
  spocp_iobuf_t *io ;

  io = ( spocp_iobuf_t *) Calloc ( 1, sizeof( spocp_iobuf_t )) ;

  io->w = io->r = io->buf = ( char *) Calloc ( size , sizeof( char )) ;

  io->left = size - 1 ; /*leave place for a terminating '\0' */
  io->bsize = size ;

  io->end = io->buf + io->bsize ;
  *io->w = '\0' ;

  pthread_mutex_init( &io->lock, NULL ) ;
  pthread_cond_init( &io->empty, NULL ) ;

  return io ;
}

void iobuf_insert( spocp_iobuf_t *io, int where, char *what, int len )
{
  char   *dest, *src ;
  size_t dlen ;

  pthread_mutex_lock( &io->lock ) ;
  src = io->r + where ;
  dest = src + len ;
  dlen = io->w - io->r ;

  io->w = io->r + dlen + len ;
  io->left = io->bsize - len - dlen ;

  /* should check if there is place enough */
  memmove( dest, src, dlen ) ;
  memcpy( src, what, len ) ;

  pthread_mutex_unlock( &io->lock ) ;

}

void iobuf_shift( spocp_iobuf_t *io )
{
  int len ;

   LOG( SPOCP_DEBUG )
  traceLog( "Shifting buffer b:%p r:%p w:%p left:%d bsize:%d",
            io->buf, io->r, io->w, io->left, io->bsize ) ;

  pthread_mutex_lock( &io->lock ) ;

  while( io->r <= io->w && WHITESPACE(*io->r)) io->r++ ;

  if( io->r >= io->w ) { /* nothing in buffer */
    io->r = io->w = io->buf ;
    io->left = io->bsize - 1 ;
    *io->w = 0 ;
    /* traceLog( "DONE Shifting buffer b:%p r:%p w:%p left:%d",
             io->buf, io->r, io->w, io->left ) ; */
  }
  else {   /* something in the buffer, shift it to the front */
    len = io->w - io->r ;
    memmove( io->buf, io->r, len ) ;
    io->r = io->buf ;
    io->w = io->buf + len ;
    io->left = io->bsize - len ;
    *io->w = '\0' ;
  } 

  /*
  traceLog( "DONE Shifting buffer b:%p r:%p w:%p left:%d",
             io->buf, io->r, io->w, io->left ) ;
  */

  pthread_mutex_unlock( &io->lock ) ;

}

spocp_result_t iobuf_resize( spocp_iobuf_t *io, int increase ) 
{
  int   nr, nw ;
  char *tmp ;

  if( io == 0 ) return SPOCP_OPERATIONSERROR ;

  if( io->bsize == SPOCP_MAXBUF ) return SPOCP_BUF_OVERFLOW ;

  pthread_mutex_lock( &io->lock ) ;

  nr = io->bsize ;

  if( increase ) {
    if( io->bsize + increase > SPOCP_MAXBUF ) return SPOCP_BUF_OVERFLOW ;
    else if( increase < SPOCP_IOBUFSIZE ) io->bsize += SPOCP_IOBUFSIZE ;
    else io->bsize += increase ;
  }
  else {
    if( io->bsize + SPOCP_IOBUFSIZE > SPOCP_MAXBUF ) io->bsize += SPOCP_MAXBUF ;
    else io->bsize += SPOCP_IOBUFSIZE ;
  }

  /* the increase should be usable */
  io->left += io->bsize - nr ;

  nr = io->r - io->buf ;
  nw = io->w - io->buf ;

  tmp = Realloc( io->buf, io->bsize ) ;

  io->buf = tmp ;
  io->end = io->buf + io->bsize ;
  *io->end = '\0' ;
  io->r = io->buf + nr ;
  io->w = io->buf + nw ;

  pthread_mutex_unlock( &io->lock ) ;

  return SPOCP_SUCCESS ;
}

spocp_result_t iobuf_add( spocp_iobuf_t *io, char *s )
{
  spocp_result_t rc ;
  int   n ;

  if( s == 0 || *s == 0 ) return SPOCP_SUCCESS ; /* no error */

  n = strlen( s ) ;

  /* LOG( SPOCP_DEBUG ) 
    traceLog( "Add to IObuf \"%s\" %d/%d", s, n, io->left) ;
  */

  /* are the place enough ? If not make some */
  if( (int) io->left < n )  
    if(( rc = iobuf_resize( io, n - io->left )) != SPOCP_SUCCESS ) return rc ;

  pthread_mutex_lock( &io->lock ) ;

  strcat( io->w, s ) ;

  io->w += n ;
  *io->w = '\0' ;
  io->left -= n ;

  pthread_mutex_unlock( &io->lock ) ;
  return SPOCP_SUCCESS ;
}

spocp_result_t iobuf_add_octet( spocp_iobuf_t *io, octet_t *s )
{
  spocp_result_t rc ;
  size_t         l, lf ;
  char           lenfield[8] ;

  if( s == 0 || s->len == 0 ) return SPOCP_SUCCESS ; /* no error */

  /* the complete length of the bytestring, with length field */
  lf = DIGITS( s->len ) + 1 ;
  l = s->len + lf ;

  pthread_mutex_lock( &io->lock ) ;

  /* are the place enough ? If not make some */
  if( io->left < l ) 
    if(( rc = iobuf_resize( io, l - io->left )) != SPOCP_SUCCESS ) return rc ;

  snprintf( lenfield, 8, "%d:", s->len ) ;

  strcpy( io->w, lenfield ) ;
  io->w += lf ;

  memcpy( io->w, s->val, s->len ) ;
  io->w += s->len ;

  *io->w = '\0' ;
  io->left -= l ;

  pthread_mutex_unlock( &io->lock ) ;

  return SPOCP_SUCCESS ;
}

void iobuf_flush( spocp_iobuf_t *io )
{
  pthread_mutex_lock( &io->lock ) ;

  io->r = io->w = io->buf ;
  *io->w = 0 ;
  io->left = io->bsize - 1 ; 

  pthread_mutex_unlock( &io->lock ) ;

}

__attribute__((unused)) static void iobuf_print( char *s, spocp_iobuf_t *io )
{
  pthread_mutex_lock( &io->lock ) ;
  traceLog( "[%s] %p:%p:%p %d/%d", s, io->buf, io->r, io->w, io->left, io->bsize ) ;
  pthread_mutex_unlock( &io->lock ) ;
}

int iobuf_content( spocp_iobuf_t *io ) 
{
  int n ;

  if( io == 0 ) return -1 ;

  pthread_mutex_lock( &io->lock ) ;
  n = ( io->w - io->r ) ;
  pthread_mutex_unlock( &io->lock ) ;

  return n ;
}

