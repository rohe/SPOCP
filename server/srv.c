
/***************************************************************************
                           srv.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 **************************************************************************/

#include <stdarg.h>
#include "locl.h"

#define UNKNOWN_RULESET 256

typedef struct _rescode {
	int             code;
	char           *rc;
	char           *def;
} rescode_t;

rescode_t       rescode[] = {
	{SPOCP_WORKING, "3:100", "20:Working, please wait"},

	{SPOCP_SUCCESS, "3:200", "2:Ok"},
	{SPOCP_MULTI, "3:201", NULL},
	{SPOCP_DENIED, "3:202", "6:Denied"},
	{SPOCP_CLOSE, "3:203", "3:Bye"},
	{SPOCP_SSL_START, "3:205", "18:Ready to start TLS"},

	{SPOCP_AUTHDATA, ",3:301", NULL},
	{SPOCP_AUTHINPROGRESS, "3:300", "26:Authentication in progress"},

	{SPOCP_BUSY, "3:400", "4:Busy"},
	{SPOCP_TIMEOUT, "3:401", "7:Timeout"},
	{SPOCP_TIMELIMITEXCEEDED, "3:402", "18:Timelimit exceeded"},
	{SPOCP_REDIRECT, "3:403", NULL},

	{SPOCP_SYNTAXERROR, "3:500", "12:Syntax error"},
	{SPOCP_MISSING_ARG, "3:501", "16:Missing Argument"},
	{SPOCP_MISSING_CHAR, "3:502", "11:Input error"},
	{SPOCP_PROTOCOLERROR, "3:503", "14:Protocol error"},
	{SPOCP_UNKNOWNCOMMAND, "3:504", "15:Unknown command"},
	{SPOCP_PARAM_ERROR, "3:505", "14:Argument error"},
	{SPOCP_SSL_ERR, "3:506", "16:SSL Accept error"},
	{SPOCP_UNKNOWN_TYPE, "3:507", "17:Uknown range type"},

	{SPOCP_SIZELIMITEXCEEDED, "3:511", "18:Sizelimit exceeded"},
	{SPOCP_OPERATIONSERROR, "3:512", "15:Operation error"},
	{SPOCP_UNAVAILABLE, "3:513", "21:Service not available"},
	{SPOCP_INFO_UNAVAIL, "3:514", "23:Information unavailable"},
	{SPOCP_NOT_SUPPORTED, "3:515", "21:Command not supported"},
	{SPOCP_STATE_VIOLATION, "3:516", "18:SSL Already active"},
	{SPOCP_OTHER, "3:517", "11:Other error"},
	{SPOCP_CERT_ERR, "3:518", "20:Authentication error"},
	{SPOCP_UNWILLING, "3:519", "20:Unwilling to perform"},
	{SPOCP_EXISTS, "3:520", "14:Already exists"},
	{SPOCP_AUTHERR, "3:521", "21:Authentication error"},

	{0, NULL, NULL}
};

/***************************************************************************
 ***************************************************************************/

static rescode_t *
find_rescode(int rc)
{
	register int    i;

	for (i = 0; rescode[i].rc; i++) {
		if (rescode[i].code == rc)
			return &rescode[i];
	}

	return 0;
}

static spocp_result_t
add_response_va(spocp_iobuf_t * out, int rc, const char *fmt, va_list ap)
{
	int             n;
	spocp_result_t  sr = SPOCP_SUCCESS;
	char            buf[SPOCP_MAXLINE];
	rescode_t      *rescode = find_rescode(rc);

	if (rescode == NULL)
		rescode = find_rescode(SPOCP_OTHER);

	sr = iobuf_add(out, rescode->rc);
	if (fmt) {
		n = vsnprintf(buf, SPOCP_MAXLINE, fmt, ap);
		if (n >= SPOCP_MAXLINE)	/* OUTPUT truncated */
			sr = SPOCP_LOCAL_ERROR;
		else
			sr = iobuf_add(out, buf);
	} else {
		if (rescode->def != NULL)
			sr = iobuf_add(out, rescode->def);
	}

#if 0

	for (i = 0; rescode[i].rc; i++) {
		if (rescode[i].code == rc) {
			sr = iobuf_add(out, rescode[i].rc);
			if (fmt) {
				n = vsnprintf(buf, SPOCP_MAXLINE, fmt, ap);
				if (n >= SPOCP_MAXLINE)	/* OUTPUT truncated */
					sr = SPOCP_LOCAL_ERROR;
				else {
					oct_assign(&oct, buf);
					sr = iobuf_add_octet(out, &oct);
				}
			} else
				sr = iobuf_add(out, rescode[i].def);
			break;
		}
	}
	/*
	 * unknown response code 
	 */
	if (rescode[i].rc == 0)
		sr = add_response(out, SPOCP_OTHER, NULL);

#endif

	return sr;
}

static spocp_result_t
add_response(spocp_iobuf_t * out, int rc, const char *fmt, ...)
{
	va_list         ap;
	spocp_result_t  res;

	va_start(ap, fmt);
	res = add_response_va(out, rc, fmt, ap);
	va_end(ap);

	iobuf_add_len_tag( out );

	return res;
}

static spocp_result_t
add_response_blob(spocp_iobuf_t * out, int rc, octet_t * data)
{
	spocp_result_t  sr;

	sr = add_response_va(out, rc, NULL, NULL);

	if (sr != SPOCP_SUCCESS)
		return sr;

	if (data)
		sr = iobuf_add_octet( out, data) ;

	if (sr != SPOCP_SUCCESS)
		return sr;

	iobuf_add_len_tag( out );

	return sr;
}

/*
 * --------------------------------------------------------------------------------- 
 */

static spocp_result_t
get_oparg(octet_t * arg, octarr_t ** oa)
{
	spocp_result_t  r = SPOCP_SUCCESS;
	int             i;
	octet_t         op;
	octarr_t       *oarr = *oa;

	if (oarr)
		traceLog(LOG_INFO,"Non empty oarr ?? (n = %d)", oarr->n);

	for (i = 0; arg->len; i++) {
		if ((r = get_str(arg, &op)) != SPOCP_SUCCESS)
			break;
		oarr = octarr_add(oarr, octdup(&op));
	}

	*oa = oarr;

	return r;
}

static void
oparg_clear(conn_t * con)
{
	/*
	 * reset operation arguments 
	 */
	octarr_free(con->oparg);
	con->oparg = 0;
	oct_free(con->oppath);
	con->oppath = 0;
}

/* ---------------------------------------------------------------------- */

static spocp_result_t 
opinitial( conn_t *conn, ruleset_t **rs, int path, int min, int max)
{
	spocp_result_t r = SPOCP_SUCCESS;

	if (conn->oparg == 0 ) {
		if (min) 
			return SPOCP_MISSING_ARG;
		else
			return SPOCP_SUCCESS;
	}

	/* Too many or few arguments? There might not be a max */
	if (max && conn->oparg->n > (path+max)) return SPOCP_PARAM_ERROR;
	if (conn->oparg->n < min) return SPOCP_MISSING_ARG;

	/* Only path definitions start with '/' and they have to */
	if( path && *(conn->oparg->arr[0]->val) == '/') 
		conn->oppath = octarr_pop(conn->oparg);
	else 
		conn->oppath = 0;
	
	if (conn->oparg->n < min) return SPOCP_MISSING_ARG;

	if (( r = operation_access(conn)) == SPOCP_SUCCESS) {

		if( conn->oppath == 0 );
		else if ((ruleset_find(conn->oppath, rs)) != 1) {
			char           *str;

			str = oct2strdup(conn->oppath, '%');
			traceLog(LOG_INFO,"ERR: No \"/%s\" ruleset", str);
			free(str);

			r = SPOCP_DENIED|UNKNOWN_RULESET;
		}
	} 

	return r;
}

static spocp_result_t
postop( conn_t *conn, spocp_result_t rc, const char *msg)
{
	int wr;

	if( msg && *msg ) {
		traceLog(LOG_INFO,"return message:%s", msg);
	}
	add_response(conn->out, rc, msg);

	if (msg != NULL)
		free((char *) msg);

	if ((wr = send_results(conn)) == 0)
		rc = SPOCP_CLOSE;

	iobuf_shift(conn->in);
	oparg_clear(conn);

	return rc;
}

spocp_result_t
return_busy( conn_t *conn )
{
	return postop( conn, SPOCP_BUSY, NULL );
}

/* ---------------------------------------------------------------------- */

spocp_result_t
com_capa(conn_t * conn)
{
	spocp_result_t  r = SPOCP_SUCCESS;
	const char     *msg = NULL;

	LOG(SPOCP_INFO) traceLog(LOG_INFO,"Attempt to authenticate");
	if (conn->transaction) return SPOCP_UNWILLING;

#ifdef HAVE_SASL
	if (conn->sasl == NULL) {
		int	wr;

		wr = sasl_server_new("spocp",
				     conn->srv->hostname,
				     NULL, NULL, NULL, NULL, 0, &conn->sasl);
		if (wr != SASL_OK) {
			LOG(SPOCP_ERR)
			    traceLog(LOG_ERR,"Failed to create SASL context");
			r = SPOCP_OTHER;
			goto err;
		}
	}

	if (conn->oparg->n == 0) {	/* list auth mechs */
		const char	*mechs;
		size_t		mechlen;
		int		count;
		int		wr;

		wr = sasl_listmech(conn->sasl, NULL, NULL, " ", NULL, &mechs,
				   &mechlen, &count);
		if (wr != SASL_OK) {
			LOG(SPOCP_ERR)
			    traceLog(LOG_ERR,"Failed to generate SASL mechanism list");
			r = SPOCP_OTHER;
		} else {
			msg = mechs;
		}
	}

      err:
#endif

	return postop( conn, r, msg );
}

spocp_result_t
com_auth(conn_t * conn)
{
	spocp_result_t  r = SPOCP_SUCCESS;
	int             wr;
#ifdef HAVE_SASL
	const char     *data;
	size_t          len;
	octet_t         blob;
#endif

	LOG(SPOCP_INFO) traceLog(LOG_INFO,"Attempt to authenticate");
	if (conn->transaction) return SPOCP_UNWILLING;

#ifdef HAVE_SASL

	if (conn->sasl == NULL) {
		wr = sasl_server_new("spocp",
				     conn->srv->hostname,
				     NULL, NULL, NULL, NULL, 0, &conn->sasl);
		if (wr != SASL_OK) {
			LOG(SPOCP_ERR)
			    traceLog(LOG_ERR,"Failed to create SASL context");
			goto check;
		}
	}

	if (conn->sasl_mech == NULL) {	/* start */
		conn->sasl_mech = oct2strdup(conn->oparg->arr[0], '%');
		wr = sasl_server_start(conn->sasl,
				       conn->sasl_mech,
				       conn->oparg->n >
				       1 ? conn->oparg->arr[1]->val : NULL,
				       conn->oparg->n >
				       1 ? conn->oparg->arr[1]->len : 0, &data,
				       &len);
	} else {		/* step */
		wr = sasl_server_step(conn->sasl,
				      conn->oparg->n >
				      0 ? conn->oparg->arr[0]->val : NULL,
				      conn->oparg->n >
				      0 ? conn->oparg->arr[0]->len : 0, &data,
				      &len);
	}

	memset(&blob, 0, sizeof(blob));
	if (data) {
		blob.val = (char *) data;
		blob.len = len;
	}

      check:

	switch (wr) {
	case SASL_OK:
		wr = sasl_getprop(conn->sasl, SASL_USERNAME,
				  (const void **) &conn->sasl_username);
		add_response_blob(conn->out, SPOCP_MULTI, &blob);
		add_response(conn->out, SPOCP_SUCCESS, "Authentication OK");
		break;
	case SASL_CONTINUE:
		add_response_blob(conn->out, SPOCP_AUTHDATA, &blob);
		add_response(conn->out, SPOCP_AUTHINPROGRESS, NULL);
		break;
	default:
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"SASL start/step failed: %d",
					sasl_errstring(wr, NULL, NULL));
		add_response(conn->out, SPOCP_AUTHERR,
			     "Authentication failed");
	}

#else
	add_response(conn->out, SPOCP_NOT_SUPPORTED, NULL);
#endif

	if ((wr = send_results(conn)) == 0)
		r = SPOCP_CLOSE;

	iobuf_shift(conn->in);
	oparg_clear(conn);

	return r;
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_starttls(conn_t * conn)
{
	spocp_result_t  r = SPOCP_SUCCESS;
#if (defined(HAVE_SSL) || defined(HAVE_SASL))
	int             wr;
#endif

	if (conn->transaction) return SPOCP_UNWILLING;

	traceLog(LOG_INFO,"Attempting to start SSL/TLS");

#if !(defined(HAVE_SSL) || defined(HAVE_SASL))
	r = SPOCP_NOT_SUPPORTED;
#else

#ifdef HAVE_SASL
	if (conn->sasl != NULL) {
		LOG(SPOCP_ERR)
		    traceLog(LOG_ERR,"Layering violation: SASL already in operation");
		r = SPOCP_STATE_VIOLATION;	/* XXX definiera och ... */
	} else
#endif
#ifdef HAVE_SSL
	if (conn->ssl != NULL) {
		LOG(SPOCP_ERR)
		    traceLog(LOG_ERR,"Layering violation: SSL already in operation");
		r = SPOCP_STATE_VIOLATION;	/* XXX ... fixa informativt
						 * felmeddleande */
	} else
#endif
	if ((r = operation_access(conn)) == SPOCP_SUCCESS) {
		/*
		 * Ready to start TLS/SSL 
		 */
		add_response(conn->out, SPOCP_SSL_START, 0);
		if ((wr = send_results(conn)) == 0)
			r = SPOCP_CLOSE;

		traceLog(LOG_INFO,"Setting connection status so main wan't touch it") ;
		conn->status = CNST_SSL_NEG;	/* Negotiation in progress */

		/*
		 * whatever is in the input buffert must not be used anymore 
		 */
                traceLog(LOG_INFO,"Input buffer flush") ;
		iobuf_flush(conn->in);

		/*
		 * If I couldn't create a TLS connection fail completely 
		 */
		if ((r = tls_start(conn, conn->rs)) != SPOCP_SUCCESS) {
			LOG(SPOCP_ERR) traceLog(LOG_ERR,"Failed to start SSL");
			r = SPOCP_CLOSE;
			conn->stop = 1;
		} else
			traceLog(LOG_INFO,"SSL in operation");

		conn->status = CNST_ACTIVE;	/* Negotiation in progress */
	}
#endif

	return postop( conn, r, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_rollback(conn_t * conn)
{
	spocp_result_t  r = SPOCP_SUCCESS;

	LOG(SPOCP_INFO) traceLog(LOG_INFO,"ROLLBACK");

	if( conn->transaction ){
		opstack_free( conn->ops );
		conn->ops = 0;
	}
	else 
		r = SPOCP_UNWILLING;

	return postop( conn, r, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_logout(conn_t * conn)
{
	spocp_result_t  r = SPOCP_CLOSE;

	LOG(SPOCP_INFO) traceLog(LOG_INFO,"LOGOUT requested ");

	if (conn->transaction)
		opstack_free( conn->ops );

	/* place response in write queue before marking connection for closing */
	r = postop( conn, r, 0);

	conn->stop = 1;

	return r;
}

/*
 * --------------------------------------------------------------------------------- 
 */

/*
 * format of DELETE
 * 
 * delete = "6:DELETE" l-path length ":" ruleid
 * 
 */
static spocp_result_t
com_delete(conn_t * conn)
{
	spocp_result_t  r = SPOCP_SUCCESS;
	ruleset_t      *rs = conn->rs;

	LOG(SPOCP_INFO) traceLog(LOG_INFO,"DELETE requested ");

	/* possible pathspecification but apart from that exactly one argument */
	if ((r = opinitial(conn, &rs, 1, 1, 1)) == SPOCP_SUCCESS){
		if (conn->transaction) {
			opstack_t *ops;
			ops = opstack_new( SPOCP_DEL, rs, conn->oparg );
			conn->ops = opstack_push( conn->ops, ops );
		}
		else {
			/*
			 * get write lock, do operation and release lock 
			 * locking the whole tree, is that really necessary ? 
			 */
			pthread_rdwr_wlock(&rs->rw_lock);
			r = dbapi_rule_rm(rs->db, &conn->dbc, conn->oparg->arr[0], NULL);
			pthread_rdwr_wunlock(&rs->rw_lock);
		}
	}

	return postop( conn, r, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_commit(conn_t * conn)
{
	spocp_result_t	r = SPOCP_SUCCESS;
	opstack_t	*ops;
	ruleset_t	**rspp;
	int		i, n;

	LOG(SPOCP_INFO) traceLog(LOG_INFO,"COMMIT requested");

	if (!conn->transaction)
		r = SPOCP_UNWILLING;
	else {
		/*
		 * get write lock on all the relevant rulesets, 
		 * do operation and release locks
		 */
		for( ops = conn->ops, n = 0; r == SPOCP_SUCCESS && ops;
		    ops = ops->next )
			n++; 

		rspp = ( ruleset_t **) Calloc( n+1, sizeof( ruleset_t *));
		n = 0;

		for( ops = conn->ops; ops; ops = ops->next ) {
			for( i = 0 ; i < n; i++) 
				if (rspp[i] == ops->rs)
					break;

			if ( i == n ) {
				rspp[n++] = ops->rs;
				pthread_rdwr_wlock(&ops->rs->rw_lock);
			}
		}


		for( ops = conn->ops; r == SPOCP_SUCCESS && ops; ops = ops->next ) {
			switch( ops->oper){
			case( SPOCP_ADD ):
				r = dbapi_rule_add(&(ops->rs->db),
				    conn->srv->plugin, &conn->dbc, ops->oparg,
				    &ops->rule);
				break;
			case( SPOCP_DEL ):
				ops->rollback = rollback_info( ops->rs->db,
				    conn->oparg->arr[0]);
				r = dbapi_rule_rm(ops->rs->db, &conn->dbc,
				    ops->oparg->arr[0], NULL);
				break;
			}
		}

		/* roll back ? */
		if( r != SPOCP_SUCCESS) {
			opstack_t *last = ops;

			for( ops = conn->ops; ops != last; ops = ops->next ) {
				switch( ops->oper){
				case( SPOCP_ADD ):
					r = dbapi_rule_rm(ops->rs->db, &conn->dbc,
					    NULL, ops->rule);
					break;
				case( SPOCP_DEL ):
					r = dbapi_rule_add(&(ops->rs->db),
					    conn->srv->plugin, &conn->dbc,
					    conn->oparg, NULL);
					break;
				}
			}
		}

		for( i = 0 ; i < n ; i++ ) 
			pthread_rdwr_wunlock(&(rspp[i])->rw_lock);

		free(rspp);
		opstack_free( conn->ops );
		conn->ops = 0;
		conn->transaction = 0;
	}

	return postop( conn, r, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_query(conn_t * conn)
{
	spocp_result_t  r = SPOCP_SUCCESS;
	octarr_t       *on = 0;
	octet_t        *oct;
	spocp_iobuf_t  *out = conn->out;
	ruleset_t      *rs = conn->rs;
	char           *str;
	/*
	 * struct timeval tv ;
	 * 
	 * gettimeofday( &tv, 0 ) ; 
	 */

	if (0)
		timestamp("QUERY");

	if (conn->transaction) return SPOCP_UNWILLING;

	LOG(SPOCP_INFO) {
		str = oct2strdup(conn->oparg->arr[0], '%');
		traceLog(LOG_INFO,"[%d]QUERY:%s", conn->fd, str);
		free(str);
	}

	if ((r = opinitial( conn, &rs, 1, 1, 1)) == SPOCP_SUCCESS) {
/*
		if (0)
			timestamp("ss_allow");
*/

		pthread_rdwr_rlock(&rs->rw_lock);
		r = ss_allow(rs, conn->oparg->arr[0], &on, SUBTREE);
		pthread_rdwr_runlock(&rs->rw_lock);

/*
		if (0)
			timestamp("ss_allow done");
*/
	}

	DEBUG(SPOCP_DSRV) 
		traceLog(LOG_DEBUG, "allow returned %d", r);

	if (r == SPOCP_SUCCESS && on) {
		while ((oct = octarr_pop(on))) {
			if (add_response_blob(out, SPOCP_MULTI, oct) != SPOCP_SUCCESS) {
				r = SPOCP_OPERATIONSERROR;
				break;
			}

			str = oct2strdup(oct, '%');
			DEBUG(SPOCP_DSRV) 
				traceLog(LOG_DEBUG,"returns \"%s\"", str);
			free(str);

			/*
			if ((wr = send_results(conn)) == 0)
				r = SPOCP_CLOSE;
			oct_free(oct);

			 *
			 * wait for the master to empty the queue 
			 *
			pthread_mutex_lock(&out->lock);
			pthread_cond_wait(&out->empty, &out->lock);
			pthread_mutex_unlock(&out->lock);
			*/
		}

		octarr_free(on);
	}

	return postop( conn, r, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

/*
 * int com_x( conn_t *conn, octet_t *com, octet_t *arg ) { int r = 0 ;
 * 
 * return r ; } 
 */

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_login(conn_t * conn)
{
	spocp_result_t  r = SPOCP_DENIED;

	LOG(SPOCP_WARNING) traceLog(LOG_WARNING,"LOGIN: not supported");

	return postop( conn, r, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_begin(conn_t * conn)
{
	spocp_result_t  rc = SPOCP_SUCCESS;
	ruleset_t      *rs = conn->srv->root;

	LOG(SPOCP_INFO) traceLog(LOG_INFO,"BEGIN requested");

	if (conn->transaction)
		rc = SPOCP_EXISTS;
	else if ((rc = operation_access(conn)) == SPOCP_SUCCESS) {

		pthread_mutex_lock(&rs->transaction);
		pthread_rdwr_rlock(&rs->rw_lock);

		if ((conn->rs = ss_dup(rs, SUBTREE)) == 0)
			rc = SPOCP_OPERATIONSERROR;

		/*
		 * to allow read access 
		 */
		pthread_rdwr_runlock(&rs->rw_lock);

		if (rc != SPOCP_SUCCESS) {
			pthread_mutex_unlock(&rs->transaction);
			conn->transaction = 0;
		} else
			conn->transaction = 1;
	}

	return postop( conn, rc, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_subject(conn_t * conn)
{
	spocp_result_t  r = SPOCP_DENIED;
	ruleset_t      *rs = conn->srv->root;

	if (conn->transaction) return SPOCP_UNWILLING;

	LOG(SPOCP_INFO) traceLog(LOG_INFO,"SUBJECT definition");

	if ((r = opinitial( conn, &rs, 0, 1, 1)) == SPOCP_SUCCESS) {
		if (conn->oparg->n)
			conn->sri.subject = octarr_pop(conn->oparg);
		else
			oct_free(conn->sri.subject);
	}

	return postop( conn, r, 0);
}

/*
 * ------------------------------------------------------------------------- 
 * * ADD [ruleset_def] s-exp [blob] add = "3:ADD" [l-path] l-s-expr [
 * bytestring ] moving toward add = "3:ADD" [l-path] l-s-expr [ bcond [
 * bytestring ]]
 * 
 */

spocp_result_t
com_add(conn_t * conn)
{
	spocp_result_t	rc = SPOCP_DENIED;	/* The default */
	ruleset_t	*rs = conn->rs;
	srv_t		*srv = conn->srv;

	LOG(SPOCP_INFO) traceLog(LOG_INFO,"ADD rule");

	if ((rc = opinitial( conn, &rs, 1, 1, 3)) ==
	    (SPOCP_DENIED|UNKNOWN_RULESET)) {
		ruleset_t	*trs ;

		if ((trs = ruleset_create(conn->oppath, &rs))){
			rc = SPOCP_SUCCESS;
			rs = trs;
		}
	}

	if (rc == SPOCP_SUCCESS) {
		if (conn->transaction) {
			opstack_t *ops;
			ops = opstack_new( SPOCP_ADD, rs, conn->oparg );
			conn->ops = opstack_push( conn->ops, ops );
		}
		else {
			/*
			 * get write lock, do operation and release lock 
			 */
			pthread_rdwr_wlock(&rs->rw_lock);

			rc = dbapi_rule_add(&(rs->db), srv->plugin, &conn->dbc,
			    conn->oparg, NULL);

			pthread_rdwr_wunlock(&rs->rw_lock);
		}
	}

	return postop( conn, rc, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */
/*
 * list = "4:LIST" [l-path] *( length ":" "+"/"-" s-expr ) 
 */
spocp_result_t
com_list(conn_t * conn)
{
	spocp_result_t  rc = SPOCP_DENIED;
	ruleset_t      *rs = conn->rs;
	int             i, r = 0, wr;
	octet_t        *op;
	octarr_t       *oa = 0;	/* where the result is put */
	spocp_iobuf_t  *out = conn->out;

	LOG(SPOCP_INFO) traceLog(LOG_INFO,"LIST requested");

	if (conn->transaction) return SPOCP_UNWILLING;

	if ((rc = opinitial( conn, &rs, 1, 0, 0)) == SPOCP_SUCCESS ) {

/*	
		if (0)
			traceLog( LOG_INFO,"Getting the list" );
*/

		oa = octarr_new( 32 ) ;
		pthread_rdwr_rlock(&rs->rw_lock);
		rc = treeList(rs, conn, oa, 1);
		pthread_rdwr_runlock(&rs->rw_lock);

		
/*
		if (0)
			traceLog( LOG_INFO,"Translating list to result" );
*/

		if (oa && oa->n) {
			for (i = 0; i < oa->n; i++) {
				op = oa->arr[i];

				LOG(SPOCP_DEBUG) {
					char           *str;
					str = oct2strdup(op, '\\');
					traceLog(LOG_DEBUG,str);
					free(str);
				}

				if ((rc = add_response_blob(out, SPOCP_MULTI,
						op)) != SPOCP_SUCCESS)
					break;

				if ((wr = send_results(conn)) == 0) {
					r = SPOCP_CLOSE;
					break;
				}
			}
		}

		octarr_free(oa);
	}

	return postop( conn, rc, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_summary(conn_t * conn)
{
	spocp_result_t	rc = SPOCP_SUCCESS;
	ruleset_t	*rs = conn->rs;
	octet_t		*op;
	spocp_iobuf_t	*out = conn->out;
	element_t	*ep;
	char		*tmp;

	LOG(SPOCP_INFO) traceLog(LOG_INFO,"SUMMARYrequested");

	if (conn->transaction) return SPOCP_UNWILLING;

	if ((rc = operation_access(conn)) == SPOCP_SUCCESS) {

		pthread_rdwr_rlock(&rs->rw_lock);
		ep = get_indexes(rs->db->jp);
		pthread_rdwr_runlock(&rs->rw_lock);

		if (ep) {
			element_reduce(ep);
			op = oct_new( 512, NULL );
			if( element_print( op, ep ) == 0 )
				rc = SPOCP_OPERATIONSERROR;
			else {
				tmp = oct2strdup(op, 0);
				traceLog( LOG_INFO, "%s", tmp );
				free(tmp);
				rc = add_response_blob(out, SPOCP_MULTI, op);
			}
		}
		else 
			rc = SPOCP_OPERATIONSERROR;
	}

	return postop( conn, rc, 0);
}

/*
 * format of SHOW
 * 
 * show = "SHOW" l-path ruleid
 * 
 */
spocp_result_t
com_show(conn_t * conn)
{
	spocp_result_t	r = SPOCP_SUCCESS;
	ruleset_t	*rs = conn->rs;
	ruleinst_t	*ri = 0;
	char		*uid;

	LOG(SPOCP_INFO) traceLog(LOG_INFO,"SHOW requested ");

	/* possible pathspecification but apart from that exactly one argument */
	if ((r = opinitial(conn, &rs, 1, 1, 1)) == SPOCP_SUCCESS){
		/*
		 * get write lock, do operation and release lock 
		 */
		uid = oct2strdup( conn->oparg->arr[0], 0);
		pthread_rdwr_wlock(&rs->rw_lock);
		ri = ruleinst_find_by_uid(rs->db->ri->rules, uid);
		pthread_rdwr_wunlock(&rs->rw_lock);
		free(uid);

		if ( ri ){	
			r = add_response_blob(conn->out, SPOCP_MULTI, ri->rule);
		}
	}

	return postop( conn, r, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
do_reread_rulefile(conn_t *conn)
{
	ruleset_t	*rs = conn->rs;
	srv_t		*srv = conn->srv;
	spocp_result_t	rc;

	pthread_rdwr_wlock(&rs->rw_lock);

	/* get rid of all child databases */
	if (rs->down)
		ruleset_free(rs->down);
	if (rs->left)
		ruleset_free(rs->left);
	if (rs->right)
		ruleset_free(rs->right);

	/* reset the main database */
	db_clr( rs->db );

	rc = get_rules( srv );

	pthread_rdwr_wunlock(&rs->rw_lock);

	conn_free( conn );

	return rc;
}

/*
 * --------------------------------------------------------------------------------- 
 */

/*
 * incomming data should be of the form operationlen ":" operlen ":" oper *(
 * arglen ":" arg )
 * 
 * starttls = "8:STARTTLS"
 * 
 * "10:8:STARTTLS"
 * 
 * query = "QUERY" SP [path] spocp-exp
 * 
 * "75:5:QUERY64:(5:spocp(8:resource3:etc5:hosts)(6:action4:read)(7:subject3:foo))"
 * 
 * add = "ADD" SP [path] spocp-exp bcond blob
 * 
 * "74:3:add60:(5:spocp(8:resource3:etc5:hosts)(6:action4:read)(7:subject))4:blob"
 * 
 * delete = "DELETE" SP ruleid
 * 
 * "51:6:DELETE40:3e6d0dd469d4d32a65b31f963481a18c8de891e1"
 * 
 * list = "LIST" *( SP "+"/"-" s-expr ) ; extref are NOT expected to appear
 * 
 * "72:4:LIST6:+spocp11:-(8:resource)17:+(6:action4:read)19:-(7:subject(3:uid))"
 * 
 * logout = "LOGOUT"
 * 
 * "8:6:LOGOUT"
 * 
 * begin = "BEGIN"
 * 
 * "7:5:BEGIN"
 * 
 * commit = "COMMIT"
 * 
 * "8:6:COMMIT"
 * 
 * rollback = "ROLLBACK"
 * 
 * "10:8:ROLLBACK"
 * 
 * subjectcom = "SUBJECT" SP subject
 * 
 * "24:7:SUBJECT12:(3:uid3:foo)"
 * 
 */

spocp_result_t
get_operation(conn_t * conn, proto_op ** oper)
{
	spocp_result_t  r;
	octet_t         wo, op, arg, oa;
	int             l = 0;
	/*
	 * char *tmp ; 
	 */

	DEBUG(SPOCP_DSRV) {
		int             n;

		if ((n = iobuf_content(conn->in)))
			traceLog(LOG_DEBUG,"%d bytes in input buffer", n);
		else
			traceLog(LOG_DEBUG,"Empty input buffer");
	}

	arg.val = conn->in->r;
	arg.len = conn->in->w - conn->in->r;

	DEBUG(SPOCP_DSRV) {
		char           *tmp;

		tmp = oct2strdup(&arg, '%');
		traceLog(LOG_DEBUG,"ARG. [%s]", tmp);
		free(tmp);
	}

	/*
	 * The whole operation with all the arguments 
	 */
	if ((r = get_str(&arg, &wo)) != SPOCP_SUCCESS)
		return r;

        DEBUG(SPOCP_DSRV) traceLog(LOG_DEBUG,"OpLen: %d", wo.len ) ;
        octln( &oa, &wo) ;

	/*
	 * Just the operation specification 
	 */
	if ((r = get_str(&oa, &op)) != SPOCP_SUCCESS)
		return r;

	octln(&conn->oper, &op);

	*oper = 0;

	l = op.len;
	switch (op.len) {
	case 8:
		if (strncasecmp(op.val, "STARTTLS", 8) == 0) {
			*oper = &com_starttls;
		} else if (strncasecmp(op.val, "ROLLBACK", 8) == 0) {
			*oper = &com_rollback;
		}

		break;

	case 7:
		if (strncasecmp(op.val, "SUBJECT", 7) == 0) 
			*oper = &com_subject;
		else if (strncasecmp(op.val, "SUMMARY", 7) == 0) 
			*oper = &com_summary;
		break;

	case 6:
		if (strncasecmp(op.val, "LOGOUT", 6) == 0) {
			*oper = &com_logout;
		} else if (strncasecmp(op.val, "DELETE", 6) == 0) {
			*oper = &com_delete;
		} else if (strncasecmp(op.val, "COMMIT", 6) == 0) {
			*oper = &com_commit;
		}
		break;

	case 5:
		if (strncasecmp(op.val, "QUERY", 5) == 0) {
			*oper = &com_query;
		} else if (strncasecmp(op.val, "LOGIN", 5) == 0) {
			*oper = &com_login;
		} else if (strncasecmp(op.val, "BEGIN", 5) == 0) {
			*oper = &com_begin;
		}
		/*
		 * else if( strncasecmp( op.val, "BCOND", 5 ) == 0 ) { *oper = 
		 * &com_bcond ; l = 5 ; } 
		 */
		break;

	case 4:
		if (strncasecmp(op.val, "LIST", 4) == 0) {
			*oper = &com_list;
		} else if (strncasecmp(op.val, "AUTH", 4) == 0) {
			*oper = &com_auth;
		} else if (strncasecmp(op.val, "CAPA", 4) == 0) {
			*oper = &com_capa;
		} else if (strncasecmp(op.val, "SHOW", 4) == 0) {
			*oper = &com_show;
		}
		break;

	case 3:
		if (strncasecmp(op.val, "ADD", 3) == 0) {
			*oper = &com_add;
		}
		break;

	default:
		l = 0;

	}

	if (l) {
		conn->in->r = wo.val + wo.len;
		while (conn->in->r != conn->in->w && WHITESPACE(*conn->in->r))
			conn->in->r++;
	}

	/*
	 * conn->oparg = 0 ; * should be done elsewhere 
	 */

	if ((r = get_oparg(&oa, &conn->oparg)) != SPOCP_SUCCESS)
		return r;

	if (*oper == 0)
		return SPOCP_UNKNOWNCOMMAND;
	else
		return SPOCP_SUCCESS;
}

/*
 * ================================================================================ 
 */
/*
 * Below is only used when reading and wrting to stdout, something you do when
 * you are debugging 
 */
/*
 * ================================================================================ 
 */

static void
send_and_flush_results(conn_t * conn)
{
	send_results(conn);

	spocp_conn_write(conn);
}

static int
native_server(conn_t * con)
{
	spocp_result_t  r = SPOCP_SUCCESS;
	spocp_iobuf_t  *in, *out;
	int             n, wr = 1;
	octet_t         attr;
	proto_op       *oper = 0;
	time_t          now = 0;

	in = con->in;
	out = con->out;
	time(&con->last_event);

	traceLog(LOG_INFO,"Running native server");

      AGAIN:
	while (wr && (n = spocp_conn_read(con)) >= 0) {

		time(&now);

		if (n == 0) {
			if (!con->srv->timeout)
				continue;

			if ((int) (now - con->last_event) > con->srv->timeout) {
				traceLog( LOG_INFO,
				   "Connection closed due to inactivity");
				break;
			} else
				continue;
		}

		if (n) {
			DEBUG(SPOCP_DSRV)
			    traceLog( LOG_DEBUG,"Got %d(%d new)  bytes",
			    in->w - in->r, n);
		}

		/*
		 * If no chars, obviously nothing for me to do 
		 */
		while (in->w - in->r) {
			if ((r = get_operation(con, &oper)) == SPOCP_SUCCESS) {
				r = (*oper) (con);

				DEBUG(SPOCP_DSRV) {
					traceLog(LOG_DEBUG,"command returned %d", r);
					traceLog(LOG_DEBUG,
					    "%d chars left in the input buffer",
				    	    in->w - in->r);
				}

				spocp_conn_write(con);
				oparg_clear(con);

				if (r == SPOCP_CLOSE)
					goto clearout;
				else if (r == SPOCP_MISSING_CHAR) {
					break;	/* I've got the whole package
						 * but it had internal
						 * deficienses */
				} else if (r == SPOCP_UNKNOWNCOMMAND) {	/* Unknown 
									 * keyword 
									 */
					add_response(out, r, 0);
					send_and_flush_results(con);
				}

				if (in->r == in->w)
					iobuf_flush(in);
			} else if (r == SPOCP_MISSING_CHAR) {
				LOG(SPOCP_DEBUG)
					traceLog(LOG_DEBUG,"get more input");

				if (in->left == 0) {	/* increase input
							 * buffer */
					int             l;

					attr.val = in->r;
					attr.len = in->w - in->r;
					/* try to get information on how much
					 * that is needed
					 */
					if(( l = get_len(&attr,&r)) < 0 )
						l = 2 * in->bsize;

					if ((r =
					     iobuf_resize(in,
							  l - attr.len, 1)) !=
					    SPOCP_SUCCESS) {
						add_response(out, r, 0);
						send_and_flush_results(con);
						oparg_clear(con);
						conn_iobuf_clear(con);
					}
				}
				goto AGAIN;
			} else {
				oparg_clear(con);
				conn_iobuf_clear(con);
			}
		}
		/*
		 * reset the timeout counter after the server is done with the 
		 * operations 
		 */
		time(&con->last_event);
	}

      clearout:
	conn_iobuf_clear(con);
	DEBUG(SPOCP_DSRV) traceLog(LOG_DEBUG,"(%d) Closing the connection", con->fd);

	return 0;
}


void
spocp_server(void *vp)
{
	conn_t         *co;
	/*
	 * ruleset_t *rs ; srv_t *srv ; 
	 */

	DEBUG(SPOCP_DSRV) traceLog(LOG_DEBUG,"In spocp_server");

	if (vp == 0)
		return;

	co = (conn_t *) vp;

	/*
	 * srv = co->srv ; pthread_mutex_lock( &(srv->mutex) ) ; rs =
	 * srv->root ; pthread_mutex_unlock( &(srv->mutex) ) ;
	 * 
	 * traceLog(LOG_DEBUG,"Server ready (timeout set to %d)", srv->timeout) ; 
	 */


	native_server(co);
}
