/***************************************************************************
                          free.c  -  description
                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <stdlib.h>

#include <struct.h>
#include <func.h>
#include <wrappers.h>

void ref_free( ref_t *ref )  ;
void junc_free( junc_t *juncp ) ;

void octet_free( octet_t *op ) ;
void boundary_free( boundary_t *bp ) ;
void bucket_free( buck_t *bp )  ;
void hashtable_free( phash_t *hp )  ;
void ssn_free( ssn_t *ssp ) ;
void limit_free( limit_t *lp ) ;
void branch_free( branch_t *bp ) ;

/* ======================================================== */

void octet_free( octet_t *op )
{
  if( op ) {
    if( op->val ) free( op->val ) ;
    free( op ) ;
  }
}

/* --------------------------------------------------------------- */

void atom_free( atom_t *ap )
{
  if( ap ) {
    /* ap->val.val is just a link so it shouldn't be freed */ 

    free( ap ) ;
  }
}

/* --------------------------------------------------------------- */

void range_free( range_t *rp )
{
  if( rp ) {
    switch( rp->lower.type ) {
      case SPOC_DATE:
      case SPOC_ALPHA:
        if( rp->lower.v.val.len ) free( rp->lower.v.val.val ) ;
        if( rp->upper.v.val.len ) free( rp->upper.v.val.val ) ;
        break ;

      case SPOC_TIME:
      case SPOC_NUMERIC:
      case SPOC_IPV4:
      case SPOC_IPV6:
        break ;
    }

    free( rp ) ;
  }
}

/* --------------------------------------------------------------- */

void list_free( list_t *lp )
{
  element_t *ep, *nep ;

  if( lp ) {
    for( ep = lp->head ; ep ; ep = nep ) {
      nep = ep->next ;
      element_free( ep ) ;
    }

    free(lp) ;
  }
}

/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */

void set_free( set_t *and, int flag ) 
{
  int i ;

  if( and ) {
    if( and->n && flag ) {
      for( i = 0 ; i < and->n ; i++ )
        if( and->element[i] ) element_free( and->element[i] ) ;
    }

    free( and ) ;
  }
}

/* --------------------------------------------------------------- */

void element_free( element_t *ep )
{
  if( ep ) {
    switch( ep->type ) {
      case SPOC_ATOM:
        atom_free( ep->e.atom ) ;
        break ;

      case SPOC_LIST:
        list_free( ep->e.list ) ;
        break ;

      case SPOC_OR:
        set_free( ep->e.set, 1 ) ;
        break ;

      case SPOC_PREFIX:
        atom_free( ep->e.prefix ) ;
        break ;

      case SPOC_SUFFIX:
        atom_free( ep->e.suffix ) ;
        break ;

      case SPOC_RANGE:
        range_free( ep->e.range ) ;
        break ;

    }

    free( ep ) ;
  }
}

/* --------------------------------------------------------------- */

void boundary_free( boundary_t *bp )
{
  if( bp ) {

    switch( bp->type & 0x07 ) {
      case SPOC_ALPHA :
      case SPOC_DATE :
        if( bp->v.val.len ) free( bp->v.val.val ) ;
        break ;

      case SPOC_NUMERIC :
      case SPOC_TIME :
      case SPOC_IPV4 :
      case SPOC_IPV6 :
        break ;
    }

    free( bp ) ;
  }
}

/* --------------------------------------------------------------- */

void ref_free( ref_t *ref ) 
{
  if( ref ) {
    if( ref->bc.len ) free( ref->bc.val ) ;
    if( ref->cachedval ) ll_free( ref->cachedval ) ;
    if( ref->next ) junc_free( ref->next ) ;

    free( ref ) ;
  }
}

/* --------------------------------------------------------------- */

void branch_free( branch_t *bp )
{
  int i ;

  if( bp ) {
    switch( bp->type ) {
      case SPOC_BCOND :
        erset_free( bp->val.set ) ;
        break ;

      case SPOC_ATOM:
        phash_free( bp->val.atom ) ;
        break ;

      case SPOC_LIST:
        junc_free( bp->val.list ) ;
        break ;

      case SPOC_PREFIX:
        ssn_free( bp->val.prefix ) ;
        break ;

      case SPOC_RANGE:
        for( i = 0 ; i < DATATYPES ; i++ ) sl_free_slist( bp->val.range[i] ) ;
        break ;

    }
    free( bp ) ;
  }
}

void junc_free( junc_t *juncp )
{
  int i ;
  
  if( juncp ) {
    for( i = 0 ; i < NTYPES ; i++ ) {
      if( juncp->item[i] ) branch_free( juncp->item[i] ) ;
    } 
    free( juncp ) ;
  }
}
