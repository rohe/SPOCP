
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
		traceLog("Parsing the S-expression \"%s\"", str);
		free(str);

		octln( &oct, sexp ) ;
	}


	if ((res = element_get(sexp, target)) != SPOCP_SUCCESS) {
		str = oct2strdup(sexp, '%');
		traceLog("The S-expression \"%s\" didn't parse OK", str);
		free(str);
	}
	else {
		DEBUG(SPOCP_DPARSE) {
			oct.len -= sexp->len ;
	
			str = oct2strdup(&oct, '%');
			traceLog("Query: \"%s\"", str);
			free(str);
		}
	}

	return res;
}

spocp_result_t
dbapi_allowed(db_t * db, octet_t * sexp, octarr_t ** on)
{
	element_t	*ep = 0;
	spocp_result_t	res = SPOCP_SUCCESS;
	comparam_t	comp;

	if (db == 0 || sexp == 0 || sexp->len == 0) {
		if (db == 0) {
			LOG(SPOCP_EMERG)
			    traceLog("Ain't got no rule database");
		} else {
			LOG(SPOCP_ERR)
			    traceLog
			    ("Blamey no S-expression to check, oh well");
		}
		return res;
	}

	if ((res = parse_canonsexp( sexp, &ep )) == SPOCP_SUCCESS) {

		comp.rc = SPOCP_SUCCESS;
		comp.head = ep;
		comp.blob = on;

		res = allowed(db->jp, &comp);

		element_free(ep);
	}

	return res;
}

void
dbapi_db_del(db_t * db, dbcmd_t * dbc)
{
	free_all_rules(db->ri);
	junc_free(db->jp);
	free(db);
}

spocp_result_t
dbapi_rule_rm(db_t * db, dbcmd_t * dbc, octet_t * op)
{
	int             n;
	ruleinst_t     *rt;
	char            uid[41], *sp;
	spocp_result_t  rc;

	if (op->len < 40)
		return SPOCP_SYNTAXERROR;

	for (n = 0, sp = op->val; HEX(*sp); sp++, n++);

	if (n != 40)
		return SPOCP_SYNTAXERROR;

	memcpy(uid, op->val, 40);
	uid[40] = '\0';

	traceLog("Attempt to delete rule: \"%s\"", uid);

	op->val += 40;
	op->len -= 40;

	/*
	 * first check that the rule is there 
	 */

	if ((rt = get_rule(db->ri, uid)) == 0) {
		traceLog
		    ("Deleting rule \"%s\" impossible since it doesn't exist",
		     uid);

		return SPOCP_SYNTAXERROR;
	}

	if (dbc)
		dback_delete(dbc, uid);

	rc = rule_rm(db->jp, rt->rule, rt);

	free_rule(db->ri, uid);

	if (rc == SPOCP_SUCCESS)
		traceLog("Rule successfully deleted");

	return rc;
}

spocp_result_t
dbapi_rules_list(db_t * db, dbcmd_t * dbc, octarr_t * pattern, octarr_t * oa,
		 char *rs)
{
	if (pattern == 0 || pattern->n == 0) {	/* get all */
		if(0)
			traceLog("Get ALL rules");

		return get_all_rules(db, oa, rs);
	} else
		return get_matching_rules(db, pattern, oa, rs);
}

spocp_result_t
dbapi_rule_add(db_t ** dpp, plugin_t * p, dbcmd_t * dbc, octarr_t * oa)
{
	spocp_result_t  r;
	ruleinst_t     *ri = 0;
	bcdef_t        *bcd = 0;
	octet_t        *o;
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
		o = octarr_rm(oa, 1);

		bcd = bcdef_get(db, p, dbc, o, &r);
		if (bcd == NULL && r != SPOCP_SUCCESS) {
			LOG(SPOCP_INFO) {
				tmp = oct2strdup(o, '%');
				traceLog("Unknown boundary condition:\"%s\"", tmp);
				free(tmp);
			}
			return r;
		}
	}

	LOG(SPOCP_INFO) {
		tmp = oct2strdup(oa->arr[0], '%');
		traceLog("spocp_add_rule:\"%s\"", tmp);
		free(tmp);
	}

	if ((r = add_right(&db, dbc, oa, &ri, bcd)))
		*dpp = db;

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

	new = (db_t *) calloc(1, sizeof(db_t *));

	if (new == 0) {
		LOG(SPOCP_ERR) traceLog("Memory allocation problem");
		*r = SPOCP_NO_MEMORY;
		return 0;
	}

	new->ri = ruleinfo_dup(db->ri);
	new->jp = junc_dup(db->jp, new->ri);

	return new;
}
