/*! \file lib/rvapi.c
 * \author Roland Hedberg <roland@catalogi.se>
 */
/***************************************************************************
                          rvapi.c  -  description
                             -------------------
    begin                : Thr Jan 29 2004
    copyright            : (C) 2003 by Stockholm university, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <db0.h>
#include <struct.h>
#include <func.h>
#include <proto.h>
#include <spocp.h>
#include <wrappers.h>
#include <macros.h>
#include <rvapi.h>

#define TAGCHAR(c)  ( ALPHA(c) || DIGIT(c) || (c) == '-' || (c) == '_' || (c) == '.' )

static varr_t *list_search( element_t *e, octet_t *tag, varr_t *r) ;
/* static element_t *element_dup( element_t *ep, element_t *memberof ) ; */
/* from input.c */
void set_memberof( varr_t *va, element_t *group ) ;

/********************************************************************/

atom_t *atom_dup( atom_t *ap )
{
  atom_t *new = 0 ;

  if( ap == 0 ) return 0 ;

  new = atom_new( &ap->val ) ;

  return new ;
}

/* ====================================================================== */
/*!
 * Joins a set of atoms with a separator inbetweem into one string of bytes
 * \param e A pointer to a element structure which should contain either a set or
 * a list of elements each containing a atom.
 * \param sep The separator which is to be placed between the atom values
 * \return A pointer to a a octet struct which contains the result of the concatenation
 */
octet_t *atoms_join( element_t *e, char *sep ) 
{
  char      *result = 0, *sp ;
  int        n, size = 1, slen = 0 ;
  element_t *ep ;
  octet_t   *oct = 0 ;
  void      *v ;
  varr_t    *va ;

  if( e == 0 ) return 0 ;

  if( sep && *sep ) slen = strlen( sep ) ;

  switch( e->type ) {
    case SPOC_SET :
    case SPOC_ARRAY :
      va = e->e.set ;
      for( v = varr_first( va ), n = 0 ; v ; v = varr_next( va, v )) {
        ep = (element_t *) v ;
        if( ep->type == SPOC_ATOM ) {
          size += ep->e.atom->val.len ;
          n++ ;
        }
      }
      if( slen ) size += ( n-1) * slen ;

      result = ( char * ) Malloc( size * sizeof( char )) ;

      for( sp = result, n = 0, v = varr_first(va) ; v ; v = varr_next( va, v )) {
        ep = (element_t *) v ;
        if( ep->type == SPOC_ATOM ) {
          if( n ) {
            memcpy( sp, sep, slen ) ; 
            sp += slen ;
          }
          memcpy( sp, ep->e.atom->val.val, ep->e.atom->val.len ) ; 
          sp += ep->e.atom->val.len ;
          n++ ;
        }
      }
      break ;

    case SPOC_LIST:
      for( n = 0, ep = e->e.list->head ; ep ; ep = ep->next ) {
        if( ep->type == SPOC_ATOM ) {
          size += ep->e.atom->val.len ;
          n++ ;
        }
      }
      if( slen ) size += ( n-1) * slen ;

      result = ( char * ) Malloc( size * sizeof( char )) ;

      for( n = 0, sp = result, ep = e->e.list->head ; ep ; ep = ep->next ) {
        if( ep->type == SPOC_ATOM ) {
          if( n ) {
            memcpy( sp, sep, slen ) ; 
            sp += slen ;
          }
          memcpy( sp, ep->e.atom->val.val, ep->e.atom->val.len ) ; 
          sp += ep->e.atom->val.len ;
          n++ ;
        }
      }
      break ;

  }

  if( result ) {
    oct = oct_new( 0, 0 ) ;
    oct->val = result ;
    oct->size = oct->len = size ;
  }

  return oct ;
}

/* ====================================================================== */

static list_t *list_new( void )
{
  list_t *new ;

  new = (list_t *) Malloc ( sizeof( list_t )) ;
  new->hstrlist = 0 ;
  new->head = 0 ;

  return new ;
}

/********************************************************************/

static list_t *list_dup( list_t *lp, element_t *e ) 
{
  list_t    *new = 0 ;
  element_t *ep = 0, *le ;

  if( lp == 0 ) return 0 ;

  new = list_new() ;
  new->hstrlist = lp->hstrlist ;
  new->head = ep = element_dup( lp->head, e ) ;
  for( le = lp->head->next ; le ; le = le->next ) {
    ep->next = element_dup( le, e ) ;
    ep = ep->next ;
  }

  return new ;
}

/********************************************************************/

static int boundary_cpy( boundary_t *dest, boundary_t *src )
{
  int  i ;
  char *a,*b ;

  switch( src->type & 0x07 ) {
    case SPOC_ALPHA:
    case SPOC_DATE :
      if( src->v.val.len ) {
        dest->v.val.val = Strndup( src->v.val.val, src->v.val.len ) ;
        dest->v.val.len = src->v.val.len ;
      }
      break ;

    case SPOC_NUMERIC:
    case SPOC_TIME:
      dest->v.num = src->v.num ;
      break ;

    case SPOC_IPV4:
      a = (char *) &dest->v.v6 ;
      b = (char *) &src->v.v6 ;
      for( i = 0 ; i < (int) sizeof( struct in_addr ) ; i++ ) *a++ = *b++ ;
      break ;

    case SPOC_IPV6:
      a = (char *) &dest->v.v6 ;
      b = (char *) &src->v.v6 ;
      for( i = 0 ; i < (int) sizeof( struct in6_addr ) ; i++ ) *a++ = *b++ ;
      break ;
  }

  dest->type = src->type ;

  return 0 ;
}

/********************************************************************/

static range_t *range_dup( range_t *rp )
{
  range_t *new = 0 ;

  if( rp == 0 ) return 0 ;

  new = ( range_t * ) Malloc( sizeof( range_t )) ;

  boundary_cpy( &new->lower, &rp->lower ) ;
  boundary_cpy( &new->upper, &rp->upper ) ;

  return new ;
}

/********************************************************************/

static item_t i_element_dup( item_t a, item_t b )
{
  return (item_t) element_dup( (element_t *)a, (element_t *)b ) ;
}

static varr_t *set_dup( varr_t *va, element_t *parent )
{
  varr_t *new ;
  
  new = varr_dup( va, &i_element_dup ) ;

  set_memberof( va, parent ) ;

  return new ;
}


/********************************************************************/

element_t *element_dup( element_t *ep, element_t *memberof )
{
  element_t *new = 0 ;

  if( ep == 0 ) return 0 ;

  new = element_new() ;

  new->not = ep->not ;
  new->type = ep->type ;
  new->memberof = memberof ;

  switch( ep->type ) {
    case SPOC_ATOM :
    case SPOC_PREFIX :
    case SPOC_SUFFIX :
      new->e.atom = atom_dup( ep->e.atom ) ;
      break ;

    case SPOC_LIST :
      new->e.list = list_dup( ep->e.list, new ) ;
      break ;

    case SPOC_SET :
    case SPOC_ARRAY :
      new->e.set = set_dup( ep->e.set, new ) ;
      break ;

    case SPOC_RANGE :
      new->e.range = range_dup( ep->e.range ) ;
      break ;

    default :
      break ;

  }

  return new ;
}

/* ====================================================================== */
/*!
 * Add a element to a list of elements, the element is added to the end of the
 * list. If the list is empty a new list is created and the element is added as
 * the first in the list.
 * The S-expression restriction that the first element in a list has to be a atom
 * is not inforced in this case. Any type of element can be added anywhere.
 * \param le A pointer to the element list
 * \param e The element that is to be added to the list
 * \return A pointer to the possibly newly created list
 */
element_t *element_list_add( element_t *le, element_t *e )
{
  element_t *ep, *end ;

  if( e == 0 ) return le ;

  if( le == 0 ) {
    le = element_new() ;
    le->type = SPOC_LIST ;
    le->e.list = list_new() ;
    le->e.list->head = element_dup( e, le ) ;
  }
  else {
    if( le->type != SPOC_LIST ) return 0 ;

    /* next to last element */
    for( ep = le->e.list->head ; ep->next ; ep = ep->next ) ;

    end = ep->next ;
    ep->next = element_dup( e, le ) ;
    ep->next->next = end ;
  }

  return le ;
}

/* ====================================================================== */

/*!
 * Adds a element to an array of elements. The difference between this and 
 * element_add_lsit is that the array is unsorted.
 * \param le A pointer to the element array
 * \param e A pointer to the element to be added
 * \return A pointer to the array, which might be created by the function
 */
element_t *element_array_add( element_t *le, element_t *e )
{
  if( e == 0 ) return le ;

  if( le == 0 ) {
    le = element_new() ;
    le->type = SPOC_ARRAY ;
    le->e.set = varr_add( 0, e ) ;
  }
  else {
    if( le->type != SPOC_ARRAY ) return 0 ;
    le->e.set = varr_add( le->e.set, e ) ;
  }

  return le ;
}

/* ====================================================================== */
/*!
 * The number of elements in this element. Only returns the number of immediate 
 * elements, so if this list contains lists those lists are counted as 1 and not
 * as the number of elements in the sublists. If the element is a list, array or
 * set then the number of elements in those are returned, if it is a atom, 1 is
 * returned.
 * \param e The element in which the subelements are to be counted.
 * \return The number of subelements in the element.
 */
int element_size( element_t *e) 
{
  int i = 0 ;

  if( e == 0 ) return -1 ;

  switch( e->type ) {
    case SPOC_ATOM :
      i = 1 ;

    case SPOC_LIST :
      for( i = 0, e = e->e.list->head ; e ; e = e->next ) i++ ;
      break ;

    case SPOC_SET :
    case SPOC_ARRAY :
      i = e->e.set->n ;
      break ;
  }

  return i ;
}

/* ---------------------------------------------------------------------- */
/*!
 * There are different types of elements this function returns a indicator of
 * what kind of element it is.
 * \param e The element which type is asked for
 * \return O if atom, 1 if list, 2 if set, 3 if prefix, 4 is suffix
 *         5 if range, 9 if an array and 10 if a NULL element.
 */
int element_type( element_t *e ) 
{
  if( e == 0 ) return -1 ;

  return( e->type ) ;
}

/* ---------------------------------------------------------------------- */

static varr_t *set_search( varr_t *va, octet_t *tag, varr_t *r )
{
  element_t *ep ;
  void      *v ;

  if( va == 0 || tag == 0 || tag->len == 0 ) return 0 ;

  for( v = varr_first( va ) ; v ; varr_next( va, v ) ) {
    ep = (element_t *) v ;
    if( ep->type == SPOC_LIST )
      r = list_search( ep->e.list->head, tag, r ) ;
    else if( ep->type == SPOC_SET || ep->type == SPOC_ARRAY )
      r = set_search( ep->e.set, tag, r ) ;
  }

  return r ;
}

/* ---------------------------------------------------------------------- */

static varr_t *list_search( element_t *e, octet_t *tag, varr_t *r)
{
  if( e == 0 ) return 0 ;

  if( octcmp( tag, &(e->e.atom->val)) == 0 ) {
    r = varr_add( r, element_dup( e->memberof, e->memberof->memberof )) ;
  }

  for( e = e->next ; e ; e = e->next ) {
    if( e->type == SPOC_LIST )
      r = list_search( e->e.list->head, tag, r ) ;
    else if( e->type == SPOC_SET || e->type == SPOC_ARRAY )
      r = set_search( e->e.set, tag, r ) ;
  }

  return r ;
}

/* ---------------------------------------------------------------------- */
/*!
 */
element_t *element_find_list( element_t *e, octet_t *tag ) 
{
  element_t *re = 0 ;
  varr_t    *varr = 0 ;

  if( e == 0 ) return 0 ;

  if( e->type == SPOC_SET || e->type == SPOC_ARRAY ) {
    varr = set_search( e->e.set, tag, varr ) ;
  }
  else if( e->type == SPOC_LIST ) {
    varr = list_search( e->e.list->head, tag, varr ) ;
  }
  
  if( varr ) {
    re = element_new() ;
    re->type = SPOC_SET ;
    re->e.set = varr ;
  }

  return re ;
}

/* ---------------------------------------------------------------------- */

element_t *element_traverse( element_t *e, octarr_t *oa, int i )
{
  element_t  *ep, *rep ;
  int         j, n ;
  varr_t     *varr = 0 ;

  if( e == 0 ) return 0 ;

  if( e->type == SPOC_SET || e->type == SPOC_ARRAY ) {
    varr = e->e.set ;
    n = varr_len( varr ) ;
    for( j = 0 ; j < n ; j++ ) {
      ep = varr_nth( varr, i ) ;
      if( ep->type == SPOC_LIST ) {
        if(( rep = element_traverse( ep, oa, i )) != 0 ) return rep ;
      }
    }
  }
  else if ( e->type == SPOC_LIST ) {
    ep = e->e.list->head ;
    if( octcmp( &(ep->e.atom->val), oa->arr[i] ) == 0 ) {
      i++ ;
      if( i == oa->n ) return ep->memberof ; /* the start of the sublist */

      /* go down the list */
      for( ; ep ; ep = ep->next ) {
        if( ep->type == SPOC_LIST ) {
          if(( rep = element_traverse( ep, oa, i )) != 0 ) return rep ;
        }
      }
    }
  }

  return 0 ;
}

/* ---------------------------------------------------------------------- */

element_t *element_first( element_t *e ) 
{
  if( e == 0 ) return 0 ;

  if( e->type == SPOC_LIST ) return e->e.list->head ;
  else if( e->type == SPOC_SET || e->type == SPOC_ARRAY )
    return (element_t *) varr_nth( e->e.set, 0 ) ;
  else return e ;
}

/* ---------------------------------------------------------------------- */

element_t *element_last( element_t *e )
{
  if( e == 0 ) return 0 ;

  if( e->type == SPOC_LIST ) {
    for( e = e->e.list->head ; e->next ; e = e->next ) ;
    return e ;
  }
  else if( e->type == SPOC_SET || e->type == SPOC_ARRAY )
    return (element_t *) varr_nth( e->e.set, varr_len( e->e.set ) -1 ) ;
  else return e ;
}

/* ---------------------------------------------------------------------- */

element_t *element_nth( element_t *e, int n )
{
  int i ;

  if( e == 0 ) return 0 ;

  if( e->type == SPOC_LIST ) {
    for( i = 0, e = e->e.list->head ; i < n && e->next ; e = e->next, i++ ) ;
    return e ; 
  }
  else if( e->type == SPOC_SET || e->type == SPOC_ARRAY ) {
    return (element_t *) varr_nth( e->e.set, n ) ;
  }
  else if( n > 1 ) return 0 ;
  else return e ;
}

/* ---------------------------------------------------------------------- */
/* If start is negative then the starting point is counted from the end of
   the list (e, -2, 2 ), gives you the last two elements in the list */

element_t *element_perl_intervall( element_t *e, int start, int nr )
{
  element_t *re = 0, *ep, *next, *head ;
  int        i, n ;

  if( e == 0 ) return 0 ;

  /* only works on lists */
  if( e->type != SPOC_LIST ) return 0 ;

  for( n = 0, ep = e->e.list->head  ; ep ; ep = ep->next ) n++ ;

  if( start >= 0 ) {
    if( start+nr <= n ) {
      re = element_new() ;
      re->type = SPOC_LIST ;
      for( i = 0, ep = e->e.list->head ;  i < start ; ep = ep->next, i++ ) ;

      re->e.list = list_new() ;
      re->e.list->head = head = element_dup( ep, re ) ;      

      head->memberof = re ;
      for( i = 0, next = ep->next ; i < nr ; next = next->next, i++ ) {
        head->next = element_dup( next, re ) ;
        head = head->next ;
        head->memberof = re ;
      }  
    }
  }
  else {
    if( start + nr <= 0 && nr <= n ) {
      re = element_new() ;
      re->type = SPOC_LIST ;

      for( i = 0, ep = e->e.list->head ;  i < n + start ; ep = ep->next, i++ ) ;

      re->e.list = list_new() ;
      re->e.list->head = head = element_dup( ep, re ) ;      
      head->memberof = re ;

      for( i = 0, next = ep->next ; i < nr ; next = next->next, i++ ) {
        head->next = element_dup( next, re ) ;
        head = head->next ;
        head->memberof = re ;
      }  
    }
  }
  
  return re ;
}

/* ---------------------------------------------------------------------- */

element_t *element_intervall( element_t *e, int start, int end )
{
  element_t *re = 0, *ep, *next, *head ;
  int        i, n ;

  if( e == 0 ) return 0 ;

  /* only works on lists */
  if( e->type != SPOC_LIST ) return 0 ;

  /* The total number of elements in the list */
  for( n = 0, ep = e->e.list->head  ; ep ; ep = ep->next ) n++ ;

  /* -1 == last */
  if( start == -1 ) start = n - 1;
  if( end == -1 ) end = n - 1;

  if( start >= 0 ) {
    if( end == start ) {
      for( i = 0, ep = e->e.list->head ;  i < start ; ep = ep->next, i++ ) ;
      re = element_dup( ep, 0 ) ;      
    }
    else if( end < n ) {
      re = element_new() ;
      re->type = SPOC_LIST ;

      /* get the starting point */
      for( i = 0, ep = e->e.list->head ;  i < start ; ep = ep->next, i++ ) ;

      re->e.list = list_new() ;
      re->e.list->head = head = element_dup( ep, re ) ;      
      head->memberof = re ;
      for( next = ep->next ; i < end ; next = next->next, i++ ) {
        head->next = element_dup( next, re ) ;
        head = head->next ;
        head->memberof = re ;
      }  
    }
  }
  
  return re ;
}

/* ---------------------------------------------------------------------- */

int element_cmp( element_t *e0, element_t *e1 )
{
  int t, i, n, r = 0 ;

  if( e0 == 0 || e1 == 0 ) return -1 ;

  t = element_type( e0 ) ;
  i = element_type( e1 ) ;

  if( t != i ) return t - i ;

  switch( t ) {
    case SPOC_ATOM:
      r = octcmp( element_data( e0 ), element_data( e1 )) ;
      break ;

    case SPOC_LIST:
      n = element_size( e0 ) ;
      i = element_size( e1 ) ;

      if( n != i ) return n - i ;
      for( i = 0 ; i < n ; i++ ) {
        if(( r = element_cmp( element_nth( e0, i), element_nth( e1, i))) != 0 ) break ;
      }
      break ;

    case SPOC_SET:
      r = -1 ; 
      break ;

    case SPOC_ARRAY:
      r = -1 ; 
      break ;
  }

  return r ;
}

/* ---------------------------------------------------------------------- */

void element_reverse( element_t *e ) 
{
  if( e == 0 ) return ;

  /* the only thing that can be reversed, a set is a unordered set of element
     and can there for not be reversed */
  if( e->type == SPOC_LIST ) {
    element_t *head, *a, *b ;

    head = e->e.list->head ;

    a = head->next ;
    while( a ) {
      b = a->next ;

      /* new head */
      a->next = head ;
      head = a ;

      a = b ;
    }
    e->e.list->head = head ;
  }
}

/* ---------------------------------------------------------------------- 
void element_free( element_t *e ) defined in lib/free.c
* ---------------------------------------------------------------------- */

void *element_data( element_t *e ) 
{
  if( e == 0 ) return 0 ;

  switch( e->type ) {
    case SPOC_ATOM: 
    case SPOC_SUFFIX: 
    case SPOC_PREFIX: 
      return (void *) &e->e.atom->val ;
      break ;

    case SPOC_RANGE:
      return ( void * ) e->e.range ;
      break ;

    case SPOC_SET :
    case SPOC_ARRAY :
      return ( void * ) e->e.set;
      break ;

    case SPOC_LIST :
      return ( void *) e->e.list->head ;
      break ;

    default:
      return 0 ;
  }

  return 0 ; /* should never get here */
}

/* ---------------------------------------------------------------------- */

element_t *element_parent( element_t *e )
{
  if( e == 0 ) return 0 ;

  return e->memberof ;
}

/* ---------------------------------------------------------------------- */
/* allowed formats 
 [n], [-m], [n-m], [n-], ["last"], [-"last"], [n-"last"]
 */
char *parse_intervall( octet_t *o, int *start, int *end )
{
  char *sp ;
  int  n, i = 0 ;
  
  if(( n = octchr( o, ']' )) < 0 ) return 0 ;

  *start = *end = 0 ;

  sp = o->val ;
  if( *sp <= '9' && *sp >= '0' ) {
    *start = *sp - '0' ;
    sp++ ;
    i++ ;
    while( i < n && *sp <= '9' && *sp >= '0' ) {
      *start *= 10 ;
      *start += *sp - '0' ;
      i++ ;
    }
    if( i == n ) *end = *start ;
  }
  else if( strncmp( sp, "last", 4 ) == 0 ) {
    i += 4 ;
    if( i == n ) *start = *end = -1 ;
  }

  if( i < n ) {
    if( *sp != '-' ) return 0 ;
    i++ ;
    sp++ ;

    if( i == n ) *end = -1 ;
    else if( *sp <= '9' && *sp >= '0' ) {
      *end = *sp - '0' ;
      sp++ ;
      i++ ;
      while( i < n && *sp <= '9' && *sp >= '0' ) {
        *end *= 10 ;
        *end += *sp - '0' ;
        i++ ;
      }
    }
    else if( strncmp( sp, "last", 4 ) == 0 ) {
      i += 4 ;
      if( i == n ) *end = -1 ;
    }
  }

  if( i != n ) return 0 ;

  return o->val+n+1 ;
}

/* ---------------------------------------------------------------------- */

octarr_t *parse_path( octet_t *o )
{
  char         *sp, *np = 0;
  unsigned int i ;
  octarr_t     *oa = 0 ;
  octet_t      *oct = 0 ;

  for( sp = o->val, i = 0 ; ( TAGCHAR(*sp) || *sp == '/' ) && i < o->len ; sp++, i++ ) {
    if( *sp == '/' ) {
      if( oa == 0 ) {
        oa = octarr_new( 2 ) ;
        oct = oct_new( sp - o->val, o->val ) ;
        octarr_add( oa, oct ) ;
      }  
      else {
        oct = oct_new( sp - np, np ) ;
        octarr_add( oa, oct ) ;
      }
      np = sp+1 ;
    }  
  }

  if( np && !( *np == '*' && ( o->len - i == 1 ))) {
    oct = oct_new( sp - np, np ) ;
    octarr_add( oa, oct ) ;
  }

  o->val = sp ;
  o->len -= i ;
  
  return oa ;
}

/* ---------------------------------------------------------------------- */

/* allowed syntax
  /A/B/C - the list with tag 'C' within the list with tag 'B' within the list 
  with tag 'A'

  //D - The list/-s with the tag D in the tree

  /A/* all elements in that list except the tag == 1,...,last 

  ..A[n] The nth element, first == 0 

  ..A[n-m] elements n to m

  ..A[n,m,k] elements n,m and k

  //A | //B All list starting with either A or B

  //a[2] | /a/b/c/* is valid  
*/

element_t *element_eval( octet_t *spec, element_t *e, int *rc )
{
  element_t *re = 0, *ep = 0, *ie = 0, *sep = 0 ;
  element_t *pe = 0, *se = 0 ;
  char      *sp ;
  octet_t   tag, cpy ;
  octarr_t  *oa = 0 ;
  int        p, r, n = 0 ;

  octln( &cpy, spec ) ;
  do {
    sp = cpy.val;
    p = r = 0 ;
    pe = se = ie = 0 ;

    if( *sp == '/' ) {
      if( *(sp+1) != '/' ) { /* path */
        cpy.val++ ;
        cpy.len-- ;

        oa = parse_path( &cpy ) ;
        pe = element_traverse( e, oa, 0 ) ;
        octarr_free( oa ) ;

        if( pe == 0 ) { /* skip the rest of this part */
          goto NEXTpart;
        }
        r = 1 ;
      }

      if( cpy.len ) { 
        sp = cpy.val;
        if( *sp == '/' && *(sp+1) == '/' ) { /* search */
          if( pe == 0 ) ep = e ;
          else ep = pe ;

          sp += 2 ;
          cpy.val += 2 ;
          cpy.len -= 2 ;
          tag.val = sp ;
          for( tag.len = 0 ; TAGCHAR( *sp ) ; tag.len++, sp++ ) ;

          cpy.len -= tag.len ;
          cpy.val += tag.len ;

          se = element_find_list( ep, &tag ) ;
          if( se == 0 ) { /* skip the rest of this part */
            r = 0 ;
            goto NEXTpart;
          }
          if( varr_len( se->e.set )  == 1 ) { /* only one element, get rid of the set */
            ep = varr_pop( se->e.set ) ;
            element_free( se ) ;
            se = ep ;
          }
          r = 1 ;
        }
      }

      if( cpy.len ) {
        if( se == 0 ) {
          if( pe == 0 ) ep = e ;
          else ep = pe ;
        }
        else ep = se ;

        sp = cpy.val ;

        if( *sp == '[' ) { /* subset of the list */
          int b, e ;
 
          cpy.val++ ;
          cpy.len-- ;

          if(( sp = parse_intervall( &cpy, &b, &e )) == 0 ) {
            if( re ) element_free( re ) ;
            if( se ) element_free( se ) ;
            return 0 ;
          }
          cpy.len -= sp - cpy.val ;
          cpy.val = sp ;

          ie = element_intervall( ep, b, e ) ;
        }
        /* the whole list except the tag */
        else if( *sp == '*' || ( *sp == '/' && *(sp+1) == '*' )) { 
          ie = element_intervall( ep, 1, -1 ) ;
          cpy.val++ ;
          cpy.len-- ;
          if( *sp == '/' ) {
            cpy.val++ ;
            cpy.len-- ;
          }
        }

        if( ie == 0 ) {
          if( se ) {
            element_free( se ) ; 
            se = 0 ;
          }
          r = 0 ;
        }
        else r = 1 ;
      }
    }
    else {
      *rc = SPOCP_SYNTAXERROR ;
      return 0 ;
    }

NEXTpart:
    for( sp = cpy.val, p = 0 ; cpy.len && ( *sp == ' ' || *sp == '|' ) ; sp++ ) {
      if( *sp == '|' ) p++ ;
      cpy.val++ ;
      cpy.len-- ;
    }

    if( r ) {
      if( n ) {
        if( n == 1 ) {
          re = element_array_add( re, sep ) ;
          sep = 0 ;
        }

        if( ie ) re = element_array_add( re, ie ) ;
        else if( se) re = element_array_add( re, se ) ;
        else re = element_array_add( re, element_dup(pe, 0)) ;
      }
      else {
        if( ie ) sep = ie ;
        else if( se) sep = se ;
        else sep = element_dup(pe, 0) ;
      }
      n++ ;
    }

    ep = 0 ;
  } while( p && cpy.len ) ;

  if( sep ) return sep ;
  else      return re ;
}

/* ---------------------------------------------------------------------- */

/* takes a 'string' containing ${0}, ${1} and so on, where a substitution should 
   be made */

octet_t *element_atom_sub( octet_t *val, element_t *xp )
{
  int        subs = 0, n, p, i ;
  octet_t    oct, spec, *res = 0 ;
  element_t *vs ;
  char       tmp[256] ;

  /* no variable substitutions necessary */
  p = octstr( val, "${" ) ; 
  if( p < 0 ) return val ;

  /* If there is nothing to substitute with then it can't succeed */
  if( xp == 0 ) return 0 ;

  /* absolutely arbitrary */
  res = oct_new( val->len * 2, 0 ) ;

  oct.val = val->val ;
  oct.len = val->len ;
  oct.size = 0 ;

  while( oct.len ) {
    /* No more variable substitutions */
    if(( n = octstr( &oct, "${" )) < 0 ) {
      if( !subs ) return val ;
      else {
        if( octcat( res, oct.val, oct.len ) != SPOCP_SUCCESS ) {
          oct_free( res ) ;
          return 0 ;
        }
        return res ;
      }
    }

    if( n ) {
      if( octcat( res, oct.val, n ) != SPOCP_SUCCESS ) {
        oct_free( res ) ;
        return 0 ;
      }
      oct.val += n ;
      oct.len -= n ;
    }

    /* step past the ${ */
    oct.val += 2 ;
    oct.len -= 2 ;

   /* strange if no } is found */
    if(( n = oct_find_balancing( &oct, '{', '}' )) < 0 ) {
      oct_free( res ) ;
      return 0 ;
    }

    spec.val = oct.val ;
    spec.len = n ;

    if(( i = octtoi( &spec )) < 0 || ( vs = element_nth( xp, i )) == 0 ) {
      oct_free( res ) ;
      return 0 ;
    }

    /* allow only simple substitutions */
    if( element_type( vs ) != SPOC_ATOM  ) {
      oct_free( res ) ;
      return 0 ;
    }

    if( oct2strcpy( element_data( vs ), tmp, 256, 0, 0 ) < 0 ) {
      oct_free( res ) ;
      return 0 ;
    }

    octcat( res, tmp, strlen(tmp)) ;

    oct.val += n + 1 ;
    oct.len -= n + 1 ;

    subs++ ;
  }

  return res ;
}

