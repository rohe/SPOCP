#include <config.h>

#include <string.h>
#include <netdb.h>

#include <spocp.h>

/*
Typical rbl entry in DNS
1.5.5.192.blackholes.mail-abuse.org. IN A 127.0.0.2
*/
/* 
  argument to the call:

  rbldomain:hostaddr

  can for instance be
  blackholes.mail-abuse.org:192.5.5.1

*/

spocp_result_t rbl_test( octet_t *arg, becpool_t *bcp, octet_t *blob )
{
  char    rblhost[128] ;
  char    *hostaddr, *s, *r, *c, *d ;
  int     n, l ;
  octet_t **argv ;

  argv = oct_split( arg, ':', 0, 0, 0, &n ) ;

  if( n < 1 ) return SPOCP_MISSING_ARG ;
  if( n > 1 ) return SPOCP_SYNTAXERROR ;

  /* point to end of string */
  s = argv[1]->val + argv[1]->len - 1 ;

  r = rblhost ;

  /* reverse the order */
  hostaddr = argv[1]->val ;

  do {
    for( c = s ; *c != '.' && c >= hostaddr ; c-- ) ;
    if( *c == '.' ) d = c ;
    else d = 0 ;
    
    for( c++ ; c <= s ; ) *r++ = *c++ ;

    if( d ) {
      *r++ = '.' ;
      s = d - 1 ;
    }
  } while( d ) ;

  *r++ = '.' ;

  c = argv[0]->val ;

  /* add after the inverted hostaddr */
  l = (int ) argv[0]->len ;
  for( n = 0 ; n < l ; n++ ) *r++ = *c++ ;

  if( r[n-1] != '.' ) {
    *r++ = '.';
    *r = '\0';
  }

  if( gethostbyname(rblhost) != 0 ) return SPOCP_SUCCESS ;
  else return SPOCP_DENIED ;
}

