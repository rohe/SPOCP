/***************************************************************************

                spocp.c  -  contains the public interface to spocp

                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdlib.h>
#include <config.h>
#include <string.h>

#include <macros.h>
#include <struct.h>
#include <func.h>
#include <spocp.h>
#include <wrappers.h>
#include <proto.h>

/*
   returns
 */

spocp_result_t spocp_allowed( void *vp, octet_t *sexp, octnode_t **on, spocp_req_info_t *sri )
{
  db_t       *db = ( db_t *) vp ;
  junc_t     *ap ;
  parr_t     *gap = 0 ;
  element_t  *ep = 0 ;
  int        i ;
  octet_t    oct ;
  char       *str ;
  spocp_result_t res = SPOCP_SUCCESS ;

  if( vp == 0 || sexp == 0 || sexp->len == 0 ) {
    if( vp == 0 ) { LOG(SPOCP_EMERG) traceLog("Ain't got no rule database") ; }
    else { LOG(SPOCP_ERR) traceLog("Blamey no S-expression to check, oh well") ; }
    return res ;
  }

  ap = db->jp  ;

  DEBUG(SPOCP_DPARSE) {
    char *str ;
    str = oct2strdup( sexp, '%' ) ;
    traceLog( "Parsing the S-expression \"%s\"", str ) ;
    free( str ) ;
  }

  oct.len = sexp->len ;
  oct.val = sexp->val ;

  if(( res = element_get( sexp, &ep )) != SPOCP_SUCCESS ) {
    str = oct2strdup( sexp, '%' ) ;
    traceLog("The S-expression \"%s\" didn't parse OK", str ) ;
    free( str ) ;
    
    return res ;
  }

  oct.len -= sexp->len ;
  DEBUG(SPOCP_DPARSE) {
    str = oct2strdup( &oct, '%' ) ;
    traceLog("Query: \"%s\"", str) ;
    free( str ) ;
  }

  DEBUG(SPOCP_DPARSE) traceLog("Parsed OK, getting AV list") ;
 
  /* collect tag value pairs, used for the substitutions in
     constraint references. Might be none */
  gap = get_simple_lists( ep, 0 ) ;

  DEBUG(SPOCP_DPARSE) {
    keyval_t *kv = 0 ;

    for( i = 0 ; gap && i < gap->n ; i++ ) {
      kv = ( keyval_t * ) gap->vect[i] ;
      str = oct2strdup( kv->key, '\\' ) ;
      traceLog( "==%s", str ) ;
      free( str ) ;
      str = oct2strdup( kv->val, '\\' ) ;
      traceLog( "==>%s", str ) ;
      free( str ) ;
    }
  }

  res = allowed( ap,  ep, gap, db->bcp, on ) ;

  element_free( ep ) ;
  if( gap ) parr_free( gap ) ;

  return res ;
}

void spocp_del_database( void *vp )
{
  db_t  *db = (db_t *) vp ;

  free_all_rules( db->ri ) ;
  junc_free( db->jp ) ;
  free( db ) ;
}

/***************************************************************

 Structure of the rule file:

 line    = ( comment | global | ruledef | include )
 comment = "#" text CR
 rule    = "(" S_exp ")" [; blob] CR
 global  = key "=" value
 include = ";include" filename

 ***************************************************************/

/*
void *spocp_get_rules( void *vp, char *file, int *rc  )
{
  keyval_t *globals = 0  ;

  return (void *) read_rules( vp, file, rc, &globals ) ;
}
*/

spocp_result_t spocp_del_rule( void *vp, octet_t *op, spocp_req_info_t *sri )
{
  db_t           *db = ( db_t *) vp ;
  int            n ;
  ruleinst_t     *rt ;
  char           uid[41], *sp ;
  spocp_result_t rc ;

  if( op->len < 40 ) return SPOCP_SYNTAXERROR ;

  for( n = 0, sp = op->val ; HEX(*sp) ; sp++, n++ ) ; 

  if( n != 40 ) return SPOCP_SYNTAXERROR ;

  memcpy( uid, op->val, 40 ) ;
  uid[40] = '\0' ;

  traceLog( "Attempt to delete rule: \"%s\"", uid ) ;

  op->val += 40 ;
  op->len -= 40 ;

  /* first check that the rule is there */

  if(( rt = get_rule( db->ri, uid )) == 0 ) {
    traceLog( "Deleting rule \"%s\" impossible since it doesn't exist", uid ) ;
    
    return SPOCP_SYNTAXERROR ;
  }       

  rc = rm_rule( db->jp, rt->rule, rt ) ;

  free_rule( db->ri, uid ) ;

  if( rc == SPOCP_SUCCESS ) traceLog( "Rule successfully deleted" ) ;

  return rc ;
}

spocp_result_t
spocp_list_rules( void *vp, octet_t *pattern, octarr_t *oa, spocp_req_info_t *sri, char *rs ) 
{
  db_t *db = ( db_t *) vp ;

  /* LOG( SPOCP_INFO ) traceLog( "List rules" ) ; */

  if( pattern == 0 || pattern->len == 0 ) {
    /* get all */
    return get_all_rules( db, oa, sri, rs );
  }
  else
    return get_matching_rules( db, pattern, oa, sri, rs ) ;
}

spocp_result_t spocp_add_rule( void **vp, octet_t **arg, spocp_req_info_t *sri )
{
  spocp_result_t r ;
  db_t          *db ;
  ruleinst_t    *ri = 0 ;

  LOG( SPOCP_INFO ) traceLog( "spocp_add_rule" ) ; 

  if( vp ) db = (db_t *) *vp ;
  else return SPOCP_UNWILLINGTOPERFORM ;

  r = add_right( &db, arg, sri, &ri ) ;
  
  *vp = (void *) db ;
 
  return r;
}

int spocp_rules( void *vp )
{
  db_t  *db = (db_t *) vp ;
  
  return nrules( db->ri ) ;
}

void *spocp_dup( void *vp, spocp_result_t *r ) 
{
  db_t *db = (db_t *) vp ;
  db_t *new ;

  *r = SPOCP_SUCCESS ;

  if( vp == 0 ) {
    return 0 ;
  }

  new = (db_t *) calloc ( 1, sizeof( db_t * )) ;

  if( new == 0 ) {
    LOG( SPOCP_ERR ) traceLog( "Memory allocation problem" ) ;
    *r = SPOCP_NO_MEMORY ;
    return 0 ;
  }

  new->ri = ruleinfo_dup( db->ri ) ;
  new->jp = junc_dup( db->jp, new->ri ) ;
  
  return new ;
}
