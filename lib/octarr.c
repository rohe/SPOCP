#include <config.h>

#include <string.h>

#include <spocp.h>
#include <wrappers.h>

octarr_t *octarr_new( size_t n )
{
  octarr_t *oa ;

  oa = ( octarr_t * ) Malloc ( sizeof( octarr_t )) ;
  oa->size = n ;
  oa->n = 0 ;
  oa->arr = ( octet_t ** ) Calloc ( n, sizeof( octet_t * )) ;

  return oa ;
}

octarr_t *octarr_dup( octarr_t *old )
{
  int       i ;
  octarr_t *new ;

  new = octarr_new( old->size ) ;

  for( i = 0 ; i < old->n ; i++ ) new->arr[i] = octdup( old->arr[i] ) ;
  new->n = old->n ;

  return new ;
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

void octarr_half_free( octarr_t *oa )
{
  int i ;

  if( oa ) {
    if( oa->size ) {
      for( i = 0 ; i < oa->n ; i++ ) free( oa->arr[i] ) ;

      free( oa->arr ) ;
    }
    free( oa ) ;
  }
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

octet_t *octarr_pop( octarr_t *oa )
{
  octet_t *oct ;
  int      i ;

  if( oa == 0 || oa->n == 0 ) return 0 ;

  oct = oa->arr[0] ;

  oa->n-- ;
  for( i = 0 ; i < oa->n ; i++ ) {
    oa->arr[i] = oa->arr[i+1] ;
  }

  return oct ;  
}

/* make no attempt at weeding out duplicates */
/* not the fastest way you could do this */
octarr_t *octarr_join( octarr_t *a, octarr_t *b )
{
  octet_t *o ;

  if( a == 0 ) return b ;
  if( b == 0 ) return a ;

  while(( o = octarr_pop( b )) != 0 ) octarr_add( a, o ) ;
  
  free( b ) ;

  return a ;
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

octet_t *octarr_rm( octarr_t *oa, int n )
{
  octet_t *o ;
  int      i ;

  if( oa == 0 || n > oa->n ) return 0 ;

  o = oa->arr[n] ;
  oa->n-- ;
 
  for( i = n ; i < oa->n ; i++ ) oa->arr[i] = oa->arr[i+1] ;

  return o ;
}

/* ====================================================================== */

octarr_t *oct_split( octet_t *o, char c, char ec, int flag, int max)
{
  char          *cp, *sp ;
  octet_t       *oct = 0 ;
  octarr_t      *oa ;
  int           l, i, n = 0 ;

  if( o == 0 ) return 0 ;

  oct = octdup( o ) ;
  oa = octarr_new( 4 ) ;

  for( sp = oct->val, l = oct->len, i = 0 ; l && ( max == 0 || i < max ) ; sp = cp ) {
    for( cp = sp, n = 0 ; l ; cp++, n++, l-- ) {
      if( *cp == ec ) cp++ ; /* skip escaped characters */
      if( *cp == c ) break ;
    }
    oct->len = n ;

    octarr_add( oa, oct ) ;

    if(flag) for(cp++ ; *cp == c ; cp++, l-- ) ;
    else if( l ) cp++, l-- ;
    else {
      oct = 0 ;
      break ;
    }

    oct = oct_new( 0, 0 ) ;
    oct->val = cp ;
  }

  if( oct ) octarr_add( oa, oct ) ;

  return oa ;
}

