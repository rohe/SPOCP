/***************************************************************************
                          list.c  -  description
                             -------------------
    begin                : Tue Nov 05 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

/* Implements the ADMIN LIST command */

#include <stdlib.h>
#include <string.h>

#include <db0.h>
#include <struct.h>
#include <func.h>
#include <proto.h>
#include <spocp.h>
#include <wrappers.h>
#include <macros.h>

parr_t *get_rec_all_rules( junc_t *jp, parr_t *in ) ;
parr_t *subelem_lte_match( junc_t *, element_t *, parr_t *, parr_t *, becpool_t * ) ;
parr_t *subelem_match_lte( junc_t *, element_t *, parr_t *, parr_t *, becpool_t *, spocp_result_t * ) ;

/********************************************************************/

parr_t *get_all_bcond_followers( branch_t *bp, parr_t *pa )
{
  erset_t *bcond = bp->val.set ;
  
  for( ; bcond ; bcond = bcond->other )
    pa = parr_add( pa, bcond->next ) ;

  return pa ;
}

/********************************************************************/

parr_t *get_rule_indexes( parr_t *pa, parr_t *in )
{
  int i ;

  for( i = 0 ; i < pa->n ; i++ ) 
    in = get_rec_all_rules( pa->vect[i], in ) ;

  parr_free( pa ) ;
  return in ;
}

/****************************************************************************/

void junc_print( int lev, junc_t *jp )
{
  traceLog("|---------------------------->") ;
  if( jp->item[0] ) traceLog( "Junction[%d]: ATOM", lev ) ;
  if( jp->item[1] ) traceLog( "Junction[%d]: LIST", lev ) ;
  if( jp->item[2] ) traceLog( "Junction[%d]: SET", lev ) ;
  if( jp->item[3] ) traceLog( "Junction[%d]: PREFIX", lev ) ;
  if( jp->item[4] ) traceLog( "Junction[%d]: SUFFIX", lev ) ;
  if( jp->item[5] ) traceLog( "Junction[%d]: RANGE", lev ) ;
  if( jp->item[6] ) traceLog( "Junction[%d]: ENDOFLIST", lev ) ;
  if( jp->item[7] ) traceLog( "Junction[%d]: ENDOFRULE", lev ) ;
  if( jp->item[8] ) traceLog( "Junction[%d]: ExTREF", lev ) ;
  if( jp->item[9] ) traceLog( "Junction[%d]: ANY", lev ) ;
  if( jp->item[10] ) traceLog( "Junction[%d]: REPEAT", lev ) ;
  if( jp->item[11] ) traceLog( "Junction[%d]: REGEXP", lev ) ;
  traceLog(">----------------------------|") ;
}

/*************************************************************************/
/* Match rule elements where the subelement is LTE */
/********************************************************************/
/* get recursively all rules                                        */
/********************************************************************/

parr_t *get_rec_all_rules( junc_t *jp, parr_t *in )
{
  parr_t  *pa  = 0 ;
  index_t *id ;
  int     i ;

  if( jp->item[SPOC_ENDOFRULE] ) {
    id = jp->item[SPOC_ENDOFRULE]->val.id ;
    for( i = 0 ; i < id->n ; i++ )
      in = parr_add( in, id->arr[i] ) ;
  }

  if( jp->item[SPOC_LIST] ) 
    in = get_rec_all_rules( jp->item[SPOC_LIST]->val.list, in ) ;

  if( jp->item[SPOC_ENDOFLIST] ) 
    in = get_rec_all_rules( jp->item[SPOC_ENDOFLIST]->val.list, in ) ;
  /* --- */

  if( jp->item[SPOC_ATOM] ) 
    pa = get_all_atom_followers( jp->item[SPOC_ATOM], pa ) ;

  if( jp->item[SPOC_PREFIX] ) 
    pa = get_all_ssn_followers( jp->item[SPOC_PREFIX], SPOC_PREFIX, pa ) ;

  if( jp->item[SPOC_SUFFIX] ) 
    pa = get_all_ssn_followers( jp->item[SPOC_SUFFIX], SPOC_SUFFIX, pa ) ;

  if( jp->item[SPOC_RANGE] ) 
    pa = get_all_range_followers( jp->item[SPOC_RANGE], pa ) ;

  if( jp->item[SPOC_BCOND] ) 
    pa = get_all_bcond_followers( jp->item[SPOC_BCOND], pa ) ;
 
  /* --- */

  if( pa ) in = get_rule_indexes( pa, in ) ;

  return in ;
}

/*************************************************************************/

parr_t *get_all_to_next_listend( junc_t *jp, parr_t *in, int lev )
{
  int      i ; 
  parr_t  *pa = 0 ;
  junc_t  *ju ;

  DEBUG( SPOCP_DMATCH ) junc_print( lev, jp ) ;

  if( jp->item[SPOC_ENDOFRULE] ) ; /* shouldn't get here */

  if( jp->item[SPOC_LIST] ) 
    in = get_all_to_next_listend( jp->item[SPOC_LIST]->val.list, in, lev+1 ) ;

  if( jp->item[SPOC_ENDOFLIST] ) {
    ju = jp->item[SPOC_ENDOFLIST]->val.list ;
    lev-- ; 
    if( lev >= 0 ) pa = parr_add( pa, ju) ;
    else in = parr_add( in, ju ) ;
  }

  /* --- */

  if( jp->item[SPOC_ATOM] ) 
    pa = get_all_atom_followers( jp->item[SPOC_ATOM], pa ) ;

  if( jp->item[SPOC_PREFIX] ) 
    pa = get_all_ssn_followers( jp->item[SPOC_PREFIX], SPOC_PREFIX, pa ) ;

  if( jp->item[SPOC_SUFFIX] ) 
    pa = get_all_ssn_followers( jp->item[SPOC_SUFFIX], SPOC_SUFFIX, pa ) ;

  if( jp->item[SPOC_RANGE] ) 
    pa = get_all_range_followers( jp->item[SPOC_RANGE], pa ) ;

  if( jp->item[SPOC_BCOND] ) 
    pa = get_all_bcond_followers( jp->item[SPOC_BCOND], pa ) ;
 
  /* --- */

  if( pa ) {
    for( i = 0 ; i < pa->n ; i++ ) 
      in = get_all_to_next_listend( pa->vect[i], in, lev ) ;
  }

  return in ;
}

/*************************************************************************/

parr_t *range2range_match_lte(range_t *rp, slist_t **sarr, parr_t *pap ) 
{
  int dtype = rp->lower.type & 0x07 ;

  pap = sl_range_lte_match( sarr[dtype], rp, pap ) ;

  return pap ;
}

/*************************************************************************/

parr_t *prefix2prefix_match_lte( atom_t *ap, ssn_t *pssn, parr_t *pap )
{
  return ssn_lte_match( pssn, ap->val.val, FORWARD, pap ) ;
}

/*************************************************************************/

parr_t *suffix2suffix_match_lte( atom_t *ap, ssn_t *pssn, parr_t *pap )
{
  return ssn_lte_match( pssn, ap->val.val, BACKWARD, pap ) ;
}

/*************************************************************************/

parr_t  *
subelem_lte_match( junc_t *jp, element_t *ep, parr_t *pap, parr_t *id, becpool_t *bcp ) 
{
  branch_t *bp ;
  junc_t   *np ;
  parr_t   *nap, *arr ;
  int       i ;
  set_t    *or ;

  if(( bp = ARRFIND( jp, SPOC_BCOND))) 
    pap = erset_match( bp->val.set, ep, pap, bcp ) ;

  switch( ep->type ) {
    case SPOC_ATOM: /* Can only be other atoms */
      if(( bp = ARRFIND( jp, SPOC_ATOM)) != 0 ) {
        np = atom2atom_match( ep->e.atom, bp->val.atom ) ;
        if( np ) pap = parr_add( pap, np ) ;
      } 
      break ;

    case SPOC_PREFIX: /* prefixes or atoms */
      if(( bp = ARRFIND( jp, SPOC_ATOM)) != 0 ) {
        pap = prefix2atoms_match(ep->e.prefix->val.val, bp->val.atom, pap) ;
      } 

      if(( bp = ARRFIND( jp, SPOC_PREFIX)) != 0 ) {
        pap = prefix2prefix_match_lte(ep->e.prefix, bp->val.prefix, pap ) ;
      }
      break ;

    case SPOC_SUFFIX: /* suffixes or atoms */
      if(( bp = ARRFIND( jp, SPOC_ATOM)) != 0 ) {
        pap = suffix2atoms_match(ep->e.suffix->val.val, bp->val.atom, pap) ;
      } 

      if(( bp = ARRFIND( jp, SPOC_SUFFIX)) != 0 ) {
        pap = suffix2suffix_match_lte(ep->e.suffix, bp->val.suffix, pap ) ;
      }
      break ;

    case SPOC_RANGE : /* ranges or atoms */
      if(( bp = ARRFIND( jp, SPOC_ATOM)) != 0 ) {
        pap = range2atoms_match(ep->e.range, bp->val.atom, pap) ;
      } 

      if(( bp = ARRFIND( jp, SPOC_RANGE)) != 0 ) {
        pap = range2range_match_lte(ep->e.range, bp->val.range, pap ) ;
      }
      break ;

    case SPOC_OR :
      nap = 0 ;
      or = ep->e.set ;

      for( i = 0 ; i < or->n ; i++ ) {
        /* Since this is in effect parallell path, I have to do it over
           and over again */
        nap = subelem_lte_match( jp, or->element[i],  nap, id, bcp ) ;
      }
      pap = parr_join( pap, nap ) ;
      break ;

    case SPOC_LIST : /* other lists */
      arr = parr_add( 0, jp ) ;

      for( ep = ep->e.list->head ; ep ; ep = ep->next ) {
        nap = 0 ;
        for( i = 0 ; i < arr->n ; i++ ) {
          nap = subelem_lte_match( (junc_t *) arr->vect[i], ep,  nap, id, bcp ) ;
        }
        parr_free( arr ) ;
        /* start the next transversal, where the last left off */
        if( nap ) arr = nap ;
        else return 0 ;
      }
      break ;

  }

  return pap ;
}

/****************************************************************************/

/* Match rule elements where the subelement is LTE */

parr_t  *
subelem_match_lte( junc_t *jp, element_t *ep, parr_t *pap, parr_t *id, becpool_t *b, spocp_result_t *rc ) 
{
  branch_t *bp ;
  junc_t   *np ;
  parr_t   *nap, *arr ;
  int       i ;
  slist_t **slpp ;
  set_t    *or ;

  /* DEBUG( SPOCP_DMATCH ) traceLog( "subelem_match_lte") ; */

  /* DEBUG( SPOCP_DMATCH ) junc_print( 0, jp ) ; */

  if(( bp = ARRFIND( jp, SPOC_ENDOFRULE))) { /* Can't get much further */
    id = parr_join( id, (void *) bp->val.id ) ; 
  }

  if(( bp = ARRFIND( jp, SPOC_BCOND))) 
    pap = erset_match( bp->val.set, ep, pap, b )  ;

  switch( ep->type ) {
    case SPOC_ATOM: /* Atoms, prefixes, suffixes or ranges */
      /* traceLog( "Matching Atom: %s", ep->e.atom->val.val ) ; */
      if(( bp = ARRFIND( jp, SPOC_ATOM)) != 0 ) {
        /* traceLog( "against Atom" ) ; */
        np = atom2atom_match( ep->e.atom, bp->val.atom ) ;
        if( np ) pap = parr_add( pap, np ) ;
      } 

      if(( bp = ARRFIND( jp, SPOC_PREFIX)) != 0 ) {
        /* traceLog( "against prefix" ) ; */
        arr = atom2prefix_match(ep->e.atom, bp->val.prefix ) ;
        if( arr ) {
          pap = parr_join( pap, arr ) ;
          parr_free( arr ) ;
        }
      }

      if(( bp = ARRFIND( jp, SPOC_SUFFIX)) != 0 ) {
        /* traceLog( "against suffix" ) ; */
        arr = atom2suffix_match(ep->e.atom, bp->val.suffix ) ;
        if( arr ) {
          pap = parr_join( pap, arr ) ;
          parr_free( arr ) ;
        }
      }

      if(( bp = ARRFIND( jp, SPOC_RANGE)) != 0 ) {
        /* traceLog( "against range" ) ; */
        slpp = bp->val.range ;

        for( i = 0 ; i < DATATYPES ; i++ ) {
          if( slpp[i] ) {
            nap = atom2range_match( ep->e.atom, slpp[i], i, rc ) ;
            if( nap ) {
               pap = parr_join( pap, nap ) ;
               parr_free( nap ) ;
            } 
          }
        }
      }

      break ;

    case SPOC_PREFIX: /* only with other prefixes */
      if(( bp = ARRFIND( jp, SPOC_PREFIX)) != 0 ) {
        nap = prefix2prefix_match(ep->e.prefix, bp->val.prefix ) ;
        if( nap ) {
           pap = parr_join( pap, nap ) ;
           parr_free( nap ) ;
        } 
      }
      break ;

    case SPOC_SUFFIX: /* only with other suffixes */
      if(( bp = ARRFIND( jp, SPOC_SUFFIX)) != 0 ) {
        nap = suffix2suffix_match(ep->e.suffix, bp->val.suffix ) ;
        if( nap ) {
           pap = parr_join( pap, nap ) ;
           parr_free( nap ) ;
        } 
      }
      break ;

    case SPOC_RANGE : /* only with ranges */
      if(( bp = ARRFIND( jp, SPOC_RANGE)) != 0 ) {
        slpp = bp->val.range ;

        for( i = 0 ; i < DATATYPES ; i++ ) {
          if( slpp[i] ) {
            nap = range2range_match(ep->e.range, slpp[i] ) ;
            if( nap ) {
               pap = parr_join( pap, nap ) ;
               parr_free( nap ) ;
            }
          }
        } 
      }
      break ;

    case SPOC_OR :
      nap = 0 ;
      or = ep->e.set ; 
      for( i = 0 ; i < or->n ; i++ ) {
        nap = subelem_match_lte( jp, or->element[i],  nap, id, b, rc ) ;
      }
      break ;

    case SPOC_LIST : /* other lists */
      /* traceLog( "Matching List:" ) ; */
      if(( bp = ARRFIND( jp, SPOC_LIST)) != 0 ) {
        arr = parr_add( 0, bp->val.list ) ;

        for( ep = ep->e.list->head ; ep ; ep = ep->next ) {
          nap = 0 ;
          for( i = 0 ; i < arr->n ; i++ ) {
            nap = subelem_match_lte( (junc_t *) arr->vect[i], ep,  nap, id, b, rc ) ;
          }
          parr_free( arr ) ;
          /* start the next transversal, where the last left off */
          if( nap ) arr = nap ;
          else return 0 ;
        }
        /* if the number of elements in the ep list is fewer
           collect all alternative until the end of the arr list is found */

        /* traceLog("Gather until end of list") ; */

        for( i = 0 ; i < arr->n ; i++ )
          pap = get_all_to_next_listend( arr->vect[i], pap, 0 ) ;
      }
      break ;

  }

  return pap ;
}

/****************************************************************************/

static parr_t *adm_list( junc_t *jp, subelem_t *sub, becpool_t *bc, spocp_result_t *rc ) 
{
  parr_t    *nap = 0, *res, *pap = 0 ;
  int        i ;
  parr_t   *id ;
   
  if( jp == 0 ) return 0 ;

  id = parr_new( 4, &P_pcmp, 0, &P_pcpy, 0 ) ;

  /* The rules must be lists and I only have subelements so I have to skip
     the list tag part */

  /* just the single list tag should actually match everything */
  if( jp->item[1] == 0 ) return get_rec_all_rules( jp, 0 ) ;

  jp = jp->item[1]->val.list ;

  pap = parr_add( 0, jp ) ;

  for( ; sub ; sub = sub->next ) { 
    res = 0 ;

    for( i = 0 ; i < pap->n ; i++ ) {
      if( sub->direction == LT ) 
        nap = subelem_lte_match((junc_t *) pap->vect[i], sub->ep, nap, id, bc ) ;
      else
        nap = subelem_match_lte( (junc_t *) pap->vect[i], sub->ep, nap, id, bc, rc ) ;
    }

    parr_free( pap ) ;
    if( nap == 0 ) return 0 ;

    /* traceLog("%d roads to follow", nap->n ) ; */
    pap = nap ;
    nap = 0 ;
  }

  /*
   traceLog( "Done all subelements, %d junctions to follow through", pap->n ) ;
  */

  /* After going through all the subelement I should have a list of
     junctions which have to be followed to the end */
  
  for( i = 0 ; i < pap->n ; i++ ) {
    id = get_rec_all_rules( (junc_t *) pap->vect[i], id ) ;
  }

  return id ;
}

/********************************************************************/

subelem_t *pattern_parse( octet_t *pattern )
{
  subelem_t *res = 0, *sub, *pres = 0 ;
  char      *sp ;
  element_t *ep ;
  octet_t   oct ;
  int       l ;

  for( sp = pattern->val, l = pattern->len ; l && WHITESPACE(*sp) ; l--, sp++ ) ;

  do {
    sub = subelem_new( ) ;

    if( *sp == '+' ) sub->direction = GT ;
    else if( *sp == '-' ) sub->direction = LT ;
    else {
      subelem_free( sub ) ;
      return 0 ;
    }

    sp++ ;
    l-- ;

    if( *sp == '(' ) sub->list = 1 ;

    oct.val = sp ;
    oct.len = l ;

    if( element_get( &oct, &ep ) != SPOCP_SUCCESS ) {
      subelem_free( sub ) ;
      subelem_free( res ) ;
      return 0 ;
    }

    sp = oct.val ;
    l = oct.len ;

    sub->ep = ep ;
    if( res == 0 ) res = pres = sub ;
    else {
      pres->next = sub ; 
      pres = sub ;
    }

    while( *sp == ' ' ) sp++, l-- ;

  } while( l && *sp != '\n' && *sp != '\r' ) ;

  return res ;
}

/********************************************************************/

spocp_result_t
get_matching_rules( db_t *db, octet_t *pat, octarr_t *oa, spocp_req_info_t *sri, char *rs )
{
  spocp_result_t rc = SPOCP_SUCCESS ;
  subelem_t     *sub ;
  parr_t        *ip ;
  octet_t       *oct, *arr[2] ;
  int            i ;
  ruleinst_t    *r ;

  if(( sub = pattern_parse( pat )) == 0 ) {
    return SPOCP_SYNTAXERROR ;
  }

  /* traceLog( "Parsed the subelements OK" ) ; */

  /* get the IDs for the matching rules */
  ip = adm_list( db->jp, sub, db->bcp, &rc ) ;

  if( ip == 0 && rc != SPOCP_SUCCESS ) return rc ;

  arr[1] = 0 ;

  /* produce the array of rules */
  if( ip ) {

    if(( oa->size - oa->n ) < ip->n ) octarr_mr( oa, ip->n ) ;

    for( i = 0 ; i < ip->n ; i++ ) {
      r = parr_get_item_by_index( ip, i ) ;

      arr[0] = r->rule ;

      /* allowed to see the rule ? */
      if(( rc = allowed_by_aci( db, arr, sri, "read" )) != SPOCP_SUCCESS ) continue ; 
  
      oct = rulename_print( r, rs ) ;

      octarr_add( oa, oct ) ;
    }
  }

  return SPOCP_SUCCESS ;
}

char **get_by_uid( ruleinfo_t *ri, char *pat )
{
  char **res = 0 ;
 
  return res ;
}

/********************************************************************/

