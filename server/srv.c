/***************************************************************************
                           n_srv.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 **************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pthread.h>
#include <string.h>

#include <struct.h>
#include <spocp.h>
#include <func.h>
#include <proto.h>
#include <macros.h>
#include <wrappers.h>

#include <srv.h>

char *rc_ok = "3:2002:Ok" ;
char *rc_matched = "3:2017:Matched" ;
char *multiline = "3:201" ;
char *rc_denied = "3:2026:Denied" ;
char *rc_bye = "3:2033:Bye" ;
char *rc_transaction_complete = "3:20420:Transaction complete" ;
char *rc_service_not_available = "3:40121:Service not available" ;
char *rc_information_unavailable = "3:40223:Information unavailable" ;
char *rc_syntax_error = "3:50012:Syntax error" ;
char *rc_operation_error = "3:50115:Operation error" ;
char *rc_not_supported = "3:50213:Not supported" ;
char *rc_in_operation = "3:50320:Already in operation" ;
char *rc_too_many_arg = "3:50418:Too many arguments" ;
char *rc_unknown_id = "3:50510:Unknown ID" ;
char *rc_exists = "3:50614:Already exists" ;
char *rc_line_to_long = "3:50712:Line to long" ;
char *rc_unknown_command = "3:50815:Unknown command" ;
char *rc_access_denied = "3:50913:Access denied" ;
char *rc_argument_err = "3:51014:Argument error" ;
char *rc_already_active = "3:51114:Already active" ;
char *rc_internal_error = "3:51214:Internal error" ;
char *rc_input_error = "3:51311:Input error" ;
char *rc_protocol_error = "3:51314:Protocol error" ;
char *rc_timelimitexceeded = "3:51418:Timelimit exceeded" ;
char *rc_sizelimitexceeded = "3:51518:Sizelimit exceeded" ;
char *rc_other = "3:51611:Other error" ;

/***************************************************************************
 ***************************************************************************/


void add_response( spocp_iobuf_t *out, int rc )
{
  if( rc == SPOCP_SUCCESS ) {
    add_to_iobuf( out, rc_ok ) ;
  }
  else if( rc == SPOCP_DENIED ) {
    add_to_iobuf( out, rc_denied ) ;
  }
  else if( rc == SPOCP_CLOSE ) {
    add_to_iobuf( out, rc_bye ) ;
  }
  else if( rc == SPOCP_EXISTS ) {
    traceLog( "ERR: Already exists" ) ;
    add_to_iobuf( out, rc_exists ) ;
  }
  else if( rc == SPOCP_OPERATIONSERROR ) {
    traceLog( "ERR: Operation error" ) ;
    add_to_iobuf( out, rc_operation_error ) ;
  }
  else if( rc == SPOCP_PROTOCOLERROR ) {
    traceLog( "ERR: Protocol error" ) ;
    add_to_iobuf( out, rc_protocol_error ) ;
  }
  else if( rc == SPOCP_UNKNOWNCOMMAND ) {
    traceLog( "ERR: Unknown command" ) ;
    add_to_iobuf( out, rc_unknown_command ) ;
  }
  else if( rc == SPOCP_SYNTAXERROR ) {
    traceLog( "ERR: Syntax error" ) ;
    add_to_iobuf( out, rc_syntax_error ) ;
  }
  else if( rc == SPOCP_TIMELIMITEXCEEDED ) {
    traceLog( "ERR: Timelimit exceeded" ) ;
    add_to_iobuf( out, rc_timelimitexceeded ) ;
  }
  else if( rc == SPOCP_SIZELIMITEXCEEDED ) {
    traceLog( "ERR: Sizelimit exceeded" ) ;
    add_to_iobuf( out, rc_sizelimitexceeded ) ;
  }
  else if( rc == SPOCP_UNAVAILABLE ) {
    traceLog( "ERR: Unknown ID" ) ;
    add_to_iobuf( out, rc_unknown_id ) ;
  }
  else if( rc == SPOCP_MISSING_ARG) {
    traceLog( "ERR: Argument error" ) ;
    add_to_iobuf( out, rc_argument_err ) ;
  }
  else if( rc == SPOCP_MISSING_CHAR ) {
    traceLog( "ERR: Missing bytes in input" ) ;
    add_to_iobuf( out, rc_input_error ) ;
  }
  else if( rc == SPOCP_OTHER ) {
    traceLog( "ERR: Other error" ) ;
    add_to_iobuf( out, rc_other ) ;
  }
  else if( rc == SPOCP_BUF_OVERFLOW ) {
    traceLog( "ERR: Buffer overflow" ) ;
    add_to_iobuf( out, rc_operation_error ) ;
  }
  else {
    traceLog( "ERR: errortype [%d]", rc ) ;
    add_to_iobuf( out, rc_operation_error ) ;
  }
}

/*
static int sexp_err( conn_t *conn, int l ) 
{
  if( l != conn->oparg.len ) 
    traceLog( "error in received data") ; 
  else 
    traceLog( "Syntax error in S-EXP ") ; 

  shift_buffer( conn->in ) ;

  add_to_iobuf( conn->out, rc_syntax_error ) ;
  send_results( conn ) ;

  return -1 ;
}
*/

/* --------------------------------------------------------------------------------- */

spocp_result_t com_starttls( conn_t *conn )
{
  spocp_result_t  r = SPOCP_SUCCESS ;
  int             wr ;
  spocp_iobuf_t  *out = conn->out ;

  LOG( SPOCP_INFO ) traceLog("Attempting to start SSL/TLS") ;

#ifndef HAVE_LIBSSL

  add_to_iobuf( out, rc_not_supported ) ;
  LOG( SPOCP_ERR ) traceLog("SSL not supported") ;
  wr = send_results( conn ) ;
  return SPOCP_CLOSE ;

#else

  if( conn->tls >= 0) {
    LOG( SPOCP_ERR ) traceLog("SSL already in operation") ;
    add_to_iobuf( out, rc_in_operation ) ;
  }
  else {
    add_to_iobuf( out, rc_ok ) ;
  }

  if(( wr = send_results( conn )) == 0 ) return SPOCP_CLOSE ;

  /* what ever is in the input buffert must not be used anymore */
  clear_buffers( conn ) ;

  /* If I couldn't create a TLS connection fail completely */
  if((r = tls_start( conn, rs )) != SPOCP_SUCCESS ) {
    LOG( SPOCP_ERR ) traceLog("Failed to start SSL") ;
    r = SPOCP_CLOSE ;
  }
  else traceLog("SSL in operation") ;

  /* conn->tls is set by tls_start */
#endif

  return r ;
}

/* --------------------------------------------------------------------------------- */

spocp_result_t com_rollback( conn_t *conn )
{
  spocp_result_t  r = SPOCP_SUCCESS ;
  int             wr ;

  LOG( SPOCP_INFO ) traceLog("ROLLBACK") ;

  /* remove the copy I've been working on */
  /* conn->rs != conn->srv->root */
  ss_del_db( conn->rs , SUBTREE ) ;
  
  /* make a new copy of the existing database */
  pthread_rdwr_rlock( &conn->srv->root->rw_lock ) ;
  conn->rs = ss_dup( conn->srv->root, SUBTREE ) ;
  pthread_rdwr_runlock( &conn->srv->root->rw_lock ) ;

  add_response( conn->out, r ) ;
    
  if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;

  return r ;
}

/* --------------------------------------------------------------------------------- */

spocp_result_t com_logout( conn_t *conn )
{
  spocp_result_t  r = SPOCP_CLOSE ;
  int             wr ;

  LOG( SPOCP_INFO ) traceLog("LOGOUT requested ") ;

  add_response( conn->out, r ) ;
    
  if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;

  return r ;
}

/* --------------------------------------------------------------------------------- */

/* format of DELETE

  delete       = "6:DELETE" l-path length ":" ruleid
 
*/
spocp_result_t com_delete( conn_t *conn )
{
  spocp_result_t   r = SPOCP_SUCCESS ;
  int              wr ;
  ruleset_t        *rs = conn->rs, *trs ;
  spocp_req_info_t *sri = &conn->sri ;
  octet_t          id, rsn ;

  LOG( SPOCP_INFO ) traceLog("DELETE requested ") ;

  /* while within a transaction I have a local copy */
  if( conn->transaction ) rs = conn->rs ;
  else                    rs = conn->srv->root ;

  /* first argument might be ruleset name */
  if(( r = get_str( &conn->oparg, &rsn )) != SPOCP_SUCCESS ) goto D_DONE ;

  if( conn->oparg.len != 0 ) { /* more arguments */
    if(( r = get_str( &conn->oparg, &id )) != SPOCP_SUCCESS ) goto D_DONE ;
  }
  else {
    id.len = rsn.len ;
    id.val = rsn.val ;
    rsn.len = 0 ;
    rsn.val = 0 ;
  }

  if(( trs = find_ruleset( &rsn, rs )) == 0 ) {
    char *str ;

    str = oct2strdup( &rsn, '%' ) ;
    traceLog("ERR: No \"/%s\" ruleset", str) ;
    free( str ) ;

    r = SPOCP_DENIED ;
  }
  else if(( r = rs_access_allowed( trs, sri, REMOVE )) == SPOCP_SUCCESS ) {
    /* get write lock, do operation and release lock */
    pthread_rdwr_wlock( &rs->rw_lock ) ;
    r = ss_del_rule( trs, &id, SUBTREE, sri ) ;
    pthread_rdwr_wunlock( &rs->rw_lock ) ;
  }

D_DONE :

  add_response( conn->out, r ) ;

  if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;
 
  return r ;
}

/* --------------------------------------------------------------------------------- */

spocp_result_t com_commit( conn_t *conn )
{
  spocp_result_t   r = SPOCP_SUCCESS ;
  int              wr ;
  ruleset_t        *rs ;

  LOG( SPOCP_INFO ) traceLog("COMMIT requested") ;
    
  if( !conn->transaction )  r = SPOCP_DENIED ;
  else {
    rs = conn->srv->root ;

    /* wait until the coast is clear */
    pthread_mutex_lock( &(conn->srv->mutex)) ;

    /* make the switch */
    conn->srv->root = conn->rs ;

    /* release the lock, so the new copy can start being used */
    pthread_mutex_unlock( &(conn->srv->mutex) ) ; 

    /* delete the old copy */
    ss_del_db( rs, SUBTREE ) ;
    
    conn->transaction = 0 ;
  }

  add_response( conn->out, r ) ;

  if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;
 
  return r ;
}

/* --------------------------------------------------------------------------------- */

spocp_result_t com_query( conn_t *conn )
{
  spocp_result_t    r = SPOCP_SUCCESS ;
  int               wr ;
  octnode_t        *on = 0, *non ;
  octet_t           rsn, sexp ;
  spocp_iobuf_t    *out = conn->out ;
  ruleset_t        *trs, *rs ;
  char             *str, buf[1024] ;
  spocp_req_info_t *sri = &conn->sri ;
  /*
  struct timeval   tv ;

  gettimeofday( &tv, 0 ) ;
  */

  if(0) timestamp( "QUERY" ) ;

  LOG( SPOCP_INFO ) traceLog("QUERY") ;

  /* while within a transaction I have a local copy */

  if( conn->transaction ) rs = conn->rs ;
  else                    rs = conn->srv->root ;

  /* first argument might be ruleset name */
  if(( r = get_str( &conn->oparg, &rsn )) != SPOCP_SUCCESS ) goto Q_DONE ;

  if( conn->oparg.len != 0 ) { /* more arguments */
    if(( r = get_str( &conn->oparg, &sexp )) != SPOCP_SUCCESS ) goto Q_DONE ;
  }
  else {
    sexp.len = rsn.len ;
    sexp.val = rsn.val ;
    rsn.len = 0 ;
    rsn.val = 0 ;
  }

  if(( trs = find_ruleset( &rsn, rs )) == 0 ) {
    str = oct2strdup( &rsn, '%' ) ;
    traceLog("ERR: No \"/%s\" ruleset", str) ;
    free( str ) ;
    r = SPOCP_DENIED ;
    goto Q_DONE ;
  }
    
  if(( r = get_pathname( trs, buf, 1024 )) != SPOCP_SUCCESS ) goto Q_DONE ;
  
  DEBUG( SPOCP_DPARSE ) traceLog( "Matching against ruleset \"%s\"", buf ) ;

  if(0) timestamp( "ss_allow" ) ;
  r = ss_allow( trs, &sexp, &on, sri, SUBTREE ) ;
  if(0) timestamp( "ss_allow done" ) ;

Q_DONE:
  if( r == SPOCP_SUCCESS ) {
    
    if( on ) {

      for( non = on ; non ; non = non->next ) {
        add_to_iobuf( out, multiline) ;
        add_bytestr_to_iobuf( out, &non->oct ) ;

        str = oct2strdup( &non->oct, '%' ) ;
        traceLog("returns \"%s\"", str) ;
        free( str ) ;

        if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;
      }

      octnode_free( on ) ;
    }
  }

  add_response( out, r ) ;
    
  shift_buffer( conn->in ) ;

  if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;
  if(0) timestamp( "Wrote result" ) ; 

  return r ;
}

/* --------------------------------------------------------------------------------- */

/*
int com_x( conn_t *conn, octet_t *com, octet_t *arg )
{
  int r = 0 ;

  return r ;
}
*/

/* --------------------------------------------------------------------------------- */

spocp_result_t com_login( conn_t *conn )
{
  spocp_result_t r = SPOCP_DENIED ;
  int            wr ;

  LOG( SPOCP_WARNING ) traceLog("LOGIN: not supported") ;

  add_response( conn->out, r ) ;

  if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;

  return r ;
}

/* --------------------------------------------------------------------------------- */

spocp_result_t com_begin( conn_t *conn )
{
  spocp_result_t    rc = SPOCP_SUCCESS ;
  int               wr ;
  ruleset_t        *rs = conn->srv->root ;

  LOG( SPOCP_INFO ) traceLog("BEGIN requested") ;
    
  if( conn->transaction ) rc = SPOCP_EXISTS ;
  else if(( rc = rs_access_allowed( rs, &conn->sri, BEGIN )) == SPOCP_SUCCESS ) {

    pthread_mutex_lock( &rs->transaction ) ;
    pthread_rdwr_rlock( &rs->rw_lock ) ;

    if(( conn->rs = ss_dup( rs, SUBTREE )) == 0 ) rc = SPOCP_OPERATIONSERROR ;

    /* to allow read access */
    pthread_rdwr_runlock( &rs->rw_lock ) ; 

    if( rc != SPOCP_SUCCESS ) {
      pthread_mutex_unlock( &rs->transaction ) ;
      conn->transaction = 0 ;
    }
    else
      conn->transaction = 1 ;
  }
  
  add_response( conn->out, rc ) ;

  if(( wr = send_results( conn )) == 0 ) rc = SPOCP_CLOSE ;

  return rc ;
}

/* --------------------------------------------------------------------------------- */

spocp_result_t com_subject( conn_t *conn )
{
  spocp_result_t    r = SPOCP_DENIED ;
  int               wr ;
  ruleset_t        *rs ;
  spocp_req_info_t *sri = &conn->sri ;
  octet_t           subj ;

  LOG( SPOCP_INFO ) traceLog("SUBJECT definition") ;

  /* while within a transaction I have a local copy */
  if( conn->transaction ) rs = conn->rs ;
  else                    rs = conn->srv->root ;

  if( conn->oparg.len == 0 ) {
    r = SPOCP_SUCCESS ; /* this is allowed */
    if( sri->subject.size ) {
      sri->subject.size = sri->subject.len = 0 ;
      free( sri->subject.val ) ;
    }
    LOG( SPOCP_DEBUG ) {
      traceLog("Subject: [anonymous]" ) ;
    }
  } /* only one argument */
  else if(( r = get_str( &conn->oparg, &subj )) == SPOCP_SUCCESS ) {
  
    /* should I make a sanity check of the subject definition ?? */

    sri->subject.size = 0 ;
    octcpy( &sri->subject, &subj ) ;

    LOG( SPOCP_DEBUG ) {
      char *tmp ;
      tmp = oct2strdup( &subj, '\\' ) ;
      traceLog("Subject: [%s]", tmp ) ;
      free( tmp ) ;
    }
  }

  add_response( conn->out, r ) ;

  if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;

  return r ;
}

/* --------------------------------------------------------------------------------- */

spocp_result_t com_aci( conn_t *conn )
{
  ruleset_t        *rs = conn->rs ;
  int              wr ;
  ruleset_t        *trs ;
  octet_t          rsn, sexp ;
  char             buf[1024] ;
  spocp_req_info_t *sri = &conn->sri ;
  spocp_result_t   r = SPOCP_DENIED ;
  
  LOG( SPOCP_INFO ) traceLog("add ACI") ;

  /* while within a transaction I have a local copy */
  if( conn->transaction ) rs = conn->rs ;
  else                    rs = conn->srv->root ;

  /* first argument might be ruleset name */
  if(( r = get_str( &conn->oparg, &rsn )) != SPOCP_SUCCESS ) goto A_DONE ;

  if( conn->oparg.len != 0 ) { /* more arguments */
    if(( r = get_str( &conn->oparg, &sexp )) != SPOCP_SUCCESS ) goto A_DONE ;
  }
  else {
    sexp.len = rsn.len ;
    sexp.val = rsn.val ;
    rsn.len = 0 ;
    rsn.val = 0 ;
  }

  /* if the ruleset exists create_ruleset just hands you a pointer to it */
  trs = create_ruleset( &rsn, &rs ) ;
    
  if(( r = rs_access_allowed( trs, sri, ADD )) != SPOCP_SUCCESS ) goto A_DONE ;

  if(( r = get_pathname( trs, buf, 1024 )) != SPOCP_SUCCESS )  goto A_DONE ;

  traceLog( "Add ACI for ruleset %s", buf ) ;

  /* should be just one s-exp, if it ain't something is broken */
  pthread_rdwr_wlock( &trs->rw_lock ) ;
  r = aci_add( &(trs->db), &sexp, sri ) ;
  pthread_rdwr_wunlock( &trs->rw_lock ) ;

A_DONE:
  add_response( conn->out, r ) ;

  if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;

  return r ;
}

/* --------------------------------------------------------------------------------- 
 *
 * ADD [ruleset_def] s-exp [blob]
 *
  add          = "3:ADD" [l-path] l-s-expr [ bytestring ]
 
 */

spocp_result_t com_add( conn_t *conn )
{
  spocp_result_t rc = SPOCP_DENIED ; /* The default */
  ruleset_t      *rs = conn->rs ;
  int            wr ;
  spocp_iobuf_t  *out = conn->out ;
  ruleset_t      *trs ;
  octet_t        rsn, sexp, blob, *argv[3] ;
  
  LOG( SPOCP_INFO ) traceLog("ADD rule") ;

  /* while within a transaction I have a local copy */
  if( conn->transaction ) rs = conn->rs ;
  else                    rs = conn->srv->root ;

  /* first argument might be ruleset name */
  if(( rc = get_str( &conn->oparg, &rsn )) != SPOCP_SUCCESS ) goto ADD_DONE ;

  if( conn->oparg.len != 0 ) { /* more arguments */
    if(( rc = get_str( &conn->oparg, &sexp )) != SPOCP_SUCCESS ) goto ADD_DONE ;
  }
  else {
    sexp.len = rsn.len ;
    sexp.val = rsn.val ;
    rsn.len = 0 ;
    rsn.val = 0 ;
  }

  if( conn->oparg.len ) { /* a blob too */
    if(( rc = get_str( &conn->oparg, &blob )) != SPOCP_SUCCESS ) goto ADD_DONE ;
  }
  else blob.len = 0 ;

  /* if I have two arguments than it could be path + rule or rule + blob 
     It's easy to see the difference between path and rule so I'll use that */

  if( blob.len == 0 && rsn.len != 0 ) {
    if( *rsn.val == '(' ) { /* a s-expression has to start with a '(' */
      blob.len = sexp.len ;
      blob.val = sexp.val ;
      sexp.len = rsn.len ;
      sexp.val = rsn.val ;
      rsn.len = 0 ;
      rsn.val = 0 ;
    }
  }

  /* if the ruleset exists create_ruleset just hands you a pointer to it */
  trs = create_ruleset( &rsn, &rs ) ;
    
  if(( rc = rs_access_allowed( trs, &conn->sri, ADD )) == SPOCP_SUCCESS ) {
    argv[0] = &sexp ;
    if( blob.len ) argv[1] = &blob ;
    else argv[1] = 0 ;
    argv[2] = 0 ;

    /* If a transaction is in operation wait for it to complete */
    pthread_mutex_lock( &trs->transaction )  ;
    /* get write lock, do operation and release lock */
    pthread_rdwr_wlock( &trs->rw_lock ) ;

    rc = ss_add_rule( trs, argv, &conn->sri ) ; 

    pthread_rdwr_wunlock( &trs->rw_lock ) ;
    pthread_mutex_unlock( &trs->transaction ) ;
    
    while( conn->in->r < conn->in->w && WHITESPACE( *conn->in->r )) conn->in->r++ ;
    shift_buffer( conn->in ) ;
  }
   
ADD_DONE: 
  add_response( out, rc ) ;

  if(( wr = send_results( conn )) == 0 ) rc = SPOCP_CLOSE ;

  return rc ;
}

/* --------------------------------------------------------------------------------- */
/* 
   list         = "4:LIST" [l-path] *( length ":" "+"/"-" s-expr ) 
 */
spocp_result_t com_list( conn_t *conn )
{
  spocp_result_t   rc = SPOCP_DENIED;
  ruleset_t        *rs, *trs ;
  spocp_req_info_t *sri = &conn->sri ;
  int              i, r = 0, wr ;
  octet_t          *op, rsn, larg ;
  octarr_t         *oa = 0 ;
  spocp_iobuf_t    *out = conn->out ;
    
  LOG( SPOCP_INFO ) traceLog("LIST requested") ;
    
  /* while within a transaction I have a local copy */
  if( conn->transaction ) rs = conn->rs ;
  else                    rs = conn->srv->root ;

  oa = octarr_new( 4 ) ;

  larg.len = conn->oparg.len ;
  larg.val = conn->oparg.val ;

  if( larg.len ) {
    /* first argument might be ruleset name */
    if(( r = get_str( &larg, &rsn )) != SPOCP_SUCCESS ) goto L_DONE ;

    if( *rsn.val == '/' ) { /* it's a ruleset */

      if(( trs = find_ruleset( &rsn, rs )) == 0 ) {
        char *str ;

        str = oct2strdup( &rsn, '%' ) ;
        traceLog("ERR: No \"/%s\" ruleset", str) ;
        free( str ) ;
        r = SPOCP_DENIED ;
        goto L_DONE ;
      }
      conn->oparg.len = larg.len ;
      conn->oparg.val = larg.val ;
    }
    else trs = rs ;
  }
  else trs = rs ;


  rc = treeList( trs, &conn->oparg, sri, oa, 1 ) ;
    
  if( oa && oa->n ) {
    for( i = 0 ; i < oa->n ; i++ ) {
      op = oa->arr[i] ;

      LOG( SPOCP_DEBUG ) {
        char *str ;
        str = oct2strdup( op, '\\' ) ;
        traceLog( str ) ;
        free( str ) ;
      }

      if((rc = add_to_iobuf( out, multiline )) != SPOCP_SUCCESS ) break ;
      if((rc = add_bytestr_to_iobuf( out, op )) != SPOCP_SUCCESS ) break ;
      if(( wr = send_results( conn )) == 0 ) {
        r = SPOCP_CLOSE ;
        break ;
      }
    }
  
    octarr_free( oa ) ; 
  }

L_DONE:
  add_response( out, rc ) ;
  
  if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;

  return r ;
}

/* --------------------------------------------------------------------------------- */
 
/* incomming data should be of the form operationlen ":" operlen ":" oper *( arglen ":" arg ) 
   
 starttls     = "8:STARTTLS"

   "10:8:STARTTLS"

 query        = "QUERY" SP [path] spocp-exp

   "75:5:QUERY64:(5:spocp(8:resource3:etc5:hosts)(6:action4:read)(7:subject3:foo))"
 
 add          = "ADD" SP [path] extspocp-exp

   "74:3:add60:(5:spocp(8:resource3:etc5:hosts)(6:action4:read)(7:subject))4:blob"

 delete       = "DELETE" SP ruleid

   "51:6:DELETE40:3e6d0dd469d4d32a65b31f963481a18c8de891e1"

 list         = "LIST" *( SP "+"/"-" s-expr ) ; extref are NOT expected to appear

   "72:4:LIST6:+spocp11:-(8:resource)17:+(6:action4:read)19:-(7:subject(3:uid))"

 aci          = "ACI" SP [path] spocp-exp

   "40:3:ACI32:/umu/(3:add(5:spocp)(7:subject))"

 logout       = "LOGOUT" 

   "8:6:LOGOUT"

 begin        = "BEGIN"

   "7:5:BEGIN"

 commit       = "COMMIT"

   "8:6:COMMIT"

 rollback     = "ROLLBACK"

   "10:8:ROLLBACK"

 subjectcom   = "SUBJECT" SP subject

   "24:7:SUBJECT12:(3:uid3:foo)"
*/

spocp_result_t get_operation( conn_t *conn, proto_op **oper )
{
  spocp_result_t r ;
  octet_t        op, arg ;
  int            l = 0 ;
  /* char    *tmp ; */
 
  arg.val = conn->in->r ;
  arg.len = conn->in->w - conn->in->r ;

  /* The whole operation with all the arguments */

  if(( r = get_str( &arg, &conn->oper )) != SPOCP_SUCCESS ) return r ;

  conn->oparg.val = conn->oper.val ;
  conn->oparg.len = conn->oper.len ;

  /* Just the operation specification */

  if(( r = get_str( &conn->oparg, &op )) != SPOCP_SUCCESS ) return r ;

  *oper = 0 ;

  switch( op.len ) {
    case 8:
      if( strncasecmp( op.val, "STARTTLS", 8 ) == 0 ) {
        *oper = &com_starttls ;
        l = 8 ;
      } 
      else if( strncasecmp( op.val, "ROLLBACK", 8 ) == 0 ) {
        *oper = &com_rollback ;
        l = 8 ;
      } 
      
      break ;

    case 6:
      if( strncasecmp( op.val, "LOGOUT", 6 ) == 0 ) {
        *oper = &com_logout ;
        l = 6 ;
      }
      else if( strncasecmp( op.val, "DELETE", 6 ) == 0 ) {
        *oper = &com_delete;
        l = 6 ;
      }
      else if( strncasecmp( op.val, "COMMIT", 6 ) == 0 ) {
        *oper = &com_commit ;
        l = 6 ;
      }
      break ;

    case 5:
      if( strncasecmp( op.val, "QUERY", 5 ) == 0 ) {
        *oper = &com_query ;
        l = 5 ;
      }
      else if( strncasecmp( op.val, "LOGIN", 5 ) == 0 ) {
        *oper = &com_login ; 
        l = 5 ;
      }
      else if( strncasecmp( op.val, "BEGIN", 5 ) == 0 ) {
        *oper = &com_begin ; 
        l = 5 ;
      }
      break ;

    case 7:
      if( strncasecmp( op.val, "SUBJECT", 7 ) == 0 ) {
        *oper = &com_subject ; 
        l = 7 ;
      }
      break ;

    case 3:
      if( strncasecmp( op.val, "ACI", 3 ) == 0 ) {
        *oper = &com_aci ; 
        l = 3 ;
      }
      else if( strncasecmp( op.val, "ADD", 3 ) == 0 ) {
        *oper = &com_add ;
        l = 3 ;
      }
      break ;

    case 4:
      if( strncasecmp( op.val, "LIST", 4 ) == 0 ) {
        *oper = &com_list ;
        l = 4 ;
      }
      break ;
  }
  
  if( l ) {
    conn->in->r = conn->oparg.val + conn->oparg.len ;
    while( conn->in->r != conn->in->w && WHITESPACE(*conn->in->r) ) conn->in->r++ ;
  }

  if( *oper == 0 ) return SPOCP_UNKNOWNCOMMAND ;
  else return SPOCP_SUCCESS ;
}

/* --------------------------------------------------------------------------------- */

int native_server( conn_t *con )
{
  spocp_result_t   r = SPOCP_SUCCESS ;
  spocp_iobuf_t    *in, *out ;
  int              n, wr = 1 ;
  octet_t          attr ;
  proto_op         *oper = 0 ;
  time_t           now = 0 ;

  in = con->in ;
  out = con->out ;
  time( &con->last_event ) ;

  traceLog( "Running native server" ) ;

AGAIN:
  while( wr && (n = spocp_io_read( con )) >= 0 ) {

    time( &now ) ;

    if( n == 0 ) {
      if( !con->srv->timeout ) continue ;
 
      if( (int)(now - con->last_event) > con->srv->timeout ) {
        traceLog("Connection closed due to inactivity" ) ;
        break ;
      }
      else continue ;
    }

    if( n ) { 
      DEBUG( SPOCP_DSRV ) traceLog("Got %d(%d new)  bytes", in->w - in->r, n ) ;
    }

    /* If no chars, obviously nothing for me to do */
    while( in->w - in->r ) {
      if(( r = get_operation( con, &oper )) == SPOCP_SUCCESS ) {
        r = (*oper)( con ) ;

        LOG( SPOCP_DEBUG ) traceLog( "command returned %d", r ) ;
        LOG( SPOCP_DEBUG ) traceLog( "%d chars left in the input buffer", in->w - in->r ) ;

        if( r == SPOCP_CLOSE ) goto clearout;
        else if( r == SPOCP_MISSING_CHAR ) {
          break ; /* I've got the whole package but it had internal deficienses */
        }
        else if( r == SPOCP_UNKNOWNCOMMAND ) {  /* Unknown keyword */
          add_response( out, r ) ;
          wr = send_results( con ) ;
        }
/*
        else if( r == SPOCP_SYNTAXERROR ) { * totally screwed up, reset and start again *
          clear_buffers( con ) ;
        }
*/

        if( in->r == in->w ) flush_io_buffer( in ) ;
      }
      else if( r == SPOCP_MISSING_CHAR ) {
        traceLog( "get more input" ) ;
        if( in->left == 0 ) { /* increase input buffer */
          int l ;

          attr.val = in->r ;
          attr.len = in->w - in->r ;
          l = get_len( &attr ) ;
          if(( r = iobuf_resize( in, l - attr.len )) != SPOCP_SUCCESS ) {
            add_response( out, r ) ;
            wr = send_results( con ) ;
            clear_buffers( con ) ;
          }
        }
        goto AGAIN ;
      }
      else {
        clear_buffers( con ) ;
      }
    }
    /* reset the timeout counter after the server is done with the operations */
    time( &con->last_event ) ;
  }

clearout:
  clear_buffers( con ) ;
  DEBUG( SPOCP_DSRV ) traceLog( "(%d) Closing the connection", con->fd ) ;

  return 0 ; 
}


void spocp_server( void *vp )
{
  conn_t    *co  ;
  /*
  ruleset_t *rs ; 
  srv_t     *srv ;
  */

  DEBUG( SPOCP_DSRV ) traceLog("In spocp_server") ;

  if( vp == 0 ) return ;

  co = (conn_t *) vp ;

  /*
  srv = co->srv ; 
  pthread_mutex_lock( &(srv->mutex) ) ;
  rs = srv->root ;
  pthread_mutex_unlock( &(srv->mutex) ) ;

  traceLog("Server ready (timeout set to %d)", srv->timeout) ;
  */


  native_server( co ) ;
}
