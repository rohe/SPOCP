/***************************************************************************
                          ssn.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <string.h>

#include <db0.h>
#include <struct.h>
#include <func.h>
#include <wrappers.h>

parr_t *get_rec_all_ssn_followers( ssn_t *ssn, parr_t *pa ) ;

ssn_t *ssn_new( char ch )
{
  ssn_t *ssn ;

  ssn = ( ssn_t *) Malloc ( sizeof( ssn_t )) ;

  ssn->ch = ch ;
  ssn->left = ssn->right = ssn->down = 0 ;
  ssn->next = 0 ;
  ssn->refc = 1 ;

  return ssn ;
}

junc_t *ssn_insert_forward( ssn_t **top, char *str )
{
  ssn_t   *pssn = 0 ;
  unsigned char *s ;
 
  s = ( unsigned char *) str ;

  if( *top == 0 ) {
    pssn = *top = ssn_new( *s ) ;

    for( s++; *s ; s++ ) {
      pssn->down = ssn_new( *s ) ;
      pssn = pssn->down ;
    }

    pssn->next = junc_new( ) ;
  }
  else {
    pssn = *top ;
    /* follow the branches as far as possible */
    while( *s ) {
      if( *s == pssn->ch ) {

        /* counter to keep track of if this node is used anymore */
        pssn->refc++ ;

        if( pssn->down ) {
          pssn = pssn->down ;
          s++ ;
        }
        else if( *++s ) {
          pssn->down = ssn_new( *s ) ;
          pssn = pssn->down ;
          break ;
        }
        else break ;
      }
      else if( *s < pssn->ch ) {
        if( pssn->left ) pssn = pssn->left ;
        else {
          pssn->left = ssn_new( *s ) ;
          pssn = pssn->left ;
          break ;
        }
      }
      else if( *s > pssn->ch ) {
        if( pssn->right ) pssn = pssn->right ;
        else {
          pssn->right = ssn_new( *s ) ;
          pssn = pssn->right ;
          break ;
        }
      }
    }

    if( *s == 0) {
      if( pssn->next == 0 )
        pssn->next = junc_new( ) ;
    }
    else {
      for( s++ ; *s ; s++ ) {
        pssn->down = ssn_new( *s ) ;
        pssn = pssn->down ;
      }

      pssn->next = junc_new( ) ;
    }
  }

  return pssn->next ;
}

junc_t *ssn_insert_backward( ssn_t **top, char *str )
{
  ssn_t   *pssn ;
  unsigned char *s ;
 
  s = ( unsigned char *) &str[ strlen(str) -1 ] ;

  if( *top == 0 ) {
    pssn = *top = ssn_new( *s ) ;

    for( s--; s >= ( unsigned char *) str ; s-- ) {
      pssn->down = ssn_new( *s ) ;
      pssn = pssn->down ;
    }

    pssn->next = junc_new( ) ;
  }
  else {
    pssn = *top ;
    /* follow the branches as far as possible */
    while( *s ) {
      if( *s == pssn->ch ) {

        pssn->refc++ ;

        if( pssn->down ) {
          pssn = pssn->down ;
          s-- ;
        }
        else if( *--s ) {
          pssn->down = ssn_new( *s ) ;
          pssn = pssn->down ;
          break ;
        }
        else break ;
      }
      else if( *s < pssn->ch ) {
        if( pssn->left ) pssn = pssn->left ;
        else {
          pssn->left = ssn_new( *s ) ;
          pssn = pssn->left ;
          break ;
        }
      }
      else if( *s > pssn->ch ) {
        if( pssn->right ) pssn = pssn->right ;
        else {
          pssn->right = ssn_new( *s ) ;
          pssn = pssn->right ;
          break ;
        }
      }
    }

    if( *s == 0) {
      if( pssn->next == 0 )
        pssn->next = junc_new( ) ;
    }
    else {
      for( s-- ; s >= ( unsigned char *) str ; s-- ) {
        pssn->down = ssn_new( *s ) ;
        pssn = pssn->down ;
      }

      pssn->next = junc_new( ) ;
    }
  }

  return pssn->next ;
}

junc_t *ssn_insert( ssn_t **top, char *str, int direction )
{
  if( direction == FORWARD ) return ssn_insert_forward( top, str ) ;
  else                       return ssn_insert_backward( top, str ) ;
}

parr_t *ssn_match( ssn_t *pssn, char *sp, int direction )
{
  parr_t        *res = 0 ;
  unsigned char *ucp ;

  if( pssn == 0 ) return 0 ;

  if( direction == FORWARD ) ucp = (unsigned char *) sp ;
  else                       ucp = (unsigned char *) &sp[ strlen(sp) -1 ] ;

  while( (direction == FORWARD && *ucp) ||
         (direction == BACKWARD && ucp >= (unsigned char *) sp) ) {
    if( *ucp == pssn->ch ) {

      /* have to collect the 'next hops' along the way */
      /* Could do this while storing the rule */
      if( pssn->next ) res = parr_add( res, pssn->next ) ;

      if( pssn->down ) {
        pssn = pssn->down ;
        if( direction == FORWARD ) ucp++ ;
        else ucp-- ;
      }
      else break ;
    }
    else if( *ucp < pssn->ch ) {
      if( pssn->left ) pssn = pssn->left ;
      else break ;
    }
    else if( *ucp > pssn->ch ) {
      if( pssn->right ) pssn = pssn->right ;
      else break ;
    }
  }

  return res ;
}

parr_t *ssn_lte_match( ssn_t *pssn, char *sp, int direction, parr_t *res )
{
  unsigned char *ucp ;
  ssn_t         *ps = 0 ; 
  int            f = 0 ;

  if( direction == FORWARD ) ucp = (unsigned char *) sp ;
  else                       ucp = (unsigned char *) &sp[ strlen(sp) -1 ] ;

  while( (direction == FORWARD && *ucp) ||
         (direction == BACKWARD && ucp >= (unsigned char *) sp) ) {

    if( *ucp == pssn->ch ) {
      ps = pssn ;  /* to keep track of where I've been */

      if( pssn->down ) {
        pssn = pssn->down ;
        if( direction == FORWARD ) ucp++ ;

        else ucp-- ;
      }
      else {
        f++ ;
        break ;
      }
    }
    else if( *ucp < pssn->ch ) {
      if( pssn->left ) pssn = pssn->left ;
      else {
        f++ ;
        break ;
      }
    }
    else if( *ucp > pssn->ch ) {
      if( pssn->right ) pssn = pssn->right ;
      else {
        f++ ;
        break ;
      }
    }
  }

  /* have to get a grip on why I left the while-loop */
  if( f == 0 ) { /* because I reached the 'end' of the string */
    /* get everything below and including this node */
    res = get_rec_all_ssn_followers( ps, res ) ;
  }

  return res ;
}

junc_t *ssn_free( ssn_t *pssn )
{
  junc_t *juncp = 0 ;
  ssn_t  *down ;

  while( pssn ) {
    if( pssn->down ) {
      /* there should not be anything to the left or right. No next either */
      down = pssn->down ;
      free( pssn ) ;
      pssn = down ;
    }
    else {
      juncp = pssn->next ;
      free( pssn ) ;
      pssn = 0 ;
    }
  }

  return juncp ;
}

junc_t *ssn_delete( ssn_t **top, char *sp, int direction )
{
  unsigned char *up ;
  ssn_t         *pssn = *top, *ssn, *prev = 0 ;
  junc_t        *jp ;

  if( direction == FORWARD ) up = (unsigned char *) sp ;
  else                       up = (unsigned char *) &sp[ strlen(sp) -1 ] ;

  while( (direction == FORWARD && *up) ||
         (direction == BACKWARD && up >= (unsigned char *) sp) ) {
    if( *up == pssn->ch ) {

      pssn->refc-- ;

      /* Only one linked list onward from here */
      if( pssn->refc  == 0 ) {
        if( *top == pssn ) {
          *top = 0 ;
          jp = 0 ;
          ssn_free( pssn ) ;
        }
        else {
          /* First make sure noone will follow this track anymore */
          if( prev->down == pssn )      prev->down = 0 ;
          else if( prev->left == pssn ) prev->left = 0 ;
          else                          prev->right = 0 ;

          /* follow down until the 'next' link is found */
          for( ssn = pssn ; ssn->down ; ssn = ssn->down ) ;
          jp = ssn->next ; 

          ssn_free( pssn ) ;
          junc_free( jp ) ;
        }

        return 0 ;
      }

      if( pssn->down ) {
        prev = pssn ; /* keep track on where we where */
        pssn = pssn->down ;
        if( direction == FORWARD ) up++ ;
        else up-- ;
      }
      else break ;
    }
    else if( *up < pssn->ch ) {
      if( pssn->left ) {
        prev = pssn ;
        pssn = pssn->left ;
      }
      else break ; /* this should never happen */
    }
    else if( *up > pssn->ch ) {
      if( pssn->right ) {
        prev = pssn ;
        pssn = pssn->right ;
      }
      else break  /* this should never happen */;
    }
  }

  return pssn->next ;
}

parr_t *get_rec_all_ssn_followers( ssn_t *ssn, parr_t *pa )
{
  if( ssn == 0 ) return pa ;

  if( ssn->next ) pa = parr_add( pa, ssn->next ) ; 
  if( ssn->down ) pa = get_rec_all_ssn_followers( ssn->down, pa ) ;
  if( ssn->left ) pa = get_rec_all_ssn_followers( ssn->left, pa ) ;
  if( ssn->right ) pa = get_rec_all_ssn_followers( ssn->right, pa ) ;

  return pa ;
}

parr_t *get_all_ssn_followers( branch_t *bp, int type, parr_t *pa )
{
  ssn_t *ssn ;

  if( type == SPOC_PREFIX ) ssn = bp->val.prefix ;
  else ssn = bp->val.suffix ;

  return get_rec_all_ssn_followers( ssn, pa ) ;
}

ssn_t *ssn_dup( ssn_t *old, ruleinfo_t *ri ) 
{
  ssn_t *new ;

  if( old == 0 ) return 0 ;

  new = ssn_new( old->ch ) ;

  new->refc = old->refc ;

  new->left = ssn_dup( old->left, ri ) ;  
  new->right = ssn_dup( old->right, ri ) ;  
  new->down = ssn_dup( old->down, ri ) ;  
  new->next = junc_dup( old->next, ri ) ;

  return new ;
}

