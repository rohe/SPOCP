/***************************************************************************
                          print.c  -  description
                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <struct.h>
#include <func.h>
#include <wrappers.h>
#include <macros.h>

/* should I put a limit to this ?? Well, can't be more than 6 (INT_MAX/3600) 

#define DIGITS(n) ( (n) >= 100000 ? 6 : (n) >= 10000 ? 5 : (n) >= 1000 ? 4 : (n) >= 100 ? 3 : ( (n) >= 10 ? 2 : 1 ) )
*/

void itohms( int sec, char *sp )
{
  int   h, m, s, f = 0, e, n, d, i ;

  h = sec / 3600 ; /* hours */
  e = sec % 3600 ; /* minutes and seconds */
  m = e / 60 ; /* minutes */
  s = e % 60 ; /* seconds */

  if( h ) {
    f++ ;
    n = DIGITS( h ) ;

    for( i = 1, d = 1 ; i < n ; i++ ) d *= 10 ; 

    for( i = 1 ; i < n ; i++ ) {
      n = h / d ;
      h %= d ;
      *sp++ = 0x30 + n ;
      d /= 10 ;
    } 
    *sp++ = 0x30 + h ;
    *sp++ = ':' ;
  }
  
  if( m ) {
    n = m / 10 ;
    m %= 10 ;
    *sp++ = 0x30 + n ;
    *sp++ = 0x30 + m ;
    *sp++ = ':' ;
  }
  else if( f ) {
    *sp++ = '0' ;
    *sp++ = '0' ;
    *sp++ = ':' ;
  }

  if( s ) {
    n = s / 10 ;
    s %= 10 ;
    *sp++ = 0x30 + n ;
    *sp++ = 0x30 + s ;
  }
  else if( f ) {
    *sp++ = '0' ;
    *sp++ = '0' ;
  }
  *sp = '\0' ;
}

char *range_to_txt( range_t *rp ) 
{
  char *res, *sp, dst[ INET6_ADDRSTRLEN + 1] ;

  memset( dst, 0, INET6_ADDRSTRLEN + 1 ) ;

  res = (char *) Calloc( 512, sizeof( char )) ;

  switch( rp->lower.type ) {
    case SPOC_NUMERIC :
      strcat( res, "numeric" ) ;
      if( rp->lower.v.num ) {
        if( rp->lower.type & GLE ) strcat( res, "ge ") ;
        else                      strcat( res, "gt " ) ;

        sp = res + strlen(res) ;
        sprintf( sp, "%ld", rp->lower.v.num ) ;
      }
      if( rp->upper.v.num ) {
        if( rp->upper.type & GLE ) strcat( res, "le ") ;
        else                      strcat( res, "lt " ) ;

        sp = res + strlen(res) ;
        sprintf(sp, "%ld", rp->lower.v.num ) ;
      }
      break ;

    case SPOC_TIME :
      strcat( res, "time" ) ;
      if( rp->lower.v.num ) {
        if( rp->lower.type & GLE ) strcat( res, "ge ") ;
        else                      strcat( res, "gt " ) ;
 
        sp = res + strlen(res) ;
        itohms( rp->lower.v.num, sp ) ;
      }
      if( rp->upper.v.num ) {
        if( rp->upper.type & GLE ) strcat( res, "ge ") ;
        else                      strcat( res, "gt " ) ;
 
        sp = res + strlen(res) ;
        itohms( rp->upper.v.num, sp ) ;
      }
      break ;

    case SPOC_IPV4 :
      strcat( res, "ipv4" ) ;
      if( rp->lower.v.num != 0 ) {
        if( rp->lower.type & GLE ) strcat( res, "ge ") ;
        else                      strcat( res, "gt " ) ;

        inet_ntop( AF_INET, (void *) &rp->lower.v.v4, dst,
                   INET6_ADDRSTRLEN +1 ) ; 
        strcat( res, dst ) ;
      }
      if( rp->upper.v.num !=  0 ) {
        if( rp->upper.type & GLE ) strcat( res, "le ") ;
        else                      strcat( res, "lt " ) ;

        inet_ntop( AF_INET, (void *) &rp->lower.v.v4, dst,
                   INET6_ADDRSTRLEN +1 ) ; 
        strcat( res, dst ) ;
      }
      break ;

    case SPOC_IPV6 :
      strcat( res, "ipv6" ) ;
      if( rp->lower.v.num ) {
        if( rp->lower.type & GLE ) strcat( res, "ge ") ;
        else                      strcat( res, "gt " ) ;

        inet_ntop( AF_INET6, (void *) &rp->lower.v.v6, dst,
                   INET6_ADDRSTRLEN +1 ) ; 
        strcat( res, dst ) ;
      }
      if( rp->upper.v.num ) {
        if( rp->upper.type & GLE ) strcat( res, "le ") ;
        else                      strcat( res, "lt " ) ;

        inet_ntop( AF_INET6, (void *) &rp->lower.v.v6, dst,
                   INET6_ADDRSTRLEN +1 ) ; 
        strcat( res, dst ) ;
      }
      break ;

    case SPOC_ALPHA :
      strcat( res, "alpha " ) ;
      if( rp->lower.v.val.len ) {
        if( rp->lower.type & GLE ) strcat( res, "ge ") ;
        else                      strcat( res, "gt " ) ;

        strncat(res, rp->lower.v.val.val, rp->lower.v.val.len ) ;
      }
      if( rp->upper.v.val.len ) {
        if( rp->upper.type & GLE ) strcat( res, "ge ") ;
        else                      strcat( res, "gt " ) ;

        strncat(res, rp->upper.v.val.val, rp->upper.v.val.len ) ;
      }
      break ;

    case SPOC_DATE :
      strcat( res, "date " ) ;
      if( rp->lower.v.val.len ) {
        if( rp->lower.type & GLE ) strcat( res, "ge ") ;
        else                      strcat( res, "gt " ) ;

        strncat(res, rp->lower.v.val.val, rp->lower.v.val.len ) ;
      }
      if( rp->upper.v.val.len ) {
        if( rp->upper.type & GLE ) strcat( res, "le ") ;
        else                      strcat( res, "lt " ) ;

        strncat(res, rp->upper.v.val.val, rp->upper.v.val.len ) ;
      }
      break ;

  }
 
  return res ;
}

char *element_to_txt( element_t *ep )
{
  char  tmp[4096], *sp, *cp ;
  int   i ;
  set_t *set ;

  memset( tmp, 0, 4096 ) ;

  for( sp = tmp ; ep ; ep = ep->next ) {
    if( strlen( tmp ) ) strcat( sp, " " ) ;

    switch( ep->type ) {
      case SPOC_ATOM :
        strcat( sp, ep->e.atom->val.val ) ; 
        break ;
  
      case SPOC_PREFIX :
        strcat( sp, ep->e.prefix->val.val ) ; 
        break ;
  
      case SPOC_SUFFIX :
        strcat( sp, ep->e.prefix->val.val ) ; 
        break ;
  
      case SPOC_OR :
        strcat( sp, "(* or " ) ;
        set = ep->e.set ;
        for( i = 0 ; i < set->n ; i++ ) {
          cp = element_to_txt( set->element[i] ) ;
          strcat( sp, cp ) ;
          free( cp ) ;
        }
        strcat( sp, ")" ) ;
        break ;

      case SPOC_LIST :
        strcat( sp, "(" ) ;
        cp = element_to_txt( ep->e.list->head ) ;
        strcat( sp, cp ) ;
        free( cp ) ;
        strcat( sp, ")" ) ;
        break ;

      case SPOC_RANGE :
        strcat( sp, "(* range " ) ;
        cp = range_to_txt( ep->e.range ) ;
        strcat( sp, cp ) ;
        free( cp ) ;
        strcat( sp, ")" ) ;
        break ;

    }
  }

  return Strdup( tmp ) ;
}
