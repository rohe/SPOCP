/*! \file lib/aci.c
 *  \author Roland Hedberg <roland@catalogix.se>
 */
#include <string.h>

#include <spocp.h>
#include <func.h>
#include <struct.h>
#include <db0.h>
#include <wrappers.h>
#include <macros.h>

char *actions[6] = { "compare", "LIST", "ADD", "DELETE", "check", NULL } ;

/*!
 * A successfull matching of a query against the ruledatabase returns a pointer
 * to a junction where one of the branches is a array of pointer to rule instances.
 * Presently this array will never contain more than one pointer, since the software
 * does not allow several identical rules to be added. The possible static return 
 * information is not included when uniqueness is checked. So two 'rules' with the
 * same rightsdefinition but different static return information can not be added.
 * \param ap A pointer to the junction where the endoflist array are
 * \return on success a pointer to the first element in the endoflist array 
 *         otherwise 0
 */
ruleinst_t *allowing_rule( junc_t *ap )
{
  junc_t     *nap ;
  branch_t   *dbp, *ndb ;

  if(( dbp = ARRFIND( ap, SPOC_ENDOFLIST))) {
    nap = dbp->val.list ;

    while(( ndb = ARRFIND( nap, SPOC_ENDOFLIST))) {
      dbp = ndb ;
      nap = dbp->val.list ;
    }

    if(( dbp = ARRFIND( dbp->val.list, SPOC_ENDOFRULE ))) {
      if( dbp->val.id->n ) return dbp->val.id->arr[0] ;
    }
  }
  else if(( dbp = ARRFIND( ap, SPOC_ENDOFRULE ))) {
    if( dbp->val.id->n ) return dbp->val.id->arr[0] ;
  }

  return 0 ;
}

/*!
 * The function which is the starting point for access control, that is matching the query * against a specific ruleset. 
 * \param ap A pointer to the start of the ruletree
 *        ep A pointer to the parsed query s-expression
 *        gap An array of variables gotten from the query and for usage in 
 *            variable substitions before running eventual backends.
 *        bcp A pointer to the pool of connections to information sources used by the
 *            backends
 *        on  A handle that should be used when blobs are to be returned
 * \return SPOCP_SUCCESS on success otherwise an appropriate error code
 */
spocp_result_t allowed(
  junc_t *ap,
  element_t *ep,
  parr_t *gap,
  becpool_t *bcp,
  octnode_t **on )
{  
  ruleinst_t    *rt = 0 ;
  spocp_result_t rc = SPOCP_SUCCESS, res = SPOCP_DENIED ;
  size_t         size ;

  if(( ap = element_match_r( ap, ep, gap, bcp, &rc, on ))) {
   
    rt = allowing_rule( ap ) ;

    if( rt ) {
      traceLog( "ALLOWED by %s", rt->uid ) ;

      res = SPOCP_SUCCESS ;

      if( rt->blob ) {
        size = rt->blob->size ;
        /* to protect against deleting */
        rt->blob->size = 0 ;
        *on = octnode_add( *on, rt->blob ) ;
        rt->blob->size = size ;
      }
    }
  }
  else if( rc != SPOCP_SUCCESS ) res = rc ;

  return res ;
}

/*!
 * Stores a Access control information for the Spocp rules
 * \param db A pointer to the database structure
 *        ep A pointer to the parsed S-expression
 *        rt A pointer to where the information about the rule is stored
 * \return SPOCP_SUCCESS on success otherwise relevant error code
 */
static spocp_result_t aci_store( db_t *db, element_t *ep, ruleinst_t *rt )
{
  spocp_result_t  r = SPOCP_SUCCESS ;

  if( db->acit == 0 ) db->acit = junc_new() ;

  if( element_add( db->plugins, db->acit, ep, rt ) == 0 ) r = SPOCP_OPERATIONSERROR ;

  return r ;
}

/*! 
 * \param
 * \result
 */

spocp_result_t aci_add( db_t **db, octet_t *arg, spocp_req_info_t *sri )
{
  octet_t        rule, *a[2] ;
  element_t      *ep ;
  ruleinst_t     *rt = 0 ;
  spocp_result_t sr = SPOCP_SUCCESS ;
  
  a[0] = arg ;
  a[1] = 0 ;

  if(( sr = allowed_by_aci( *db, a, sri, "ADD" )) != SPOCP_SUCCESS ) {
    return sr ;
  } 

  if( *db == 0 ) {
    *db = db_new( ) ;
  }

  /* decouple */
  rule.val = arg->val ;
  rule.len = arg->len ;

  if(( sr = element_get( arg, &ep )) == SPOCP_SUCCESS  ) {

    if( ep == 0 ) 
      sr = SPOCP_SYNTAXERROR ;
    else {
      rule.len -= arg->len ;

      rt = aci_save( *db, &rule ) ;

      sr = aci_store( *db, ep, rt ) ;  
    }
  }

  /* ----------------------------- */

  return sr ;
}

/*! 
 * \param
 * \result
 */
/* creates a element to check agains the ACI rules */
/* do I have to go over creating the string representation, probably not 
   but it's easier   
   The string representation looks like this 
   (3:aci(8:resource arg)(6:action ... )(7:subject ... ))

   If action == 'add' the arg[0] is the rule sexp and arg[1] is if present the blob
   If action == 'delete' or 'list' then arg[0] is the rule sexp

   (3:aci(8:resource)(6:action)(7:subject)) - shortest possible, length = 40
*/

static spocp_result_t aci_sexp( octet_t **arg, octet_t *subject, char *action, element_t **ep )
{
  size_t         i, l, ls, la, dl ;
  char           *str, len[16] ;
  octet_t        sexp ;
  spocp_result_t rc ;

  for( i = 0, l = 0, dl = 0 ; arg[i] ; i++ ) {
    l += arg[i]->len ;
    dl += DIGITS( arg[i]->len ) + 1 ;
  }

  la = strlen( action ) ;
  dl += DIGITS( la ) ;

  if( subject->len ) { 
    ls = subject->len ;
    dl += DIGITS( ls ) + 1 ;
  }
  else ls = 0 ;

  sexp.size = l + ls + la + 41 + dl ;

  sexp.val = str = (char *) Calloc( sexp.size+1, sizeof( char )) ;
  
  sprintf(str, "(3:aci(8:resource" ) ;
  str += strlen( str ) ;

  /* no length on the rule argument since it's handled as a s-expr */
  memcpy( str, arg[0]->val, arg[0]->len ) ;
  str += arg[0]->len ;

  /* the following arguments is to be treated as strings */
  for( i = 1 ; arg[i] ; i++ ) {
    sprintf( len, "%d:", arg[i]->len ) ;
    l = strlen(len) ;
    memcpy( str, len, l ) ;
    str +=  l ;
    memcpy( str, arg[i]->val, arg[i]->len ) ;
    str += arg[i]->len ;
  }
  *str++ = ')' ; /* end of resource part */

  /* action part */
  sprintf(str, "(6:action%d:%s)", la, action ) ;
  str += la + 11 + DIGITS( la ) ;

  /* and finally the subject part 
     Now is the subject a atom or a s-expr ? 
     I treat it as a s-expr 
   */
  sprintf(str, "(7:subject") ;
  str += 10 ;
  if( subject->len ) {
    memcpy( str, subject->val, subject->len ) ; 
    str += subject->len ;
  }
  *str++ = ')' ;  /* end of subject part */
  *str++ = ')' ;  /* end of the whole sexp */

  sexp.len = str - sexp.val ;

  DEBUG( SPOCP_DPARSE ) {
    char *tmp ;
    tmp = oct2strdup( &sexp, '\\' ) ;
    traceLog( "aci_sexp: [%s]", tmp ) ;
    free( tmp ) ;
  }

  str = sexp.val ;

  rc = element_get( &sexp, ep ) ;

  free( str ) ;

  return rc ;
}

/*! 
 * \param
 * \result
 */

int no_branch( junc_t *jp )
{
  int i ; 

  for( i = 0 ; i < NTYPES ; i++ ) if(jp->item[i]) return 0 ;

  return 1 ;
}

/*! 
 * \param
 * \result
 */

spocp_result_t allowed_by_aci( db_t *db, octet_t **arg, spocp_req_info_t *sri, char *action )
{
  element_t      *ep = 0 ;
  spocp_result_t rc = SPOCP_SUCCESS ;
  octnode_t      *on = 0 ;

  /* If no ACI's at all everythings allowed
     'root' access marked by there being no requester info */
  if( db == 0 || db->raci == 0 || sri == 0 ) return SPOCP_SUCCESS ;

  if(( rc = aci_sexp( arg, &sri->subject, action, &ep )) != SPOCP_SUCCESS ) {
    return rc ;
  }
  
  /* Could there be any blobs returned ?? */
  rc = allowed( db->acit, ep, 0, db->bcp, &on ) ;

  if( on ) octnode_free( on ) ;

  return rc ;
}

