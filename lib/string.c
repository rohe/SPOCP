/***************************************************************************
                          string.c  -  description
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

#include <func.h>

#include <struct.h>
#include <wrappers.h>
#include <spocp.h>
#include <macros.h>

#define SMAX(a,b) ((a) < (b) ? b : a)

/* -1 if no proper s-exp 
   0 if missing chars
   otherwise the number of characters */

int sexp_len( octet_t *sexp )
{
  spocp_result_t r ;
  int            depth = 0  ;
  octet_t        oct, str ;

  oct.val = sexp->val ;
  oct.len = sexp->len ;

  /* first one should be a '(' */
  if( *oct.val != '(' ) return -1 ;

  oct.val++ ;
  oct.len-- ;

  while( oct.len ) {
    if( *oct.val == '(' ) {      /* start of list */
      depth++ ;

      oct.len-- ;
      oct.val++ ;
    }
    else if( *oct.val == ')' ) { /* end of list */
      depth-- ;

      if( depth < 0 ) {
        oct.len-- ;
        break ;                   /* done */
      }

      oct.len-- ;
      oct.val++ ;
    }
    else {
      r = get_str( &oct, &str ) ;
      if( r == SPOCP_SYNTAXERROR ) return -1 ;
      else if( r == SPOCP_MISSING_CHAR ) return 0 ;
    }
  }

  if( depth >= 0 ) return 0 ;

  return (sexp->len - oct.len) ;
}

/* --------------------------------------------------------------------- */

strarr_t *strarr_new( int size )
{
  strarr_t *new ;

  new = ( strarr_t * ) Malloc ( sizeof( strarr_t )) ;

  new->size = size ;
  new->argc = 0 ;
  if( size ) new->argv = ( char ** ) Calloc( size, sizeof( char * )) ;
  else new->argv = 0 ;

  return new ;
}

strarr_t *strarr_add( strarr_t *sa, char *arg )
{
  char **arr ;

  if( sa == 0 ) sa = strarr_new( 4 ) ;

  if( sa->argc == sa->size ) {
     if( sa->size ) sa->size *= 2 ;
     else sa->size = 4 ;
 
     arr = ( char ** ) Realloc( sa->argv, sa->size * sizeof( char * ) ) ;
     sa->argv = arr ;
  }

  sa->argv[ sa->argc++ ] = strdup( arg ) ;

  return sa ;
}

void strarr_free( strarr_t *sa )
{
  int i ;

  if( sa ) {
    for( i = 0 ; i < sa->argc ; i++ ) free( sa->argv[i] ) ;
    free( sa->argv ) ;
    free( sa ) ;
  }
}

/* --------------------------------------------------------------------- */

char *find_balancing(char *p, char left, char right)
{
  int seen = 0;
  char *q = p;

  for( ; *q ; q++) {
    /* if( *q == '\\' ) q += 2 ;  skip escaped characters */

    if (*q == left) seen++;
    else if (*q == right) {
      if (seen==0)  return(q);
      else  seen--;
    }
  }
  return(NULL);
}

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

octet_t *octdup( octet_t *oct )
{
  octet_t *dup ;

  dup = ( octet_t * ) Malloc ( sizeof(octet_t)) ;

  dup->size = dup->len = oct->len ;
  dup->val = ( char * ) Malloc ( oct->len+1 ) ;
  dup->val = memcpy( dup->val, oct->val, oct->len ) ;
  dup->val[dup->len] = 0 ;

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
  int    n, m, j ;
  size_t nl, k ;

  nl = strlen( needle ) ;
  if( nl > o->len ) return -1 ; 

  m = o->len - nl + 1 ;

  for( n = 0 ; n < m ; n++ ) {
    if( o->val[n] == needle[0] ) {
      for( j = n+1, k = 1 ; k < nl ; k++, j++ )
        if( o->val[j] != needle[(int) k] ) break ;
   
      if( k == nl ) return n ; 
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

octet_t *oct_new( size_t len, char *val )
{
  octet_t *new ;

  new = (octet_t *) Malloc ( sizeof( octet_t )) ;

  new->val = (char *) Calloc ( len, sizeof( char )) ;
  new->size = len ;

  if( val ) {
     new->len = strlen( val ) ;
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

void rmcrlf(char *s)
{
  char *cp ;
  int flag = 0 ;

  for( cp = &s[strlen(s)-1] ; cp >= s && ( *cp == '\r' || *cp == '\n' ) ;
       flag++,cp-- ) ;

  if(flag) {
    *++cp = '\0' ;
  }
}

char *rmlt( char *s )
{
  char *cp ;
  int  f = 0 ;

  cp = &s[ strlen( s ) -1 ] ;

  while( *cp == ' ' || *cp == '\t' ) cp--, f++ ;
  if( f ) *(++cp) = '\0' ;

  for( cp = s ; *cp == ' ' || *cp == '\t' ; cp++ ) ;

  return cp ;
}

char **line_split(char *s, char c, char ec, int flag, int max, int *parts)
{
  char **res, *cp, *sp ;
  register char ch ;
  char *tmp ;
  int  i, n = 0 ;

  if(flag ) {
    while( *s == c ) s++ ;
    for( i = 0, cp = &s[strlen(s)-1] ; *cp == c ; cp--, i++ ) ;
    if( i ) *(++cp) = '\0' ;
  }

  if( max == 0 ) {
    /* find out how many strings there are going to be */
    for( cp = s ; *cp ; cp++ )
      if( *cp == c ) n++ ;
    max = n + 1 ;
  }
  else n = max ;

  res = ( char **) Calloc ( n+3, sizeof(char *)) ;
  tmp = Strdup(s) ;

  for( sp = tmp, i = 0, max-- ; sp && i < max ; sp = cp, i++ ) {
    for( cp = sp ; *cp ; cp++) {
      ch = *cp ;
      if( ch == ec ) cp += 2 ; /* skip escaped characters */
      if( ch == c || ch == '\0' ) break ;
    }

    if( *cp == '\0' ) break ;

    *cp = '\0' ;
    if( *sp ) res[i] = Strdup(sp) ;

    if(flag) for(cp++ ; *cp == c ; cp++ ) ;
    else cp++ ;
  }

  res[i] = Strdup(sp) ;

  *parts = i ;

  free(tmp) ;

  return res ;
}

octet_t **oct_split( octet_t *o, char c, char ec, int flag, int max, int *parts)
{
  char          *cp, *sp ;
  char          *tmp ;
  octet_t       **res = 0 ;
  int           l, i, n = 0 ;

  if( o == 0 ) return 0 ;

  sp = o->val ;
  l = o->len ;

  if(flag ) {
    for( ; l && *sp == c ; sp++, l-- ) ;
    if( l == 0 ) return 0 ;
    for( cp = &o->val[ l - 1 ] ; *cp == c ; cp--, l-- ) ;
  }

  if( max == 0 ) {
    /* find out how many strings there are going to be */
    for( cp = sp, i = l ; i ; cp++, i-- ) {
      if( ec && *cp == ec ) cp++ ;
      else if( *cp == c ) n++ ;
    }
    max = n + 1 ;
  }
  else n = max ;

  tmp = ( char * ) Calloc ( l + 1, sizeof( char )) ;
  memcpy( tmp, sp, l ) ;

  res = ( octet_t **) Calloc ( (n+2), sizeof(octet_t *)) ;
  for( i = 0 ; i <= n ; i++ )
    res[i] = ( octet_t * ) Calloc (1, sizeof( octet_t )) ;

  res[0]->size = l ; /* so that the string is only freed once */

  for( sp = tmp, i = 0, max-- ; l && i < max ; sp = cp, i++ ) {
    res[i]->val = sp ;
    res[i]->len = 0 ;

    for( cp = sp, n = 0 ; l ; cp++, n++, l-- ) {
      if( *cp == ec ) cp++ ; /* skip escaped characters */
      if( *cp == c ) break ;
    }

    res[i]->len = n ;

    if(flag) for(cp++ ; *cp == c ; cp++, l-- ) ;
    else cp++, l-- ;
  }

  res[i]->val = sp ;
  res[i]->len = l ;

  *parts = i ;

  return res ;
}

char **arrdup( int n, char **arr )
{
  char **new_arr ;
  int    i ;

  new_arr = ( char ** ) Malloc ( n * sizeof( char * )) ;
  for ( i = 0 ; i < n ; i++ )
    new_arr[i] = Strdup( arr[i] ) ;

  return new_arr ;
}

void charmatrix_free( char **arr )
{
  int i ;

  if( arr ) {
    for( i = 0 ; arr[i] != 0 ; i++ ) {
      free(arr[i]) ;
    }
    free(arr) ;
  }
}

int expand_string( char *src, keyval_t *kvp, char *dest, size_t size )
{
  char     *sp, *cp, *pp ;
  int      l ;
  keyval_t *vp ;

  if( kvp == 0 ) {
    if( strlen( src ) <  size ) strcpy( dest, src ) ;
    return 1 ;
  }

  *dest = '\0' ;
  size-- ; /* last '\0' must also have a place */

  sp = src ;
  
  while(( cp = strstr( sp, "$$(" ))) {
    if(( l = cp - sp) ) {
      size -= l ;
      if( size < 0 ) return 0 ;
      strncat( dest, sp, l ) ;
    } 

    cp += 3 ;
    for( pp = cp ; *pp && *pp != ')' ; pp++ ) ;
    if( pp == 0 ) return 0 ;

    l = pp-cp ;

    for( vp = kvp ; vp ; vp = vp->next ) {
      if( oct2strncmp( vp->key, cp, l ) == 0 ) { 
        size -= vp->val->len ;
        if( size < 0 ) return 0 ;
        strncat( dest, vp->val->val, vp->val->len ) ;
        break ;
      }
    }

    if( vp == 0 ) {
      *pp = '\0' ;
      LOG( SPOCP_WARNING ) traceLog("Unkown global: \"%s\"", cp) ;
      return 0 ;
    }

    sp = pp+1 ;
  }

  if( size < strlen( sp )) return 0 ;

  strcat( dest, sp ) ;

  return 1 ;
}

spocp_result_t
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

octarr_t *octarr_new( size_t n )
{
  octarr_t *oa ;

  oa = ( octarr_t * ) Malloc ( sizeof( octarr_t )) ;
  oa->size = n ;
  oa->n = 0 ;
  oa->arr = ( octet_t ** ) Calloc ( n, sizeof( octet_t * )) ;

  return oa ;
}

octarr_t *octarr_add( octarr_t *oa, octet_t *op )
{
  octet_t **arr ;

  if( oa == 0 ) {
    oa = octarr_new( 4 ) ;
  }

  if( oa->size == 0 ) {
    oa->size = 2 ;
    oa->arr = ( octet_t **) Malloc ( oa->size * sizeof( octet_t * )) ;
    oa->n = 0 ;
  }
  else if( oa->n == oa->size ) {
    oa->size *= 2 ;
    arr = (octet_t ** ) Realloc( oa->arr, oa->size * sizeof( octet_t * )) ;
    oa->arr = arr ;
  }

  oa->arr[oa->n++] = op ;

  return oa ;
}

void octarr_mr( octarr_t *oa, size_t n )
{
  octet_t **arr ;

  oa->size *= 2 ;

  while(( oa->size - oa->n ) < (int) n )  oa->size *= 2 ;

  arr = (octet_t ** ) Realloc( oa->arr, oa->size * sizeof( octet_t * )) ;
  oa->arr = arr ;
}

void octarr_free( octarr_t *oa )
{
  int i ;

  if( oa ) {
    if( oa->size ) {
      for( i = 0 ; i < oa->n ; i++ ) oct_free( oa->arr[i] ) ;

      free( oa->arr ) ;
    }
    free( oa ) ;
  }
}

char *safe_strcat( char *dest, char *src, int *size )
{
  char *tmp ;
  int  dl, sl ;

  if( src == 0 || *size <= 0 ) return 0 ;

  dl = strlen(dest) ;
  sl = strlen(src) ;

  if( sl + dl  > *size ) {
    *size = sl + dl + 32 ;
    tmp = Realloc( dest, *size ) ;
    dest = tmp ;
  }

  strcat( dest, src ) ;

  return dest ;
}

