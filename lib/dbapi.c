
/***************************************************************************

                dbapi.c  -  contains the internal database interface 
                             -------------------

    begin                : Thu Mar 25 2004
    copyright            : (C) 2004 by Stockholm university, Sweden
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
#include <db0.h>
#include <dbapi.h>
#include <plugin.h>

/*
 * returns 
 */

spocp_result_t 
parse_canonsexp( octet_t *sexp, element_t **target)
{
	spocp_result_t	res = SPOCP_SUCCESS;
	char		*str;
	octet_t		oct;

	DEBUG(SPOCP_DPARSE) {
		str = oct2strdup(sexp, '%');
		traceLog(LOG_DEBUG,"Parsing the S-expression \"%s\"", str);
		Free(str);

		octln( &oct, sexp ) ;
	}


	if ((res = element_get(sexp, target)) != SPOCP_SUCCESS) {
		str = oct2strdup(sexp, '%');
		traceLog(LOG_INFO,"The S-expression \"%s\" didn't parse OK", str);
		Free(str);
	}
	else {
		DEBUG(SPOCP_DPARSE) {
			oct.len -= sexp->len ;
	
			str = oct2strdup(&oct, '%');
			traceLog(LOG_DEBUG,"Query: \"%s\"", str);
			Free(str);
		}
	}

	return res;
}

spocp_result_t
dbapi_allowed(db_t * db, octet_t * sexp, resset_t **rpp)
{
	element_t	*ep = 0;
	spocp_result_t	res = SPOCP_SUCCESS;
	comparam_t	comp;
	octarr_t	*on = 0;

	if (db == 0 || sexp == 0 || sexp->len == 0) {
		if (db == 0) {
			LOG(SPOCP_EMERG)
			    traceLog( LOG_CRIT,"Ain't got no rule database");
		} else {
			LOG(SPOCP_ERR)
				traceLog( LOG_NOTICE,
				    "Blamey no S-expression to check, oh well");
		}
		return res;
	}

	memset(&comp,0, sizeof(comparam_t)) ;

	if ((res = parse_canonsexp( sexp, &ep )) == SPOCP_SUCCESS) {

		comp.rc = SPOCP_SUCCESS;
		comp.head = ep;
		comp.blob = &on;

		res = allowed(db->jp, &comp, rpp);

		element_free(ep);
	}

	return res;
}

void
dbapi_db_del(db_t * db, dbcmd_t * dbc)
{
	free_all_rules(db->ri);
	junc_free(db->jp);
	Free(db);
}

spocp_result_t
check_uid( octet_t *uid)
{
	char	*sp;
	int	n;

	if (uid->len != SHA1HASHLEN)
		return SPOCP_SYNTAXERROR;

	for (n = 0, sp = uid->val; HEX(*sp) && n < SHA1HASHLEN; sp++, n++);

	if (n != SHA1HASHLEN)
		return SPOCP_SYNTAXERROR;

	return SPOCP_SUCCESS;
}

spocp_result_t
dbapi_rule_rm(db_t * db, dbcmd_t * dbc, octet_t * op, void *rule)
{
	ruleinst_t     *rt;
	spocp_result_t  rc;

	if (rule) rt = (ruleinst_t *) rule;
	else if ((rc = check_uid( op )) == SPOCP_SUCCESS ){
		
		if ((rt = get_rule(db->ri, op)) == 0) {
			char uid[SHA1HASHLEN +1];

			oct2strcpy( op, uid, SHA1HASHLEN +1, 0 );
			traceLog( LOG_NOTICE,
			    "Deleting rule \"%s\" impossible since it doesn't exist",
			     uid);

			return SPOCP_SYNTAXERROR;
		}
	}
	else
		return rc;

	if (dbc)
		dback_delete(dbc, rt->uid);

	rc = rule_rm(db->jp, rt->rule, rt);

	free_rule(db->ri, rt->uid);

	if (rc == SPOCP_SUCCESS)
		traceLog( LOG_INFO,"Rule successfully deleted");

	return rc;
}

octarr_t *
rollback_info( db_t *db, octet_t *op)
{
	ruleinst_t	*ri;
	octarr_t	*res = 0;

	if (check_uid(op) == SPOCP_SUCCESS &&(ri = get_rule(db->ri, op))) {
		res = octarr_add(res, octcln(ri->rule));
		if (ri->bcexp) res = octarr_add(res, octcln(ri->bcexp));
		if (ri->blob) {
			if (ri->bcexp == 0)  {
				octet_t tmp;
				oct_assign(&tmp, "NULL");
				res = octarr_add(res, octdup(&tmp));
			}
			res = octarr_add(res, octcln(ri->blob));
		}
	}
	
	return res;
}

spocp_result_t
dbapi_rules_list(db_t * db, dbcmd_t * dbc, octarr_t * pattern, octarr_t * oa,
		 char *rs)
{
	if (pattern == 0 || pattern->n == 0) {	/* get all */
		if(0)
			traceLog(LOG_INFO,"Get ALL rules");

		return get_all_rules(db, oa, rs);
	} else
		return get_matching_rules(db, pattern, oa, rs);
}

spocp_result_t
dbapi_rule_add(db_t ** dpp, plugin_t * p, dbcmd_t * dbc, octarr_t * oa, 
	void **rule)
{
	spocp_result_t  r;
	ruleinst_t     *ri = 0;
	bcdef_t        *bcd = 0;
	octet_t        *bcexp = 0;
	db_t           *db;
	char		*tmp;

	if (!oa || oa->n == 0)
		return SPOCP_MISSING_ARG;

	if (dpp)
		db = *dpp;
	else
		return SPOCP_UNWILLING;

	if (db == 0)
		db = db_new();

	if (oa->n > 1) {
		/*
		* pick out the second ( = index 1 ) octet, this is the 
		* boundary condition 
		*/
		bcexp = octarr_rm(oa, 1);

		if (oct2strcmp( bcexp, "NULL") != 0) {
			bcd = bcdef_get(db, p, dbc, bcexp, &r);
			if (bcd == NULL && r != SPOCP_SUCCESS) {
				LOG(SPOCP_INFO) {
					tmp = oct2strdup(bcexp, '%');
					traceLog(LOG_INFO,
					    "Unknown boundary condition:\"%s\"",
					    tmp);
					Free(tmp);
				}
				return r;
			}
		}
	}

	LOG(SPOCP_INFO) {
		tmp = oct2strdup(oa->arr[0], '%');
		traceLog(LOG_INFO,"spocp_add_rule:\"%s\"", tmp);
		Free(tmp);
	}

	if ((r = add_right(&db, dbc, oa, &ri, bcd))) {
		*dpp = db;

		if (bcd)
			ri->bcexp = octdup( bcexp ) ;
		if (rule)
			*rule = (void *) ri;
	}

	return r;
}

void           *
dbapi_db_dup(db_t * db, spocp_result_t * r)
{
	db_t           *new;

	*r = SPOCP_SUCCESS;

	if (db == 0) {
		return 0;
	}

	new = (db_t *) Calloc(1, sizeof(db_t *));

	if (new == 0) {
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"Memory allocation problem");
		*r = SPOCP_NO_MEMORY;
		return 0;
	}

	new->ri = ruleinfo_dup(db->ri);
	new->jp = junc_dup(db->jp, new->ri);

	return new;
}
