#include "locl.h"
#include <sexptool.h>

/*
typedef char   *(argfunc) (conn_t * r);

typedef struct {
	char           *var;
	argfunc        *af;
	char            type;
	char            dynamic;
} arg_t;
*/

static sexpargfunc get_path;

static sexpargfunc get_action;
static sexpargfunc get_arguments;
static sexpargfunc get_ip;
static sexpargfunc get_host;
static sexpargfunc get_inv_host;
static sexpargfunc get_subject;
#ifdef HAVE_SSL
static sexpargfunc get_ssl_vers;
static sexpargfunc get_ssl_cipher;
static sexpargfunc get_ssl_subject;
static sexpargfunc get_ssl_issuer;
#endif
static sexpargfunc get_transpsec;


const sexparg_t           transf[] = {
	{"ip", get_ip, 'a', FALSE},
	{"host", get_host, 'a', FALSE},
	{"invhost", get_inv_host, 'l', FALSE},
	{"subject", get_subject, 'l', FALSE},
	{"operation", get_action, 'l', TRUE},
	{"arguments", get_arguments, 'l', TRUE},
	{"path", get_path, 'l', TRUE},
#ifdef HAVE_SSL
	{"ssl_vers", get_ssl_vers, 'a', FALSE},
	{"ssl_cipher", get_ssl_cipher, 'a', FALSE},
	{"ssl_subject", get_ssl_subject, 'a', FALSE},
	{"ssl_issuer", get_ssl_issuer, 'a', FALSE},
#endif
	{"transportsec", get_transpsec, 'l', FALSE}
};

int             ntransf = (sizeof(transf) / sizeof(transf[0]));

#ifdef HAVE_SSL

#define TPSEC_X509	0
#define TPSEC_X509_WCC	1
#define TPSEC_KERB	2

char           *tpsec[] = {
	"(12:TransportSec(4:vers%{ssl_vers})(12:chiphersuite%{ssl_cipher}))",
	"(12:TransportSec(4:vers%{ssl_vers})(12:chiphersuite%{ssl_cipher})(7:autname4:X509(7:subject%{ssl_subject})(6:issuer%{ssl_issuer})))",
	"(12:TransportSec(4:vers%{kerb_vers})(7:autname8:kerberos%{kerb_realm}%{kerb_localpart}))"
};

sexparg_t         **tpsec_X509;
sexparg_t         **tpsec_X509_wcc;

#endif

char           *srvquery =
    "(6:server(2:ip%{ip})(4:host%{host})%{transportsec})";
char           *operquery =
    "(9:operation%{operation}(6:server(2:ip%{ip})(4:host%{host})%{transportsec}))";

sexparg_t         **srvq;
sexparg_t         **operq;

/*
 * ---------------------------------------------------------------------- 
 */

/*
 * static void argt_free( sexparg_t **arg ) { int i ;
 * 
 * if( arg ) { for( i = 0 ; arg[i] || arg[i]->type != 0 ; i++ ) { if(
 * arg[i]->var ) free( arg[i]->var ) ; free( arg[i] ) ; } if( arg[i] ) free(
 * arg[i] ) ;
 * 
 * free( arg ) ; } } 
 */

/*
 * ---------------------------------------------------------------------- 
 */

static char    *
get_ip(void * conn)
{
	return ((( conn_t *) conn)->sri.hostaddr);
}

static char    *
get_host(void * conn)
{
	return ((( conn_t *) conn)->sri.hostname);
}

static char    *
get_subject(void * vp)
{
	char            sexp[512], *sp;
	unsigned int    size = 512;
	conn_t          *conn = ( conn_t * ) vp ;

	if (conn->sri.sexp_subject == 0) {
		sp = sexp_printv(sexp, &size, "o", conn->sri.subject);
		if (sp)
			conn->sri.sexp_subject = strdup(sexp);
	}

	return (conn->sri.sexp_subject);
}

static char    *
get_inv_host(void * vp)
{
	char          **arr, format[16], list[512], *sp;
	int             n, i, j;
	unsigned int    size = 512;
	conn_t		*conn = ( conn_t *) vp;

	if (conn->sri.invhost)
		return conn->sri.invhost;

	arr = line_split(conn->sri.hostname, '.', 0, 0, 0, &n);

	for (i = 0, j = n; i < (n + 1) / 2; i++, j--) {
		if (i == j)
			break;
		sp = arr[i];
		arr[i] = arr[j];
		arr[j] = sp;
	}

	for (i = 0; i <= n; i++)
		format[i] = 'a';
	format[i] = '\0';

	sexp_printa(list, &size, format, (void **) arr);

	conn->sri.invhost = strdup(list);

	return (conn->sri.invhost);
}

static char    *
get_path(void * vp)
{
	char           *res, *rp;
	unsigned int    size;
	const conn_t	*conn = ( conn_t * ) vp;

	if (conn->oppath == 0)
		return 0;

	size = conn->oppath->len + 9;

	res = (char *) Malloc(size * sizeof(char));

	rp = sexp_printv(res, &size, "o", conn->oppath);

	if (rp == 0)
		free(res);

	return rp;
}

static char    *
get_action(void * vp)
{
	char           *res, *rp;
	const conn_t	*conn = ( conn_t * ) vp;
	unsigned int    size = conn->oper.len + 9;

	/*
	 * traceLog(LOG_DEBUG,"Get_action") ; 
	 */

	res = (char *) Malloc(size * sizeof(char));

	rp = sexp_printv(res, &size, "o", &conn->oper);

	if (rp == 0)
		free(res);

	return rp;
}

static char    *
get_arguments(void * vp)
{
	char           *res, *rp, format[32];
	unsigned int    size;
	int             i;
	const conn_t 	*conn = ( conn_t *) vp ;

	if (conn->oparg == 0)
		return 0;

	for (i = 0, size = 0; i < conn->oparg->n; i++) {
		size += conn->oparg->arr[i]->len + 5;
		format[i] = 'o';
	}
	format[i] = '\0';

	res = (char *) Malloc(size * sizeof(char));

	rp = sexp_printa(res, &size, format, (void **) conn->oparg->arr);

	if (rp == 0)
		free(res);

	return rp;
}

#ifdef HAVE_SSL
static char    *
get_ssl_vers(void * c)
{
	return ((( conn_t *) c)->ssl_vers);
}

static char    *
get_ssl_cipher(void * c)
{
	return ((( conn_t * ) c)->cipher);
}

static char    *
get_ssl_subject(void * c)
{
	return ((( conn_t *) c)->subjectDN);
}

static char    *
get_ssl_issuer(void * c)
{
	return ((( conn_t *) c)->issuerDN);
}
#endif

/*
 * ---------------------------------------------------------------------- 
 */

static char    *
get_transpsec(void * vp)
{
	/*
	 * XXX fixa SPOCP_LAYER-jox här 
	 */
#ifdef HAVE_SSL
	conn_t 	*conn = ( conn_t * ) vp ;

	if (conn->ssl != NULL) {
		if (conn->transpsec == 0) {
			if (conn->subjectDN)
				conn->transpsec =
				    sexp_constr((void *) conn, tpsec_X509_wcc);
			else
				conn->transpsec =
				    sexp_constr((void *) conn, tpsec_X509);
		}
		return conn->transpsec;
	} else
#endif
		return "";

}

/*
 * ---------------------------------------------------------------------- 
 * ---------------------------------------------------------------------- 
 */

void
saci_init(void)
{
	/* sexparg_t **tp_sec_kerb ; 
	 */

#ifdef HAVE_SSL
	tpsec_X509 = parse_format(tpsec[TPSEC_X509], transf, ntransf);
	if (tpsec_X509 == 0)
		traceLog(LOG_ERR,"Could not parse TPSEC_X509");
	else
		LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"Parsed TPSEC_X509 OK");

	tpsec_X509_wcc = parse_format(tpsec[TPSEC_X509_WCC], transf, ntransf);
	if (tpsec_X509_wcc == 0)
		traceLog(LOG_ERR,"Could not parse TPSEC_X509_WCC");
	else
		LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"Parsed TPSEC_X509_WCC OK");
#endif
	/*
	 * tpsec_kerb = parse_format( tpsec[TPSEC_KERB], transf ) ; 
	 */

	srvq = parse_format(srvquery, transf, ntransf);
	if (srvq == 0)
		traceLog(LOG_ERR,"*ERR* Could not parse srvquery");
	else
		LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"Parsed srvquery OK");

	operq = parse_format(operquery, transf, ntransf);
	if (operq == 0)
		traceLog(LOG_ERR,"*ERR* Could not parse operquery");
	else
		LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"Parsed operquery OK");

}

/*
 * ---------------------------------------------------------------------- 
 */

static spocp_result_t
spocp_access(conn_t * con, sexparg_t ** arg, char *path)
{
	spocp_result_t  res = SPOCP_DENIED;	/* the default */
	ruleset_t      *rs;
	octet_t         oct;
	char           *sexp;
	element_t      *ep = 0;
	comparam_t      comp;

	/*
	 * no ruleset or rules means everything is allowed 
	 */
	if (con->rs == 0 || rules(con->rs->db) == 0) {
		if (0)
			traceLog(LOG_ERR,"No rules to tell me what to do");
		return SPOCP_SUCCESS;
	}

	oct_assign(&oct, path);

	if (0)
		traceLog(LOG_DEBUG,"Looking for ruleset [%s](%p)", path, con->rs);

	rs = con->rs;

	/*
	 * No ruleset means everything is allowed !!! 
	 */
	if (ruleset_find(&oct, &rs) == 0 || rs->db == 0)
		return SPOCP_SUCCESS;
	/*
	 * The same if there is no rules in the ruleset 
	 */
	if (rs->db->ri->rules == 0)
		return SPOCP_SUCCESS;

	if (0)
		traceLog(LOG_DEBUG,"Making the internal access query");
	sexp = sexp_constr(con, arg);

	LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"Internal access Query: \"%s\" in \"%s\"",
				  sexp, rs->name);

	/*
	 * is it a valid assumption to assume only 'printable' characters in
	 * sexp ? 
	 */
	oct_assign(&oct, sexp);

	if ((res = element_get(&oct, &ep)) != SPOCP_SUCCESS) {
		traceLog(LOG_ERR,"The S-expression \"%s\" didn't parse OK", sexp);

		free(sexp);
		return res;
	}

	if (oct.len) {		/* shouldn't be */
		free(sexp);
		element_free(ep);
		return SPOCP_DENIED;
	}

	comp.rc = SPOCP_SUCCESS;
	comp.head = ep;
	comp.blob = 0;

	res = allowed(rs->db->jp, &comp);

	free(sexp);
	element_free(ep);

	return res;
}

spocp_result_t
server_access(conn_t * con)
{
	char            path[MAXNAMLEN + 1];

	sprintf(path, "%s/server", localcontext);

	return spocp_access(con, srvq, path);
}

spocp_result_t
operation_access(conn_t * con)
{
	char            path[MAXNAMLEN + 1];
	spocp_result_t  r;

	sprintf(path, "%s/operation", localcontext);

	r = spocp_access(con, operq, path);

	if (r != SPOCP_SUCCESS)
		traceLog(LOG_INFO,"Operation disallowed");

	return r;
}
