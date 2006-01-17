
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

spocp_result_t com_find(work_info_t *wi) ;

typedef struct _rescode {
    int             code;
    char           *rc;
    char           *def;
} rescode_t;

rescode_t       rescode[] = {
    {SPOCP_WORKING, "100", "Working, please wait"},

    {SPOCP_SUCCESS, "200", "Ok"},
    {SPOCP_MULTI, "201", NULL},
    {SPOCP_DENIED, "202", "Denied"},
    {SPOCP_CLOSE, "203", "Bye"},
    {SPOCP_SSL_START, "205", "Ready to start TLS"},

    {SPOCP_AUTHDATA, "301", NULL},
    {SPOCP_AUTHINPROGRESS, "300", "Authentication in progress"},

    {SPOCP_BUSY, "400", "Busy"},
    {SPOCP_TIMEOUT, "401", "Timeout"},
    {SPOCP_TIMELIMITEXCEEDED, "402", "Timelimit exceeded"},
    {SPOCP_REDIRECT, "403", NULL},

    {SPOCP_SYNTAXERROR, "500", "Syntax error"},
    {SPOCP_MISSING_ARG, "501", "Missing Argument"},
    {SPOCP_MISSING_CHAR, "502", "Input error"},
    /*{SPOCP_PROTOCOLERROR, "503", "Protocol error"},*/
    {SPOCP_UNKNOWNCOMMAND, "504", "Unknown command"},
    {SPOCP_PARAM_ERROR, "505", "Argument error"},
    {SPOCP_SSL_ERR, "506", "SSL Accept error"},
    {SPOCP_UNKNOWN_TYPE, "507", "Uknown range type"},

    {SPOCP_SIZELIMITEXCEEDED, "511", "Sizelimit exceeded"},
    {SPOCP_OPERATIONSERROR, "512", "Operation error"},
    {SPOCP_UNAVAILABLE, "513", "Service not available"},
    {SPOCP_NOT_SUPPORTED, "515", "Command not supported"},
    {SPOCP_STATE_VIOLATION, "516", "SSL Already active"},
    {SPOCP_OTHER, "517", "Other error"},
    {SPOCP_CERT_ERR, "518", "Authentication error"},
    {SPOCP_UNWILLING, "519", "Unwilling to perform"},
    {SPOCP_EXISTS, "520", "Already exists"},
    {SPOCP_AUTHERR, "521", "Authentication error"},

    {0, NULL, NULL}
};

/***************************************************************************
 ***************************************************************************/

static
int lvadd( octet_t *o, char *str, int len )
{
    char    lf[32];
    int nr;

    nr = snprintf(lf, 32, "%d:", len);
    octcat(o, lf, strlen(lf));
    octcat(o, str, len );

    return 1;
}

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
add_response_va(octet_t *out, int rc, const char *fmt, va_list ap)
{
    int             n;
    spocp_result_t  sr = SPOCP_SUCCESS;
    char            buf[SPOCP_MAXLINE];
    rescode_t      *rescode = find_rescode(rc);

    if (rescode == NULL)
        rescode = find_rescode(SPOCP_OTHER);

    lvadd(out, rescode->rc, strlen(rescode->rc));

    if (fmt) {
        /* Flawfinder: ignore */
        n = vsnprintf(buf, SPOCP_MAXLINE, fmt, ap);
        if (n >= SPOCP_MAXLINE) /* OUTPUT truncated */
            sr = SPOCP_LOCAL_ERROR;
        else
            lvadd(out, buf, strlen(buf));
    } else {
        if (rescode->def != NULL)
            lvadd(out, rescode->def, strlen(rescode->def));
    }

#ifdef AVLUS
    oct_print(LOG_INFO, "response_va", out );
#endif

    return sr;
}

static spocp_result_t
add_response(spocp_iobuf_t * out, int rc, const char *fmt, ...)
{
    va_list         ap;
    spocp_result_t  res;
    octet_t     *oct;

    oct = oct_new(512,NULL);
    va_start(ap, fmt);
    res = add_response_va(oct, rc, fmt, ap);
    va_end(ap);

    iobuf_add_octet( out, oct);
#ifdef AVLUS
    traceLog(LOG_DEBUG,"total response: %s",out->buf);
#endif
    oct_free(oct);

    return res;
}

static spocp_result_t
add_response_blob(spocp_iobuf_t * out, int rc, octet_t * data)
{
    spocp_result_t  sr;
    octet_t     *oct;

    oct = oct_new(512,NULL);
    sr = add_response_va(oct, rc, NULL, NULL);

    if (sr != SPOCP_SUCCESS)
        return sr;

    if (data) {
#ifdef AVLUS
        oct_print(LOG_DEBUG,"add response",data);
#endif
        sr = lvadd( oct, data->val, data->len) ;
    }

    if (sr != SPOCP_SUCCESS) {
        oct_free(oct);
        return sr;
    }

    iobuf_add_octet( out, oct);
#ifdef AVLUS
    traceLog(LOG_DEBUG,"total response: %s",out->buf);
#endif
    oct_free(oct);

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

/* ---------------------------------------------------------------------- */

static spocp_result_t 
opinitial( work_info_t *wi, ruleset_t **rs, int path, int min, int max)
{
    spocp_result_t r = SPOCP_SUCCESS;
    octarr_t    *oparg = wi->oparg;

    if (oparg == 0 ) {
        if (min) 
            return SPOCP_MISSING_ARG;
        else
            return SPOCP_SUCCESS;
    }

#ifdef AVLUS
    traceLog( LOG_INFO,
        "opinitial(1): max=%d, min=%d, path=%d .. oparg->n=%d",
        max,min,path, oparg->n);
#endif
    /* Too many or few arguments? There might not be a max */
    if (max && oparg->n > (path+max)) return SPOCP_PARAM_ERROR;
    if (oparg->n < min) return SPOCP_MISSING_ARG;

    /* Only path definitions start with '/' and they have to */
    if( path && *(oparg->arr[0]->val) == '/') 
        wi->oppath = octarr_pop(oparg);
    else 
        wi->oppath = 0;
    
#ifdef AVLUS
    traceLog( LOG_INFO,
        "opinitial(2): max=%d, min=%d, path=%d .. oparg->n=%d",
        max,min,path, oparg->n);
#endif

    if (oparg->n < min) return SPOCP_MISSING_ARG;

    /* allowed to run the operation ? */
    if (( r = operation_access(wi)) == SPOCP_SUCCESS) {

        /* if a ruleset path is defined and exists use it */
        if( wi->oppath != 0 &&
            (*rs = ruleset_find(wi->oppath, *rs)) == NULL) {
            char           *str;

            str = oct2strdup(wi->oppath, '%');
            traceLog(LOG_INFO,"ERR: No \"%s\" ruleset", str);
            Free(str);

            r = UNKNOWN_RULESET;
        }
    } 

    return r;
}

static spocp_result_t
postop( work_info_t *wi, spocp_result_t rc, const char *msg)
{
    int ar;

    if( msg && *msg ) {
        traceLog(LOG_INFO,"return message:%s", msg);
        add_response(wi->buf, rc, msg);
        Free((char *) msg);
    }
    else {
        ar = add_response(wi->buf, rc, 0);
        /*
        traceLog( LOG_DEBUG, "add_response returned %d", ar);
        */
    }

#ifdef AVLUS
    iobuf_print("postop work buffer",wi->buf);
#endif
    iobuf_shift(wi->conn->in);

    return rc;
}

spocp_result_t
return_busy( work_info_t *wi )
{
    return postop( wi, SPOCP_BUSY, NULL );
}

/* ---------------------------------------------------------------------- *
 * Get the SASL capabilities
 * ---------------------------------------------------------------------- */


spocp_result_t
com_capa(work_info_t *wi)
{
    spocp_result_t  r = SPOCP_SUCCESS;
    const char  *msg = NULL;
    conn_t      *conn = wi->conn;

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"Attempt to authenticate");
    if (conn->transaction)
        return postop( wi, SPOCP_UNWILLING, 0);

#ifdef HAVE_SASL
    if (conn->sasl == NULL) {
        int wr;

        wr = sasl_server_new("spocp",
                     conn->srv->hostname,
                     NULL, NULL, NULL, NULL, 0, &conn->sasl);
        if (wr != SASL_OK) {
            LOG(SPOCP_ERR)
                traceLog(LOG_ERR,"Failed to create SASL context");
            r = SPOCP_OTHER;
            goto capa_done;
        }
    }

    { /* get and list auth mechs available to us */
        const char  *mechs;
        size_t      mechlen;
        int     count;
        int     wr;

        wr = sasl_listmech(conn->sasl, NULL, "SASL:", " SASL:", NULL, &mechs,
                   &mechlen, &count);
        if (wr != SASL_OK) {
            LOG(SPOCP_ERR)
                traceLog(LOG_ERR,"Failed to generate SASL mechanism list");
            r = SPOCP_OTHER;
        } else {
            msg = mechs;
        }
    }

      capa_done:
#endif

    add_response(wi->buf, r, msg);
    iobuf_shift(wi->conn->in);
    return(r);
}

spocp_result_t
com_auth(work_info_t *wi)
{
    const char  *msg = NULL;
    conn_t      *conn = wi->conn;
#ifdef HAVE_SASL
    const char     *data;
    size_t          len;
    octet_t         blob;
    int             wr = -1;
    int             r;
#endif

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"Attempt to authenticate");
    if (conn->transaction) 
        return postop(wi, SPOCP_UNWILLING ,msg);

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

    if (conn->sasl_mech == NULL) {  /* start */
        conn->sasl_mech = oct2strdup(wi->oparg->arr[0], '%');
        if(strstr(conn->sasl_mech, "SASL:"))
            conn->sasl_mech += 5;
        else
            goto check;
        wr = sasl_server_start(conn->sasl,
                conn->sasl_mech,
                wi->oparg->n > 1 ? wi->oparg->arr[1]->val : NULL,
                wi->oparg->n > 1 ? wi->oparg->arr[1]->len : 0,
                &data,
                &len);
    } else {        /* step */
        wr = sasl_server_step(conn->sasl,
                wi->oparg->n > 0 ? wi->oparg->arr[0]->val : NULL,
                wi->oparg->n > 0 ? wi->oparg->arr[0]->len : 0,
                &data,
                &len);
    }

    memset(&blob, 0, sizeof(blob));
    if (data) {
        blob.val = (char *) data;
        blob.len = len;
    }

      check:

    switch (wr) {
        /* finished with authentication */
    case SASL_OK:
        wr = sasl_getprop(conn->sasl, SASL_USERNAME,
                  (const void **) &conn->sasl_username);
        add_response_blob(wi->buf, SPOCP_MULTI, &blob);
        r = SPOCP_SUCCESS;
        msg = Strdup("Authentication OK");
        break;
        /* we need to step some */
    case SASL_CONTINUE:
        add_response_blob(wi->buf, SPOCP_AUTHDATA, &blob);
        r = SPOCP_AUTHINPROGRESS;
        break;
    default:
        LOG(SPOCP_ERR) traceLog(LOG_ERR,"SASL start/step failed: %s",
                    sasl_errstring(wr, NULL, NULL));
        r = SPOCP_AUTHERR;
        msg = Strdup("Authentication failed");
        conn->sasl_mech -= 5;
        free(conn->sasl_mech);
        conn->sasl_mech = NULL;
        sasl_dispose(&conn->sasl);
        conn->sasl = NULL;
    }

    return postop(wi, r, msg);

#else
    return postop( wi, SPOCP_NOT_SUPPORTED, NULL);
#endif
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_starttls(work_info_t *wi)
{
    spocp_result_t  r = SPOCP_SSL_START;
    conn_t      *conn = wi->conn;

    if (conn->transaction)
        return postop(wi, SPOCP_UNWILLING, NULL);

    traceLog(LOG_INFO,"Attempting to start SSL/TLS");

#if !(defined(HAVE_SSL) || defined(HAVE_SASL))
    r = SPOCP_NOT_SUPPORTED;
#else

#ifdef HAVE_SASL
    if (conn->sasl != NULL) {
        LOG(SPOCP_ERR)
            traceLog(LOG_ERR,"Layering violation: SASL already in operation");
        r = SPOCP_STATE_VIOLATION;  /* XXX definiera och ... */
    } else
#endif
#ifdef HAVE_SSL
    if (conn->ssl != NULL) {
        LOG(SPOCP_ERR)
            traceLog(LOG_ERR,"Layering violation: SSL already in operation");
        r = SPOCP_STATE_VIOLATION;  /* XXX ... fixa informativt
                         * felmeddleande */
    } else
#endif
    if ((r = operation_access(wi)) == SPOCP_SUCCESS) {
        /*
         * Ready to start TLS/SSL 
         */
        r = SPOCP_SSL_START;
        traceLog(LOG_INFO,"Setting connection status so main won't touch it") ;
        conn->sslstatus = REQUEST;    /* Negotiation in progress */
    }
#endif

    return postop( wi, r, 0);
}

spocp_result_t
com_tlsneg(work_info_t *wi)
{
    spocp_result_t  r = SPOCP_SUCCESS;
    conn_t          *conn = wi->conn;

    traceLog(LOG_INFO,"SSLNeg %d", conn->sslstatus);
    if (conn->sslstatus != NEGOTIATION) {
       r = SPOCP_DENIED;
    }
    /*
     * If I couldn't create a TLS connection fail completely 
     */
    else{
        traceLog(LOG_INFO,"Waiting for SSL negotiation");
        if ((r = tls_start(conn, conn->rs)) != SPOCP_SUCCESS) {
            LOG(SPOCP_ERR) traceLog(LOG_ERR,"Failed to start SSL");
            r = SPOCP_CLOSE;
            conn->stop = 1;
        } else
            traceLog(LOG_INFO,"SSL in operation");

        conn->sslstatus = ACTIVE; /* Negotiation done */
    }

    return postop( wi, r, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_rollback(work_info_t *wi)
{
    spocp_result_t  r = SPOCP_SUCCESS;
    conn_t      *conn = wi->conn;

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"ROLLBACK");

    if (conn->srv->readonly) {
        r = SPOCP_DENIED;
    } else if( conn->transaction ){
        opstack_free( conn->ops );
        conn->ops = 0;
    } else { 
        r = SPOCP_UNWILLING;
    }

    return postop( wi, r, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_logout(work_info_t *wi)
{
    spocp_result_t  r = SPOCP_CLOSE;
    conn_t      *conn = wi->conn;

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"LOGOUT requested ");

    if (conn->transaction)
        opstack_free( conn->ops );

    /* place response in write queue before marking connection for closing */
    r = postop( wi, r, 0);

    conn->stop = 1;

    return r;
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_noop(work_info_t *wi)
{
    return postop( wi, SPOCP_SUCCESS, 0 );
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
com_delete(work_info_t *wi)
{
    spocp_result_t  r = SPOCP_SUCCESS;
    conn_t      *conn = wi->conn;
    ruleset_t   *rs = conn->rs;
    octet_t     *oct;

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"DELETE requested ");

    if (conn->srv->readonly) {
        r = SPOCP_DENIED;
    } else {
        /* possible path specification but apart from that
           exactly one argument */
        r = opinitial( wi, &rs, 1, 1, 1);
    }

    if (r == SPOCP_SUCCESS){
        if (rs->db->ri->rules == NULL) {
            traceLog(LOG_INFO, "No rule to remove");
            r = SPOCP_DENIED;
        }
        else if (conn->transaction) {
            opstack_t *ops;
            ops = opstack_new( SPOCP_DEL, rs, wi->oparg );
            conn->ops = opstack_push( conn->ops, ops );
        }
        else {
            oct = wi->oparg->arr[0] ;
                
            if ( oct->len != SHA1HASHLEN ) 
                return postop( wi, SPOCP_SYNTAXERROR, 0);
            /*
             * get write lock, do operation and release lock 
             * locking the whole tree, is that really necessary ? 
             */
            pthread_rdwr_wlock(&rs->rw_lock);
            r = dbapi_rule_rm(rs->db, &conn->dbc, oct, NULL);
            pthread_rdwr_wunlock(&rs->rw_lock);
        }
    }

    return postop( wi, r, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_commit(work_info_t *wi)
{
    spocp_result_t  r = SPOCP_SUCCESS;
    conn_t      *conn = wi->conn;
    opstack_t   *ops;
    ruleset_t   **rspp;
    int     i, n;

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"COMMIT requested");

    if (conn->srv->readonly) {
        r = SPOCP_DENIED;
    } else if (!conn->transaction)
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
                    wi->oparg->arr[0]);
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
                        wi->oparg, NULL);
                    break;
                }
            }
        }

        for( i = 0 ; i < n ; i++ ) 
            pthread_rdwr_wunlock(&(rspp[i])->rw_lock);

        Free(rspp);
        opstack_free( conn->ops );
        conn->ops = 0;
        conn->transaction = 0;
    }

    return postop( wi, r, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

static spocp_result_t
_query(work_info_t *wi, int all)
{
    spocp_result_t  r = SPOCP_SUCCESS;
    conn_t      *conn = wi->conn;
    resset_t    *rset = 0;
    octet_t     *oct;
    spocp_iobuf_t   *out = wi->buf;
    ruleset_t   *rs = conn->rs;
    char        *str, *type;

    /*
     * struct timeval tv ;
     * 
     * gettimeofday( &tv, 0 ) ; 
     */

#ifdef AVLUS
    timestamp("QUERY");
#endif

    if (conn->transaction)
        return postop(wi, SPOCP_UNWILLING,0);

    LOG(SPOCP_INFO) {
        if ( wi->oparg ) {
            if (all)
                type = "BSEARCH";
            else
                type = "QUERY";

            if (wi->oparg->n == 1) {
                str = oct2strdup(wi->oparg->arr[0], '%');
                traceLog(LOG_INFO,"[%d]%s:%s", conn->fd,type,str);
                Free(str);
            }
            else if (wi->oparg->n == 2) {
                char *pa, *se;

                pa = oct2strdup(wi->oparg->arr[0], '%');
                se = oct2strdup(wi->oparg->arr[1], '%');
                traceLog(LOG_INFO,"[%d]%s:%s %s",
                    conn->fd, type, pa, se);
                Free(pa);
                Free(se);
            }
        }
    }

    /* ruleset_tree( rs, 0 ); */
    r = opinitial( wi, &rs, 1, 1, 1);
    if (r == SPOCP_SUCCESS) {
/*
        if (0)
            timestamp("ss_allow");
*/

        pthread_rdwr_rlock(&rs->rw_lock);
        r = ss_allow(rs, wi->oparg->arr[0], &rset, SUBTREE|all);
        pthread_rdwr_runlock(&rs->rw_lock);

#ifdef AVLUS
        timestamp("ss_allow done");
#endif
    }

    DEBUG(SPOCP_DSRV) { 
        traceLog(LOG_DEBUG, "allow returned %d", r);
#ifdef AVLUS
        if (rset)
            resset_print( rset );
#endif
    }

    if (r == SPOCP_SUCCESS && rset) {
        resset_t *trs = rset;
        octarr_t *on;

        for( ; trs; trs = trs->next) {
            if (trs->blob == 0)
                continue;

            on = trs->blob;

#ifdef AVLUS
            octarr_print(LOG_DEBUG,on);
#endif
            while (on && (oct = octarr_pop(on))) {
                if (add_response_blob(out, SPOCP_MULTI, oct)
                    != SPOCP_SUCCESS) {
                    r = SPOCP_OPERATIONSERROR;
                    break;
                }

                oct_print(LOG_INFO,"returns blob", oct);

                oct_free(oct);
            }
        }
    }

    if (rset)
        resset_free(rset);

    return postop( wi, r, 0);
}

spocp_result_t
com_query(work_info_t *wi)
{
        return _query( wi, 0);
}

static spocp_result_t
com_bsearch(work_info_t *wi)
{
        return _query( wi, 0x10);
}

/*
 * --------------------------------------------------------------------------------- 
 */

/*
 * int com_x( conn_t *conn, octarr_t *oparg)
 * {
 * int r = 0 ;
 * return postop(wi, r, 0) ;
 * } 
 */

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_login(work_info_t *wi)
{
    spocp_result_t  r = SPOCP_DENIED;

    LOG(SPOCP_WARNING) traceLog(LOG_WARNING,"LOGIN: not supported");

    return postop( wi, r, 0);
}

/*
 * --------------------------------------------------------------------------------- 
 */

spocp_result_t
com_begin(work_info_t *wi)
{
    spocp_result_t  r = SPOCP_SUCCESS;
    conn_t      *conn = wi->conn;
    ruleset_t      *rs = conn->srv->root;

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"BEGIN requested");

    if (conn->srv->readonly) {
        r = SPOCP_DENIED;
    } else if (conn->transaction) {
        r = SPOCP_EXISTS;
    } else if ((r = operation_access(wi)) == SPOCP_SUCCESS) {

        pthread_mutex_lock(&rs->transaction);
        pthread_rdwr_rlock(&rs->rw_lock);

        if ((conn->rs = ss_dup(rs, SUBTREE)) == 0)
            r = SPOCP_OPERATIONSERROR;

        /*
         * to allow read access 
         */
        pthread_rdwr_runlock(&rs->rw_lock);

        if (r != SPOCP_SUCCESS) {
            pthread_mutex_unlock(&rs->transaction);
            conn->transaction = 0;
        } else
            conn->transaction = 1;
    }

    return postop( wi, r, 0);
}

/*
 * ----------------------------------------------------------------------------- 
 *  Setting the subject definition, that is 'who' is trying to run the command
 * -----------------------------------------------------------------------------
 */

spocp_result_t
com_subject(work_info_t *wi)
{
    spocp_result_t  r = SPOCP_DENIED;
    conn_t      *conn = wi->conn;
    ruleset_t   *rs = conn->srv->root;

    if (conn->transaction)
        return postop( wi, SPOCP_UNWILLING, 0);

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"SUBJECT definition");

    if (conn->srv->readonly) {
        r = SPOCP_DENIED;
    } else {
        r = opinitial(wi, &rs, 0, 1, 1) ;
    }

    if (r == SPOCP_SUCCESS) {
        if (wi->oparg->n)
            conn->sri.subject = octarr_pop(wi->oparg);
        else
            oct_free(conn->sri.subject);
    }

    return postop( wi, r, 0);
}

/*
 * ------------------------------------------------------------------------- 
 * * ADD [ruleset_def] s-exp [blob] add = "3:ADD" [l-path] l-s-expr [
 * bytestring ] moving toward add = "3:ADD" [l-path] l-s-expr [ bcond [
 * bytestring ]]
 * 
 */

spocp_result_t
com_add(work_info_t *wi)
{
    spocp_result_t  rc = SPOCP_DENIED;  /* The default */
    conn_t      *conn = wi->conn;
    ruleset_t   *rs = conn->rs;
    srv_t       *srv = conn->srv;

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"ADD rule");

    if (conn->srv->readonly) {
        rc = SPOCP_DENIED;
    } else {
        rc = opinitial( wi, &rs, 1, 1, 3);
    }

    if (rc == UNKNOWN_RULESET) {
        octet_t     oct;

        octln(&oct, wi->oppath);
        /* set to NULL by opinitial */
        rs = conn->rs;
        if (ruleset_create(wi->oppath, rs) != NULL ){
            rc = SPOCP_SUCCESS;
            rs = ruleset_find(&oct, rs);
        }
    }

    if (rule_access(wi) != SPOCP_SUCCESS) {
        traceLog(LOG_INFO, "Operation disallowed");
        rc = SPOCP_DENIED;
    }

    if (rc == SPOCP_SUCCESS) {
        if (conn->transaction) {
            opstack_t *ops;
            ops = opstack_new( SPOCP_ADD, rs, wi->oparg );
            conn->ops = opstack_push( conn->ops, ops );
        }
        else {
            /*
             * get write lock, do operation and release lock 
             */
            pthread_rdwr_wlock(&rs->rw_lock);

            rc = dbapi_rule_add(&(rs->db), srv->plugin, &conn->dbc,
                wi->oparg, NULL);

            pthread_rdwr_wunlock(&rs->rw_lock);
        }
    }

    return postop( wi, rc, 0);
}

/*
 * ---------------------------------------------------------------------------- 
 */
/*
 * list = "4:LIST" [l-path] *( length ":" "+"/"-" s-expr ) 
 */
spocp_result_t
com_list(work_info_t *wi)
{
    spocp_result_t  rc = SPOCP_DENIED;
    conn_t      *conn = wi->conn;
    ruleset_t      *rs = conn->rs;
    int             i, r = 0, wr;
    octet_t        *op;
    octarr_t       *oa = 0; /* where the result is put */
    spocp_iobuf_t  *out = wi->buf;

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"LIST requested");

    if (conn->transaction) {
        r = SPOCP_UNWILLING;
    } else {
        rc = opinitial(wi, &rs, 1, 0, 0) ;
    }

    if (rc == SPOCP_SUCCESS ) {

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
                    Free(str);
                }

                if ((rc = add_response_blob(out, SPOCP_MULTI,
                        op)) != SPOCP_SUCCESS)
                    break;

                if ((wr = send_results(conn)) == 0) {
                    rc = SPOCP_CLOSE;
                    break;
                }
            }
        }

        octarr_free(oa);
    }

    return postop( wi, rc, 0);
}

/*
 * --------------------------------------------------------------------------- 
 *  Provides you with a list of all the rule indeces
 * --------------------------------------------------------------------------- 
 */

spocp_result_t
com_summary(work_info_t *wi)
{
    spocp_result_t  rc = SPOCP_SUCCESS;
    conn_t      *conn = wi->conn;
    ruleset_t   *rs = conn->rs;
    octet_t      oct;
    spocp_iobuf_t   *out = wi->buf;
    int          i, wr;
    varr_t      *ia = NULL;

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"SUMMARY requested");

    if (conn->transaction)
        return postop(wi, SPOCP_UNWILLING,0);

    if ((rc = operation_access(wi)) == SPOCP_SUCCESS) {

        pthread_rdwr_rlock(&rs->rw_lock);
        ia = collect_indexes(rs);
        pthread_rdwr_runlock(&rs->rw_lock);

        if (ia) {
            for (i = 0; i < ia->n ; i++ ){
                octset(&oct,((ruleinst_t *)(ia->arr[i]))->uid, SHA1HASHLEN);
                oct_print(LOG_INFO, "rule", &oct);
                if ((rc = add_response_blob(out, SPOCP_MULTI, &oct))
                    != SPOCP_SUCCESS)
                    break;

                if ((wr = send_results(conn)) == 0) {
                    rc = SPOCP_CLOSE;
                    break;
                }
            }
            varr_free(ia);
        }
        else 
            rc = SPOCP_OPERATIONSERROR;
    }

    return postop( wi, rc, 0);
}

/*
 * format of SHOW
 * 
 * show = "SHOW" l-path ruleid
 * 
 * Present information about a specific rule, given the rule id
 */
spocp_result_t
com_show(work_info_t *wi)
{
    spocp_result_t  r = SPOCP_SUCCESS;
    ruleset_t   *rs = wi->conn->rs;
    ruleinst_t  *ri = 0;
    char        *uid;
    octet_t     *oct;

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"SHOW requested ");

    /* possible path specification but apart from that exactly one argument */
    r = opinitial(wi, &rs, 1, 1, 1) ;

    if (r == SPOCP_SUCCESS){
        /*
         * get write lock, do operation and release lock 
         */
        uid = oct2strdup( wi->oparg->arr[0], 0);
        pthread_rdwr_wlock(&rs->rw_lock);
        ri = ruleinst_find_by_uid(rs->db->ri->rules, uid);
        pthread_rdwr_wunlock(&rs->rw_lock);
        Free(uid);

        if ( ri ){  
            oct = ruleinst_print(ri, rs->name);
            r = add_response_blob(wi->buf, SPOCP_MULTI, oct);
            oct_free( oct );
        }
    }

    return postop( wi, r, 0);
}

spocp_result_t
com_find(work_info_t *wi)
{
    spocp_result_t  r = SPOCP_SUCCESS;
    /*
    ruleset_t   *rs = wi->conn->rs;
    ruleinst_t  *ri = 0;
    char        *uid;
    */

    LOG(SPOCP_INFO) traceLog(LOG_INFO,"FIND requested ");

    return postop( wi, r, 0);
}

/*
 * ----------------------------------------------------------------------------- 
 */

spocp_result_t
do_reread_rulefile( work_info_t *wi)
{
    conn_t      *conn = wi->conn;
    ruleset_t   *rs = conn->rs;
    srv_t       *srv = conn->srv;
    spocp_result_t  rc;

    pthread_rdwr_wlock(&rs->rw_lock);

    /* get rid of all child databases */
    if (rs->down)
        ruleset_free(rs->down, 1);

    ruleset_free_onelevel(rs);

    /* reset the main database */
    db_clr( rs->db );

    rc = get_rules( srv );

    pthread_rdwr_wunlock(&rs->rw_lock);

    conn_free( conn );

    return rc;
}

/*
 * ----------------------------------------------------------------------------- 
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
get_operation(work_info_t *wi )
{
    spocp_result_t  r;
    octet_t         wo, op, arg, oa;
    int             l = 0;
    conn_t      *conn = wi->conn;
    proto_op    *oper = 0;
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
        Free(tmp);
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

    octln(&wi->oper, &op);

    l = op.len;
    switch (op.len) {
    case 8:
        if (strncasecmp(op.val, "STARTTLS", 8) == 0) {
            oper = &com_starttls;
        } else if (strncasecmp(op.val, "ROLLBACK", 8) == 0) {
            oper = &com_rollback;
        }

        break;

    case 7:
        if (strncasecmp(op.val, "SUBJECT", 7) == 0) 
            oper = &com_subject;
        else if (strncasecmp(op.val, "SUMMARY", 7) == 0) 
            oper = &com_summary;
        else if (strncasecmp(op.val, "BSEARCH", 7) == 0) 
            oper = &com_bsearch;
        break;

    case 6:
        if (strncasecmp(op.val, "LOGOUT", 6) == 0) {
            oper = &com_logout;
        } else if (strncasecmp(op.val, "DELETE", 6) == 0) {
            oper = &com_delete;
        } else if (strncasecmp(op.val, "COMMIT", 6) == 0) {
            oper = &com_commit;
        } else if (strncasecmp(op.val, "TLSNEG", 6) == 0) {
            oper = &com_tlsneg;
        }
        break;

    case 5:
        if (strncasecmp(op.val, "QUERY", 5) == 0) {
            oper = &com_query;
        } else if (strncasecmp(op.val, "LOGIN", 5) == 0) {
            oper = &com_login;
        } else if (strncasecmp(op.val, "BEGIN", 5) == 0) {
            oper = &com_begin;
        }
        /*
         * else if( strncasecmp( op.val, "BCOND", 5 ) == 0 ) { *oper = 
         * &com_bcond ; l = 5 ; } 
         */
        break;

    case 4:
        if (strncasecmp(op.val, "LIST", 4) == 0) {
            oper = &com_list;
        } else if (strncasecmp(op.val, "AUTH", 4) == 0) {
            oper = &com_auth;
        } else if (strncasecmp(op.val, "CAPA", 4) == 0) {
            oper = &com_capa;
        } else if (strncasecmp(op.val, "SHOW", 4) == 0) {
            oper = &com_show;
        } else if (strncasecmp(op.val, "FIND", 4) == 0) {
            oper = &com_find;
        } else if (strncasecmp(op.val, "NOOP", 4) == 0) {
            oper = &com_noop;
        }
        break;

    case 3:
        if (strncasecmp(op.val, "ADD", 3) == 0) {
            oper = &com_add;
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
     * oparg = 0 ; * should be done elsewhere 
     */

    if ((r = get_oparg(&oa, &wi->oparg)) != SPOCP_SUCCESS) {
        octarr_free(wi->oparg);
        wi->oparg = 0;
        return r;
    }

    if (*oper == 0) {
        return SPOCP_UNKNOWNCOMMAND;
        traceLog(LOG_INFO,"Unknown command");
    }
    else {
        wi->routine = oper;
        return SPOCP_SUCCESS;
    }
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

static void work_info_init( work_info_t *wi )
{
    memset( wi, 0, sizeof( work_info_t ));
    wi->buf = iobuf_new( 4096 );
}

static void work_info_reset( work_info_t *wi )
{
    wi->routine = 0;
    wi->conn = 0;
    wi->oparg = 0;
    wi->oppath = 0;
    wi->oper.val = 0;
    wi->oper.len = 0;
    iobuf_flush(wi->buf);
}

static int
native_server(conn_t * con)
{
    spocp_result_t  r = SPOCP_SUCCESS;
    spocp_iobuf_t  *in, *out;
    int             n, wr = 1;
    octet_t         attr;
    time_t          now = 0;
    work_info_t wi;

    in = con->in;
    out = con->out;
    time(&con->last_event);

    work_info_init(&wi);

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
            work_info_reset(&wi);
            wi.conn = con;
            if ((r = get_operation(&wi))
                == SPOCP_SUCCESS) {
                r = (wi.routine) (&wi);

                DEBUG(SPOCP_DSRV) {
                    traceLog(LOG_DEBUG,"command returned %d", r);
                    traceLog(LOG_DEBUG,
                        "%d chars left in the input buffer",
                            in->w - in->r);
                }
                reply_add( con->head, &wi );
                if ( send_results( con ) == 0 )
                    r = SPOCP_CLOSE ;

                octarr_free(wi.oparg);

                if (r == SPOCP_CLOSE)
                    goto clearout;
                else if (r == SPOCP_MISSING_CHAR) {
                    break;  /* I've got the whole package
                         * but it had internal
                         * deficienses */
                } else if (r == SPOCP_UNKNOWNCOMMAND) { /* Unknown 
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

                if (in->left == 0) {    /* increase input
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
                        octarr_free(wi.oparg);
                        conn_iobuf_clear(con);
                    }
                }
                goto AGAIN;
            } else {
                octarr_free(wi.oparg);
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
    iobuf_free(wi.buf);

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
