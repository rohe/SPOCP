/***************************************************************************
                          subelem.c  -  description
                             -------------------
    begin                : Thr May 15 2003
    copyright            : (C) 2003 by Umeå University, Sweden
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

/********************************************************************/

atom_t *atom_dup( atom_t *ap )
{
  atom_t *new = 0 ;

  if( ap == 0 ) return 0 ;

  new = atom_new( &ap->val ) ;

  return new ;
}

/********************************************************************/

list_t *list_dup( list_t *lp, element_t *set, element_t *memberof ) 
{
  list_t *new = 0 ;

  if( lp == 0 ) return 0 ;

  new = ( list_t * ) Malloc ( sizeof( list_t ) ) ;
  new->hstrlist = lp->hstrlist ;
  new->head = element_dup( lp->head, 0, 0 ) ;

  return new ;
}

/********************************************************************/

int boundary_cpy( boundary_t *dest, boundary_t *src )
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

range_t *range_dup( range_t *rp )
{
  range_t *new = 0 ;

  if( rp == 0 ) return 0 ;

  new = ( range_t * ) Malloc( sizeof( range_t )) ;

  boundary_cpy( &new->lower, &rp->lower ) ;
  boundary_cpy( &new->upper, &rp->upper ) ;

  return new ;
}

/********************************************************************/

set_t *set_dup( set_t *ap )
{
  set_t *new ;
  int   i ;

  new = set_new( ap->size ) ;
  new->n = ap->n ;

  for( i = 0 ; i < ap->n ; i++ )
    new->element[i] = element_dup( ap->element[i], 0, 0 ) ;
  
  return new ;
}

/********************************************************************/

element_t *element_dup( element_t *ep, element_t *set, element_t *memberof )
{
  element_t *new = 0 ;

  if( ep == 0 ) return 0 ;

  new = element_new() ;

  new->not = ep->not ;
  new->type = ep->type ;

  switch( ep->type ) {
    case SPOC_ATOM :
      new->e.atom = atom_dup( ep->e.atom ) ;
      break ;

    case SPOC_LIST :
      new->e.list = list_dup( ep->e.list, set, new ) ;
      break ;

    case SPOC_OR :
      new->e.set = set_dup( ep->e.set ) ;
      break ;

    case SPOC_PREFIX :
      new->e.prefix = atom_dup( ep->e.prefix ) ;
      break ;

    case SPOC_SUFFIX :
      new->e.suffix = atom_dup( ep->e.suffix ) ;
      break ;

    case SPOC_RANGE :
      new->e.range = range_dup( ep->e.range ) ;
      break ;

    case SPOC_REPEAT :
      break ;

  }

  return new ;
}

/********************************************************************/

subelem_t *subelem_new() 
{
  subelem_t *s ;

  s = ( subelem_t * ) Calloc ( 1, sizeof( subelem_t )) ;

  return s ;
}

/********************************************************************/

void subelem_free( subelem_t *sep )
{
  if( sep ) {
    if( sep->ep ) element_free( sep->ep ) ;
    if( sep->next ) subelem_free( sep->next ) ;
    free( sep ) ;
  }
}

/********************************************************************/

int to_subelements( octet_t *arg, octarr_t *oa )
{
  int     depth = 0 ;
  octet_t oct, *op, str ;

  /* first one should be a '(' */
  if( *arg->val == '(' ) {
    arg->val++ ;
    arg->len-- ;
  }
  else return -1 ;

  oct.val = arg->val ;
  oct.len = 0 ;

  while( arg->len ) {
    if( *arg->val == '(' ) {      /* start of list */
      depth++ ;
      if( depth == 1 ) {
        oct.val = arg->val ;
      }

      arg->len-- ;
      arg->val++ ;
    }
    else if( *arg->val == ')' ) { /* end of list */
      depth-- ;
      if( depth == 0 ) {
        oct.len = arg->val - oct.val + 1 ;
        op = octdup( &oct ) ; 
        octarr_add( oa, op ) ;
        oct.val = arg->val + 1 ;
      }
      else if( depth < 0 ) {
        arg->len-- ;
        arg->val++ ;
        break ;                   /* done */
      }

      arg->len-- ;
      arg->val++ ;
    }
    else {
      if( get_str( arg, &str ) != SPOCP_SUCCESS ) return -1 ;

      if( depth == 0 ) {
        oct.len = arg->val - oct.val ;
        op = octdup( &oct ) ; 
        octarr_add( oa, op ) ;
        oct.val = arg->val ;
        oct.len = 0 ;
      }
    }
  }

  return 1 ;
}

/********************************************************************/

subelem_t *octarr2subelem( octarr_t *oa, int dir )
{
  subelem_t *head = 0, *pres = 0, *tmp ;
  element_t *ep ;
  int       i ;

  for( i = 0 ; i < oa->n ; i++ ) {
    tmp = subelem_new() ;
    tmp->direction = dir ;

    if( *(oa->arr[i]->val) == '(' ) tmp->list = 1 ;
    else                            tmp->list = 0 ;

    if( element_get( oa->arr[i], &ep ) != SPOCP_SUCCESS ) {
      subelem_free( tmp ) ;
      subelem_free( head ) ;
      return 0 ;
    }
  
    tmp->ep = ep ;

    if( head == 0 ) head = pres = tmp ;
    else {
      pres->next = tmp ;
      pres = pres->next ;
    }
  }
  
  return head ;
}

/********************************************************************/

subelem_t *subelement_dup( subelem_t *se )
{
  subelem_t *new = 0 ;

  if( se == 0 ) return 0 ;

  new = subelem_new() ;

  new->direction = se->direction ;
  new->list = se->list;
  new->ep = element_dup( se->ep, 0, 0 ) ;

  if( se->next ) new->next = subelement_dup( se->next ) ;

  return new ;
}


