#include <db0.h>
#include <wrappers.h>
#include <sha1.h>
#include <func.h>
#include <plugin.h>
#include <spocp.h>
#include <rvapi.h>

typedef struct _stree {
  int list ;
  octet_t val ;
  struct _stree *next ;
  struct _stree *part ;
} stree_t ;

bcspec_t *bcspec_new( plugin_t *plt, octet_t *spec )
{
  octet_t   oct ;
  bcspec_t  *bcs = 0 ;
  plugin_t  *p ;
  int       n ;

  /* two parts name ":" typespec */
  if(( n = octchr( spec, ':' )) < 0 ) return 0 ;
  
  oct.len = spec->len - n - 1 ;
  oct.val = spec->val + n + 1 ;
  spec->len = n ;

  if(( p = plugin_match( plt, spec )) == 0 ) return 0 ;
  
  bcs = ( bcspec_t *) Malloc( sizeof( bcspec_t )) ;

  bcs->plugin = p ;
  bcs->name = oct2strdup( spec, 0 ) ;
  bcs->spec = octdup( &oct ) ;

  return bcs ; 
}

void bcspec_free( bcspec_t *bcs )
{
  if( bcs ) {
    if( bcs->name ) free( bcs->name ) ;
    if( bcs->args ) octarr_free( bcs->args ) ;
    if( bcs->spec ) oct_free( bcs->spec ) ;

    free( bcs ) ;
  }
}


bcexp_t *bcexp_new()
{
  return ( bcexp_t * ) Calloc( 1, sizeof( bcexp_t )) ;
}

static bcexp_t *varr_bcexp_pop( varr_t *va )
{
  return ( bcexp_t *) varr_pop( va ) ;
}

void bcexp_free( bcexp_t *bce ) 
{
  bcexp_t *bp ;

  if( bce ) {
    switch( bce->type ) {
      case AND:
      case OR :
        while(( bp = varr_bcexp_pop( bce->val.arr )) != 0 ) bcexp_free( bp ) ;        
        if( bce->val.arr ) free( bce->val.arr ) ;
        break ;

      case NOT:
        bcexp_free( bce->val.single ) ;
        break ;

      case REF:
        break ;

      case SPEC:
        bcspec_free( bce->val.spec ) ;
      default :
        break ;

    }
    free( bce ) ;
  }
}

void bcdef_free( bcdef_t *bcd )
{
  bcexp_t *bce ;

  if( bcd ) {
    if( bcd->name ) free( bcd->name ) ;
    if( bcd->exp ) bcexp_free( bcd->exp ) ;
    while( bcd->users && bcd->users->n ) {
      bce = varr_bcexp_pop( bcd->users ) ;
      /* don't free the bce it's just a pointer */
    }

    if( bcd->users ) free( bcd->users ) ;
  }
}

bcdef_t *bcdef_new( void )
{
  bcdef_t *new ;

  new = ( bcdef_t * ) Calloc( 1, sizeof( bcdef_t )) ;

  return new ;
}

bcdef_t *bcdef_find( bcdef_t *bcd, octet_t *pattern )
{
  for( ; bcd ; bcd = bcd->next ) 
    if( oct2strcmp( pattern, bcd->name ) == 0 ) return bcd ;

  return 0 ;
}

bcdef_t *bcdef_append( bcdef_t *bcd, bcdef_t *new )
{
  if( bcd == 0 ) return new ;
  for( ; bcd->next ; bcd = bcd->next ) ;
  bcd->next = new ;
  new->prev = bcd ;

  return bcd ;
}

bcdef_t *bcdef_rm( bcdef_t *root, bcdef_t *rm ) 
{
  if( rm == root ) {
    if( root->next ) {
      root->next->prev = 0 ;
      return root->next ;
    }
    return 0 ;
  }
  
  if( rm->next ) rm->next->prev = rm->prev ;
  if( rm->prev ) rm->prev->next = rm->next ;

  return root ;
}

/* ---------------------------------------------------------------------- */

/*
  bcond        = bcondexpr / bcondref

  bcondexpr    = bcondspec / bcondvar

  bcondvar     = bcondor / bcondand / bcondnot / bcondref

  bcondspec    = bcondname ":" typespecific

  bcondname    = 1*dirchar

  typespecific = octetstring

  bcondref     = "(" "ref" 1*dirchar ")"

  bcondand     = "(" "and" 1*bcondvar ")"

  bcondor      = "(" "or" 1*bcondvar ")"

  bcondnot     = "(" "not"  bcondvar ")"

  arg          = ( path / search ) [ "*" / intervall ]
  
  intervall    = "[" single / prefix / suffix / both "]"

  single       = num 
  prefix       = num "-"
  suffix       = "-" ( num / "last" ) 
  both         = num "-" ( num / "last" )
  num          = 1*DIG
  DIG          = %x30-39
*/

static char *make_name( char *str, octet_t *input )
{
  struct sha1_context ctx ;
  unsigned char       sha1sum[21], *ucp ;
  int                 j ;

  if( input == 0 || input->len == 0 ) return 0 ;

  sha1_starts( &ctx ) ;

  sha1_update( &ctx, (uint8 *) input->val, input->len ) ;

  sha1_finish( &ctx, (unsigned char *) sha1sum ) ;

  for( j = 1, ucp = (unsigned char *) str ; j < 20; j++, ucp += 2 )
    sprintf( (char *) ucp, "%02x", sha1sum[j] );

  str[0] = '_' ;
  str[41] = '_' ;
  str[42] = '\0' ;

  return str ;
}

varr_t *varr_bcexp_add( varr_t *va, bcexp_t *bce )
{
  return varr_add( va, (void *) bce ) ;
}

/* ----------------------------------------------------------------------------- */

static void stree_free( stree_t *stp )
{
  if( stp ) {
    if( stp->part ) stree_free( stp->part ) ;
    if( stp->next ) stree_free( stp->next ) ;
    if( stp->val.size ) free( stp->val.val ) ;
    free( stp ) ;
  }
}

/* ---------------------------------------------------------------------- */

static stree_t *parse_sexp( octet_t *sexp )
{
  stree_t *ptp, *ntp = 0, *ptr ;

  if( *sexp->val == '(' ) {
    ptp = ( stree_t * ) calloc ( 1, sizeof( stree_t )) ;
    ptp->list = 1 ;
    sexp->val++ ;
    sexp->len-- ;
    while( sexp->len && *sexp->val != ')' ) {
      if(( ptr = parse_sexp( sexp )) == 0 ) {
        stree_free( ptp ) ;
        return 0;
      }

      if( ptp->part == 0 ) ntp = ptp->part = ptr ;
      else {
        ntp->next = ptr ;
        ntp = ntp->next ;
      }
    }

    if( *sexp->val == ')' ) {
      sexp->val++ ;
      sexp->len-- ;
    }
    else { /* error */
      stree_free( ptp ) ;
      return 0 ;
    }
  }
  else {
    ptp = ( stree_t * ) calloc ( 1, sizeof( stree_t )) ;
    if( get_str( sexp, &ptp->val ) != SPOCP_SUCCESS ) {
      stree_free( ptp ) ;
      return 0 ;
    }
  }
    
  return ptp ;
}

/* ---------------------------------------------------------------------- */

bcexp_t *transv_stree( plugin_t *plt, stree_t *st, bcdef_t *list, bcdef_t *parent )
{
  bcexp_t   *bce, *tmp ;
  bcdef_t   *bcd ;
  bcspec_t  *bcs ;

  if( !st->list ) {
    if( !st->next ) { /* should be a bcond spec */
      if(( bcs = bcspec_new( plt, &st->val)) == 0 ) return 0 ;
      bce = bcexp_new() ;
      bce->type = SPEC ;
      bce->parent = parent ;
      bce->val.spec = bcs ;
    }
    else {
      if( oct2strcmp( &st->val, "ref" ) == 0 ) {
        /* if the ref isn't there this def isn't valid */
        if(( bcd = bcdef_find( list, &st->next->val )) == 0 ) return 0 ;
        bce = bcexp_new() ;
        bce->type = REF ;
        bce->parent = parent ;
        bce->val.ref = bcd ;
        /* so I know who to notify if anything happens to this ref */
        bcd->users = varr_add( bcd->users, bce ) ;
      }
      else if( oct2strcmp( &st->val, "and" ) == 0 ) {
        bce = bcexp_new() ;
        bce->type = AND ;
        bce->parent = parent ;
        for( st = st->next ; st ; st = st->next ) {
          if(( tmp = transv_stree( plt, st, list, parent )) == 0 ) {
            bcexp_free( bce ) ; 
            return 0 ;
          }
          bce->val.arr = varr_add( bce->val.arr, (void * ) tmp ) ; 
        }
      }
      else if( oct2strcmp( &st->val, "or" ) == 0 ) {
        bce = bcexp_new() ;
        bce->type = OR ;
        bce->parent = parent ;
        for( st = st->next ; st ; st = st->next ) {
          if(( tmp = transv_stree( plt, st, list, parent )) == 0 ) {
            bcexp_free( bce ) ; 
            return 0 ;
          }
          bce->val.arr = varr_add( bce->val.arr, (void * ) tmp ) ; 
        }
      }
      else if( oct2strcmp( &st->val, "not" ) == 0 ) {
        if( st->next->next ) return 0 ;
        bce = bcexp_new() ;
        bce->type = NOT ;
        bce->parent = parent ;
        if(( bce->val.single = transv_stree( plt, st->next, list, parent )) == 0 ){
          free( bce ) ;
          return 0 ;
        }
      }
      else return 0 ;
    }

    return bce ;
  }
  else return transv_stree( plt, st->part, list, parent ) ;
}

/* ---------------------------------------------------------------------- */

void bcexp_deref( bcexp_t *bce )
{
  bcdef_t *bcd ;
  
  bcd = bce->val.ref ;

  bcd->users = varr_rm( bcd->users, bce ) ;
}

void bcexp_delink( bcexp_t *bce )
{
  int     i, n ;
  varr_t *va ;

  switch( bce->type ) {
    case SPEC: 
      break ;

    case AND:
    case OR:
      va = bce->val.arr ;
      n = varr_len( va ) ;
      for( i = 0 ; i < n ; i++ ) bcexp_delink( ( bcexp_t * ) varr_nth( va, i ) ) ;
      break ;

    case NOT: 
      bcexp_delink( bce->val.single ) ;
      break ;

    case REF: 
      bcexp_deref( bce ) ;
      break ;

  }
}

void bcdef_delink( bcdef_t *bcd )
{
  bcexp_delink( bcd->exp ) ;  
}

/* ---------------------------------------------------------------------- */

bcdef_t *bcdef_add( plugin_t *plt, octet_t *name, octet_t *data, bcdef_t **list ) 
{
  bcexp_t   *bce ;
  bcdef_t   *bcd, *bc ;
  char       cname[45] ;
  stree_t   *st ;

  if( *data->val == '(' ) {
    if(( st = parse_sexp( data )) == 0 ) return 0 ;
  }
  else {
    st = ( stree_t * ) Malloc( sizeof( stree_t )) ;
    st->list = 0 ;
    st->next = st->part = 0 ;
    st->val.size = 0 ; /* otherwise octcpy might core dump */
    octcpy( &st->val, data) ;
  }

  bcd = bcdef_new() ;

  bce = transv_stree( plt, st, *list, bcd ) ;

  stree_free( st ) ;

  if( bce == 0 ) {
    bcdef_free( bcd ) ;
    return 0 ;
  }

  if( bce->type != REF ) {
    if( name == 0 || name->len == 0 ) {
      make_name( cname, data ) ;
      bcd->name = Strdup( cname ) ;
    }
    else {
      bcd->name = oct2strdup( name, 0 ) ;
    }
  }

  bcd->exp = bce ;

  if( bcd->name ) {
    if( *list == 0 ) *list = bcd ;
    else {
      for( bc = *list ; bc->next ; bc = bc->next ) ;
      bc->next = bcd ;
      bcd->prev = bc ;
    }
  }
 
  return bcd ;
}

/* ---------------------------------------------------------------------- */

spocp_result_t bcdef_del( bcdef_t **root, octet_t *name ) 
{
  bcdef_t *bcd ;

  bcd = bcdef_find( *root, name ) ;
  
  if( bcd->users && varr_len( bcd->users ) > 0 ) return SPOCP_UNWILLING ;  
  if( bcd->rules && varr_len( bcd->rules ) > 0 ) return SPOCP_UNWILLING ;  

  /* this boundary conditions might have links to other those links 
     should be removed */
  bcdef_delink( bcd ) ;

  *root = bcdef_rm( *root, bcd ) ;

  bcdef_free( bcd ) ;

  return SPOCP_SUCCESS ;
}

/* ---------------------------------------------------------------------- */

spocp_result_t
  bcdef_replace( plugin_t *pl, octet_t *name, octet_t *data, bcdef_t **root ) 
{
  bcdef_t    *old, *new ;
  bcexp_t    *bce ;
  ruleinst_t *ri ;

  old = bcdef_find( *root, name ) ;
  
  if(( new = bcdef_add( pl, name, data, root )) == 0 ) return SPOCP_PROTOCOLERROR ;

  /* if nothing to replace this is just an add */
  if( old == 0 ) return SPOCP_SUCCESS ;

  /* links to old should point to new instead */
  while(( bce = (bcexp_t *) varr_pop( old->users ))) {
    bce->val.ref = new ;
    new->users = varr_add( new->users, (void *) bce ) ;
  }
  while(( ri = (ruleinst_t *) varr_pop( old->rules ))) {
    ri->bcond = new ;
    new->rules = varr_add( new->rules, (void *) ri ) ;
  }
  
  /* this boundary conditions might have links to other those links 
     should be removed */
  bcdef_delink( old ) ;

  *root = bcdef_rm( *root, old ) ;

  bcdef_free( old ) ;

  return SPOCP_SUCCESS ;
}

/* ---------------------------------------------------------------------- */

/* expects a sepc of the form {...}{...}:.... */

spocp_result_t bcond_eval( element_t *qp, element_t *rp, bcspec_t *bcond, octet_t *oct ) 
{
  octet_t    spec ;
  element_t  *tmp, *xp = 0 ;
  int        sl, rl, rc ;
 
  octln( &spec, bcond->spec ) ;

  while( spec.len && *spec.val == '{' ) {
    spec.len-- ;
    spec.val++ ;
    if(( sl = octchr( &spec, '}' )) < 0 ) break ;

    rl = spec.len - ( sl + 1 ) ;
    spec.len = sl ;

    if(( tmp = element_eval( &spec, qp, &rc )) == NULL ) {
      tmp = element_new()  ; 
      tmp->type = SPOC_NULL ;
    }
    xp = element_list_add( xp, tmp ) ;

    spec.val += sl + 1  ;
    spec.len = rl ;
  }

  if( *spec.val != ':' ) return SPOCP_SYNTAXERROR ;
  spec.val++ ;
  spec.len-- ;

  rc = bcond->plugin->test( qp, rp, xp, &spec, &bcond->plugin->dyn, oct ) ;

  if( xp ) element_free( xp );

  return rc ;
}

/* ---------------------------------------------------------------------- */

spocp_result_t bcexp_eval( element_t *qp, element_t *rp, bcexp_t *bce, octarr_t **oa )
{
  int            n, i ;
  spocp_result_t r = SPOCP_DENIED ;
  octet_t        oct ;
  octarr_t      *lo = 0, *po = 0 ;

  oct.size = oct.len = 0 ;

  switch( bce->type ) {
     case SPEC:
       r = bcond_eval( qp, rp, bce->val.spec, &oct ) ;
       if( r == SPOCP_SUCCESS && oct.len ) lo = octarr_add( 0, octdup( &oct )) ;
       break ;

     case NOT:
       r = bcexp_eval( qp, rp, bce->val.single, &lo ) ;
       if( r == SPOCP_SUCCESS ) {
         if( lo ) octarr_free( lo ) ;
         r = SPOCP_DENIED ;
       }
       else if( r == SPOCP_DENIED ) r = SPOCP_SUCCESS ;
       break ;

     case AND:
       n = varr_len( bce->val.arr ) ;
       r = SPOCP_SUCCESS ;
       for( i = 0 ; r == SPOCP_SUCCESS && i < n ; i++ ) {
         r = bcexp_eval( qp, rp, (bcexp_t *) varr_nth( bce->val.arr, i ), &po) ;
         if( r == SPOCP_SUCCESS && po ) lo = octarr_join( lo, po ) ;
       }
       if( r != SPOCP_SUCCESS ) octarr_free( lo ) ;
       break ;

     case OR:
       n = varr_len( bce->val.arr ) ;
       r = SPOCP_DENIED ;
       for( i = 0 ; r != SPOCP_SUCCESS && i < n ; i++ ) {
         r = bcexp_eval( qp, rp, (bcexp_t *) varr_nth( bce->val.arr, i ), &lo) ;
       }
       break ;

     case REF:
       r = bcexp_eval( qp, rp, bce->val.ref->exp, &lo ) ;
       break ;
  }

  if( r == SPOCP_SUCCESS && lo ) *oa = octarr_join( *oa, lo ) ;

  return r ;
}

/* ---------------------------------------------------------------------- */

spocp_result_t bcond_check( element_t *ep, index_t *id, octarr_t **oa )
{
  int            i ;
  spocp_result_t r = SPOCP_DENIED ;
  ruleinst_t     *ri = 0 ;
  char           *str ;

  /* find the top */
  while( ep->memberof ) ep = ep->memberof ; 

  for( i = 0 ; i < id->n ; i++ ) {
    ri = id->arr[i] ;

    if( ri->bcond == 0 ) {
      if( ri->blob ) *oa = octarr_add( *oa, octdup( ri->blob )) ;
      r = SPOCP_SUCCESS ;
      break ;
    }

    r = bcexp_eval( ep, ri->ep, ri->bcond->exp, oa ) ;
    DEBUG( SPOCP_DMATCH ) {
      traceLog( "boundary condition \"%s\" returned %d\n", ri->bcond->name, r ) ;
    }
    if( r == SPOCP_SUCCESS ) break ;
  }

  if( r == SPOCP_SUCCESS ) { /* if so ri has to be defined */
    str = oct2strdup( ri->rule, '%' ) ;
    traceLog( "Matched rule \"%s\"",str ) ;
    free( str ) ;
  }

  return r ;
}

/* If it's a reference to a boundary condition it should be of the
   form (ref <name>)
*/
spocp_result_t is_bcref( octet_t *o, octet_t *res )
{
  octet_t lc ;
  octet_t op ;
  spocp_result_t r ;

  if( *o->val == '(' ) {

    octln( &lc, o ) ;
    lc.val++ ;
    lc.len-- ;

    if(( r = get_str( &lc, &op )) != SPOCP_SUCCESS ) return r ;
    if( oct2strcmp( &op, "ref" ) != 0 ) return SPOCP_SYNTAXERROR ;

    if(( r = get_str( &lc, &op )) != SPOCP_SUCCESS ) return r ;
    if(!( *lc.val == ')' && lc.len == 1 )) return SPOCP_SYNTAXERROR ;

    octln( res, &op ) ;

    return SPOCP_SUCCESS ;
  }

  return SPOCP_SYNTAXERROR ;
}

bcdef_t *bcdef_get( octet_t *o, db_t *db, spocp_result_t *rc )
{
  bcdef_t       *bcd = 0 ;
  spocp_result_t r ;
  octet_t        br ;
 
  if( oct2strcmp( o, "NULL" ) == 0 ) bcd = NULL ;
  else if(( r = is_bcref( o, &br )) == SPOCP_SUCCESS ) {
    bcd = bcdef_find( db->bcdef, &br ) ;
    if( bcd == NULL ) *rc = SPOCP_MISSING_ARG ;
  }
  else {
    bcd = bcdef_add( db->plugins, 0, o, &db->bcdef ) ;
    if( bcd == 0 ) *rc = SPOCP_SYNTAXERROR ;
  }

  return bcd ;
}
