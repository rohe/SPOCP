/***************************************************************************
                          element.c  -  description
                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <string.h>
#include <netinet/in.h>

#include <struct.h>
#include <func.h>
#include <wrappers.h>

static int range_cmp( range_t *ra, range_t *rb ) ;
static int list_cmp( list_t *la, list_t *lb ) ;

static int atom2range_cmp( atom_t *a, range_t *ran ) ;
static int atom2prefix_cmp( atom_t *a, atom_t *pre ) ;
static int atom2suffix_cmp( atom_t *a, atom_t *suf ) ;
/*
static int atom2and_cmp( atom_t *a, set_t *and ) ;
*/

/* =============================================================== */
/*

This is the matrix over comparisions, that I'm using

X = makes sense
O = can make sense in very special cases

        | atom | prefix | range | list | suffix |
--------+------+--------+-------+------|--------|
atom    |  X   |   X    |   X   |      |   X    |
--------+------+--------+-------+------|--------|
prefix  |      |   X    |       |      |        |
--------|------+--------+-------+------|--------|
range   |  0   |        |   X   |      |        |
--------+------+--------+-------+------|--------|
list    |      |        |       |  X   |        |
---------------------------------------+--------|
suffix  |      |        |       |      |   X    |
---------------------------------------+--------|

 */

static int atom2X_cmp( atom_t *a, element_t *e )
{
  int v = 1 ;

  switch( e->type ) {
    case SPOC_PREFIX:
      v = atom2prefix_cmp( a, e->e.prefix ) ;
      break ;

    case SPOC_SUFFIX:
      v = atom2suffix_cmp( a, e->e.prefix ) ;
      break ;

    case SPOC_RANGE :
      v = atom2range_cmp( a, e->e.range ) ;
      break ;

    default:
      break ;
  }

  return v ;
}

/* Atom compared to range/prefix,suffix or and */
static int atom2range_cmp( atom_t *a, range_t *ran )
{
  int             i, r = 1, rl, ru ;
  long            l ;
  struct in_addr  v4 ;
  struct in6_addr v6 ;

  switch( ran->lower.type & 0x07 ) {
    case SPOC_NUMERIC:
    case SPOC_TIME:
      if( is_numeric( &a->val, &l ) != SPOCP_SUCCESS ) r = 1 ;
      else if( l >= ran->lower.v.num && l <= ran->upper.v.num ) r = -1 ;
      break ;

    case SPOC_DATE:
      if( is_date( &a->val ) != SPOCP_SUCCESS ) r = 1 ;

    case SPOC_ALPHA :
      if( octcmp( &a->val, &ran->lower.v.val ) >= 0 &&
          octcmp( &a->val, &ran->upper.v.val ) <= 0 ) return -1 ;

      break ;

    case SPOC_IPV4:
      if( is_ipv4( &a->val, &v4 ) != SPOCP_SUCCESS ) r = 1 ;

      if(( v4.s_addr >= ran->lower.v.v4.s_addr) && 
         ( v4.s_addr <= ran->upper.v.v4.s_addr) ) r = -1 ; 

      break ;

    case SPOC_IPV6: /* Can be viewed as a array of unsigned ints  */
      if( is_ipv6( &a->val, &v6 ) != SPOCP_SUCCESS ) r = 1 ;

      for( i = 0, rl = 0 ; i < 16 ; i++ )
        if(( rl = v6.s6_addr[i] - ran->lower.v.v6.s6_addr[i]) != 0 ) break ;
      for( i = 0, rl = 0 ; i < 16 ; i++ )
        if(( ru = v6.s6_addr[i] - ran->upper.v.v6.s6_addr[i]) != 0 ) break ;

      if( rl >= 0 && ru <= 0 ) r = -1 ;
      break ;

  }

  return 0 ;
}

static int atom2prefix_cmp( atom_t *a, atom_t *pre )
{
  /* the length of the prefix has to be shorter or egual to the length
     of the atom value */

  if( a->val.len < pre->val.len ) return 1 ;
  
  return strncmp( pre->val.val, a->val.val, pre->val.len ) ;
}

static int atom2suffix_cmp( atom_t *a, atom_t *suf )
{
  /* the length of the prefix has to be shorter or egual to the length
     of the atom value */

  if( a->val.len < suf->val.len ) return 1 ;
  
  return strncmp( suf->val.val, &a->val.val[a->val.len - suf->val.len], suf->val.len ) ;
}

/* -------------------------------------------------------------------------*/

/* simple version looking at the low limit and if that is
   absent in both range definitions look at the highend limit */
static int range_cmp( range_t *ra, range_t *rb )
{
  int v ;

  if(( v = ra->lower.type - rb->lower.type) != 0 ) return v ;

  if(( v = boundary_xcmp( &ra->lower, &rb->lower)) != 0 ) return v ;

  return boundary_xcmp( &ra->upper, &rb->upper ) ;
}

/* is ea more specific than eb  */
/*
  (* and S0 S1...Sn) <= (* and T0 T1...Tm)
  if and only if n >= m, and for every Ti (i = 0,..,m)
  there exists at least one Sj (j=0,..,n) such that Sj <= Ti

static int and_cmp( set_t *a, set_t *b )
{
  element_t **ea, **eb ;
  int i, j ;

  if( b->n > a->n ) return 0 ;

  eb = b->element ;
  for( i = 0 ; i < b->n ; i++ ) { 
    ea = a->element ;
    for( j = 0 ; j < a->n ; j++ ) {
      if( element_cmp( ea[j], eb[i] ) == 1 ) break ;
    }
    if( j == a->n ) return 0 ;
  }

  return 1 ;
}
*/

/*  */
static int list_cmp( list_t *la, list_t *lb )
{
  int v ;
  element_t *ea, *eb ;

  /* comparing the hash values over the lists, fast if they are exactly the same */
  if( la->hstrlist == lb->hstrlist ) return 0 ;

  ea = la->head ;
  eb = lb->head ;

  for( ; ea && eb ; ea = ea->next, eb = eb->next ) {
    if(( v = element_cmp( ea, eb ))) return v ;
  }

  /* more elements in ea means it is more specific == less */
  if( ea == 0 && eb == 0 ) return 0 ;
  else if( ea ) return -1 ;
  else return 1 ;
}

/* Is element a more specific than element b ?? */
int element_cmp( element_t *ea, element_t *eb )
{
  int v ;

  v = ea->type - eb->type ;

  if( v == 0 ) {
    switch( ea->type ) {
      case SPOC_ATOM:
        v = ea->e.atom->hash - eb->e.atom->hash ;
        break ;

      case SPOC_PREFIX:
        v = ea->e.prefix->hash - eb->e.prefix->hash ;
        break ;

      case SPOC_SUFFIX:
        v = ea->e.prefix->hash - eb->e.prefix->hash ;
        break ;

      case SPOC_RANGE:
        v = range_cmp( ea->e.range, eb->e.range ) ;
        break ;

      case SPOC_LIST:
        v = list_cmp( ea->e.list, eb->e.list ) ;
        break ;
    }
  }
  else { /* not the same type */
    switch( ea->type ) {
      case SPOC_ATOM : 
        atom2X_cmp( ea->e.atom, eb ) ;
        break ;

      case SPOC_PREFIX : /* everything else is in fact invalid */
      case SPOC_SUFFIX :
      case SPOC_RANGE :
        break ;
    }
  }

  return v ;
}

/*
element_t *find_list( element_t *ep, char *tag )
{
  element_t *head, *nep = 0 ;

  switch( ep->type ) {
    case SPOC_LIST :
      head = ep->e.list->head ;
      if( oct2strcmp( &head->e.atom->val, tag) == 0 ) return head ;
      else nep = find_list( head->next, tag ) ;
      break ;

  }

  return nep ;
}
*/

int only_atoms( element_t *ep )
{
  while( ep ) {
    if( ep->type != SPOC_ATOM ) return 0 ;
    ep = ep->next ;
  }

  return 1 ;
}

void keyval_free( keyval_t *kv )
{
  if( kv ) {
    if( kv->key ) oct_free( kv->key ) ;
    if( kv->val ) oct_free( kv->val ) ;

    free( kv ) ;
  }
}

void P_free_keyval( void *vp )
{
  keyval_free( (keyval_t *) vp ) ;
}

parr_t *get_simple_lists( element_t *ep, parr_t *ap )
{
  element_t *head, *tep ;
  int       l, n ;
  keyval_t  *kv = 0 ;

  switch( ep->type ) {
    case SPOC_LIST :
      head = ep->e.list->head ;

      /* could be a list with only the tag */
      if( head->next == 0 ) ;
      else if( only_atoms( head->next ) ) { /* I know the first is an atom */
        kv = ( keyval_t * ) Calloc( 1, sizeof( keyval_t )) ;
        kv->key = octdup( &head->e.atom->val ) ;
        head = head->next ;
 
        for( tep = head, l = 0, n = 0 ; tep ; tep = tep->next, n++ )
          l += strlen( tep->e.atom->val.val ) ;

        kv->val = oct_new( l+n+1, 0 ) ;

        memcpy( kv->val->val, head->e.atom->val.val, head->e.atom->val.len ) ;
        kv->val->len = head->e.atom->val.len ;
 
        for( head = head->next ; head ; head = head->next ) {
          if( octcat( kv->val, "\t", 1 ) != SPOCP_SUCCESS ) {
            keyval_free( kv ) ;
            return 0 ;
          }
          if( octcat( kv->val, head->e.atom->val.val, head->e.atom->val.len ) != SPOCP_SUCCESS ) {
            keyval_free( kv ) ;
            return 0 ;
          }
        }

        ap = parr_add( ap, (void *) kv ) ;
      }
      else { /* skip the tag since that isn't interesting */
        for( head = head->next ; head ; head = head->next )
          ap = get_simple_lists( head, ap ) ;
      }
      break ;

    case SPOC_OR :
      break ;

    default: /* just don't do anything */
      break ;
  }

  if( ap && ap->ff == 0 ) ap->ff = &P_free_keyval ;
 
  return ap ;
}

