/***************************************************************************
                           con.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 **************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <wrappers.h>
#include <proto.h>
#include <macros.h>
#include <struct.h>
#include <spocp.h>
#include <func.h>

#include <srv.h>

void spocp_read_and_drop( conn_t *ct, unsigned int num ) ;
void clear_buffers( conn_t *con ) ;

int spocp_writen( conn_t *ct, char *str, size_t n )
{
  int    nleft, nwritten ;
  char   *sp ;
  fd_set wset ;
  int    retval ;
  struct timeval to ;
  int    fd = ct->fd ;

  /* can happen during testing; it's hard to write to stdin so
     just fake it */

  if( fd == STDIN_FILENO ) {
    octet_t oct ;

    oct.val = str ;
    oct.len = n ;

   /*
    tmp = oct2strdup( &oct, '\\' ) ;
    traceLog( "REPLY: [%s]", tmp ) ;
    free( tmp ) ;
   */

    return( n ) ;
  }

  errno = 0 ;

  /* wait max 3 seconds */
  to.tv_sec = 3 ;
  to.tv_usec = 0 ;

  sp = str ;
  nleft = n ;
  FD_ZERO(&wset) ;
  while( nleft > 0 ) {
    FD_SET(fd,&wset) ;
    retval = select(fd+1,NULL,&wset,NULL,&to) ;

    if( retval ) {
      if(( nwritten = write(fd, sp, nleft)) <= 0 ) { ;
        if( errno == EINTR ) nwritten = 0  ;
        else return -1 ;
      }
      
      nleft -= nwritten ;
      sp += nwritten ;
    }
    else { /* timed out */
      break ;
    }
  }

  /* fdatasync( fd ) ; */

  return(n) ;
}                  

int spocp_readn( conn_t *ct, char *str, size_t max )
{
  fd_set rset ;
  int retval, n = 0 ;
  struct timeval to ;
  int    fd = ct->fd ;

  errno = 0 ;

  /* wait max 0.1 seconds */
  to.tv_sec = 0 ;
  to.tv_usec = 100000 ;

  FD_ZERO(&rset) ;

  FD_SET(fd,&rset) ;
  retval = select(fd+1,&rset,NULL,NULL,&to) ;

  if( retval ) {
    if(( n = read(fd, str, max)) <= 0 ) {
      if( errno == EINTR || errno == 0 ) n = 0 ;
      else return -1 ;
    }
  }
  else { /* timed out */
    ;
  }

  /* fdatasync( fd ) ; */

  return(n) ;
}

/* times out if it doesn't receive any bytes in 15 seconds */
void spocp_read_and_drop( conn_t *conn, unsigned int num )
{
  char buf[4096] ;
  int n, lc = 0 ;

  do {
    if( num > 4096 ) n = spocp_readn( conn, buf, 4096 ) ;
    else n = spocp_readn( conn, buf, num ) ;

    if( n == 0 ) {
      lc++ ;
      continue ;
    }
    else lc = 0 ;

    num -= n ;
    fprintf( stderr, "Got %d, remains %d\n", n, num ) ;
  } while( num && lc < 5 ) ;

  return ;
}

int spocp_io_read( conn_t *conn )
{
  int n ;
  spocp_iobuf_t *io = conn->in ;

  if(( n = conn->readn( conn, io->w, io->left )) <= 0 ) return n ;

  io->left -= n ;
  io->w += n ;
  *io->w = '\0' ;

  return n ;
}

int spocp_io_write( conn_t *conn )
{
  int n ;
  spocp_iobuf_t *io = conn->out ;

  if(( n = conn->writen( conn, io->r, io->w - io->r )) <= 0 )  {
    return n ;
  }

  LOG( SPOCP_DEBUG ) traceLog( "spocp_io_write wrote %d bytes", n ) ;

  io->r += n ;
  io->left -= n ;

  shift_buffer( io ) ;

  return n ;
}

void spocp_io_insert( spocp_iobuf_t *io, int where, char *what, int len )
{
  char   *dest, *src ;
  size_t dlen ;

  src = io->r + where ;
  dest = src + len ;
  dlen = io->w - io->r ;

  io->w = io->r + dlen + len ;
  io->left = io->bsize - len - dlen ;

  /* should check if there is place enough */
  memmove( dest, src, dlen ) ;
  memcpy( src, what, len ) ;
}

void shift_buffer( spocp_iobuf_t *io )
{
  int len ;

  /* LOG( SPOCP_DEBUG )
  traceLog( "Shifting buffer b:%p r:%p w:%p left:%d bsize:%d",
            io->buf, io->r, io->w, io->left, io->bsize ) ;
  */

  while( io->r <= io->w && WHITESPACE(*io->r)) io->r++ ;

  if( io->r >= io->w ) { /* nothing in buffer */
    io->r = io->w = io->buf ;
    io->left = io->bsize - 1 ;
    *io->w = 0 ;
    /* traceLog( "DONE Shifting buffer b:%p r:%p w:%p left:%d",
             io->buf, io->r, io->w, io->left ) ; */
    return ;
  }
     
  /* something in the buffer, shift it to the front */
  len = io->w - io->r ;
  memmove( io->buf, io->r, len ) ;
  io->r = io->buf ;
  io->w = io->buf + len ;
  io->left = io->bsize - len ;
  *io->w = '\0' ;

  /*
  traceLog( "DONE Shifting buffer b:%p r:%p w:%p left:%d",
             io->buf, io->r, io->w, io->left ) ;
  */
}

spocp_result_t iobuf_resize( spocp_iobuf_t *io, int increase ) 
{
  int   nr, nw ;
  char *tmp ;

  if( io == 0 ) return SPOCP_OPERATIONSERROR ;

  if( io->bsize == SPOCP_MAXBUF ) return SPOCP_BUF_OVERFLOW ;

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

  return SPOCP_SUCCESS ;
}

spocp_result_t add_to_iobuf( spocp_iobuf_t *io, char *s )
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

  strcat( io->w, s ) ;

  io->w += n ;
  *io->w = '\0' ;
  io->left -= n ;

  return SPOCP_SUCCESS ;
}

spocp_result_t add_bytestr_to_iobuf( spocp_iobuf_t *io, octet_t *s )
{
  spocp_result_t rc ;
  size_t         l, lf ;
  char           lenfield[8] ;

  if( s == 0 || s->len == 0 ) return SPOCP_SUCCESS ; /* no error */

  /* the complete length of the bytestring, with length field */
  lf = DIGITS( s->len ) + 1 ;
  l = s->len + lf ;

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

  return SPOCP_SUCCESS ;
}

void flush_io_buffer( spocp_iobuf_t *io )
{
  io->r = io->w = io->buf ;
  *io->w = 0 ;
  io->left = io->bsize - 1 ; 
}

void print_io_buffer( char *s, spocp_iobuf_t *io )
{
  traceLog( "[%s] %p:%p:%p %d/%d", s, io->buf, io->r, io->w, io->left, io->bsize ) ;
}

void clear_buffers( conn_t *con )
{
  flush_io_buffer( con->in ) ;
  flush_io_buffer( con->out ) ;
}

int spocp_close( conn_t *conn ) 
{
  traceLog( "connection on %d closed", conn->fd ) ;
  close( conn->fd ) ;
  conn->fd = 0 ;

  return TRUE ;
}

void free_connection( conn_t *conn ) 
{
  if( conn ) {
    if( conn->fd ) spocp_close( conn ) ;
    free( conn->in ) ;
    free( conn->out ) ;
  }
}

void init_connection( conn_t *conn )
{
  spocp_iobuf_t      *in, *out ;

  /* traceLog("Init Connection") ; */

  conn->in = in = ( spocp_iobuf_t *) Calloc ( 1, sizeof( spocp_iobuf_t )) ;
  conn->out = out = ( spocp_iobuf_t *) Calloc ( 1, sizeof( spocp_iobuf_t )) ;

  in->w = in->r = in->buf = ( char *) Calloc ( SPOCP_IOBUFSIZE, sizeof( char )) ;

  out->left = in->left = SPOCP_IOBUFSIZE - 1 ; /*leave place for a terminating '\0' */
  out->bsize = in->bsize = SPOCP_IOBUFSIZE ;

  in->end = in->buf + in->bsize ;
  *in->w = '\0' ;
  out->w = out->r = out->buf = ( char *) Calloc ( SPOCP_IOBUFSIZE, sizeof( char)) ;
  out->end = out->buf + out->bsize ;
  *out->w = '\0' ;

  conn->readn = spocp_readn;
  conn->writen = spocp_writen;
  conn->close = spocp_close;

  conn->tls = -1 ;
}

int send_results( conn_t *conn )
{
  spocp_iobuf_t *out = conn->out ;
  size_t         len, n, loop = 5 ;
  char           ldef[16] ;
  int            nr ;

  if(0) timestamp( "Send results" ) ;
  len = out->w - out->r ;

  nr = snprintf( ldef, 16, "%d:", len ) ;
  spocp_io_insert( out, 0, ldef, nr ) ;

  len += nr ;

  LOG( SPOCP_INFO ) {
    *out->w = '\0' ; 
    traceLog( "SEND_RESULT: [%s]", out->r) ;
  }

  /* only five tries to get the stuff out the door */
  while( len && loop ) {
    if(( n = spocp_io_write( conn )) < 0 ) {
      /* discard results */
      out->r = out->w = out->buf ;
      out->left = out->bsize ;
      break ;
    }
    len -= n ;
    loop-- ;
  }

  if( len ) {
    traceLog( "Failed to send result, [%d] bytes remains", len ) ;
    return 0 ;
  }
  
  /* managed to send everything, reset buffer */
  flush_io_buffer( out ) ;

  if(0) timestamp( "Send done" ) ;

  return 1;
}

char *next_line( conn_t *conn )
{
  char          *s, *b ;
  spocp_iobuf_t *io = conn->in ;
  
  if(( s = strpbrk( io->r, "\r\n" )) == 0 ) { /* try to read some more */
    spocp_io_read( conn ) ; 
    if(( s = strpbrk( io->r, "\r\n" )) == 0 ) {  /* fail !! */
      return 0 ;
    }
  } 

  if( *s == '\r' && *(s+1) == '\n' ) {
    *s = '\0' ;
    s += 2 ;
  }
  else if( *s == '\n' ) *s++ = '\0' ;
  else return 0 ;

  b = io->r ;
  io->r = s ;

  return b ;
}

void con_reset( conn_t *con ) 
{
  if( con ) {
    if( con->subjectDN ) {
      free( con->subjectDN ) ;
      con->subjectDN = 0 ;
    }
    if( con->issuerDN ) {
      free( con->issuerDN ) ;
      con->issuerDN = 0 ;
    }
    if( con->cipher ) {
      free( con->cipher ) ;
      con->cipher = 0 ;
    }
    if( con->ssl_vers ){
      free( con->ssl_vers ) ;
      con->ssl_vers = 0 ;
    }
    if( con->transpsec ){
      free( con->transpsec ) ;
      con->transpsec = 0 ;
    }
  }
}

int spocp_send_results( conn_t *conn )
{
  return send_results( conn ) ;
}

/* this is only used when reading from stdin */
conn_t *spocp_open_connection( int fd, srv_t *srv )
{
  conn_t *con ;
  pool_item_t *pi ;

  pi = get_item( srv->connections ) ;
  if( pi == 0 ) {
    traceLog( "No items" ) ;
    return 0 ;
  }

  con = (conn_t *) pi->info ;

  if( con ) {
    con->fd = fd ;
    if( fd == STDIN_FILENO ) con->fdw = STDOUT_FILENO ; 
    else con->fdw = -1 ;

    con->srv = srv ;
  }

  return con ;
}

