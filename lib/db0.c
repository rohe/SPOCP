/***************************************************************************
                          db0.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <string.h>

#include <macros.h>
#include <db0.h>
#include <struct.h>
#include <func.h>
#include <wrappers.h>
#include <spocp.h>
#include <sha1.h>

junc_t  *element_add( plugin_t *pl, junc_t *dvp, element_t *ep, ruleinst_t *ri ) ;
junc_t  *rm_next( junc_t *ap, branch_t *bp ) ;
char    *set_item_list( junc_t *dv ) ;
junc_t  *atom_add( branch_t *bp, atom_t *ap ) ;
junc_t  *extref_add( branch_t *bp, atom_t *ap ) ;
junc_t  *list_end( junc_t *arr ) ;
junc_t  *list_add( plugin_t *pl, branch_t *bp, list_t *lp, ruleinst_t *ri ) ;
limit_t *limit_new( ) ;
junc_t  *range_add( branch_t *bp, range_t *rp ) ;
junc_t  *prefix_add( branch_t *bp, atom_t *ap ) ;
junc_t  *rule_close( junc_t *ap, ruleinst_t *ri ) ;

erset_t *erset_dup( erset_t *old, ruleinfo_t *ri ) ;

/*
junc_t  *list_close( junc_t *ap, element_t *ep, ruleinst_t *ri, int *eor ) ;
static int  last_element( element_t *ep ) ;
*/

ruleinst_t *ruleinst_dup( ruleinst_t *ri )  ;
void       ruleinst_free( ruleinst_t *ri ) ;

void       P_ruleinst_free( void * ) ;

void       *P_raci_dup( void *vp ) ;
void       P_raci_free( void *vp ) ;

int P_uid_match( void  *vp, void *pattern ) ;

/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */

char item[NTYPES+1] ;

/************************************************************
*                                                           *
************************************************************/
/*

Arguments:

Returns:
*/

junc_t *junc_new( )
{
  junc_t *ap ;

  ap = ( junc_t * ) Calloc ( 1, sizeof( junc_t )) ;

  return ap ;
}

/************************************************************
*                                                           *
************************************************************/
/*

Arguments:

Returns:
*/

junc_t *branch_add( junc_t *ap, branch_t *bp )
{
  if( ap == 0 ) ap = junc_new() ;

  ap->item[bp->type] = bp ;
  bp->parent = ap ;

  return ap ;
}

/************************************************************
*                                                   &P_match_uid        *
************************************************************/
/*

Arguments:

Returns:
*/

junc_t *junc_dup( junc_t *jp, ruleinfo_t *ri )
{
  junc_t   *new ;
  branch_t *nbp, *obp ;
  int      i, j ;

  if( jp == 0 ) return 0 ;

  new = junc_new() ;

  for( i = 0 ; i < NTYPES ; i++ ) {
    if( jp->item[i] ) {
      nbp = new->item[i] = ( branch_t *) Calloc (1, sizeof( branch_t )) ;
      obp = jp->item[i] ;
    
      nbp->type  = obp->type ;
      nbp->count = obp->count ;
      nbp->parent = new ;

      switch( i ) {
        case SPOC_ATOM :
          nbp->val.atom = phash_dup( obp->val.atom, ri ) ;
          break ;

        case SPOC_LIST :
          nbp->val.list = junc_dup( obp->val.list, ri ) ;
          break ;

        case SPOC_PREFIX :
          nbp->val.prefix = ssn_dup( obp->val.prefix, ri ) ;
          break ;

        case SPOC_SUFFIX :
          nbp->val.suffix = ssn_dup( obp->val.suffix, ri ) ;
          break ;

        case SPOC_RANGE :
          for( j = 0 ; j < DATATYPES ; j++ ) {
            nbp->val.range[j] = sl_dup( obp->val.range[j], ri ) ;
          }
          break ;

        case SPOC_ENDOFLIST :
          nbp->val.list = junc_dup( obp->val.list, ri ) ;
          break ;

        case SPOC_ENDOFRULE :
          nbp->val.id = index_dup( obp->val.id, ri ) ;
          break ;

        case SPOC_BCOND:
          nbp->val.set = erset_dup( obp->val.set, ri ) ;
          break ;

        case SPOC_REPEAT :
          break ;

      }
    }
  }

  return new ;
}

/************************************************************
*                                                           *
************************************************************/
/*

Arguments:

Returns:
*/

char *set_item_list( junc_t *dv )
{
  int i ;

  for( i = 0 ; i < NTYPES ; i++ ) {
    if( dv && dv->item[i] ) item[i] = '1' ;
    else item[i] = '0' ;
  }

  item[NTYPES] = '\0' ;

  return item ;
}

/************************************************************
*                                                           *
************************************************************/
/*

Arguments:

Returns:
*/
/************************************************************
*                                                           *
************************************************************/
/*

Arguments:

Returns:
*/

junc_t *atom_add( branch_t *bp, atom_t *ap )
{
  buck_t   *bucket = 0 ;

  if( bp->val.atom == 0 )
    bp->val.atom = phash_new( 3, 50 ) ;

  bucket = phash_insert( bp->val.atom, ap, ap->hash ) ;
  bucket->refc++ ;

  DEBUG(SPOCP_DSTORE) traceLog( "Atom \"%s\" [%d]", ap->val.val, bucket->refc ) ;

  if( bucket->next == 0 ) bucket->next = junc_new( ) ;

  return bucket->next ;
}


/************************************************************/

char *P_print_junc( item_t i )
{
  junc_print( 0, (junc_t *) i ) ;

  return 0 ;
}
  
void P_free_junc( item_t i )
{
  junc_free(( junc_t *) i ) ;
}

item_t P_junc_dup( item_t i, item_t j )
{
  return junc_dup( ( junc_t * ) i , ( ruleinfo_t *) j ) ; 
}


/************************************************************
*                                                           *
************************************************************/
/*

Arguments:

Returns:
*/

static ref_t *ref_get( plugin_t *top, atom_t *ap )
{
  char           *cp ;
  ref_t          *ref = 0 ;
  int            l, inv = 0 ;
  octet_t        beo, arg, oct ;
  spocp_result_t rc ;
  plugin_t       *pl ;

  beo.val = ap->val.val ;
  beo.len = ap->val.len ;

  if( *beo.val == '!' ) {
    beo.val++ ;
    beo.len-- ;
    inv = 1 ;
  }

  /* typespecification only ascii */
  if(( cp = index( beo.val, ':' )) == 0 ) {
    LOG( SPOCP_WARNING ) traceLog( "No argument to external reference") ;
    arg.val = 0 ;
    arg.len = 0 ;
  }
  else {
    l = cp - beo.val ;
    arg.val = ++cp ;
    arg.len = beo.len - l - 1 ;
    beo.len = l  ;
  }

  /* check if known external reference */
  if(( pl = plugin_match( top, &beo )) == 0 ) {
    LOG( SPOCP_WARNING ) traceLog( "unknown external \"%s\" reference", beo.val ) ;
    return 0 ;
  }

  DEBUG(SPOCP_DSTORE) traceLog( "add external \"%s\" reference", beo.val ) ;

  ref = ( ref_t *) Calloc( 1, sizeof( ref_t )) ;

  ref->bc.val = 0 ;
  ref->bc.size = ref->bc.len = 0 ;

  ref->plugin = pl ;

  if( inv ) {
    oct.val = beo.val ;
    oct.len = ap->val.len - 1 ;
    rc = octcpy( &ref->bc, &oct ) ;
  }
  else rc = octcpy( &ref->bc, &ap->val ) ;

  if( arg.len ) {
    ref->arg.val = ref->bc.val + beo.len + 1 ;
    ref->arg.len = arg.len ;
  }

  ref->cachedval = 0 ;
  ref->next = 0 ;
  ref->inv = inv ;

  /* set cachetime */

  ref->ctime = cachetime_set( &ref->arg , pl ) ;

  return ref ;
}

junc_t *erset_add( plugin_t *pl, branch_t *bp, set_t *set )
{
  int       i ;
  erset_t   *es, *next ;

  es = erset_new( set->n ) ;

  for( i = 0 ; i < set->n ; i++ ) {
    if(( es->ref[i] = ref_get( pl, set->element[i]->e.atom )) == 0 ) {
      /* clean up */
      es->n = i ;
      erset_free( es ) ;
      return 0 ;
    }
  }

  es->n = set->n ;

  if( bp->val.set == 0 ) {
    bp->val.set = es ;
    es->next = junc_new() ;
  }
  else {
    /* the simple way to start with */
    for( next = bp->val.set ; next->other ; next = next->other ) ;

    next->other = es ;
  }

  return es->next ;
}


/************************************************************
*                                                           *
************************************************************/
/*

Arguments:

Returns:
*/

junc_t *list_end( junc_t *arr )
{
  branch_t *db ;

  db = ARRFIND( arr, SPOC_ENDOFLIST ) ;

  if( db == 0 ) {
    db = ( branch_t * ) Calloc ( 1, sizeof( branch_t )) ;
    db->type = SPOC_ENDOFLIST;
    db->count = 1 ;
    db->val.list = junc_new( ) ;
    branch_add( arr, db ) ;
  }
  else {
    db->count++ ;
  }

  DEBUG(SPOCP_DSTORE) traceLog("List end [%d]", db->count ) ;

  arr = db->val.list ;

  return arr ;
}

/************************************************************
*                                                           *
************************************************************/
/*

Arguments:

Returns:
*/

junc_t *list_add( plugin_t *pl, branch_t *bp, list_t *lp, ruleinst_t *ri )
{
  element_t *elp ;
  junc_t    *jp ;

  if( bp->val.list == 0 ) {
    jp = bp->val.list = junc_new( ) ;
  }
  else
    jp = bp->val.list ;

  elp = lp->head ; 

  /* fails, means I should clean up */
  if(( jp = element_add( pl, jp, elp, ri )) == 0 ) {
    traceLog( "List add failed") ;
    return 0 ;
  }

  return jp ;
}

/************************************************************
*                                                           *
************************************************************/

void list_clean( junc_t *jp, list_t *lp )
{
  element_t *ep = lp->head, *next ;
  buck_t    *buck ;
  branch_t  *bp ;

  for( ; ep ; ep = next ) {
    next = ep->next ;
    if( jp == 0 || ( bp = ARRFIND( jp, ep->type )) == 0 ) return ;

    switch( ep->type ) {
      case SPOC_ATOM :
        buck = phash_search( bp->val.atom, ep->e.atom, ep->e.atom->hash ) ;
        jp = buck->next ;
        buck->refc-- ;
        bp->count-- ;
        if( buck->refc == 0 ) {
          buck->val.val = 0  ;
          buck->hash = 0 ;
          junc_free( buck->next ) ;
          buck->next = 0 ;
          next = 0 ;
        }
        break ;

      case SPOC_LIST :
        bp->count-- ;
        if( bp->count == 0 ) {
          junc_free( bp->val.list ) ;
          bp->val.list = 0 ;
        }
        else list_clean( bp->val.list, ep->e.list ) ;
        break ;
    } 
  }
}

/************************************************************
*                                                           *
************************************************************/
/*

Arguments:

Returns:
*/

limit_t *limit_new( )
{
  limit_t *lp ;

  lp = ( limit_t * ) Calloc ( 1, sizeof( limit_t )) ;

  return lp ;
}

/************************************************************
*                                                           *
************************************************************/
/*

Arguments:

Returns:
*/

junc_t *range_add( branch_t *bp, range_t *rp )
{
  int      i = rp->lower.type & 0x07 ;
  junc_t   *jp ;

  if( bp->val.range[i] == 0 ) {
    bp->val.range[i] = sl_init( 4 ) ;
  }

  jp = sl_range_add( bp->val.range[i], rp ) ;

  return jp ;
}

/************************************************************
*            adds a prefix to a prefix branch               *
************************************************************/
/*

Arguments:
  bp	pointer to a branch
  ap    pointer to a atom

Returns: pointer to next node in the tree
*/

junc_t *prefix_add( branch_t *bp, atom_t *ap )
{
  /* assumes that the string to add is '\0' terminated */
  return ssn_insert( &bp->val.prefix, ap->val.val, FORWARD ) ;
}

/************************************************************
*            adds a suffix to a suffix branch               *
************************************************************/
/*

Arguments:
  bp	pointer to a branch
  ap    pointer to a atom

Returns: pointer to next node in the tree
*/

junc_t *suffix_add( branch_t *bp, atom_t *ap )
{
  /* assumes that the string to add is '\0' terminated */
  return ssn_insert( &bp->val.suffix, ap->val.val, BACKWARD ) ;
}

/************************************************************
*      Is this the last element in a S-expression ?         *
************************************************************/
/*
 assumes list as most basic unit 

Arguments:
  ep	element

Returns: 1 if it is the last element otherwise 0

static int last_element( element_t *ep )
{
  while( ep->next == 0  && ep->memberof != 0 ) ep = ep->memberof ;

  if( ep->next == 0 && ep->memberof == 0 ) return 1 ;
  else return 0 ;
}
*/

/************************************************************
*           Adds end-of-rule marker to the tree             *
************************************************************/
/*

Arguments:
  ap	node in the tree
  id    ID for this rule

Returns: pointer to next node in the tree
*/

junc_t *rule_end( junc_t *ap, ruleinst_t *ri )
{
  branch_t *bp ;

  if(( bp = ARRFIND( ap, SPOC_ENDOFLIST )) == 0 ) {
    DEBUG( SPOCP_DSTORE ) traceLog("New rule end" ) ;

    bp = ( branch_t *) Calloc (1, sizeof( branch_t )) ;
    bp->type = SPOC_ENDOFRULE ;
    bp->count = 1 ;
    bp->val.id = index_add( bp->val.id, (void *) ri ) ;

    ap = branch_add( ap, bp ) ;
  }
  else { /* If rules contains references this can happen, otherwise not */
    bp->count++ ;
    DEBUG( SPOCP_DSTORE ) traceLog("Old rule end: count [%d]", bp->count ) ;
    index_add( bp->val.id, (void *) ri ) ;
  }

  DEBUG( SPOCP_DSTORE ) traceLog( "done rule %s", ri->uid ) ;

  return ap ;
}

/************************************************************
*           Adds end-of-list marker to the tree             *
************************************************************/
/*

Arguments:
  ap	node in the tree
  ep    end of list element 
  id    ID for this rule

Returns: pointer to next node in the tree

Should mark in some way that end of rule has been reached 
*/

junc_t *list_close( junc_t *ap, element_t *ep, ruleinst_t *ri, int *eor )
{
  element_t *parent ;

  do {
    ap = list_end(ap) ;

    parent = ep->memberof ;

    DEBUG( SPOCP_DSTORE ) {
      traceLog( "end_list that started with \"%s\"",
               parent->e.list->head->e.atom->val.val ) ;
    }

    ep = parent ;
  } while( ep->type == SPOC_LIST && ep->next == 0 && ep->memberof != 0 ) ;

  if( parent->memberof == 0 ) {
    rule_end( ap, ri ) ;
    *eor = 1 ;
  }
  else *eor = 0 ;

  return ap ;
}

static junc_t *add_next( plugin_t *plugin, junc_t *jp, element_t *ep, ruleinst_t *ri )
{
  int eor = 0 ;

  if( ep->next ) jp = element_add( plugin, jp, ep->next, ri ) ;
  else if( ep->memberof) {
    if( ep->memberof->type == SPOC_OR )  ; /* handed upstream */
    else if( ep->type != SPOC_LIST ) /* a list never closes itself */ 
      jp = list_close( jp, ep, ri, &eor ) ;
  }

  return jp ;
}

/************************************************************
*           Add a s-expression element                      *
************************************************************/
/*
The representation of the S-expression representing rules are
as a tree. A every node in the tree there are a couple of 
different types of branches that can appear. This routine 
chooses which type it should be and then adds this element to 
that branch.

Arguments:
  ap	node in the tree
  ep    element to store
  rt    pointer to the ruleinstance

Returns: pointer to the next node in the tree
*/


junc_t *element_add( plugin_t *pl, junc_t *jp, element_t *ep, ruleinst_t *rt )
{
  branch_t  *bp = 0 ;
  set_t     *set ;
  junc_t    *ap = 0 ;
  int       i ;

  if( ep->type == SPOC_OR ) {
    set = ep->e.set ;
    for( i = 0 ; i < set->n ; i++ ) {
      if(( ap = element_add( pl, jp, set->element[i], rt )) == 0 ) break ;
      ap = add_next( pl, ap, ep, rt ) ;
    }
  }
  else {
    bp = ARRFIND( jp, ep->type );

    /* DEBUG( SPOCP_DSTORE ) traceLog("Items: %s", set_item_list( jp ) ) ; */

    if( bp == 0 ) {
      bp = ( branch_t *) Calloc (1, sizeof( branch_t )) ;
      bp->type = ep->type ;
      bp->count = 1 ;
      ap = branch_add( jp, bp ) ;
    }
    else {
      bp->count++ ;
    }

    DEBUG( SPOCP_DSTORE ) traceLog("Branch [%d] [%d]", bp->type, bp->count ) ;

    switch( ep->type ) {
      case SPOC_ATOM:
        if(( ap = atom_add( bp, ep->e.atom ))) ap = add_next( pl, ap, ep, rt ) ;
        break ;

      case SPOC_PREFIX:
        if(( ap = prefix_add( bp, ep->e.prefix ))) ap = add_next( pl, ap, ep, rt ) ;
        break ;
  
      case SPOC_SUFFIX:
        if(( ap = suffix_add( bp, ep->e.prefix ))) ap = add_next( pl, ap, ep, rt ) ;
        break ;
  
      case SPOC_RANGE:
        if(( ap = range_add( bp, ep->e.range ))) ap = add_next( pl, ap, ep, rt ) ;
        break ;
  
      case SPOC_LIST:
        if(( ap = list_add( pl, bp, ep->e.list, rt ))) ap = add_next( pl, ap, ep, rt ) ;
        break ;
  
      case SPOC_OR:
        break ;
  /*
      case SPOC_E....F:
        if(( ap = erset_add( bp, ep->e.set ))) ap = add_next( pl, ap, ep, rt ) ;
        break ;
  */
      case SPOC_BCOND:
        if(( ap = erset_add( pl, bp, ep->e.set ))) ap = add_next( pl, ap, ep, rt ) ;
        break ;
  
    }

    if( ap == 0 && bp != 0 ) {
      bp->count-- ;

      if( bp->count == 0 ) {
        DEBUG( SPOCP_DSTORE ) traceLog( "Freeing type %d branch", bp->type ) ;
        branch_free( bp ) ;
        jp->item[ep->type] = 0 ;
      }
    }
  }

  return ap ;
}

/************************************************************
                 RULE INFO functions 
 ************************************************************/
 
/* ----------  raci ------------ */

raci_t *raci_new( )
{
  raci_t *sa = 0 ;

  sa = ( raci_t * ) Calloc ( 1, sizeof( raci_t )) ;

  return sa ;
}

raci_t *raci_dup( raci_t *sa, ruleinfo_t *ri )
{
  raci_t *new ;

  if( sa == 0 ) return 0 ;

  new = raci_new() ;

  new->resource = subelement_dup( sa->resource ) ;
  new->action   = sa->action ;
  new->subject  = element_dup( sa->subject, 0, 0 ) ;

  return new ;
}

void *P_raci_dup( void *vp ) 
{
  return (void *) raci_dup( (raci_t *) vp, 0 ) ;
}

void raci_free( raci_t *ra )
{
  if( ra ) {
    if( ra->resource ) subelem_free( ra->resource ) ;
    if( ra->subject ) element_free( ra->subject ) ;
    
    free( ra ) ;
  }
}

void P_raci_free( void *vp )
{
  raci_free( (raci_t *) vp ) ;
}

/* mostly PLACEHOLDER */

int P_raci_print( void *vp )
{
  raci_t *sa = ( raci_t * ) vp ;

  traceLog( "action: %u", sa->action ) ;

  return 0 ;
}

/* ---------------  ruleinst ----------------------------- */

ruleinst_t *ruleinst_new( octet_t *rule, octet_t *blob )
{
  struct sha1_context ctx ;
  ruleinst_t          *rip ;
  unsigned char       sha1sum[21], *ucp ;
  int                 j ;

  rip = ( ruleinst_t * ) Calloc ( 1, sizeof( ruleinst_t )) ;

  if( rule && rule->len ) rip->rule = octdup( rule ) ;
  else rip->rule = 0 ;

  if( blob && blob->len ) rip->blob = octdup( blob ) ;
  else rip->blob = 0 ;

  if( rule ) {
    sha1_starts( &ctx ) ;

    sha1_update( &ctx, (uint8 *) rule->val, rule->len ) ;
    if( blob ) sha1_update( &ctx, (uint8 *) blob->val, blob->len ) ;

    sha1_finish( &ctx, (unsigned char *) sha1sum ) ;

    for( j = 0, ucp = (unsigned char *) rip->uid ; j < 20; j++, ucp += 2 )
      sprintf( (char *) ucp, "%02x", sha1sum[j] );

    DEBUG( SPOCP_DSTORE ) traceLog( "New rule (%s)", rip->uid ) ;
  }

  rip->ep = 0 ;
  rip->raci = 0 ;
  rip->alias = 0 ;
  rip->aci = 0 ;
 
  return rip ;
}

ruleinst_t *ruleinst_dup( ruleinst_t *ri ) 
{
  ruleinst_t *new ;

  if( ri == 0 ) return 0 ;

  new = ruleinst_new( ri->rule, ri->blob ) ;

  new = ( ruleinst_t *) Calloc ( 1, sizeof( ruleinst_t )) ;

  strcpy( new->uid, ri->uid ) ;
  new->rule = octdup( ri->rule ) ;
  /* if( ri->ep ) new->ep =  element_dup( ri->ep ) ; */ 
  if( ri->alias ) new->alias = ll_dup( ri->alias ) ;
  if( ri->aci ) new->aci = ll_dup( ri->aci ) ;

  return new ;
}

item_t P_ruleinst_dup( item_t i, item_t j )
{
  return (void *) ruleinst_dup( (ruleinst_t *) i ) ;
}

void ruleinst_free( ruleinst_t *rt )
{
  if( rt ) {
    if( rt->rule )  oct_free( rt->rule ) ;
    if( rt->blob )  oct_free( rt->blob ) ;
    if( rt->aci )   ll_free( rt->aci ) ;
    if( rt->alias ) ll_free( rt->alias ) ;
    if( rt->raci )  raci_free( rt->raci ) ;
/*    if( rt->ep )    element_rm( rt->ep ) ; */

    free( rt ) ;
  }
}

int uid_match( ruleinst_t *rt, char *uid )
{
  /* traceLog( "%s <> %s", rt->uid, uid ) ; */

  return strcmp( rt->uid, uid ) ;
}

/* --------- ruleinst functions ----------------------------- */

int P_match_uid( void  *vp, void *pattern )
{
  return uid_match( (ruleinst_t *) vp, (char *) pattern ) ;
}

void P_ruleinst_free( void *vp )
{
  ruleinst_free(( ruleinst_t * ) vp ) ;
}

int P_ruleinst_cmp( void *a, void *b )
{
  ruleinst_t *ra, *rb ;
 
  if( a == 0 && b == 0 ) return 0 ;
  if( a == 0 || b == 0 ) return 1 ;

  ra = ( ruleinst_t *) a ;
  rb = ( ruleinst_t *) b ;

  return strcmp( ra->uid, rb->uid ) ;
}

/* PLACEHOLDER */
char *P_ruleinst_print( void *vp ) 
{
  return 0 ;
}

Key P_ruleinst_key( item_t i )
{
  ruleinst_t *ri = ( ruleinst_t *) i ;

  return ri->uid ;
}


/* --------- ruleinfo ----------------------------- */

ruleinfo_t *ruleinfo_new( void )
{
  return (ruleinfo_t *) Calloc( 1, sizeof( ruleinfo_t )) ;
}

ruleinfo_t *ruleinfo_dup( ruleinfo_t *ri )
{
  ruleinfo_t *new ;

  if( ri == 0 ) return 0 ;

  new = ( ruleinfo_t * ) Calloc ( 1, sizeof( ruleinfo_t )) ;
  new->rules = rbt_dup( ri->rules, 1 ) ;

  return new ;
}

void ruleinfo_free( ruleinfo_t *ri )
{ 
  if( ri ) {
    rbt_free( ri->rules ) ;
    free( ri ) ;
  }
}

ruleinst_t *ruleinst_find_by_uid( rbt_t *rules, char *uid ) 
{
  return (ruleinst_t *) rbt_search( rules, uid ) ;
}

/* ---------------------------------------- */

/*
void rm_raci( ruleinst_t *rip, raci_t *ap )
{
}

*/

int free_rule( ruleinfo_t *ri, char *uid )
{
  return rbt_remove( ri->rules, uid ) ;
}

void free_all_rules( ruleinfo_t *ri )
{
  ruleinfo_free( ri ) ;
}

ruleinst_t *save_rule( db_t *db, octet_t *rule, octet_t *blob )
{
  ruleinfo_t  *ri ;
  ruleinst_t  *rt ;

  if( db->ri == 0 ) db->ri = ri = ruleinfo_new() ;
  else ri = db->ri ;

  rt = ruleinst_new( rule, blob ) ;

  if( ri->rules == 0 ) 
    ri->rules = rbt_init( &P_ruleinst_cmp, &P_ruleinst_free, &P_ruleinst_key,
                        &P_ruleinst_dup, &P_ruleinst_print ) ; 
  else {
    if( ruleinst_find_by_uid( ri->rules, rt->uid ) != 0 ) {
      ruleinst_free( rt ) ;
      return 0 ;
    }
  }

  rbt_insert( ri->rules, (item_t) rt ) ;

  return rt ;
}

ruleinst_t *aci_save( db_t *db, octet_t *rule )
{
  ruleinfo_t  *ri ;
  ruleinst_t  *rt ;

  if( db->raci == 0 ) db->raci = ri = ruleinfo_new() ;
  else ri = db->raci ;

  rt = ruleinst_new( rule, 0 ) ;

  if( ri->rules == 0 ) 
    ri->rules = rbt_init( &P_ruleinst_cmp, &P_ruleinst_free, &P_ruleinst_key,
                        &P_ruleinst_dup, &P_ruleinst_print ) ; 
  else {
    if( ruleinst_find_by_uid( ri->rules, rt->uid ) != 0 ) {
      ruleinst_free( rt ) ;
      return 0 ;
    }
  }

  rbt_insert( ri->rules, (item_t) rt ) ;

  return rt ;
}

int nrules( ruleinfo_t *ri )
{
  if( ri == 0 ) return 0 ;
  else          return rbt_items( ri->rules ) ;
}

int rules( db_t *db ) 
{
  if( db == 0 || db->ri == 0 || db->ri->rules == 0 ||  db->ri->rules->head == 0 )
    return 0 ;
  else return 1 ;
}

parr_t *get_rule_inx( ruleinfo_t *ri ) 
{
  if( ri == 0 ) return 0 ;

  return rbt2parr( ri->rules ) ;
}

ruleinst_t *get_rule( ruleinfo_t  *ri, char *uid ) 
{
  if( ri == 0 || uid == 0 ) return 0 ;

  return ruleinst_find_by_uid( ri->rules, uid ) ;
}

/* 
  creates an output string that looks like this

  path ruleid rule [ blob ]

 */
octet_t *rulename_print(  ruleinst_t *r, char *rs )
{
  octet_t *oct ;
  int      l, lr ;
  char     flen[1024] ;


  if( rs ) {
    lr = strlen(rs) ;
    snprintf( flen, 1024, "%d:%s", lr, rs ) ;
  }
  else {
    strcpy( flen, "1:/" ) ;
    lr = 1 ;
  }

  l = r->rule->len + DIGITS( r->rule->len) + lr + DIGITS(lr) + 5 + 40 + 1 ;

  if( r->blob && r->blob->len ) {
    l += r->blob->len + 1 + DIGITS( r->blob->len ) ;
  }

  oct =  oct_new( l, 0 ) ;

  octcat( oct, flen, strlen(flen) ) ;

  octcat( oct, "40:", 3 ) ;

  octcat( oct, r->uid, 40 ) ;

  snprintf( flen, 1024, "%d:", r->rule->len ) ;
  octcat( oct, flen, strlen(flen) ) ;

  octcat( oct,  r->rule->val, r->rule->len ) ;
  
  if( r->blob && r->blob->len ) {
    snprintf( flen, 1024, "%d:", r->blob->len ) ;
    octcat( oct, flen, strlen(flen) ) ;
    octcat( oct, r->blob->val, r->blob->len ) ;
  }

  return oct ;
}
 
spocp_result_t get_all_rules( db_t *db, octarr_t *oa, spocp_req_info_t *sri, char *rs )
{
  int            i, n ;
  ruleinst_t     *r ;
  parr_t         *pa = 0, *pb = 0 ;
  octet_t        *oct, *arg[2] ;
  spocp_result_t rc = SPOCP_SUCCESS ;

  n = nrules( db->ri ) ;
  n += nrules( db->raci ) ;

  if( n == 0 ) return rc ;

  if(( oa->size - oa->n ) < n ) octarr_mr( oa, n ) ;

  if( db->ri ) pa = rbt2parr( db->ri->rules ) ;
  if( db->raci ) pb = rbt2parr( db->raci->rules ) ;

  if( pa && pb ) parr_join( pa, pb ) ;
  else if( pb ) pa = pb ;

  arg[1] = 0 ;

  for( i = 0 ; i < pa->n ; i++ ) {
    r = ( ruleinst_t * ) parr_get_item_by_index( pa, i ) ;

    /* allowed to see the rule  */
    arg[0] = r->rule ;
    if( allowed_by_aci( db, arg, sri, "LIST" ) != SPOCP_SUCCESS ) continue ; 
     
    if(( oct = rulename_print( r, rs )) == 0 ) {
      rc = SPOCP_OPERATIONSERROR ;
      octarr_free( oa ) ;
      goto gar_done ;
    }

    octarr_add( oa, oct ) ;
  }

gar_done:
  /* dont't remove the items since I've only used pointers */
  parr_free( pa ) ;
  parr_free( pb ) ;

  return rc ;
}

octet_t *get_blob( ruleinst_t  *ri ) 
{

  if( ri == 0 ) return 0 ;
  else return ri->blob ;
}

db_t *db_new( ) 
{
  db_t *db ;
  
  db = ( db_t * ) Calloc ( 1, sizeof( db_t )) ;
  db->jp = junc_new() ;
  db->ri = ruleinfo_new( ) ;
  db->bcp = becpool_new( 16, 0 ) ;

  return db ;
}

/************************************************************
*    Add a rightdescription to the rules database           *
************************************************************/
/*

Arguments:
  db	Pointer to the incore representation of the rules database
  ep	Pointer to a parsed S-expression
  rt    Pointer to the rule instance

Returns:	TRUE if OK
*/

spocp_result_t store_right( db_t *db, element_t *ep, ruleinst_t *rt )
{
  int         r ;

  if( db->jp == 0 ) db->jp = junc_new() ;

  if( element_add( db->plugins, db->jp, ep, rt ) == 0 ) r = SPOCP_OPERATIONSERROR ;
  else r = SPOCP_SUCCESS ;

  return r ;
}

/************************************************************
*    Store a rights description in the rules database           *
************************************************************/
/*

Arguments:
  db	Pointer to the incore representation of the rules database

Returns:	TRUE if OK
*/

spocp_result_t add_right( db_t **db, octet_t **arg, spocp_req_info_t *sri, ruleinst_t **ri )
{
  element_t      *ep ;
  int            r = 0 ;
  octet_t        rule, blob, oct, *a[2] ;
  ruleinst_t     *rt ;
  spocp_result_t rc ;

  /*
  LOG( SPOCP_WARNING ) traceLog("Adding new rule: \"%s\"", rp->val) ;
  */

  /* If there is some ACI that is relevant for this, find it now
     when it comes to aci checking the blob shouldn't be part of it, so there should
     only be one octet_t argument
   */

  a[0] = arg[0] ;
  a[1] = 0 ;
  if(( rc = allowed_by_aci( *db, arg, sri, "ADD" )) != SPOCP_SUCCESS ) {
    return rc ;
  } 

  if( *db == 0 ) {
    *db = db_new( ) ;
  }

  oct.len = rule.len = arg[0]->len ;
  oct.val = rule.val = arg[0]->val ;

  if( arg[1] ) {
    blob.len = arg[1]->len ;
    blob.val = arg[1]->val ;
  }
  else blob.len = 0 ;

  if(( r = element_get( &oct, &ep )) == SPOCP_SUCCESS ) {

    /* stuff left ?? */
    /* just ignore it */
    if( oct.len ) {
      rule.len -= oct.len ; 
    }

    if(( rt = save_rule( *db, &rule, &blob )) == 0 ) {
      element_free( ep ) ;
      return SPOCP_EXISTS ;
    }

    /* right == rule */
    if(( r = store_right( *db, ep, rt )) != SPOCP_SUCCESS ) { 
      element_free( ep ) ;

      /* remove ruleinstance */
      free_rule( (*db)->ri , rt->uid ) ;

      LOG( SPOCP_WARNING ) traceLog("Error while adding rule") ;
    }

    *ri = rt ;
  }
  
  return r ;
}

int P_print_str( void *vp )
{
  traceLog( "%s", (char *) vp ) ;
 
  return 0 ;
}

int ruleinst_print( ruleinst_t *ri )
{
  char *str ;

  if( ri == 0 ) return 0 ;
  traceLog( "uid: %s", ri->uid ) ;

  str = oct2strdup( ri->rule, '%' ) ;
  traceLog( "rule: %s", str ) ;

  if( ri->blob ) {
    str = oct2strdup( ri->blob, '%' ) ;
    traceLog( "rule: %s", str ) ;
  }

  if( ri->alias ) ll_print( ri->alias ) ;
  if( ri->aci ) ll_print( ri->aci ) ;

  return 0 ;
}

int P_print_ruleinstance( void *vp )
{
  return ruleinst_print( (ruleinst_t *) vp ) ;
}

int ruleinfo_print( ruleinfo_t *r )
{
  if( r == 0 ) return 0 ;

  rbt_print( r->rules ) ;

  return 0 ;
}

