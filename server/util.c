#include "locl.h"
RCSID("$Id$");

/* Making two arrays och octet string into one.
   No checking for similar strings are made */
octet_t **join_octarr( octet_t **arr0, octet_t **arr1 )
{
  int  i, j, n, m ;
  octet_t **res ;

  if( arr0 == 0 ) return arr1 ;
  if( arr1 == 0 ) return arr0 ;
  if( arr1 == 0 && arr0 == 0 ) return 0 ;

  for( n = 0 ; arr0[n]  ; n++ ) ;
  for( m = 0 ; arr1[m]  ; m++ ) ;

  res = ( octet_t ** ) realloc ( arr0, n+m+1 * sizeof( octet_t * )) ; 

  for( i = n, j = 0 ; j < m ; j++, i++ ) 
    res[i] = arr1[j] ; 

  res[i] = 0 ;
  free( arr1 ) ;

  return res ;
}

char *rm_lt_sp( char *s, int shift )
{
  char *sp, *cp ;
  int f ;

  if( s == 0 ) return 0 ;

  for( sp = s ; *sp == ' ' || *sp == '\t' ; sp++ ) ;
  for( cp = &sp[ strlen(sp) -1 ] ; *cp == ' ' || *cp == '\t' ; cp-- ) *cp = '\0' ;

  if( shift && sp != s ) {
    f = strlen(sp) ;
    memmove( s, sp, f ) ;
    s[f] = '\0' ;

    return s ;
  }
  else return sp ;
}

