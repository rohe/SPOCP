/***************************************************************************
                                canonsexp.c
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto.h>

#define BUFSIZE 1024

/* I don't think there will be any element in a S-expression
   with a length exceeding 9999 */

#define DIGITS(n) ( (n) >= 1000 ? 4 : (n) >= 100 ? 3 : ( (n) >= 10 ? 2 : 1 ) )

static char *print_len_spec( char *s, int n )
{
  int  sz = DIGITS( n ), i ;

  switch( sz ) {
    case 3:
      i = n/100 ;
      *s++ = '0' + i ;
      n -= 100*i ;

    case 2:
      i = n/10 ;
      *s++ = '0' + i ;
      n -= 10*i ;

    case 1:
      i = n ;
      *s++ = '0' + i ;

  }
  *s++ = ':' ;

  return s ;
}

static char *find_balancing(char *p, char left, char right)
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

/* expects s-expressions of the user friendly form
   (spocp (resource (printer hp)))

   and converts it to the canonical form
 */
static char *sexp_to_canonical( char *canon, char *sexp )
{
  char *sp, *cp, *lp, *ep, c ;
  int  n, len ;

  sp = sexp, cp = canon ;
  len = 0 ;
  ep = 0 ;

  while( *sp ) {
    if( LISTDELIM(*sp) ) {
      c = *sp ;
      *sp++ = 0 ;

      if( ep ) {
        cp = print_len_spec( cp, strlen(ep) ) ;
        strcpy( cp, ep ) ;
        cp += strlen( ep ) ;
        ep = 0 ;
      }

      *cp++ = c ;
    }
    else if( WHITESPACE(*sp )) {
      *sp++ = 0 ;

      if( ep ) {
        cp = print_len_spec( cp, strlen(ep) ) ;
        strcpy( cp, ep ) ;
        cp += strlen( ep ) ;
        ep = 0 ;
      }

      while( WHITESPACE(*sp) ) sp++ ;
    }
    else if( *sp == '"' ) {
      lp = ep = ++sp ;
      while( *lp && *lp != '"' ) lp++ ;

      if( *lp ) {
        *lp++ = 0 ;
        n = ep - lp ;

        cp = print_len_spec( cp, n ) ;
        strcpy( cp, ep ) ;
        cp += strlen( ep ) ;
        ep = 0 ;
        sp = ep ;
      }
      else {
        free( canon ) ;
        return 0 ;
      }
    }
    else{
      if( ep == 0 ) ep = sp ;
      sp++ ;
    }
  }

  *cp = 0 ;

  return canon ;
}

int main( int argc, char **argv )
{
  char adv[BUFSIZE], canon[BUFSIZE], *bp, *sp ;

  while( fgets( adv, BUFSIZE, stdin )) {
    sp = adv ;
    while( *sp != '(' ) sp++ ; /* go find the start */

    /* fun if you try to split one S-expression over serveral lines */
    bp = find_balancing( sp+1, '(', ')' ) ; 
    if( bp == 0 ) continue ; 

    if( *++bp == ';' ) {
      *bp = '\0' ;
      sexp_to_canonical( canon, sp ) ;
      printf( "%s;%s\n",canon, bp ) ;
    }
    else {
      sexp_to_canonical( canon, sp ) ;
      printf( "%s\n",canon ) ;
    }
  }

  return 0 ;
}
