
/***************************************************************************
                          ldapproxy.c  -  description
                             -------------------
    begin                : Fri Jun 11 2004
    copyright            : (C) 2003 by Stockholm University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/


/* */
/* format on ldapproxy
 * ldapproxy ":" orgdomain ":" uid [":" attributnamn ]
 * 
 * ldap server is found through the normal means
 * ldap.<orgdomain>, NAPTR (RFC 2915, 2916)
 * NAPTR
 *	_ldap._tcp.su.se IN SRV 0 0 389 ldap.su.se
 * + RFC 2247 => dc=su,dc=se
 *
 * res_query( "_ldap._tcp.umu.se", ns_t_srv, ns_c_in, ans[BUFSIZ], BUFSIZ);
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#ifdef __APPLE__
#include <arpa/nameser_compat.h>
#endif

#include "lber.h"
#include "ldap.h"

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>
#include <func.h>

befunc ldapproxy_test;

#ifndef HAVE_INTXX_T
typedef unsigned short	u_int16_t;
typedef unsigned int	u_int32_t;
#endif

typedef union {
    HEADER		hdr;
    unsigned char	buf[BUFSIZ];
} dns_packet_t;

typedef	struct {
	u_int16_t	type;
	u_int16_t	rr_class;
	u_int32_t	ttl;
	u_int16_t	rdlength;
	u_int16_t	priority;
	u_int16_t	weight;
	u_int16_t	port;
} rr_t;


typedef struct _srvrec {
	u_int16_t	priority;
	u_int16_t	weight;
	u_int16_t	port;
	char 		*srv;
	struct _srvrec	*next;
} srvrec_t ;

/* ---------------------------------------------------------------------- */

static void
srvrec_free( srvrec_t *sr )
{
	if (sr){
		if (sr->srv)
			free( sr->srv );
		if (sr->next)
			srvrec_free( sr->next );

		free( sr );
	}
}

/* ---------------------------------------------------------------------- */

static srvrec_t *
dnssrv_lookup( char *domain, char *proto )
{
	char		*dname, host[256];
	unsigned char	*sp, *eos ;
	int		dsize, n, nansv, npack, len;
	dns_packet_t	dns;
	rr_t		rr;
	srvrec_t	*srv = 0, *nsrv;

	dsize = strlen(domain)+strlen(proto)+8;
	dname = (char *) calloc(dsize , sizeof( char ));

	snprintf( dname, dsize-1, "_%s._tcp.%s", proto, domain);

	printf( "dname: %s\n", dname );

	res_init() ;

	if((n = res_query( dname, C_IN, T_SRV, dns.buf, BUFSIZ)) == -1 ||
	    n < (int) sizeof(HEADER)){
		return 0;
	}

	eos = dns.buf + n;

	npack = ntohs(dns.hdr.qdcount);
	nansv = ntohs(dns.hdr.ancount);

	sp = dns.buf + sizeof( HEADER );

	/* skip the packet records */
	while( npack > 0 && sp < eos ) {
		npack--;
		len = dn_expand( dns.buf, eos, sp, host, 256 );
		if( len < 0 ) 
			return NULL;
		sp += len + QFIXEDSZ ;
	}

	while( nansv > 0 && sp < eos ) {
		nansv--;
		len = dn_expand( dns.buf, eos, sp, host, 256 );
		if( len < 0 ) {
			return NULL;
		}

		sp += len;
		
		GETSHORT(rr.type,sp); 
		GETSHORT(rr.rr_class,sp); 
		GETLONG(rr.ttl,sp); 
		GETSHORT(rr.rdlength,sp); 
		GETSHORT(rr.priority,sp); 
		GETSHORT(rr.weight,sp); 
		GETSHORT(rr.port,sp); 
		
		len = dn_expand( dns.buf, eos, sp, host, 256 );
		if( len < 0 ) {
			return NULL;
		}

		nsrv = ( srvrec_t * ) calloc (1, sizeof( srvrec_t ));

		nsrv->srv = strdup(host);
		nsrv->priority = rr.priority;
		nsrv->weight = rr.weight;
		nsrv->port = rr.port;

		nsrv->next = srv;
		srv = nsrv;
	}

	free(dname);

	return srv;
}

/* ---------------------------------------------------------------------- */

static char *
fqdn2dn( char *fqdn )
{
	int	n, i, len;
	char	**arr, *dn, *tp;

	arr = line_split( fqdn, '.', 0, 0, 0, &n );

	for( i=0, len=0; arr[i]; i++) len += strlen(arr[i]) + 4 ;

	dn = ( char *) calloc (len+1, sizeof( char ));

	for( i=0, tp=dn; arr[i]; i++ ) {
		sprintf( tp, "dc=%s,", arr[i]);
		tp += strlen( tp);
	}
	--tp;
	*tp = '\0';

	for( i=0; arr[i]; i++ ) free( arr[i] );
	free(arr);

	return dn;
}

/* ---------------------------------------------------------------------- */

static int
P_ldapclose(void *con)
{
	return ldap_unbind_s((LDAP *) con);
}


/* ---------------------------------------------------------------------- */

static char	***
get_results(LDAP * ld, LDAPMessage * res)
{
	int             i, j, nr;
	LDAPMessage    *e;
	BerElement     *be = NULL;
	char          **vect = 0, *ap;
	char		***ava;

	nr = ldap_count_entries(ld, res);

	if( nr != 1 ) return NULL;

	e = ldap_first_entry(ld, res);

	for (ap = ldap_first_attribute(ld, e, &be), nr=0 ; ap != NULL;
	     ap = ldap_next_attribute(ld, e, be), nr++) ;

	ava = ( char *** ) calloc( nr+1, sizeof( char **));

	/*
	 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "Found %d matching entries", nr ) ; 
	 */

	for (ap = ldap_first_attribute(ld, e, &be), j=0 ; ap != NULL;
	     ap = ldap_next_attribute(ld, e, be), j++) {

		vect = ldap_get_values(ld, e, ap);

		for (i = 0; vect[i]; i++);

		ava[j] = ( char **) calloc( i+2, sizeof( char * ));
 
		ava[j][0] = strdup( ap );

		for (i = 0; vect[i]; i++) 
			ava[j][i+1] = strdup(vect[i]);

		ldap_value_free(vect);
	}

	return ava;
}

/* ---------------------------------------------------------------------- */
/*
 * <nya>
 * 	<nyaRoll>....</nyaRoll>
 * 	<ladokInstId>....</ladokInstId>
 * 	<givenName>....</givenName>
 *      <telephoneNumber>....</telephoneNumber>
 *      <mobileTelephoneNumber>....</mobileTelephoneNumber>
 *      <uid>....</uid>
 *      <sn>....</sn>
 *      <o>....</o>
 * </nya>
 */
/* ---------------------------------------------------------------------- */
/*
 * server is [ user [ : password ] @ ] host 
 */

static LDAP           *
open_conn(char *server, int port, spocp_result_t * ret)
{
	LDAP           *ld = 0;
	int             rc, vers = 3;
	char           *user = 0, *passwd = 0;

	if ((passwd = index(server, '@'))) {
		user = server;
		*passwd = 0;
		server = passwd + 1;
		if ((passwd = index(user, ':'))) {
			*passwd++ = 0;
		}
	}

	/*
	 * LOG( SPOCP_DEBUG) traceLog(LOG_DEBUG,"Opening LDAP con to %s", server ) ; 
	 */

	if ((ld = ldap_init(server, port)) == 0) {
		/*
		 * LOG( SPOCP_WARNING ) traceLog(LOG_WARNING, "Error: Couldn't initialize
		 * the LDAP server") ; 
		 */
		*ret = SPOCP_INFO_UNAVAIL;
		return 0;
	}

	/*
	 * set version to 3, surprisingly enough openldap 2.1.4 seems to use 2 
	 * as default 
	 */

	if (ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &vers) !=
	    LDAP_SUCCESS) {
		/*
		 * LOG( SPOCP_WARNING ) traceLog(LOG_WARNING, "Error: Couldn't set the
		 * version") ; 
		 */
		*ret = SPOCP_INFO_UNAVAIL;
		ldap_unbind(ld);
		return 0;
	}

	/*
	 * automatic follow referrals 
	 */
	if (ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_ON) !=
	    LDAP_SUCCESS) {
		/*
		 * LOG( SPOCP_WARNING ) traceLog(LOG_WARNING,
			"Error: Couldn't set follow referrals") ; 
		 */
		*ret = SPOCP_INFO_UNAVAIL;
		ldap_unbind(ld);
		return 0;
	}


	if ((rc = ldap_simple_bind_s(ld, user, passwd)) != LDAP_SUCCESS) {
		/*
		 * LOG( SPOCP_WARNING ) traceLog(LOG_WARNING, "LDAP bind failed to %s",
		 * server ) ; 
		 */
		*ret = SPOCP_INFO_UNAVAIL;
		ldap_unbind(ld);
		return 0;
	}

	/*
	 * if( use_tls && ldap_start_tls_s( ld, NULL, NULL ) != LDAP_SUCCESS ) 
	 * { if( use_tls > 1 ) { LOG( SPOCP_WARNING ) traceLog(LOG_WARNING,
	 * "LDAP_START_TLS failed" ) ; *ret = -1 ; ldap_unbind( ld ) ; return
	 * 0 ; } } 
	 */

	/*
	 * LOG( SPOCP_DEBUG) traceLog(LOG_DEBUG,"Bound OK" ) ; 
	 */

	return ld;
}

/* ---------------------------------------------------------------------- */

static char	***
do_ldap_query( LDAP * ld, const char *base, const char *filter, spocp_result_t * ret)
{
	int             rc, scope;
	LDAPMessage	*res = 0;
	char		*attr[] = { "nyaRole", "ladokInstId", "sn", 
				"givenName", "telephoneNumber", "mobileTelephoneNumber",
				"uid", "o",  NULL };
	char		***ava = NULL;

	/*
	 * No base for the queries, nothing to do then 
	 */
	if (base == 0)
		return 0;

        *ret = SPOCP_SUCCESS;

	scope = LDAP_SCOPE_SUBTREE;

	/*
	 * val might be 0, that is I'm not really interested in the set of
	 * values 
	 */

	LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG,"Using filter: %s, base: %s, scope: %d",
				filter, base, scope ) ; 

	if ((rc = ldap_search_s(ld, base, scope, filter, attr, 0, &res))) {
		ldap_perror(ld, "During search");
			*ret = SPOCP_OTHER;
	} else {
		if ((ava = get_results(ld, res)) == 0 ) 
			*ret = SPOCP_DENIED; /* ?? */
		
		if (res)
			ldap_msgfree(res);
	}

	LOG( SPOCP_DEBUG ) {
		if (ava == 0)
			traceLog(LOG_DEBUG, "LDAP return NULL (%d)", *ret );
		else
			traceLog(LOG_DEBUG, "LDAP return a set (%d)", *ret);
	} 

	return ava;
}

/* ---------------------------------------------------------------------- */

static char *
do_xml( char ***ava )
{
	char	*xml, *xp, *attr;
	int	len = 0, i, j, alen;
	
	for( i = 0; ava[i] ; i++) {
		alen = strlen(ava[i][0]);
		for( j = 1; ava[i][j] ; j++ ) {
			len += 2*alen + 5 + strlen( ava[i][j] );
		}
	}

	xp = xml = ( char * ) calloc ( len+14, sizeof( char ));

	sprintf(xp, "<nya>" );
	xp += strlen(xp);
	for( i = 0; ava[i] ; i++) {
		attr = ava[i][0];
		for( j = 1; ava[i][j] ; j++ ) {
			sprintf(xp, "<%s>%s</%s>", attr, ava[i][j], attr);
			xp += strlen(xp);
		}
	}
	sprintf(xp, "</nya>" );
	
	return xml;
}

/* ---------------------------------------------------------------------- */

spocp_result_t
ldapproxy_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t	r = SPOCP_DENIED;
	LDAP		*ld = 0;
	becon_t		*bc = 0;
	octarr_t	*argv;
	octet_t		*oct, *domain;
	pdyn_t		*dyn = cpp->pd;
	char		***ava = 0, *tmp, *filter, *fqdn;
	char 		*attr = 0, *val, *dn;
	char 		*xml;
	srvrec_t	*sr, *nsr;
	int		i, j;
	
	if (cpp->arg == 0 || cpp->arg->len == 0)
		return SPOCP_MISSING_ARG;

	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	argv = oct_split( oct, ':', '\\', 0,0 );

	traceLog( LOG_DEBUG,"argc: %d", argv->n);

	domain = argv->arr[0];

	fqdn = oct2strdup( domain, 0);

	if (dyn == 0 || (bc = becon_get(domain, dyn->bcp)) == 0) {
		/* find the server/-s */
		if(( sr = dnssrv_lookup( fqdn, "ldap")) == 0 ) {
			traceLog(LOG_DEBUG,"SRV record lookup failed");
			sr = ( srvrec_t * ) calloc( 1, sizeof( srvrec_t ));
			sr->srv = ( char * ) calloc( domain->len+6, sizeof( char ));
			tmp = oct2strdup( domain, 0 );
			snprintf(sr->srv, domain->len+5,"ldap.%s", tmp );
			free( tmp ); 
		}

		for( nsr = sr ; nsr ; nsr = nsr->next ) {
			/* should pick the one with the lowest priority first */
			LOG(SPOCP_DEBUG)
			    traceLog(LOG_DEBUG, "Trying %s:%d", nsr->srv, nsr->port);
			ld = open_conn(nsr->srv, nsr->port, &r);

			if (ld)
				break ;
		}

		srvrec_free( sr );

		if (ld == 0)
			r = SPOCP_UNAVAILABLE;
		else if (dyn && dyn->size) {
			if (!dyn->bcp)
				dyn->bcp = becpool_new(dyn->size);
			bc = becon_push(domain, &P_ldapclose, (void *) ld, dyn->bcp);
		}
	} else
		ld = (LDAP *) bc->con;

	if (r != SPOCP_UNAVAILABLE) {
		/* get the baseDN */
		dn = fqdn2dn( fqdn );
		free( fqdn );

		/* create the filter */
		val = oct2strdup( argv->arr[1], 0 );
		if( argv->n == 3 ) 
			attr = oct2strdup( argv->arr[2], 0 );
		else
			attr = "uid";

		filter = (char *)malloc( strlen(attr) + strlen(val) +4);
		sprintf( filter, "(%s=%s)", attr, val );

		traceLog(LOG_DEBUG, "Filter: %s, DN: %s", filter, dn);

		/* do the stuff */
	
		ava = do_ldap_query( ld, dn, filter, &r);

		if (bc)
			becon_return(bc);
		else
			ldap_unbind_s(ld);

		free(val);
		free(filter);
		free(dn);

		if( argv->n == 3) 
			free(attr);

	}

	/* create the blob */

	if (ava) {
		xml = do_xml( ava );
		oct_assign( blob, xml );
		blob->size = blob->len;
		traceLog(LOG_DEBUG, "%s", xml);

		for( i = 0; ava[i] ; i++) {
			for( j = 0; ava[i][j] ; j++) free( ava[i][j] );
			free( ava[i] );
		}
		free(ava);
	}

	if (oct != cpp->arg)
		oct_free(oct);

	return r;
}

plugin_t        ldapproxy_module = {
	SPOCP20_PLUGIN_STUFF,
	ldapproxy_test,
	NULL,
	NULL,
	NULL
};
