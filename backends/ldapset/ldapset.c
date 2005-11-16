
/***************************************************************************
			ldapset.c  -  description
			 -------------------
 ***************************************************************************/

#include <config.h>

/*
 * Format of the whole string ldapset = ldap_server *10( ";" DN) ";" dnvset
 * 
 * 
 * Format of the query part
 * 
 * dnvset	=	base |
 *			'(' dnvset ')' |
 *			'{' dnvset ext attribute '}' |
 *			dnvset SP conj SP dnvset |
 *			dnvset ext dnattribute |
 *			'<' dn '>'
 *
 * valvset	=	'"' string '"' |
 *			'(' valvset ')' |
 *			dnvset ext attribute |
 *			valvset SP conj SP valvset
 * 
 * base		=	'\' d
 * 
 * conj		=	"&" | "|"
 * ext		=	"/" | "%" | "$"	
 *	; base, onelevel resp. subtree search
 *	Distinguished names in a valueset can be stripped of their most
 *	significant RDN. This is represented by:
 *			"/../" | "%..%" | "$..$"
 *	If the two most significant RDNs are to be stripp of this is 
 *	represented by "/../../", "%..%..%" and "$..$..$" and so on.
 *
 *	If more RDNs then is available is configured to be stripped of the
 *	resultset becomes NULL 
 * 
 * a		=	%x41-5A / %x61-7A ; lower and upper case ASCII
 * d		=	%x30-39
 * k		=	a / d / "-" / ";"
 * anhstring	=	1*k
 * SP		=	0x20
 * 
 * 
 * attribute-list = attribute *[ "," attribute ] attribute = a [ anhstring]
 *	; as defined by [RFC2252]
 * 
 * dnattribute-list = dnattribute *[ "," dnattribute ]
 * 	; dnattribute = any attribute name which have attributetype
 *	distinguishedName (1.3.6.1.4.1.1466.115.121.1.12) like member,
 *	owner, roleOccupant, seeAlso, modifiersName, creatorsName,...
 * 
 * Ex, <cn=Group>/member & user {user$mail &
 * [sven.svensson@minorg.se]}/title,personalTitle & [mib] <cn=Group>/member &
 * {user$mail & [tw@minorg.se]} 
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lber.h>
#include <ldap.h>

#include <spocp.h>
#include <be.h>
#include <wrappers.h>
#include <plugin.h>
#include <rvapi.h>
#include <func.h>

#include "ldapset.h"

/* #define AVLUS 1 */


/*
 * ========================================================== 
 */

static char	*ls_strdup( char *s );

lsln_t	*get_results(LDAP *, LDAPMessage *, lsln_t *, lsln_t *, int, int);

lsln_t	*do_ldap_query(vset_t * vsetp, LDAP * ld, spocp_result_t * ret);
LDAP	*open_conn(char *server, spocp_result_t * ret);

befunc	ldapset_test;

/* Should get a set of DN on which I have to strip of one or more RDNs
 * If stripping RDNs from two DNs results in the same DN then only
 * one copy should be placed in the resultset 
 */

static char *
_striprdn( char *sdn, int n ) {
	LDAPDN	dn = NULL, ndn;
	LDAPRDN	*rdn;
	char 	*s;
	int	r,i;

#ifdef AVLUS
	traceLog(LOG_DEBUG,"stripping %d rdns from %s",n,sdn);
#endif
	r = ldap_str2dn( sdn, &dn, LDAP_DN_FORMAT_LDAPV3 );

	/* how many parts are there */
	for( rdn = dn, i = 0; rdn[i]; i++ );
	
	if ( i < n ) {
		/* Can't go higher than this */
		s = strdup("");
	}
	else {

		rdn += n;
		ndn = rdn;
		r=ldap_dn2str(ndn, &s, LDAP_DN_FORMAT_LDAPV3|LDAP_DN_PEDANTIC);
	}

	ldap_memfree( dn );

#ifdef AVLUS
	traceLog(LOG_DEBUG,"_striprdn returned %s", s );
#endif
	return s;
}

static lsln_t *
striprdn( lsln_t *ll, int n)
{
	lsln_t	*tl, *nl = 0;
	char	*s;

	for( tl = ll; tl; tl = tl->next ) {
		s = _striprdn( tl->str, n);
		nl = lsln_add( nl, s );
	}

	lsln_free(ll);

	return nl;
}

/*
 * ========================================================== 
 */


static char *
ls_strdup( char *s )
{
	char *new, *sp, *cp;

	if( strpbrk( s, "*()\\") == NULL ) {
		return strdup( s );
	}

	/* Probably much to big but better safe than sorry */
	new = ( char * ) malloc((strlen(s)*3+1) * sizeof( char ));

	for( sp = s, cp = new; *sp; sp++ ) {
		switch( *sp ) {
		case '*':
		*cp++ = '\\';
		*cp++ = '2';
		*cp++ = 'a';
		break;
		case '(':
		*cp++ = '\\';
		*cp++ = '2';
		*cp++ = '8';
		break;
		case ')':
		*cp++ = '\\';
		*cp++ = '2';
		*cp++ = '9';
		break;
		case '\\':
		*cp++ = '\\';
		*cp++ = '5';
		*cp++ = 'c';
		break;
		default:
		*cp++ = *sp;
		}
	}
	*cp = '\0';

	return new;
}


/*
 * ========================================================== 
 */

static char	*
safe_strcat(char *dest, char *src, int *size)
{
	char	*tmp;
	int	 dl, sl;

	if (src == 0 || *size <= 0)
		return 0;

	dl = strlen(dest);
	sl = strlen(src);

	if (sl + dl + 1 > *size) {
		*size = sl + dl + 128;
		tmp = realloc(dest, *size);
		dest = tmp;
	}

	strcat(dest, src);

	return dest;
}

lsln_t	 *
get_results(LDAP * ld,
	LDAPMessage * res, lsln_t * attr, lsln_t * val, int type, int ao)
{
	int	 i, nr, wildcard = 0;
	lsln_t	 *set = 0;
	LDAPMessage	*e;
	BerElement	*be = NULL;
	char	**vect = 0, *ap, *dn;

	nr = ldap_count_entries(ld, res);

	/*
	* could I have to many ?? 
	*/
	/*
	* LOG( SPOCP_DEBUG )
	*     traceLog(LOG_DEBUG, "Found %d matching entries", nr ) ; 
	*/
	if ( val && index(val->str,'*'))
		wildcard = 1;

	if (ao == SC_OR) {
		/*
		* traceLog(LOG_DEBUG,"SC_OR") ; 
		*/
		if (type == SC_VAL)
			set = lsln_dup(val);

		for (e = ldap_first_entry(ld, res); e != NULL;
			e = ldap_next_entry(ld, e)) {

			/*
			* I'm might not be really interested in the attribute 
			* values, only interested in the DNs and those are
			* not listed among the val's 
			*/
			if (type == SC_DN) {
				/*
				* LOG( SPOCP_DEBUG )
				*     traceLog(LOG_DEBUG, "Collecting DNs" ) ; 
				*/
				dn = ldap_get_dn(ld, e);
				set = lsln_add(set, ls_strdup(dn));
				ldap_memfree(dn);
			} else {
				/*
				* LOG( SPOCP_DEBUG )
				*     traceLog(LOG_DEBUG,
				*     "Collecting attribute values" ) ; 
				*/
				for (ap = ldap_first_attribute(ld, e, &be);
					ap != NULL;
					ap = ldap_next_attribute(ld, e, be)) {
					/*
					* unsigned comparision between
					* attribute names 
					*/
					/*
					* LOG( SPOCP_DEBUG )
					*     traceLog(LOG_DEBUG,
					*     "Got attribute: %s", ap ) ; 
					*/

					if (lsln_find(attr, ap) == 0)
						continue;

					vect = ldap_get_values(ld, e, ap);

					for (i = 0; vect[i]; i++) {
						/*
						* LOG( SPOCP_DEBUG )
						* traceLog(LOG_DEBUG,
						* "Among the ones I look for"); 
						*/
						/*
						* disregard if not a value
						* I'm looking for 
						*/
						if ( !wildcard &&
						    lsln_find(val,vect[i])==0)
							continue;

						set = lsln_add(set,
						    ls_strdup(vect[i]));
					}

					ldap_value_free(vect);
				}
			}
		}
	} else {
		/*
		* traceLog(LOG_DEBUG,"SC_AND") ; 
		*/
		for (e = ldap_first_entry(ld, res); e != NULL;
			e = ldap_next_entry(ld, e)) {

			if (type == SC_DN) {
				/*
				* LOG( SPOCP_DEBUG )
				* traceLog(LOG_DEBUG, "Collecting DNs" ) ; 
				*/
				dn = ldap_get_dn(ld, e);
				set = lsln_add(set, ls_strdup(dn));
				ldap_memfree(dn);
			} else {
				/*
				* LOG( SPOCP_DEBUG )
				*    traceLog(LOG_DEBUG,
				*    "Collecting attribute values" ) ; 
				*/

				for (ap = ldap_first_attribute(ld, e, &be);
					ap != NULL;
					ap = ldap_next_attribute(ld, e, be)) {

					/*
					* LOG( SPOCP_DEBUG )
					*     traceLog(LOG_DEBUG,
					*     "Got attribute: %s", ap ) ; 
					*/

					/*
					* uninteresting attribute 
					*/
					if (lsln_find(attr, ap) == 0)
						continue;

					vect = ldap_get_values(ld, e, ap);

					for (i = 0; vect[i]; i++) {
						if (!wildcard &&
						    lsln_find(val,vect[i])==0)
							continue;

						set = lsln_add(set,
						    ls_strdup(vect[i]));
					}

					ldap_value_free(vect);
				}
			}
		}
	}

	/*
	* if( set ) { lsln_t *tmp ;
	* 
	* for( tmp = lsln_first( set ), i = 0 ; tmp ; tmp = lsln_next( tmp ), 
	* i++ )
	*   LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "set[%d]: %s", i, (char *)
	* lsln_get_val( tmp )) ; } 
	*/

	return set;
}

/*
 * server is [ user [ : password ] @ ] host 
 */

LDAP	*
open_conn(char *server, spocp_result_t * ret)
{
	LDAP	*ld = 0;
	int	 rc, vers = 3;
	char	*user = 0, *passwd = 0;

	if ((passwd = index(server, '@'))) {
		user = server;
		*passwd = 0;
		server = passwd + 1;
		if ((passwd = index(user, ':'))) {
			*passwd++ = 0;
		}
	}

	LOG( SPOCP_DEBUG)
		traceLog(LOG_DEBUG,"Opening LDAP con to %s", server ) ; 

	if ((ld = ldap_init(server, 0)) == 0) {
		LOG( SPOCP_WARNING )
			traceLog(LOG_DEBUG,
				"Error: Couldn't initialize the LDAP server") ; 
		*ret = SPOCP_UNAVAILABLE;
		return 0;
	}

	/*
	* set version to 3, surprisingly enough openldap 2.1.4 seems to use 2 
	* as default 
	*/

	if (ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &vers) !=
		LDAP_SUCCESS) {
		LOG( SPOCP_WARNING )
			traceLog(LOG_WARNING,
			    "Error: Couldn't set the version") ; 
		*ret = SPOCP_UNAVAILABLE;
		ldap_unbind(ld);
		return 0;
	}

	/*
	* automatic follow referrals 
	*/
	if (ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_ON) !=
		LDAP_SUCCESS) {
		LOG( SPOCP_WARNING )
			traceLog(LOG_WARNING,
			    "Error: Couldn't set follow referrals") ; 
		*ret = SPOCP_UNAVAILABLE;
		ldap_unbind(ld);
		return 0;
	}


	if ((rc = ldap_simple_bind_s(ld, user, passwd)) != LDAP_SUCCESS) {
		LOG( SPOCP_WARNING )
			traceLog(LOG_WARNING,
			    "LDAP bind failed to %s", server ) ; 
		*ret = SPOCP_UNAVAILABLE;
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

lsln_t	 *
do_ldap_query(vset_t * vsetp, LDAP * ld, spocp_result_t * ret)
{
	int	 rc, scope, size = 256, na;
	lsln_t	 *arr = 0, *sa = 0;
	lsln_t	 *attrset = vsetp->attset, *dn = vsetp->dn, *val =
		vsetp->val;
	char	*base, *va, **attr;
	char	*filter;
	LDAPMessage	*res = 0;

	/*
	* openended queries are not allowed 
	*/
	if (vsetp->attset == 0)
		return 0;

	/*
	* No base for the queries, nothing to do then 
	*/
	if (dn == 0)
		return 0;

	/*
	* AND anything to the empty set and you will get the empty set 
	*/
	if ((vsetp->type == SC_AND) && (val == 0))
		return 0;

	*ret = SPOCP_SUCCESS;
	filter = (char *) malloc(size * sizeof(char));

	na = lsln_values(vsetp->attset);
	attr = lsln_to_arr(vsetp->attset);

	if (vsetp->scope == SC_BASE)
		scope = LDAP_SCOPE_BASE;
	else if (vsetp->scope == SC_ONELEV)
		scope = LDAP_SCOPE_ONELEVEL;
	else
		scope = LDAP_SCOPE_SUBTREE;

	memset(filter, 0, size);

	/*
	* same filter for all queries, so just do it once 
	*/

	if (na > 1)
		filter = safe_strcat(filter, "(|", &size);

	for (; attrset; attrset = attrset->next) {
		if (val) {
			if (lsln_values(val) == 1) {
				va = lsln_get_val(val);
				filter = safe_strcat(filter, "(", &size);
				filter =
				    safe_strcat(filter,lsln_get_val(attrset),
					&size);
				filter = safe_strcat(filter, "=", &size);
				filter = safe_strcat(filter, va, &size);
				filter = safe_strcat(filter, ")", &size);
			} else {
				filter = safe_strcat(filter, "(|", &size);
				for (val = vsetp->val; val; val = val->next) {
					va = lsln_get_val(val);
					filter =
						safe_strcat(filter, "(", &size);
					filter =
						safe_strcat(filter,
							lsln_get_val(attrset),
							&size);
					filter =
						safe_strcat(filter, "=", &size);
					filter =
						safe_strcat(filter, va, &size);
					filter =
						safe_strcat(filter, ")", &size);
				}
				filter = safe_strcat(filter, ")", &size);
			}
		} else {
			filter = safe_strcat(filter, "(", &size);
			filter =
				safe_strcat(filter,lsln_get_val(attrset),&size);
			filter = safe_strcat(filter, "=*)", &size);
		}
	}

	if (na > 1)
		filter = safe_strcat(filter, ")", &size);

	/*
	* val might be 0, that is I'm not really interested in the set of
	* values 
	*/

	for (; dn; dn = dn->next) {

		base = lsln_get_val(dn);
		res = 0;

		LOG( SPOCP_DEBUG ) {
			char *ftmp, *btmp;
			ftmp = str_esc(filter, strlen(filter));
			btmp = str_esc(base, strlen(base));
			traceLog(LOG_DEBUG,
				"Using filter: %s, base: %s, scope: %d",
				ftmp, btmp, scope ) ; 
			free(ftmp);
			free(btmp);
		}

		if ((rc =
			ldap_search_s(ld,base,scope,filter,attr,0,&res))) {
			ldap_perror(ld, "During search");
			lsln_free(arr);
			arr = 0;
			*ret = SPOCP_OTHER;
		} else {
			if ((sa =
				get_results(ld, res, vsetp->attset, val,
					vsetp->restype, vsetp->type))) {

				if( vsetp->striprdn) 
					sa = striprdn( sa, vsetp->striprdn );

				arr = lsln_join(arr, sa);
				lsln_free(sa);
			}
		}

		if (res)
			ldap_msgfree(res);
	}

	free(filter);
	free(attr);
	/*
	*/

	LOG( SPOCP_DEBUG ) {
		if (arr == 0)
			traceLog(LOG_DEBUG, "LDAP return NULL (%d)", *ret );
		else {
			traceLog(LOG_DEBUG, "LDAP returned a set (%d)", *ret);
			lsln_print( arr );
		}
	} 

	return arr;
}

static int
P_ldapclose(void *con)
{
	return ldap_unbind_s((LDAP *) con);
}

/*
 */
spocp_result_t
ldapset_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  r = SPOCP_DENIED, rc = SPOCP_SUCCESS ;
	scnode_t	*scp;
	vset_t	 *vset, *res;
	char	*ldaphost, *tmp;
	octet_t	*oct, *o, cb;
	LDAP	*ld = 0;
	becon_t		*bc = 0;
	octarr_t	*argv;
	int	 cv = 0;
	pdyn_t	 *dyn = cpp->pd;

	if (cpp->arg == 0 || cpp->arg->len == 0)
		return SPOCP_MISSING_ARG;

	LOG( SPOCP_DEBUG ) {
		tmp = oct2strdup( cpp->arg, '%' );
		LOG( SPOCP_DEBUG) traceLog( LOG_DEBUG , "Ldapset test on: \"%s\"", tmp );
		free(tmp);
	}

	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	LOG( SPOCP_DEBUG ) {
		tmp = oct2strdup( oct, '%' );
		LOG( SPOCP_DEBUG) traceLog( LOG_DEBUG , "Expanded: \"%s\"", tmp);
		free(tmp);
	}

	cb.len = 0;
	if (dyn)
		cv = cached(dyn->cv, oct, &cb);

	if (cv) {
		LOG( SPOCP_DEBUG) traceLog(LOG_DEBUG,"ldapset: cache hit");

		if (cv == EXPIRED) {
			cached_rm(dyn->cv, oct);
			cv = 0;
		}
	}

	if (cv == 0) {

		argv = oct_split(oct, ';', '\\', 0, 0);

#ifdef AVLUS
		octarr_print(LOG_DEBUG, argv);
#endif

		o = argv->arr[argv->n - 1];

		if ((scp = scnode_get(o, &rc)) != 0) {
#ifdef AVLUS
			traceLog(LOG_DEBUG,"scnode_get done");
			{
				char in[32];

				memset( in, 0, 32);
				scnode_print( scp, in );
			}
#endif
			if ((vset = vset_get(scp, SC_UNDEF, argv, &rc)) != 0) {
				while (vset->up)
					vset = vset->up;

#ifdef AVLUS
				traceLog(LOG_DEBUG, "vset");
				vset_print(vset, 0);
#endif

				o = argv->arr[0];

				if (dyn == 0 || (bc = becon_get(o, dyn->bcp)) == 0) {

					ldaphost = oct2strdup(o, 0);
					ld = open_conn(ldaphost, &rc);
					free(ldaphost);

					if (ld == 0) {
						traceLog( LOG_INFO, "LDAP host (%s) unavailable",
							ldaphost);
						r = SPOCP_UNAVAILABLE;
					}
					else if (dyn && dyn->size) {
						if (!dyn->bcp)
							dyn->bcp =
								becpool_new(dyn->size);
						bc = becon_push(o, &P_ldapclose,
								(void *) ld, dyn->bcp);
					}
				} else
					ld = (LDAP *) bc->con;

				if (ld != 0) {
					res = vset_compact(vset, ld, &rc);

					if (res != 0) {
						if (res->restype == SC_DN && res->dn)
							r = SPOCP_SUCCESS;
						else if ((res->restype == SC_VAL
								|| res->restype == SC_UNDEF) && res->val)
							r = SPOCP_SUCCESS;
						else
							r = SPOCP_DENIED;

						vset_free(res);
					} else {
/*
						if (rc)
							r = rc;
						else
*/
						r = SPOCP_DENIED;
					}

					if (bc)
						becon_return(bc);
					else
						ldap_unbind_s(ld);
				} else
					vset_free(vset);

			} else
				r = rc;

			scnode_free(scp);
		} else {
			traceLog(LOG_DEBUG,"Error while parsing boundary condition");
			r = rc;
		}

		octarr_free(argv);
	}

	if (cv == (CACHED | SPOCP_SUCCESS)) {
		if (cb.len)
			octln(blob, &cb);
		r = SPOCP_SUCCESS;
	} else if (cv == (CACHED | SPOCP_DENIED)) {
		r = SPOCP_DENIED;
	} else {
		if (dyn && dyn->ct && (r == SPOCP_SUCCESS || r == SPOCP_DENIED)) {
			time_t	t;
			t = cachetime_set(oct, dyn->ct);
			dyn->cv =
				cache_value(dyn->cv, oct, t, (r | CACHED), 0);
		}
	}

	if (oct != cpp->arg)
		oct_free(oct);

	LOG( SPOCP_DEBUG) traceLog(LOG_DEBUG,"ldapset => %d", r);

	return r;
}

plugin_t	ldapset_module = {
	SPOCP20_PLUGIN_STUFF,
	ldapset_test,
	NULL,
	NULL,
	NULL
};
