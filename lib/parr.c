/***************************************************************************
                          parr.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>
#include <string.h>

#define START_ARR_SIZE  4

#include <struct.h>
#include <wrappers.h>
#include <func.h>


parr_t *parr_new( int size, cmpfn *cf, ffunc *ff, dfunc *df, pfunc *pf )
{
  parr_t *ap ;

  ap = ( parr_t * ) Malloc ( sizeof( parr_t )) ;
  ap->n = 0 ;
  ap->size = size ;
  ap->vect = ( item_t * ) Calloc ( size, sizeof( item_t  ) ) ;
  ap->refc = ( int * ) Calloc ( size, sizeof( int )) ;
  ap->cf = cf ;
  ap->ff = ff ;
  ap->df = df ;
  ap->pf = pf ;

  return ap ;
}

/* --------------------------------------------------------------- */

parr_t *parr_add( parr_t *ap, item_t vp )
{
  item_t *vpp ;
  int   *arr ;

  if( !ap ) ap = parr_new( START_ARR_SIZE, 0, 0, 0, 0 ) ;

  if( ap->n == ap->size ) {
    if( ap->size) ap->size *= 2 ;
    else ap->size = START_ARR_SIZE ;
    vpp = ( item_t * ) Realloc ( ap->vect, ap->size * sizeof( item_t )) ;
    ap->vect = vpp ;
    arr = ( int * ) Realloc( ap->refc, ap->size * sizeof( int )) ;
    ap->refc = arr ;
  }

  ap->refc[ ap->n ] = 1 ;
  ap->vect[ ap->n++ ] = vp ;

  return ap ;
}

/* Don't add a pointer to the same item twice, increase ref counter instead */
/* unsorted list of items */
parr_t *parr_add_nondup( parr_t *ap, item_t vp )
{
  int i ;

  if( ap ) {
    for( i = 0 ; i < ap->n ; i++ )
      if( ap->cf && (ap->cf( ap->vect[i], vp) == 0 )) { 
        ap->refc[i] += 1 ;
        return ap ;
      }
  }

  return parr_add( ap, vp ) ;
}

/* --------------------------------------------------------------- */

void parr_nullterm( parr_t *pp )
{
  parr_add( pp, 0 ) ;

  /* don't really want to add a null item */
  pp->n-- ;
}

/* ---------------------------------------------------------------- */

parr_t *parr_dup( parr_t *ap, item_t ref )
{
  parr_t *res ;
  int     i ;

  if( !ap ) return 0 ;

  res = parr_new( ap->n, ap->cf, ap->ff, ap->df, ap->pf ) ;

  for( i = 0 ; i < ap->n ; i++ ) {
    res->refc[i] = ap->refc[i] ;
    if( ap->df ) res->vect[i] = ap->df( ap->vect[i], ref ) ;
    else         res->vect[i] = ap->vect[i] ;
  }

  res->n = ap->n ;

  return res ;
}

/* --------------------------------------------------------------- */

parr_t *parr_join( parr_t *to, parr_t *from, int dup )
{
  int i ;

  if( !to ) {
    if( dup ) to = parr_dup( from, 0 ) ;
    else to = from ;
  } 
  else if( from ) {
    for( i = 0 ; i < from->n ; i++ )
      to = parr_add_nondup( to, from->vect[i] ) ;
  }

  return to ;
}

/* produce a array of items that appear in either of the arrays
   if it appears in both remove it from the 'to' array */

parr_t *parr_xor( parr_t *to, parr_t *from, int dup )
{
  int i, r ;

  if( !to ) {
    if( dup ) to = parr_dup( from, 0 ) ;
    else to = from ;
  }
  else if( from ) {
    for( i = 0 ; i < from->n ; i++ ) {
      if( to && ( r = parr_find_index( to, from->vect[i] )) >= 0 ) {
        parr_rm_index( &to, r ) ;
      }
      else to = parr_add_nondup( to, from->vect[i] ) ;
    }
  }

  return to ;
}

/* return the first common items there is in the two arrays */

item_t parr_common( parr_t *avp1, parr_t *avp2 )
{
  int i, j ;

  if( !avp1 || !avp2 ) return 0 ;

  for( i = 0 ; i < avp1->n ; i++ ) {
    for( j = 0 ; j < avp2->n ; j++ ) {
      if( avp1->cf ) {
        if( avp1->cf( avp1->vect[i], avp2->vect[j]) == 0 )
          return avp1->vect[i] ;
      }
      else if( avp1->vect[i] == avp1->vect[j] ) 
        return avp1->vect[i] ;
    }
  }

  return 0 ;
}

/* create a new array the contains itemsthat appear in both
   arrays. I don't care about ref counters here */
parr_t *parr_or( parr_t *a1, parr_t *a2 )
{
  parr_t *res = 0 ;
  int i, j ;

  if( !a1 || !a2 ) return 0 ;

  for( i = 0 ; i < a1->n ; i++ ) {
    for( j = 0 ; j < a2->n ; j++ ) {
      if( a1->cf ) {
        if( a1->cf(a1->vect[i], a2->vect[j]) == 0 )
          res = parr_add( res, a1->vect[i] ) ;
      }
      else if( a1->vect[i] == a2->vect[j] ) 
        res = parr_add( res, a1->vect[i] ) ;
    }
  }

  return res ;
}

/* create a new array the contains pointers that appear in either of the 
   base arrays. I don't care about ref counters here */
parr_t *parr_and( parr_t *a1, parr_t *a2 )
{
  parr_t *res = 0 ;

  res = parr_dup( a1, 0 ) ;

  return parr_join( res, a2, 1 ) ;
}

/* =============================================================== */

void parr_free( parr_t *ap )
{
  int i ;

  if( ap ) {
    if( ap->vect ) {
      if( ap->ff ) 
        for( i = 0 ; i < ap->n ; i++ ) ap->ff( ap->vect[i] ) ;

      free( ap->vect ) ;
    }

    if( ap->refc ) free( ap->refc ) ;
    free( ap ) ;
  }
}

/* =============================================================== */

int P_strcasecmp( item_t a, item_t b ) 
{
  return strcasecmp( (char *) a, (char *) b ) ;
}

int P_strcmp( item_t a, item_t b ) 
{
  return strcmp( (char *) a, (char *) b ) ;
}

item_t P_strdup( item_t vp )
{
  return (item_t ) Strdup(( char *) vp ) ;
}

char *P_strdup_char( item_t vp )
{
  return (char *) Strdup(( char *) vp ) ;
}

int P_pcmp( item_t a, item_t b )
{
  if( a == b ) return 0 ; 
  else return 1 ;
}

item_t P_pcpy( item_t i, item_t j )
{
  return i ;
}

/* =============================================================== */

void P_free( item_t vp )
{
  if( vp ) free( vp ) ;
}

void P_null_free( item_t vp )
{
}

/* =============================================================== */

int parr_find_index( parr_t *pp, item_t pattern )
{
  int i ;

  if( pp == 0 ) return -1 ;

  for( i = 0 ; i < pp->n ; i++ )
    if( pp->cf ) {
      if( pp->cf( pp->vect[i], pattern ) == 0 ) return i ;
    }
    else if( pp->vect[i] == pattern ) return i ;
 
  return -1 ;
}

item_t parr_find( parr_t *pp, item_t pattern )
{
  int i ;

  if( pp == 0 ) return 0 ;

  i = parr_find_index( pp, pattern ) ;

  if( i == -1 ) return 0 ;
  else return pp->vect[i] ;
}

/* ---------------------------------------------------------------- */

item_t parr_nth( parr_t *pp, int i )
{
  if( pp == 0 ) return 0 ;

  if( i < 0 || i >= pp->n ) return 0 ;

  return pp->vect[i] ;
}


/* ---------------------------------------------------------------- */

void parr_rm_index( parr_t **ppp, int i )
{
  int     j ;
  parr_t *pp = *ppp ;

  if( pp == 0 ) return ;

  if( i >= pp->n ) return ;

  /* If there are more than one reference to this item don't 
     delete it */

  if( pp->refc[i] > 1 ) {
    pp->refc[i] -= 1 ;
    return ; 
  }

  /* the last item and the refcount for that item is 1 */
  if( pp->n == 1 && i == 0 && pp->refc[0] == 1 ) {
    parr_free( pp ) ;
    *ppp = 0 ;
    return  ;
  }

  pp->ff( pp->vect[i] ) ;

  /* reshuffle */
  for( j = i ; j < pp->n ; j++ ) {
    pp->vect[j] = pp->vect[j+1] ;
    pp->refc[j] = pp->refc[j+1] ;
  }

  pp->n-- ;
}

int parr_get_index( parr_t *pa, item_t vp )
{
  int i ;

  if( pa == 0 || vp == 0 ) return -1 ;

  for( i = 0 ; i < pa->n ; i++ ) 
    if( pa->vect[i] == vp ) return i ;

  return -1 ;
}

void parr_rm_item( parr_t **ppp, item_t vp )
{
  int i ;

  if(( i = parr_get_index( *ppp, vp )) < 0 ) return ;

  parr_rm_index( ppp, i ) ;
}

/* ---------------------------------------------------------------- */

int parr_items( parr_t *pp )
{
  if( pp == 0 ) return -1 ;

  return pp->n ;
}

/* Decrease the refcount with i and if it reaches or is less than 0
   remove the pointer */

void parr_dec_refcount( parr_t **ppp, int delta )
{
  int     j, k, n ;
  parr_t *pp = *ppp ;

  if( pp == 0 ) return ;

  n = pp->n ;

  for( j = pp->n -1 ; j >= 0 ; j-- ) {
    pp->refc[j] -= delta ;

    if( pp->refc[j] <= 0 ) {

      /* reshuffle, one step to the left */

      for( k = j ; k < n ; k++ ) {
        pp->vect[k] = pp->vect[k+1] ;
        pp->refc[k] = pp->refc[k+1] ;
      }

      n-- ;
    }
  }

  pp->n = n ;

  if( pp->n == 0 ) {
    parr_free( pp ) ;
    *ppp = 0 ;
  }
}

/* ---------------------------------------------------------------- */

parr_t *ll2parr( ll_t *ll )
{
  parr_t *pp ;
  int    i ;
  node_t *np ;

  pp = parr_new( ll->n, ll->cf, 0, ll->df, ll->pf ) ;

  for( i = 0, np = ll->head ; i < ll->n ; i++, np = np->next ) {
    pp->refc[i] = 1 ;
    pp->vect[i] = np->payload ;
  }

  pp->n = ll->n ;

  return pp ;
}

