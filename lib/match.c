/***************************************************************************
                          match.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>
#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include <struct.h>
#include <func.h>
#include <spocp.h>
#include <macros.h>
#include <wrappers.h>

static junc_t *do_bcond( branch_t *, element_t *, parr_t *, becpool_t *, spocp_result_t *, octnode_t ** );

typedef struct _joct {
  junc_t *jp ;
  octnode_t *on ;
} joct_t ;

/* =============================================================== */

joct_t *joct_new( junc_t *jp, octnode_t *on )
{
  joct_t *jo ;

  jo = ( joct_t * ) Malloc ( sizeof( joct_t )) ;
  jo->jp = jp ;
  jo->on = on ;
 
  return jo ;
}

void joct_free( joct_t *jo )
{
  if( jo ) {
    /* don't free the junc_t pointer */
    octnode_free( jo->on ) ;
    free( jo ) ;
  }
}

int joct_cmp( joct_t *a, joct_t *b ) 
{
  if( a ) {
    if( b ) {
      if( a->jp == b->jp ) return octnode_cmp( a->on, b->on ) ;
      else return a->jp - b->jp ; 
    }
    else return 1 ;
  }
  else if( b ) return -1 ;
  else return 0 ;
}

int P_joctcmp( void *a, void *b )
{
  return joct_cmp(( joct_t *) a, ( joct_t * ) b ) ;
}

/* =============================================================== */
/*
  This routines tries to match a S-expression, broken down into
  it's parts as a linked structure of elements, with a tree.
  Important is to realize that at every branching point in the tree
  there might easily be more than one branch that originates there.
  Every branch at a branching point is typed (atom, prefix, suffix, range,
  boundary condition, list, endOfList, endOfRule) and they are
  therefore checked in a preferred order (fastest->slowest).
  Each branch is in turn checked all the way down to a leaf(endOFRule)
  is found or until no sub branch matches.
 */

/* =============================================================== */


junc_t *atom2atom_match( atom_t *ap, phash_t *pp )
{
  buck_t *bp ;

  DEBUG( SPOCP_DMATCH )
  { 
    traceLog( "<<<<<<<<<<<<<<<<<<<" ) ;
    phash_print( pp ) ;
    traceLog( ">>>>>>>>>>>>>>>>>>>" ) ;
  }

  bp = phash_search( pp, ap, ap->hash ) ;

  if( bp ) return bp->next ;
  else return 0 ;
}

parr_t *atom2prefix_match( atom_t *ap, ssn_t *pp )
{
  parr_t *avp = 0;

  DEBUG( SPOCP_DMATCH )
  { 
    traceLog( "----%s*-----", ap->val.val ) ;
  }

  avp = ssn_match( pp, ap->val.val, FORWARD ) ;

  return avp ;
}

parr_t *atom2suffix_match( atom_t *ap, ssn_t *pp )
{
  parr_t *avp = 0;

  avp = ssn_match( pp, ap->val.val, BACKWARD ) ;

  return avp ;
}

parr_t *atom2range_match( atom_t *ap, slist_t *slp, int vtype, spocp_result_t *rc )
{
  boundary_t  value ;
  octet_t     *vo ;
  int         r = 1 ;

  value.type = vtype | GLE ;
  vo = &value.v.val ;

  DEBUG( SPOCP_DMATCH ) traceLog( "-ATOM->RANGE: %s", ap->val.val ) ;

  /* make certain that the atom is of the right type */
  switch( vtype ) {
    case SPOC_DATE:
      if( is_date( &ap->val ) == SPOCP_SUCCESS) {
        vo->val = ap->val.val ;
        vo->len = ap->val.len - 1 ; /* without the Z at the end */
      }
      else r = 0 ;
      break ;

    case SPOC_TIME:
      if( is_time( &ap->val ) == SPOCP_SUCCESS)
        hms2int( &ap->val, &value.v.num ) ;
      else r = 0 ;
      break ;

    case SPOC_NUMERIC:
      if( is_numeric( &ap->val, &value.v.num) != SPOCP_SUCCESS ) r = 0 ;
      break ;

    case SPOC_IPV4:
      if( is_ipv4( &ap->val, &value.v.v4) != SPOCP_SUCCESS ) r = 0 ;
      break ;

    case SPOC_IPV6:
      if( is_ipv6( &ap->val, &value.v.v6) != SPOCP_SUCCESS ) r = 0 ;
      break ;

    case SPOC_ALPHA:
      vo->val = ap->val.val ;
      vo->len = ap->val.len ;
      break ;
  }

  if( !r ) {
    *rc = SPOCP_SYNTAXERROR ;
    DEBUG( SPOCP_DMATCH ) traceLog( "Wrong format [%s]", ap->val.val ) ;
    return 0 ;
  }
  else {
    DEBUG( SPOCP_DMATCH ) { traceLog( "skiplist match" ) ; }

    return sl_match( slp, &value ) ; 
  }
}

parr_t *prefix2prefix_match( atom_t *prefa, ssn_t *prefix )
{
  return atom2prefix_match( prefa, prefix ) ;
}

parr_t *suffix2suffix_match( atom_t *prefa, ssn_t *suffix )
{
  return atom2suffix_match( prefa, suffix ) ;
}

parr_t *range2range_match( range_t *ra, slist_t *slp )
{
  return sl_range_match( slp, ra ) ;
}

element_t *skip_list_tails( element_t *ep )
{
  for( ep = ep->memberof ; ep && ep->next == 0 ; ep = ep->memberof ) ;

  if( ep ) return ep->next ;
  else return 0 ;
}

/*

This is the matrix over comparisions, that I'm using

X = makes sense
O = can make sense in very special cases, should be
    handled while storing rules. Which means I don't
    care about them here

        | atom | prefix | range | set | list | suffix |
--------+------+--------+-------+-----+------|--------|
atom    |  X   |   X    |   X   |  X  |      |   X    |
--------+------+--------+-------+-----+------|--------|
prefix  |      |   X    |       |     |      |        |
--------|------+--------+-------+-----+------|--------|
range   |  0   |        |   X   |  0  |      |        |
--------+------+--------+-------+-----+------|--------|
set     |  0   |   0    |   0   |  X  |  0   |   0    |
--------+------+--------+-------+-----+------|--------|
list    |      |        |       |     |  X   |        |
---------------------------------------------+--------|
suffix  |      |        |       |     |      |   X    |
---------------------------------------------+--------|

 */

static junc_t * ending(
  junc_t *jp,
  element_t *ep,
  parr_t *gjp,
  becpool_t *bcp,
  spocp_result_t *rc,
  octnode_t **on )
{
  branch_t  *bp ;
  element_t *nep ;
  junc_t    *vl = 0 ;

  if( !jp ) return 0 ;

  if( jp->item[ SPOC_ENDOFRULE ] ) return jp ;

  if( jp->item[ SPOC_ENDOFLIST ] ) {

    bp = jp->item[SPOC_ENDOFLIST] ;

    if( ep ) {
      /* better safe then sorry, this should not be necessary  FIX */
      if( ep->next == 0 && ep->memberof == 0 ) {
        vl = bp->val.list ;

        if( vl && vl->item[SPOC_ENDOFRULE] ) return vl ;
        else return 0 ;
      }

      nep = ep->memberof ;

      DEBUG( SPOCP_DMATCH ) {
        if( nep->type == SPOC_LIST ) {
          char *tmp ;
          tmp = oct2strdup( &nep->e.list->head->e.atom->val, '\\' ) ;
          traceLog("found end of list [%s]", tmp ) ; 
          free( tmp ) ;
        }
      }

      vl = bp->val.list ;

      while( nep->memberof ) {

        if( vl->item[SPOC_BCOND] &&
            (jp = do_bcond( vl->item[SPOC_BCOND], nep, gjp, bcp, rc, on )) != 0 )
            return jp ;

        if( vl->item[SPOC_ENDOFLIST] ) 
          bp = vl->item[SPOC_ENDOFLIST] ;
        else
          break ;

        nep = nep->memberof ;

        DEBUG( SPOCP_DMATCH ) {
          if( nep->type == SPOC_LIST ) {
            char *tmp ;
            tmp = oct2strdup( &nep->e.list->head->e.atom->val, '\\' ) ;
            traceLog("found end of list [%s]", tmp ) ; 
            free( tmp ) ;
          }
        }

        vl = bp->val.list ;
      }

      if( nep->next ) {
        jp = element_match_r( vl, nep->next, gjp, bcp, rc, on ) ;
        if( jp && jp->item[SPOC_ENDOFRULE] ) return jp ;
      }
      else if( vl->item[SPOC_ENDOFRULE] ) return vl ;
    }
    else {
      jp = bp->val.list ;
      if( jp->item[SPOC_ENDOFRULE] ) return jp ;
    }
  }
  else if( jp->item[SPOC_ENDOFRULE] ) return jp ;

  return 0 ;
}

/*
void *extref_cmp( extref_t *erp, element_t *ep, parr_t *pap, becpool_t *bcp )
{
  ref_t  *rp ;
  junc_t *jp = 0 ;

#ifdef HAVE_LIBPTHREAD
  pthread_mutex_lock( &erp->mutex ) ;
#endif

  * comparing two boundary conditions *
  for( rp = erp->refl ; rp ; rp = rp->nextref ) {
    jp = 0 ;
    if( octcmp( &ep->e.atom->val, &rp->bc ) == 0 ) {

      jp = element_match_r( rp->next, ep, pap, bcp ) ;
      if( jp && jp->item[SPOC_ENDOFRULE] ) return (void *) jp ;

      break ;
    }
  }

#ifdef HAVE_LIBPTHREAD
  pthread_mutex_unlock( &erp->mutex ) ;
#endif

  return 0 ;
}
*/

static void *erset_skip(
  erset_t *erp,
  element_t *ep,
  parr_t *pap,
  becpool_t *bcp,
  spocp_result_t *rc,
  octnode_t **on )
{
  junc_t  *jp ;

  for( ; erp ; erp = erp->other) {
    jp = element_match_r( erp->next, ep, pap, bcp, rc, on ) ;
    if( jp && jp->item[SPOC_ENDOFRULE] ) return (void *) jp ;
  }

  return 0 ;
}


static junc_t * do_erset(
  erset_t *es,
  element_t *ep,
  parr_t *pap,
  becpool_t *bcp,
  spocp_result_t *rc,
  octnode_t **on) 
{
  size_t     i ;
  junc_t    *jp ;
  octet_t    blob ;
  
  for( i = 0 ; i < es->n ; i++ ) {
    memset( &blob, 0, sizeof( octet_t )) ;
    if( do_ref( es->ref[i], pap, bcp, &blob ) != TRUE ) {
      if( *on ) {
        octnode_free( *on ) ;
        *on = 0 ;
      }
      return 0 ;
    }
    *on = octnode_add( *on, &blob ) ;
  }
  
  DEBUG( SPOCP_DBCOND ) traceLog("External references evaluated to TRUE") ;

  jp = element_match_r( es->next, ep, pap, bcp, rc, on ) ;

  if( jp && jp->item[SPOC_ENDOFRULE] ) return jp ;
  else return 0 ;
}

static parr_t * do_er(
  erset_t *es,
  element_t *ep,
  parr_t *pap,
  becpool_t *bcp,
  spocp_result_t *rc)
{
  junc_t    *jp ;
  octnode_t *on ;
  parr_t    *parr = 0 ;
  joct_t    *jo ;

  for( ; es ; es = es->other ) {
    on = 0 ;
    if(( jp = do_erset( es, ep, pap, bcp, rc, &on )) != 0 ) {
      if( parr == 0 ) parr = parr_new( 4, &P_pcmp, 0, &P_pcpy, 0 ) ;
      jo = joct_new( jp, on ) ;
      parr_add( parr, (void *) jo ) ;
      break ;
    }
  }

  return parr ;
}

/*****************************************************************/

static junc_t *do_branches(
  parr_t *avp,
  element_t *ep,
  parr_t *pap,
  becpool_t *bcp,
  spocp_result_t *rc,
  octnode_t **on )
{
  int     i ;
  junc_t *jp = 0, *ea = 0 ;
  joct_t *jo ;

  /* DEBUG( SPOCP_DMATCH ) traceLog( "do_branches: %d", parr_items( avp )) ; */

  for( i = parr_items( avp ) - 1 ; i >= 0 ; i-- ) {
    jo = ( joct_t * ) parr_get_item_by_index( avp, i) ;
    if(( ea = ending( jo->jp, ep, pap, bcp, rc, &jo->on ))) break ;
  }

  /* DEBUG( SPOCP_DMATCH ) traceLog( "do_branches returned %d", i ) ; */
  if( i < 0 ) jp = 0 ;
  else {
    jp = ea ;
    if( jo->on ) {
      *on = octnode_join( *on, jo->on ) ;
      jo->on = 0 ; /* so it want get accidentally removed */
    }
  }

  /* clean up */
  parr_free( avp ) ;

  return jp ;
}

/*****************************************************************/

static junc_t * do_bcond(
  branch_t *bp,
  element_t *ep,
  parr_t *pap,
  becpool_t *bcp,
  spocp_result_t *rc,
  octnode_t **on )
{
  erset_t  *es ;
  parr_t   *avp ;
  junc_t   *jp = 0 ;

  if( bcp == 0 ) {
    jp = erset_skip( bp->val.set, ep, pap, bcp, rc, on ) ;
  }
  else if( ep && ep->type == SPOC_BCOND ) {
    DEBUG( SPOCP_DMATCH ) traceLog( "extref_cmp") ;
    if(( es = er_cmp( bp->val.set, ep )) != 0 ) jp = es->next ;
  }
  else {
    DEBUG( SPOCP_DMATCH ) traceLog( "do_er") ;
    if(( avp = do_er( bp->val.set, ep, pap, bcp, rc ))) { 
      jp = do_branches( avp, ep, pap, bcp, rc, on ) ;
    }
  }

  return jp ;
}

/*****************************************************************/
static parr_t *do_arr(
  parr_t *a,
  parr_t *p,
  element_t *e,
  parr_t *pap,
  becpool_t *bcp,
  spocp_result_t *rc)
{
  junc_t    *jp ;
  joct_t    *jo ;
  int        j ;
  octnode_t *on ;

  for( j = 0 ; j < a->n ; j++ ) {
    jp = a->vect[j] ;

    jp = element_match_r( jp, e, pap, bcp, rc, &on ) ;

    if( jp ) {
      /* Storing pointers in a array, don't want them delete when the array is deleted */
      if( p == 0 ) p = parr_new( 4, &P_joctcmp, 0, 0, 0 ) ;

      jo = joct_new( jp, on ) ;
      parr_add_nondup( p, (void *) jo ) ;
    }
  }

  parr_free( a ) ;

  return p ;
}
/*****************************************************************/

static parr_t *
atom_match(
  junc_t *db,
  element_t *ep,
  parr_t *pap,
  becpool_t *bcp,
  spocp_result_t *rc )
{
  branch_t  *bp ;
  junc_t    *jp = 0, *ju ;
  parr_t    *parr = 0, *avp = 0 ;
  element_t *nep ;
  slist_t  **slp ;
  int        i ;
  atom_t    *atom ;
  octnode_t *on = 0 ;
  joct_t    *jo ;

  if( ep == 0 ) return 0 ;

  atom = ep->e.atom ;
  nep = ep->next ;

  if(( bp = ARRFIND( db, SPOC_ATOM )) != 0 ) {
    if(( ju = atom2atom_match( atom, bp->val.atom ))) {
      DEBUG( SPOCP_DMATCH ) traceLog("Matched atom %s", atom->val.val ) ;

      jp = element_match_r( ju, nep, pap, bcp, rc, &on ) ;

      if( jp ) {
        if( parr == 0 ) parr = parr_new( 4, &P_joctcmp, 0, 0, 0 ) ;
        jo = joct_new( jp, on ) ;
        parr_add( parr, (void *) jo ) ;
      }

      /* Is this really necessary */
      if( nep == 0 ) {
        on = 0 ;
        if(( jp = ending( ju, ep, pap, bcp, rc, &on )) != 0 ) {
          if( parr == 0 ) parr = parr_new( 4, &P_joctcmp, 0, 0, 0 ) ;
          jo = joct_new( jp, on ) ;
          parr_add( parr, (void *) jo ) ;
        }
      }
    }
    else {
      DEBUG( SPOCP_DMATCH ) traceLog("Failed to matched atom %s", atom->val.val ) ;
    }
  }

  if(( bp = ARRFIND( db, SPOC_PREFIX )) != 0 ) {

    avp = atom2prefix_match( atom, bp->val.prefix ) ;

    if( avp ) {
      DEBUG( SPOCP_DMATCH ) traceLog("matched prefix" ) ;
      parr = do_arr( avp, parr, nep, pap, bcp, rc ) ;
    }
  }

  if(( bp = ARRFIND( db, SPOC_SUFFIX )) != 0 ) {

    avp = atom2suffix_match( atom, bp->val.suffix ) ;

    if( avp ) {
      DEBUG( SPOCP_DMATCH ) traceLog("matched suffix" ) ;
      parr = do_arr( avp, parr, nep, pap, bcp, rc ) ;
    }
  }

  if(( bp = ARRFIND( db, SPOC_RANGE )) != 0 ) {
    /* how do I get to know the type ?
       I don't so I have to test everyone */

    slp = bp->val.range ;

    for( i = 0 ; i < DATATYPES ; i++ ) {
      if( slp[i] ) {
        avp = atom2range_match( atom, slp[i], i, rc ) ;
        if( avp ) parr = do_arr( avp, parr, nep, pap, bcp, rc ) ;
      }
    }
  }

  return parr ;
}

/*****************************************************************/
/* recursive matching of nodes */

junc_t * element_match_r(
  junc_t *db,
  element_t *ep,
  parr_t *pap,
  becpool_t *bcp,
  spocp_result_t *rc,
  octnode_t **on )
{
  branch_t    *bp ;
  junc_t      *jp = 0 ;
  parr_t      *avp = 0 ;
  int          i ;
  slist_t    **slp ;
  set_t       *set ;
  octnode_t   *non = 0 ;

  if( db == 0 ) return 0 ;

  /* DEBUG( SPOCP_DMATCH ) traceLog( "element_match_r") ; */
  /* may have several roads to follow, this might just be one of them */
  if(( jp = ending( db, ep, pap, bcp, rc, &non ))) {
    *on = octnode_join( *on, non ) ;
    return jp ;
  }
  else if( non ) {
    octnode_free( non ) ;
    non = 0 ;
  }

  /* boundary conditions must be evaluated when they appear. 
     The absence of a be cpool means don't check boundary conditions just skip past  */
  if(( bp = ARRFIND(db,SPOC_BCOND))) {
    if(( jp = do_bcond( bp, ep, pap, bcp, rc, &non )) != 0 ) {
      *on = octnode_join( *on, non ) ;
      return jp ;
    }
    else if( non ) {
      octnode_free( non ) ;
      non = 0 ;
    }
  }

  if( ep == 0 ) { 
    DEBUG( SPOCP_DMATCH ) traceLog("No more elements in input") ;
    return jp ;
  }

  switch( ep->type ) {
    case SPOC_ATOM:
      DEBUG( SPOCP_DMATCH ) traceLog( "Checking ATOM" ) ;
      if(( avp = atom_match( db, ep, pap, bcp, rc ))) {
        jp = do_branches( avp, ep, pap, bcp, rc, &non ) ;
      }
      break ;

    case SPOC_OR:
      /* only one item in the set has to match */
      set = ep->e.set ;

      for( i = 0 ; i < set->n ; i++  ) {
        jp = element_match_r( db, set->element[i] , pap, bcp, rc, &non ) ;
        if( jp && ( ARRFIND( jp, SPOC_ENDOFRULE))) break ;
      }

      break ;

    case SPOC_PREFIX:
      if(( bp = ARRFIND( db, SPOC_PREFIX )) != 0 ) {
        avp = prefix2prefix_match( ep->e.prefix, bp->val.prefix ) ;

        /* got a set of plausible branches and more elements*/
        if( avp ) {
          jp = do_branches( avp, ep, pap, bcp, rc, &non ) ;
        }
        else jp = 0 ;
      }
      break ;

    case SPOC_SUFFIX:
      if(( bp = ARRFIND( db, SPOC_SUFFIX )) != 0 ) {
        avp = suffix2suffix_match( ep->e.suffix, bp->val.suffix ) ;

        /* got a set of plausible branches and more elements*/
        if( avp ) {
          jp = do_branches( avp, ep, pap, bcp, rc, &non ) ;
        }
        else jp = 0 ;
      }
      break ;

    case SPOC_RANGE:
      if(( bp = ARRFIND( db, SPOC_RANGE )) != 0 ) {
        slp = bp->val.range ;

        i = ep->e.range->lower.type & 0x07 ;

        if( slp[i] ) /* no use testing otherwise */
          avp = range2range_match( ep->e.range, slp[i] ) ;

        /* got a set of plausible branches */
        if( avp ) {
          jp = do_branches( avp, ep, pap, bcp, rc, &non ) ;
        }
        else jp = 0 ;
      }
      break ;

    case SPOC_LIST:
      DEBUG( SPOCP_DMATCH ) traceLog( "Checking LIST" ) ;
      if(( bp = ARRFIND( db, SPOC_LIST )) != 0 ) {
        jp = element_match_r( bp->val.list, ep->e.list->head, pap, bcp, rc, &non ) ;
      }
      break ;

    case SPOC_ENDOFLIST:
      return bp->val.list ;
  }

  if( jp == 0 && non ) octnode_free( non ) ;
  else if( jp != 0 && non ) *on = octnode_join( *on, non ) ;

  return jp ;
}

