/***************************************************************************

               spocp2.c  -  contains the public interface to spocp

                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include "locl.h"

int ruleset_print( ruleset_t *rs )  ;

static spocp_result_t
rec_allow( ruleset_t *rs, element_t *ep, int scope, octarr_t **on )
{
  spocp_result_t res = SPOCP_DENIED ;
  ruleset_t      *trs ;

  switch( scope ) {
    case SUBTREE :
      if( rs->down ) {
        /* go as far to the left as possible */
        for( trs = rs->down ; trs->left ; trs = trs->left ) ;
   
        /* and over to the right side passing every node on the way */
        for(  ; trs && res != SPOCP_SUCCESS ; trs = trs->right ) 
          res = rec_allow( trs, ep, scope, on ) ;
      }

    case BASE :
      if( rs->db ) res = allowed( rs->db->jp, ep, on ) ;
      break ;
  
    case ONELEVEL :
      if( rs->down ) {
        for( trs = rs->down ; trs->left ; trs = trs->left ) ;
   
        for(  ; trs && res != SPOCP_SUCCESS ; trs = trs->right ) {
          if( rs->db ) res = allowed( trs->db->jp, ep, on ) ;
        }
      }
      break ;

  }
 
  return res ;
}

spocp_result_t skip_sexp( octet_t *sexp )
{
  spocp_result_t r ;
  element_t      *ep = 0 ;
  char           *str ;

  if(( r = element_get( sexp, &ep )) != SPOCP_SUCCESS ) {
    str = oct2strdup( sexp, '%' ) ;
    traceLog("The S-expression \"%s\" didn't parse OK", str ) ;
    free( str ) ;
  }

  element_free( ep ) ;

  return r ;
}

/*
   returns
   -1 parsing error
    0 access denied
    1 access allowed
 */

spocp_result_t ss_allow( ruleset_t *rs, octet_t *sexp, octarr_t **on, int scope )
{
  spocp_result_t res = SPOCP_DENIED ; /* this is the default */
  element_t     *ep = 0 ;
  octet_t        oct, dec ;
  char          *str ;

  if( rs == 0 || sexp == 0 || sexp->len == 0 ) {
    if( rs == 0 ) { LOG(SPOCP_EMERG) traceLog("Ain't got no rule database") ; }
    else { LOG(SPOCP_ERR) traceLog("Blamey no S-expression to check, oh well") ; }
    return SPOCP_DENIED ;
  }

  /* ruledatabase but no rules ? */
  if( rules( rs->db ) == 0 && rs->down == 0 ) {
    traceLog( "Rule database but no rules" ) ;
    return SPOCP_DENIED ;
  }

  DEBUG(SPOCP_DPARSE) {
    char *str ;
    str = oct2strdup( sexp, '\\' ) ;
    traceLog( "Parsing the S-expression \"%s\"", str ) ;
    free( str ) ;
  }

  octln( &oct, sexp ) ;
  octln( &dec, sexp ) ;

  if(( res = element_get( &dec, &ep )) != SPOCP_SUCCESS ) {
    str = oct2strdup( &oct, '\\' ) ;
    traceLog("The S-expression \"%s\" didn't parse OK", str ) ;
    free( str ) ;
    
    return res ;
  }

  /* just ignore extra bytes */
  oct.len -= dec.len ;
  str = oct2strdup( &oct, '\\' ) ;
  if( 0 ) traceLog("Query: \"%s\" in \"%s\"", str, rs->name ) ;
  free( str ) ;

  res = rec_allow( rs, ep, scope, on ) ;

  element_free( ep ) ;

  return res ;
}

void free_db( db_t *db )
{
  if( db ) {
    free_all_rules( db->ri ) ;
    junc_free( db->jp ) ;
    free( db ) ;
  }
}

void ss_del_db( ruleset_t *rs, int scope )
{
  ruleset_t *trs ;

  if( rs == 0 ) return ;

  switch( scope ) {
    case SUBTREE :
      if( rs->down ) {
        for( trs = rs->down ; trs->left ; trs = trs->left ) ;
   
        for( ; trs ; trs = trs->right ) 
          ss_del_db( trs, scope ) ;
      }

    case BASE :
      free_db(( db_t * ) rs->db )  ;
      rs->db = 0 ;
      break ;
    
  
    case ONELEVEL :
      if( rs->down ) {
        for( trs = rs->down ; trs->left ; trs = trs->left ) ;
   
        for(  ; trs ; trs = trs->right ) {
          free_db(( db_t * ) trs->db) ;
          trs->db = 0 ;
        }
      }
      break ;

  }
}

/***************************************************************

 Structure of the rule file:

 line    = ( comment | global | ruledef | include )
 comment = "#" text CR
 rule    = "(" S_exp ")" [; blob] CR
 global  = key "=" value
 include = ";include" filename

 ***************************************************************/

void *ss_get_rules( ruleset_t *rs, char *file, int *rc  )
{
  keyval_t *globals = 0  ;

  return (void *) read_rules( rs, file, rc, &globals ) ;
}

/* ------------------------------------------------------------ */

static spocp_result_t rec_del( ruleset_t *rs, char *uid, size_t *nr )
{
  ruleset_t      *trs ;
  db_t           *db ;
  ruleinst_t     *rt ;
  spocp_result_t rc = SPOCP_SUCCESS ;

  db = rs->db ;

  if( db == 0 ) return 0 ;

  if(( rt = get_rule( db->ri, uid ))) {
    if(( rc = rm_rule( db->jp, rt->rule, rt )) != SPOCP_SUCCESS ) return rc ;

    if( free_rule( db->ri, uid ) == 0 ) {
      traceLog( "Hmm, something fishy here, couldn't delete rule" ) ;
    }
    (*nr)++ ;
  } 

  if( rs->down ) {

    for( trs = rs->down ; trs->left ; trs = trs->left ) ;
   
    for( ; trs ; trs = trs->right )  
      if(( rc = rec_del( trs, uid, nr )) != SPOCP_SUCCESS ) return rc ;
  }

  return rc ;
}

spocp_result_t ss_del_rule( ruleset_t *rs, octet_t *op, int scope )
{
  size_t    n = 0 ;
  char      uid[SHA1HASHLEN+1], *sp ;

  if( rs == 0 ) return SPOCP_SUCCESS ;

  if( op->len != SHA1HASHLEN ) return SPOCP_PROTOCOLERROR ;

  /* checking the if which is supposed to be a sha1 hashvalue */
  for( n = 0, sp = op->val ; HEX(*sp) && n < op->len ; sp++, n++ ) ; 

  if( n != 40 ) {
    traceLog( "Rule identifier error, wrong length" ) ;
    return SPOCP_MISSING_ARG ;
  }

  memcpy( uid, op->val, SHA1HASHLEN ) ;
  uid[SHA1HASHLEN] = '\0' ;

  traceLog( "Attempt to delete rule: \"%s\"", uid ) ;

  /* first check that the rule is there */

  n = 0 ;
  if( rec_del( rs, uid, &n ) != SPOCP_SUCCESS ) 
    traceLog( "Error while deleting rules" ) ;

  if( n > 1 )       traceLog( "%d rules successfully deleted", n ) ;
  else if( n == 1 ) traceLog( "1 rule successfully deleted" ) ;
  else              traceLog( "No rules deleted" ) ;

  return SPOCP_SUCCESS ;
}

/* --------------------------------------------------------------------------*/

/*
static octet_t **list_rules( db_t *db, octet_t *pattern, int *rc, int scope )
{
  if( pattern == 0 || pattern->len == 0 ) {
    return get_all_rules( db->jp, db->ri );
  }
  else
    return get_matching_rules( db->jp, db->ri, pattern, rc ) ;
}

static octet_t **
rec_list_rules( ruleset_t *rs, octet_t *pattern, spocp_req_info *sri, int *rc, int scope )
{
  octet_t **res = 0, **arr ;
  ruleset_t *trs ;

  switch( scope ) {
    case SUBTREE :
      for( trs = rs->down ; trs->left ; trs = trs->left ) ;
   
      for( ; trs && ! res ; trs = trs->right ) {
        arr = rec_list_rules( trs, pattern, rc, scope ) ;
        res = join_octarr( res, arr ) ;
      }

    case BASE :
      arr = list_rules( rs->db, pattern, rc, scope ) ;
      res = join_octarr( res, arr ) ;
      break ;

    case ONELEVEL :
      for( trs = rs->down ; trs->left ; trs = trs->left ) ;
   
      for( ; trs && ! res ; trs = trs->right ) {
        arr = list_rules( trs->db, pattern, rc, scope ) ;
        res = join_octarr( res, arr ) ;
      }
      break ;
  }

  return res ;
}

octet_t **
ss_list_rules( ruleset_t *rs, octet_t *pattern, spocp_req_info *sri, int *rc, int scope ) 
{
  *rc = 1 ;

  LOG( SPOCP_INFO ) traceLog( "List rules" ) ;

  return  rec_list_rules( rs, pattern, sri, rc, scope ) ;

}
*/
/* --------------------------------------------------------------------------*/

/*
spocp_result_t ss_add_rule( ruleset_t *rs, octarr_t *oa, bcdef_t *bcd )
{
  spocp_result_t  r ;
  ruleinst_t     *ri = 0 ;

  r = add_right( &(rs->db), oa, &ri ) ;
  
  if( ri && bcd ) {
    ri->bcond = bcd ;
    bcd->rules = varr_add( bcd->rules, (void *) ri ) ;
  }

  return r;
}
*/

/* --------------------------------------------------------------------------*/

static int rec_rules( ruleset_t *rs, int scope )
{
  int         n = 0 ;
  ruleset_t *trs ;

  switch( scope ) {
    case SUBTREE :
      if( rs->down ) {
        for( trs = rs->down ; trs->left ; trs = trs->left ) ;
   
        for( ; trs ; trs = trs->right ) n += rec_rules( trs, scope ) ;
      }

    case BASE :
      n += nrules( rs->db->ri ) ;
      break ;

    case ONELEVEL :
      if( rs->down ) {
        for( trs = rs->down ; trs->left ; trs = trs->left ) ;
   
        for( ; trs ; trs = trs->right ) 
          n += nrules( trs->db->ri ) ;
      }
      break ;
  }

  return n ;
}

int ss_rules( ruleset_t *rs, int scope )
{
  return rec_rules( rs, scope ) ;
}

/* --------------------------------------------------------------------------*/
/* almost the same as spocp_dup, so I shold only have one of them */

db_t *db_dup( db_t *db ) 
{
  db_t *new ;

  if( db == 0 ) return 0 ;

  new = (db_t *) calloc ( 1, sizeof( db_t * )) ;

  if( new == 0 ) {
    LOG( SPOCP_ERR ) traceLog( "Memory allocation problem" ) ;
    return 0 ;
  }

  new->ri = ruleinfo_dup( db->ri ) ;
  new->jp = junc_dup( db->jp, new->ri ) ;
  
  return new ;
}

/*
regexp_t *regexp_dup( regexp_t *rp )
{
  return new_pattern( rp->regex ) ;
}

aci_t *aci_dup( aci_t *ap )
{
  aci_t *new ;

  if( ap == 0 ) return 0 ;

  new = (aci_t *) Calloc( 1, sizeof( aci_t )) ;

  new->access = ap->access ;
  new->string = Strdup(ap->string) ;
  new->net    = Strdup(ap->net) ;
  new->mask   = Strdup(ap->mask) ;
  new->cert   = regexp_dup( ap->cert ) ;
  
  if( ap->next ) new->next = aci_dup( ap->next ) ; 

  return new ;
}
*/

ruleset_t *ruleset_dup( ruleset_t *rs )
{
  ruleset_t *new ;
  octet_t   loc ;

  if( rs == 0 ) return 0 ;
  
  loc.val = rs->name ;
  loc.len = strlen(rs->name) ;
  new = ruleset_new( &loc ) ;

  new->db  = db_dup( rs->db ) ;

  return new ;
}

static ruleset_t *rec_dup_rules( ruleset_t *rs, int scope )
{
  ruleset_t *trs = 0, *drs = 0, *tmp = 0, *lrs = 0 ;

  switch( scope ) {
    case SUBTREE :
      if( rs->down ) {
        for( trs = rs->down ; trs->left ; trs = trs->left ) ;
   
        for( ; trs ; trs = trs->right ) {
          tmp = rec_dup_rules( trs, scope ) ;
          if( lrs == 0 ) lrs = drs = tmp ;
          else {
            drs->right = tmp ;
            tmp->left = drs ; 
            drs = tmp ;
          }
        }
        drs = lrs ;
      }

    case BASE :
      drs = ruleset_dup( rs ) ;
      if( lrs ) {
        drs->down = lrs ;
        for( ; lrs ; lrs = lrs->right ) lrs->up = drs ;
      }
      break ;

    case ONELEVEL :
      if( rs->down ) {
        for( trs = rs->down ; trs->left ; trs = trs->left ) ;
   
        for( ; trs ; trs = trs->right ) {
          drs = ruleset_dup( trs ) ;
          if( lrs == 0 ) lrs = drs = tmp ;
          else {
            drs->right = tmp ;
            tmp->left = drs ; 
            drs = tmp ;
          }
        }
        drs = lrs ;
      }
      break ;
  }

  return drs ;
}

void *ss_dup( ruleset_t *rs, int scope ) 
{
  return rec_dup_rules( rs, scope ) ;
}

/* --------------------------------------------------------- */

int print_db( db_t *db ) 
{
  if( db == 0 ) return 0 ;

  /* print_junc( db->jp ) ; */
  if( db->ri ) ruleinfo_print( db->ri ) ;

  return 0 ;
}

int ruleset_print_r( ruleset_t *rs ) 
{
  for( ; rs->left ; rs = rs->left ) ;

  for( ; rs ; rs = rs->right ) ruleset_print( rs ) ;

  return 0 ;
}

int ruleset_print( ruleset_t *rs ) 
{
  LOG( SPOCP_DEBUG ) traceLog( "rulesetname: \"%s\"", rs->name ) ;
  print_db( rs->db ) ;
  /* print_aci( rs->aci ) ; */
  if( rs->down ) ruleset_print_r( rs->down ) ;
 
  return 0 ;
}

