/***************************************************************************
                          oct.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in tdup level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <func.h>

#include <struct.h>
#include <wrappers.h>
#include <spocp.h>
#include <macros.h>

/* ====================================================================== */

int oct_find_balancing( octet_t *o, char left, char right)
{
  int seen = 0, n = o->len ;
  char *q = o->val ;

  for( ; n ; q++, n-- ) {
    if (*q == left) seen++;
    else if (*q == right) {
      if (seen==0)  return(o->len - n);
      else seen--;
    }
  }

  return -1;
}

spocp_result_t oct_resize( octet_t *o, size_t new )
{
  char *tmp ;

  if( o->size == 0 ) return SPOCP_DENIED ; /* not dynamically allocated */

  if( new < o->size ) return SPOCP_SUCCESS ; /* I don't shrink yet */

  while( o->size < new ) o->size *= 2 ;
   
  tmp = Realloc( o->val, o->size * sizeof( char )) ;

  o->val = tmp ;

  return SPOCP_SUCCESS ;
}

void oct_assign( octet_t *oct, char *str )
{
  oct->val = str ;
  oct->len = strlen( str ) ;
  oct->size = 0 ;
}

void octln( octet_t *a, octet_t *b )
{
  a->len = b->len ;
  a->val = b->val ;
  a->size = 0 ;
}

octet_t *octdup( octet_t *oct )
{
  octet_t *dup ;

  dup = ( octet_t * ) Malloc ( sizeof(octet_t)) ;

  dup->size = dup->len = oct->len ;
  if( oct->len != 0 ) {
    dup->val = ( char * ) Malloc ( oct->len+1 ) ;
    dup->val = memcpy( dup->val, oct->val, oct->len ) ;
   dup->val[dup->len] = 0 ;
  }
  else {
    dup->len = 0 ;
    dup->val = 0 ;
  }

  return dup ;
}

spocp_result_t octcpy( octet_t *cpy, octet_t *oct )
{
  if( cpy->size == 0 ) {
    cpy->val = ( char * ) Calloc( oct->len, sizeof( char )) ;
  }
  else if( cpy->size < oct->len ) {
    if( oct_resize( cpy, oct->len ) != SPOCP_SUCCESS ) return SPOCP_DENIED ;
  } 

  memcpy( cpy->val, oct->val, oct->len ) ;
  cpy->size = cpy->len = oct->len ;

  return 1;
}

void octclr( octet_t *oct )
{
  if( oct->size ) {
    free( oct->val ) ;
  }

  oct->size = oct->len = 0 ;
  oct->val = 0 ;
}

spocp_result_t octcat( octet_t *oct, char *s, size_t l )
{
  size_t n ;

  n = oct->len + l ; /* new length */

  if( n > oct->size ) {
    if( oct_resize( oct, n ) != SPOCP_SUCCESS ) return SPOCP_DENIED;
    oct->size = n ;
  }

  memcpy( oct->val + oct->len, s, l ) ;
  oct->len = n ;

  return SPOCP_SUCCESS ;
}

int oct2strcmp(  octet_t *o, char *s )
{
  size_t l, n ;
  int    r ;

  l = strlen( s ) ;

  if( l == o->len ) {
    return memcmp( s, o->val, o->len ) ;
  }
  else {
    if( l < o->len ) n = l ;
    else             n = o->len ;

    r = memcmp( s, o->val, n ) ;

    if( r == 0 ) {
      if( n == l ) return -1 ;
      else         return 1 ;
    }
    else return r ;
  }
}

int oct2strncmp(  octet_t *o, char *s, size_t l )
{
  size_t n ;
  int    r ;

  if( l == o->len ) {
    return memcmp( s, o->val, o->len ) ;
  }
  else {
    if( l < o->len ) n = l ;
    else             n = o->len ;

    r = memcmp( s, o->val, n ) ;

    if( r == 0 ) {
      if( n == l ) return -1 ;
      else         return 1 ;
    }
    else return r ;
  }
}

int octcmp( octet_t *a, octet_t *b )
{
  size_t i, n ;
  char   *ca, *cb ;

  if( a->len < b->len ) n = a->len ;
  else                  n = b->len ;

  for( i = 0, ca = a->val, cb = b->val ; i < n ; i++, ca++, cb++ ) 
    if( *ca != *cb ) return ( *ca - *cb ) ;
 
  return( a->len - b->len ) ;
}

int octncmp( octet_t *a, octet_t *b, size_t cn )
{
  size_t i, n ;
  char   *ca, *cb ;

  if( a->len < b->len ) n = a->len ;
  else                  n = b->len ;

  if( cn < n ) n = cn ;

  for( i = 0, ca = a->val, cb = b->val ; i < n ; i++, ca++, cb++ ) 
    if( *ca != *cb ) return ( *ca - *cb ) ;
 
  return( 0 ) ;
}

void octmove( octet_t *a, octet_t *b )
{
  if( a->size ) free( a->val ) ;

  a->size = b->size ;
  a->val = b->val ;
  a->len = b->len ;

  memset( b, 0, sizeof( octet_t )) ;
}

int octstr( octet_t *o, char *needle ) 
{
  int   n, m, nl ;
  char  *op, *np, *sp ;

  nl = strlen( needle ) ;
  if( nl > (int) o->len ) return -1 ; 

  m = (int) o->len - nl + 1 ;

  for( n = 0, sp = o->val ; n < m ; n++, sp++ ) {
    if( *sp == *needle ) {
      for( op = sp+1, np = needle+1 ; *np ; op++, np++ )
        if( *op != *np ) break ;
   
      if( *np == '\0' ) return n ; 
    }
  }

  if( n == m ) return -1 ;
  else return n ;
}

int octchr( octet_t *op, char ch ) 
{
  int len, i ;
  char *cp ;

  len = (int) op->len  ;

  for( i = 0, cp = op->val ; i != len ; i++, cp++ ) 
    if( *cp == ch ) return i ;

  return -1 ;
}

int octpbrk( octet_t *op, octet_t *acc )
{
  size_t i, j ;

  for( i = 0 ; i < op->len ; i++ ) {
    for( j = 0 ; j < acc->len ; j++ ) {
      if( op->val[(int) i] == acc->val[(int) j] ) return i ;
    }
  }

  return -1 ;
}

octet_t *oct_new( size_t size, char *val )
{
  octet_t *new ;

  new = (octet_t *) Malloc ( sizeof( octet_t )) ;

  if( size ) {
    new->val = (char *) Calloc ( size, sizeof( char )) ;
    new->size = size ;
    new->len = size ;
  }
  else {
    new->size = 0 ;
    new->val = 0 ;
  }

  if( val ) {
    if( !size ) {
      new->len = strlen( val ) ;
      new->val = (char *) Calloc ( new->len, sizeof( char )) ;
      new->size = new->len ;
    }

    memcpy( new->val, val, new->len ) ;
  }
  else new->len = 0;

  return new ;
}

void oct_free( octet_t *o )
{
  if( o ) {
    if( o->val && o->size ) free( o->val ) ;
    free( o ) ;
  }
}

void oct_freearr( octet_t **oa )
{ 
  if( oa ) {
    for( ; *oa != 0  ; oa++ ) {
      if( (*oa)->size ) free( (*oa)->val ) ; 
      free( *oa ) ;
    }
  }
}

/* Does escaping on the fly */
char *oct2strdup( octet_t *op, char ec )
{
  char c, *str, *cp, *sp ;
  unsigned char uc ;
  size_t  n, i ;
  
  if( op->len == 0 ) return 0 ;

  if( ec == 0 ) {
    str = ( char * ) Malloc((op->len+1)  * sizeof( char )) ;
    strncpy( str, op->val, op->len ) ;
    str[ op->len ] = '\0' ;
  }

  for( n = 1, i = 0, cp = op->val ; i < op->len ; i++, cp++ ) {
    if( *cp >= 0x20 && *cp < 0x7E && *cp != 0x25 ) n++ ;
    else n += 4 ;
  }

  str = ( char * ) Malloc( n  * sizeof( char )) ;

  for( sp = str, i = 0, cp = op->val ; i < op->len ; i++ ) {
    if( !ec ) *sp++ = *cp++ ;
    else if( *cp >= 0x20 && *cp < 0x7E && *cp != 0x25 ) *sp++ = *cp++ ;
    else {
      *sp++ = ec ;
      uc = ( unsigned char ) *cp++ ;
      c = ( uc & (char) 0xF0 ) >> 4 ;
      if( c > 9 ) *sp++ = c + (char) 0x57 ;
      else *sp++ = c + (char) 0x30 ;

      c = ( uc & (char) 0x0F ) ;
      if( c > 9 ) *sp++ = c + (char) 0x57 ;
      else *sp++ = c + (char) 0x30 ;
    }
  }

  *sp = '\0' ;

  return str ;
}

int oct2strcpy( octet_t *op, char *str, size_t len, int do_escape, char ec )
{
  if( op->len == (size_t) 0 || ( op->len + 1 > len )  ) return -1 ;

  if( !do_escape ) {
    if( op->len > len - 1 ) return -1 ;
    memcpy( str, op->val, op->len ) ;
    str[ op->len] = 0 ;
  }
  else {
    char          *sp, *cp ;
    unsigned char uc, c ;
    size_t        i ;

    for( sp = str, i = 0, cp = op->val ; i < op->len && i < len ; i++ ) {
      if( !ec ) *sp++ = *cp++ ;
      else if( *cp >= 0x20 && *cp < 0x7E && *cp != 0x25 ) *sp++ = *cp++ ;
      else {
        *sp++ = ec ;
        uc = ( unsigned char ) *cp++ ;
        c = ( uc & (char) 0xF0 ) >> 4 ;
        if( c > 9 ) *sp++ = c + (char) 0x57 ;
        else *sp++ = c + (char) 0x30 ;
  
        c = ( uc & (char) 0x0F ) ;
        if( c > 9 ) *sp++ = c + (char) 0x57 ;
        else *sp++ = c + (char) 0x30 ;
      }
    }
  
    *sp = '\0' ;
    if( i == len ) return -1 ;
  } 
  return op->len ;
}

/* ---------------------------------------------------------------------- */

static spocp_result_t
oct_replace( octet_t *oct, octet_t *insert, int where, int to_be_replaced, int noresize )
{
  int  n, r ;
  char *sp ;

  if( where > (int) oct->len ) { /* would mean undefined bytes in the middle, no good */
    return -1 ;
  }

  /* final length */
  n = oct->len + insert->len - to_be_replaced ;

  if( n - (int) oct->size > 0 ) {
    if( noresize ) return SPOCP_DENIED ;
    if( oct_resize( oct, n ) != SPOCP_SUCCESS ) return SPOCP_DENIED ;
    oct->size = ( size_t ) n ;
  }

  /* where the insert should be placed */
  sp = oct->val + where ;

  /* amount of bytes to move */
  r = oct->len - where - to_be_replaced ;

  /* move bytes, to create space for the insert */
  memmove( sp + insert->len, sp + to_be_replaced, r ) ;
  memcpy( sp, insert->val, insert->len ) ;

  oct->len = n ;

  return SPOCP_SUCCESS ;
}

/*
   -1 if I failed
    0 if I didn't do anything
    1 if something was done 
 */
spocp_result_t oct_expand( octet_t *src, keyval_t *kvp, int noresize )
{
  int      n ;
  keyval_t *vp ;
  octet_t  oct ;

  if( kvp == 0 ) return SPOCP_SUCCESS ;

  oct.size = 0 ;

  /* starts all over from the beginning of the string which
     means that it can handle recursively expansions
   */
  while(( n = octstr( src, "$$(" )) >= 0 ) {

    oct.val = src->val + n + 3 ;
    oct.len = src->len - ( n +3 ) ;

    oct.len = octchr( &oct, ')' ) ;
    if( oct.len < 0 ) return 1 ; /* is this an error or what ?? */

    for( vp = kvp ; vp ; vp = vp->next ) {
      if( octcmp( vp->key, &oct ) == 0 ) { 
        oct.val -= 3 ;
        oct.len += 4 ;
        if( oct_replace( src, vp->val, n, oct.len, noresize ) != SPOCP_SUCCESS )
          return SPOCP_DENIED ;
        break ;
      }
    }

    if( vp == 0 ) {
      LOG( SPOCP_WARNING ) traceLog("Unkown global: \"%s\"", oct.val) ;
      return SPOCP_DENIED ;
    }
  }

  return SPOCP_SUCCESS ;
}

/*
 Escape is done as '\' hex hex
 */

int oct_de_escape( octet_t *op )
{
  register char *tp, *fp, *ep;
  register char c = 0 ;
  int len ;

  if( op == 0 ) return 0 ;

  len = op->len ;
  ep = op->val + len ;

  for( fp = tp = op->val ; fp != ep  ; tp++, fp++ ) {
    if( *fp == '\\' ) {
      fp++ ;
      if( fp == ep ) return -1 ;

      if( *fp == '\\' ) {
        *tp = *fp ;
        len-- ;
      }
      else {
        if( *fp >= 'a' && *fp <= 'f' ) c = (*fp - 0x57) << 4 ;
        else if( *fp >= 'A' && *fp <= 'F' ) c = (*fp - 0x37) << 4 ;
        else if( *fp >= '0' && *fp <= '9' ) c = (*fp - 0x30) << 4 ;

        fp++ ;
        if( fp == ep ) return -1 ;

        if( *fp >= 'a' && *fp <= 'f' ) c += (*fp - 0x57) ;
        else if( *fp >= 'A' && *fp <= 'F' ) c += (*fp - 0x37) ;
        else if( *fp >= '0' && *fp <= '9' ) c += (*fp - 0x30) ;

        *tp = c ;
       
        len -= 2 ;
      }
    }
    else *tp = *fp ;
  }

  op->len = len ;

  return len ;
}

/* INT_MAX 2147483647 */
int octtoi( octet_t *o )
{
  int  r = 0, d = 1 ;
  char *cp ;

  if( o->len > 10 ) return -1 ;
  if( o->len == 10 && oct2strcmp( o, "2147483647" ) > 0 ) return -1 ; 
  
  for( cp = o->val + o->len -1 ; cp >= o->val ; cp-- ) {
    if( *cp < '0' || *cp > '9' ) return -1 ;
    r += (*cp - '0') * d ;
    d *= 10 ;
  }

  return r ;
}

