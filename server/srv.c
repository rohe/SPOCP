/***************************************************************************
                           srv.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 **************************************************************************/

#include "locl.h"

char *rc_working              = "3:10020:Working, please wait" ;

char *rc_ok                   = "3:2002:Ok" ;
char *rc_matched              = "3:2017:Matched" ;
char *multiline               = "3:201" ;
char *rc_denied               = "3:2026:Denied" ;
char *rc_bye                  = "3:2033:Bye" ;
char *rc_transaction_complete = "3:20420:Transaction complete" ;

char *rc_redirection          = "3:300" ;
char *rc_auth_in_progress     = "3:30126:Authentication in progress" ;

char *rc_syntax_error         = "3:40012:Syntax error" ;
char *rc_in_operation         = "3:40120:Already in operation" ;
/*
char *rc_too_many_arg         = "3:40218:Too many arguments" ;
char *rc_line_to_long         = "3:40312:Line too long" ;
char *rc_access_denied        = "3:40413:Access denied" ;
*/
char *rc_argument_err         = "3:40514:Argument error" ;
char *rc_not_supported        = "3:40613:Not supported" ;
char *rc_exists               = "3:40714:Already exists" ;
char *rc_input_error          = "3:40811:Input error" ;
char *rc_protocol_error       = "3:40914:Protocol error" ;
char *rc_unknown_command      = "3:41015:Unknown command" ;
char *rc_sizelimitexceeded    = "3:41118:Sizelimit exceeded" ;

char *rc_operation_error         = "3:50015:Operation error" ;
char *rc_service_not_available   = "3:50121:Service not available" ;
char *rc_information_unavailable = "3:50223:Information unavailable" ;
char *rc_unknown_id              = "3:50310:Unknown ID" ;
char *rc_already_active          = "3:50414:Already active" ;
char *rc_internal_error          = "3:50514:Internal error" ;
char *rc_timelimitexceeded       = "3:50618:Timelimit exceeded" ;
char *rc_other                   = "3:50711:Other error" ;
char *rc_auth_error              = "3:50820:Authentication error" ;
char *rc_unimplemented           = "3:50923:Command not implemented" ;
char *rc_unwilling               = "3:51020:Unwilling to perform" ;

/***************************************************************************
 ***************************************************************************/


static void add_response( spocp_iobuf_t *out, int rc )
{
  if( rc == SPOCP_SUCCESS ) {
    iobuf_add( out, rc_ok ) ;
  }
  else if( rc == SPOCP_DENIED ) {
    iobuf_add( out, rc_denied ) ;
  }
  else if( rc == SPOCP_CLOSE ) {
    iobuf_add( out, rc_bye ) ;
  }
  else if( rc == SPOCP_EXISTS ) {
    traceLog( "ERR: Already exists" ) ;
    iobuf_add( out, rc_exists ) ;
  }
  else if( rc == SPOCP_OPERATIONSERROR ) {
    traceLog( "ERR: Operation error" ) ;
    iobuf_add( out, rc_operation_error ) ;
  }
  else if( rc == SPOCP_PROTOCOLERROR ) {
    traceLog( "ERR: Protocol error" ) ;
    iobuf_add( out, rc_protocol_error ) ;
  }
  else if( rc == SPOCP_UNKNOWNCOMMAND ) {
    traceLog( "ERR: Unknown command" ) ;
    iobuf_add( out, rc_unknown_command ) ;
  }
  else if( rc == SPOCP_SYNTAXERROR ) {
    traceLog( "ERR: Syntax error" ) ;
    iobuf_add( out, rc_syntax_error ) ;
  }
  else if( rc == SPOCP_TIMELIMITEXCEEDED ) {
    traceLog( "ERR: Timelimit exceeded" ) ;
    iobuf_add( out, rc_timelimitexceeded ) ;
  }
  else if( rc == SPOCP_SIZELIMITEXCEEDED ) {
    traceLog( "ERR: Sizelimit exceeded" ) ;
    iobuf_add( out, rc_sizelimitexceeded ) ;
  }
  else if( rc == SPOCP_UNAVAILABLE ) {
    traceLog( "ERR: Unknown ID" ) ;
    iobuf_add( out, rc_unknown_id ) ;
  }
  else if( rc == SPOCP_MISSING_ARG) {
    traceLog( "ERR: Argument error" ) ;
    iobuf_add( out, rc_argument_err ) ;
  }
  else if( rc == SPOCP_MISSING_CHAR ) {
    traceLog( "ERR: Missing bytes in input" ) ;
    iobuf_add( out, rc_input_error ) ;
  }
  else if( rc == SPOCP_OTHER ) {
    traceLog( "ERR: Other error" ) ;
    iobuf_add( out, rc_other ) ;
  }
  else if( rc == SPOCP_BUF_OVERFLOW ) {
    traceLog( "ERR: Buffer overflow" ) ;
    iobuf_add( out, rc_operation_error ) ;
  }
  else if( rc == SPOCP_UNIMPLEMENTED ) {
    traceLog( "ERR: unimplemented command" ) ;
    iobuf_add( out, rc_unimplemented ) ;
  }
  else {
    traceLog( "ERR: errortype [%d]", rc ) ;
    iobuf_add( out, rc_operation_error ) ;
  }
}

/*
static int sexp_err( conn_t *conn, int l ) 
{
  if( l != conn->oparg.len ) 
    traceLog( "error in received data") ; 
  else 
    traceLog( "Syntax error in S-EXP ") ; 

  iobuf_shift( conn->in ) ;

  iobuf_add( conn->out, rc_syntax_error ) ;
  send_results( conn ) ;

  return -1 ;
}
*/

/* --------------------------------------------------------------------------------- */

static spocp_result_t get_oparg( octet_t *arg, octarr_t **oa )
{
  spocp_result_t r = SPOCP_SUCCESS ;
  int            i ; 
  octet_t        op ;
  octarr_t       *oarr = *oa ;

  if( oarr ) traceLog( "Non empty oarr ?? (n = %d)", oarr->n ) ;

  for( i = 0 ; arg->len ; i++  ) {
    if(( r = get_str( arg, &op )) != SPOCP_SUCCESS ) break ;
    oarr = octarr_add( oarr, octdup( &op )) ;
  }

  *oa = oarr ;

  return r ;
}

static void oparg_clear( conn_t *con ) 
{
  /* reset operation arguments */
  octarr_free( con->oparg ) ;
  con->oparg = 0 ;
  oct_free( con->oppath ) ;
  con->oppath = 0 ;
}

/* --------------------------------------------------------------------------------- */

spocp_result_t com_starttls( conn_t *conn )
{
  spocp_result_t  r = SPOCP_SUCCESS ;
  int             wr ;
  spocp_iobuf_t  *out = conn->out ;

  LOG( SPOCP_INFO ) traceLog("Attempting to start SSL/TLS") ;

#ifndef HAVE_LIBSSL

  iobuf_add( out, rc_not_supported ) ;
  LOG( SPOCP_ERR ) traceLog("SSL not supported") ;
  wr = send_results( conn ) ;
  return SPOCP_CLOSE ;

#else

  if( conn->tls >= 0) {
    LOG( SPOCP_ERR ) traceLog("SSL already in operation") ;
    iobuf_add( out, rc_in_operation ) ;
  }
  else {
    if(( r = operation_access( conn )) == SPOCP_SUCCESS ) iobuf_add( out, rc_ok ) ;
    else add_response( conn->out, r ) ;
  }

  if(( wr = send_results( conn )) == 0 ) return SPOCP_CLOSE ;

  if( r == SPOCP_SUCCESS ) {
    /* what ever is in the input buffert must not be used anymore */
    conn_iobuf_clear( conn ) ;

    /* If I couldn't create a TLS connection fail completely */
    if((r = tls_start( conn, rs )) != SPOCP_SUCCESS ) {
      LOG( SPOCP_ERR ) traceLog("Failed to start SSL") ;
      r = SPOCP_CLOSE ;
    }
    else traceLog("SSL in operation") ;

    /* conn->tls is set by tls_start */
  }
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
    
  conn->stop = 1 ;

  if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;

  return r ;
}

/* --------------------------------------------------------------------------------- */

/* format of DELETE

  delete       = "6:DELETE" l-path length ":" ruleid
 
*/
static spocp_result_t com_delete( conn_t *conn )
{
  spocp_result_t   r = SPOCP_SUCCESS ;
  int              wr ;
  ruleset_t        *rs = conn->rs, *trs ;

  LOG( SPOCP_INFO ) traceLog("DELETE requested ") ;

  /* while within a transaction I have a local copy */
  if( conn->transaction ) rs = conn->rs ;
  else                    rs = conn->srv->root ;

  switch( conn->oparg->n ) {
    case 1:
      conn->oppath = 0 ;
      break ;
      
    case 2:
      conn->oppath = octarr_pop( conn->oparg ) ;

      if( *(conn->oppath->val) != '/' ) {
        r = SPOCP_SYNTAXERROR ;
        goto D_DONE ;
      }
      break ;

    case 0:
      r = SPOCP_MISSING_ARG ;
      break ;

    default:
      r = SPOCP_PARAM_ERROR ;
  }

  if( operation_access( conn ) != SPOCP_SUCCESS ) goto D_DONE ;

  trs = rs ;
  if(( ruleset_find( conn->oppath, &trs )) != 1 ) {
    char *str ;

    str = oct2strdup( conn->oppath, '%' ) ;
    traceLog("ERR: No \"/%s\" ruleset", str) ;
    free( str ) ;

    r = SPOCP_DENIED ;
  }
  else {
    /* get write lock, do operation and release lock */
    /* locking the whole tree, is that really necessary ? */
    pthread_rdwr_wlock( &rs->rw_lock ) ;
    r = ss_del_rule( trs, conn->oparg->arr[0], SUBTREE ) ;
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
  octarr_t         *on = 0 ;
  octet_t          *oct ;
  spocp_iobuf_t    *out = conn->out ;
  ruleset_t        *rs, *trs ;
  char             *str, buf[1024] ;
  /*
  struct timeval   tv ;

  gettimeofday( &tv, 0 ) ;
  */

  if(0) timestamp( "QUERY" ) ;

  LOG( SPOCP_INFO ) {
    str = oct2strdup( conn->oparg->arr[0], '%' ) ;
    traceLog("[%d]QUERY:%s", conn->fd, str) ;
    free( str ) ;
  }

  /* while within a transaction I have a local copy */

  if( conn->transaction ) rs = conn->rs ;
  else                    rs = conn->srv->root ;

  /* traceLog( "OP argc: %d", conn->oparg->n) ; */

  switch( conn->oparg->n ) {
    case 1:
      conn->oppath = 0 ;
      break ;
      
    case 2:
      conn->oppath = octarr_pop( conn->oparg ) ;
      if( *(conn->oppath->val) != '/' ) {
        r = SPOCP_SYNTAXERROR ;
      }

    case 0:
      r = SPOCP_MISSING_ARG ;
      break ;

    default:
      r = SPOCP_PARAM_ERROR;
  }

  if( r == SPOCP_SUCCESS && ( r = operation_access( conn )) == SPOCP_SUCCESS ) {

    trs = rs ;

    if(( ruleset_find( conn->oppath, &trs )) != 1 ) {
      str = oct2strdup( conn->oppath, '%' ) ;
      traceLog("ERR: No \"/%s\" ruleset", str) ;
      free( str ) ;
      r = SPOCP_DENIED ;
    }
    else if(( r = pathname_get( trs, buf, 1024 )) == SPOCP_SUCCESS ) {
  
      DEBUG( SPOCP_DPARSE ) traceLog( "Matching against ruleset \"%s\"", buf ) ;

      if(0) timestamp( "ss_allow" ) ;
      pthread_rdwr_rlock( &rs->rw_lock ) ;
      r = ss_allow( trs, conn->oparg->arr[0], &on, SUBTREE ) ;
      pthread_rdwr_runlock( &rs->rw_lock ) ;
      if(0) timestamp( "ss_allow done" ) ;
    }
  }

  if( r == SPOCP_SUCCESS && on ) {
    while(( oct = octarr_pop( on ))) {
      if( iobuf_add( out, multiline) != SPOCP_SUCCESS ) {
        r = SPOCP_OPERATIONSERROR ;
        break ;
      }
      if( iobuf_add_octet( out, oct ) != SPOCP_SUCCESS ) {
        r = SPOCP_OPERATIONSERROR ;
        break ;
      }

      str = oct2strdup( oct, '%' ) ;
      traceLog("returns \"%s\"", str) ;
      free( str ) ;

      if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;
      oct_free( oct ) ;

      /*  wait for the master to empty the queue */
      pthread_mutex_lock( &conn->out->lock ) ;
      pthread_cond_wait( &conn->out->empty, &conn->out->lock ) ;
      pthread_mutex_unlock( &conn->out->lock ) ;
    }

    octarr_free( on ) ;
  }

  add_response( out, r ) ;
    
  iobuf_shift( conn->in ) ;
  oparg_clear( conn ) ;

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
  else if(( rc = operation_access( conn )) == SPOCP_SUCCESS ) {

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

  LOG( SPOCP_INFO ) traceLog("SUBJECT definition") ;

  /* while within a transaction I have a local copy */
  if( conn->transaction ) rs = conn->rs ;
  else                    rs = conn->srv->root ;

  switch( conn->oparg->n ) {
    case 0:
      oct_free( conn->sri.subject ) ;
      break ;

    case 1:
      conn->sri.subject = octarr_pop( conn->oparg ) ;
      break ;
      
    case 2: /* auth token too */
      r = SPOCP_UNIMPLEMENTED ;
      break ;

    default:
      r = SPOCP_PARAM_ERROR;
      break ;
  }

  /* ?? 
  if( operation_access( conn ) != SPOCP_SUCCESS ) goto S_DONE ;
  */

  add_response( conn->out, r ) ;

  if(( wr = send_results( conn )) == 0 ) r = SPOCP_CLOSE ;

  return r ;
}

/* --------------------------------------------------------------------------------- 
 *
 * ADD [ruleset_def] s-exp [blob]
 *
  add          = "3:ADD" [l-path] l-s-expr [ bytestring ]
moving toward
  add          = "3:ADD" [l-path] l-s-expr [ bcond [ bytestring ]]
 
 */

spocp_result_t com_add( conn_t *conn )
{
  spocp_result_t rc = SPOCP_DENIED ; /* The default */
  ruleset_t      *trs, *rs = conn->rs ;
  spocp_iobuf_t  *out = conn->out ;
  int             wr ;
  
  LOG( SPOCP_INFO ) traceLog("ADD rule") ;

  /* while within a transaction I have a local copy */
  if( conn->transaction ) rs = conn->rs ;
  else                    rs = conn->srv->root ;

  switch( conn->oparg->n ) {
    case 0:
      rc = SPOCP_MISSING_ARG ;
      break ;

    case 1:
      conn->oppath = 0 ;
      break ;
      
  /* if I have two arguments then it could be path + rule or rule + blob 
     It's easy to see the difference between path and rule so I'll use that */

    case 2: /* path and rule or rule and bcond */
    case 3: /* path, rule and bcond or rule,bcond and blob */
      if( *(conn->oparg->arr[0]->val) == '/' ) { /* First argument is path */
        conn->oppath = octarr_pop( conn->oparg ) ;
      }
      else { /* No path */
        conn->oppath = 0 ;
      }
      break ;


    case 4 : /* path, rule, bcond and blob */
      if( *(conn->oparg->arr[0]->val) != '/' ) rc = SPOCP_SYNTAXERROR ;
      else conn->oppath = octarr_pop( conn->oparg ) ;

      break ;

    default:
      rc = SPOCP_PARAM_ERROR ;
      break ;
  }

  if( rc != SPOCP_DENIED || operation_access( conn ) != SPOCP_SUCCESS ) goto ADD_DONE ;

  /* will not recreate something that already exists */
  trs = ruleset_create( conn->oppath, &rs ) ;
    
  /* If a transaction is in operation wait for it to complete */
  pthread_mutex_lock( &trs->transaction )  ;
  /* get write lock, do operation and release lock */
  pthread_rdwr_wlock( &trs->rw_lock ) ;

  rc = spocp_add_rule( (void **) &(trs->db), conn->oparg ) ; 

  pthread_rdwr_wunlock( &trs->rw_lock ) ;
  pthread_mutex_unlock( &trs->transaction ) ;
    
  while( conn->in->r < conn->in->w && WHITESPACE( *conn->in->r )) conn->in->r++ ;
  iobuf_shift( conn->in ) ;
   
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
  int              i, r = 0, wr ;
  octet_t          *op ;
  octarr_t         *oa = 0 ; /* where the result is put */
  spocp_iobuf_t    *out = conn->out ;
    
  LOG( SPOCP_INFO ) traceLog("LIST requested") ;
    
  /* while within a transaction I have a local copy */
  if( conn->transaction ) rs = conn->rs ;
  else                    rs = conn->srv->root ;

  /* first argument might be ruleset name */

  if( *(conn->oparg->arr[0]->val) == '/' ) { /* it's a ruleset */
    conn->oppath = octarr_pop( conn->oparg ) ;

    trs = rs ;
    if(( ruleset_find( conn->oppath, &trs )) != 1 ) {
      char *str ;

      str = oct2strdup( conn->oppath, '%' ) ;
      traceLog("ERR: No \"/%s\" ruleset", str) ;
      free( str ) ;
      r = SPOCP_DENIED ;
      goto L_DONE ;
    }
  }
  else trs = rs ;

  pthread_rdwr_rlock( &rs->rw_lock ) ;
  rc = treeList( trs, conn, oa, 1 ) ;
  pthread_rdwr_runlock( &rs->rw_lock ) ;
    
  if( oa && oa->n ) {
    for( i = 0 ; i < oa->n ; i++ ) {
      op = oa->arr[i] ;

      LOG( SPOCP_DEBUG ) {
        char *str ;
        str = oct2strdup( op, '\\' ) ;
        traceLog( str ) ;
        free( str ) ;
      }

      if((rc = iobuf_add( out, multiline )) != SPOCP_SUCCESS ) break ;
      if((rc = iobuf_add_octet( out, op )) != SPOCP_SUCCESS ) break ;
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

/*
spocp_result_t com_bcond( conn_t *conn )
{
  spocp_result_t   r = SPOCP_DENIED ;
  ruleset_t        *rs ;
  octarr_t         *oa ;

  LOG ( SPOCP_INFO ) traceLog( "BCOND requested" ) ;

  * while within a transaction I have a local copy *
  if( conn->transaction ) rs = conn->rs ;
  else                    rs = conn->srv->root ;

  oa = conn->oparg ;

  if( oct2strcmp( oa->arr[0], "ADD" ) == 0 )  {
    if( oa->n < 3 ) r = SPOCP_MISSING_ARG ;
    else {
      if( bcdef_add( rs->db, oa->arr[1], oa->arr[2] ) == 0 )
        r = SPOCP_PROTOCOLERROR ;
      else
        r = SPOCP_SUCCESS ;
    }
  }
  else if( oct2strcmp( oa->arr[0], "DELETE" ) == 0 )  {
    if( oa->n < 2 ) r = SPOCP_MISSING_ARG ;
    else r = bcdef_del( &rs->db->bcdef, oa->arr[1] ) ;
  }
  else if( oct2strcmp( oa->arr[0], "REPLACE" ) == 0 )  {
    if( oa->n < 3 ) r = SPOCP_MISSING_ARG ;
    else r = bcdef_replace( rs->db->plugins, oa->arr[1], oa->arr[2], &rs->db->bcdef ) ;
  }

  return r ;
}
*/ 
/* --------------------------------------------------------------------------------- */
 
/* incomming data should be of the form operationlen ":" operlen ":" oper *( arglen ":" arg ) 
   
 starttls     = "8:STARTTLS"

   "10:8:STARTTLS"

 query        = "QUERY" SP [path] spocp-exp

   "75:5:QUERY64:(5:spocp(8:resource3:etc5:hosts)(6:action4:read)(7:subject3:foo))"
 
 add          = "ADD" SP [path] spocp-exp bcond blob

   "74:3:add60:(5:spocp(8:resource3:etc5:hosts)(6:action4:read)(7:subject))4:blob"

 delete       = "DELETE" SP ruleid

   "51:6:DELETE40:3e6d0dd469d4d32a65b31f963481a18c8de891e1"

 list         = "LIST" *( SP "+"/"-" s-expr ) ; extref are NOT expected to appear

   "72:4:LIST6:+spocp11:-(8:resource)17:+(6:action4:read)19:-(7:subject(3:uid))"

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
  octet_t        wo, op, arg ;
  int            l = 0 ;
  /* char    *tmp ; */
 
  arg.val = conn->in->r ;
  arg.len = conn->in->w - conn->in->r ;

  /* The whole operation with all the arguments */
  if(( r = get_str( &arg, &wo )) != SPOCP_SUCCESS ) return r ;

  /* Just the operation specification */
  if(( r = get_str( &wo, &op )) != SPOCP_SUCCESS ) return r ;

  octln( &conn->oper, &op ) ;

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
/*
      else if( strncasecmp( op.val, "BCOND", 5 ) == 0 ) {
        *oper = &com_bcond ; 
        l = 5 ;
      }
*/
      break ;

    case 7:
      if( strncasecmp( op.val, "SUBJECT", 7 ) == 0 ) {
        *oper = &com_subject ; 
        l = 7 ;
      }
      break ;

    case 3:
      if( strncasecmp( op.val, "ADD", 3 ) == 0 ) {
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
    conn->in->r = wo.val + wo.len ;
    while( conn->in->r != conn->in->w && WHITESPACE(*conn->in->r) ) conn->in->r++ ;
  }

  /* conn->oparg = 0 ; * should be done elsewhere */

  if(( r = get_oparg( &wo, &conn->oparg )) != SPOCP_SUCCESS ) return r ;

  if( *oper == 0 ) return SPOCP_UNKNOWNCOMMAND ;
  else return SPOCP_SUCCESS ;
}

/* ================================================================================ */
/* 
   Below is only used when reading and wrting to stdout, something you do when
   you are debugging 
*/
/* ================================================================================ */

static void send_and_flush_results( conn_t *conn )
{
  send_results( conn ) ;

  spocp_conn_write( conn ) ;
}

static int native_server( conn_t *con )
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
  while( wr && (n = spocp_conn_read( con )) >= 0 ) {

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
        LOG( SPOCP_DEBUG ) traceLog( "%d chars left in the input buffer", in->w - in->r) ;

        spocp_conn_write( con ) ;
        oparg_clear( con ) ;

        if( r == SPOCP_CLOSE ) goto clearout;
        else if( r == SPOCP_MISSING_CHAR ) {
          break ; /* I've got the whole package but it had internal deficienses */
        }
        else if( r == SPOCP_UNKNOWNCOMMAND ) {  /* Unknown keyword */
          add_response( out, r ) ;
          send_and_flush_results( con ) ;
        }

        if( in->r == in->w ) iobuf_flush( in ) ;
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
            send_and_flush_results( con ) ;
            oparg_clear( con ) ;
            conn_iobuf_clear( con ) ;
          }
        }
        goto AGAIN ;
      }
      else {
        oparg_clear( con ) ;
        conn_iobuf_clear( con ) ;
      }
    }
    /* reset the timeout counter after the server is done with the operations */
    time( &con->last_event ) ;
  }

clearout:
  conn_iobuf_clear( con ) ;
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
