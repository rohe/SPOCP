
/***************************************************************************

               spocp2.c  -  contains the public interface to spocp

                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include "locl.h"

int             ruleset_print(ruleset_t * rs);

/* #define AVLUS 1 */

static          spocp_result_t
rec_allow(ruleset_t * rs, element_t * ep, int scope, resset_t **rset)
{
	spocp_result_t  res = SPOCP_DENIED, sum = SPOCP_DENIED;
	ruleset_t       *trs;
	comparam_t      comp;
	octarr_t        *oa=0;
	resset_t        *xrs=0;
	int             all = scope&0xF0 ;
	checked_t       *cr=0;

	memset(&comp,0,sizeof(comparam_t));

#ifdef AVLUS
	traceLog(LOG_INFO,"rec_allow %s %d", rs->name, scope&0x0F);
#endif

	switch (scope&0x0F) {
	case SUBTREE:
		if (rs->down) {
			/*
			 * go as far to the left as possible 
			 */
			for (trs = rs->down; trs->left; trs = trs->left)
#ifdef AVLUS
				traceLog(LOG_INFO,"Going left %s", trs->left);
#else
				;
#endif

			/*
			 * and over to the right side passing every node on
			 * the way 
			 */
			for (; trs ; trs = trs->right) {
				xrs = 0;
				res = rec_allow(trs, ep, scope, &xrs);
				if(res == SPOCP_SUCCESS)
					sum = res;
					if(xrs)
						*rset = resset_extend(*rset, xrs);

				/* If I'm not interested in everything then
				 * one is good enough */
				if( !all && res == SPOCP_SUCCESS)
					break;
			}
			res = sum;
		}
		else {
			if (rs->db)
				comp.rc = SPOCP_SUCCESS;
				comp.head = ep;
				comp.blob = &oa;
				comp.all = all;
				comp.cr = &cr;

				DEBUG(SPOCP_DMATCH)
					ruleset_print( rs );

				res = allowed(rs->db->jp, &comp, rset);
#ifdef AVLUS
			/*DEBUG(SPOCP_DSRV)*/ 
            if (rset) {
				traceLog(LOG_DEBUG, "BASE got %d and Result Set", res);
				resset_print(*rset);
				traceLog(LOG_DEBUG, "-----------------------------");
			}
			else
				traceLog(LOG_DEBUG,"No rset");
#endif
			if ( *(comp.cr))
				checked_free( *(comp.cr) );
		}
		break;

	case ONELEVEL:
		if (rs->down) {
			comp.rc = SPOCP_SUCCESS;
			comp.head = ep;
			comp.blob = &oa;
			comp.all = all;

			for (trs = rs->down; trs->left; trs = trs->left);

			for (; trs && res != SPOCP_SUCCESS; trs = trs->right) {
				xrs = 0;
				if (rs->db)
					res = allowed(trs->db->jp, &comp, &xrs);
				if(xrs)
					*rset = resset_extend(*rset, xrs);
			}
		}
#ifdef AVLUS
		/*DEBUG(SPOCP_DSRV)*/ if (rset) {
			traceLog(LOG_DEBUG,"ONELEVEL got Result Set");
			resset_print(*rset);
			traceLog(LOG_DEBUG,"---------------------------");
		}
		else
			traceLog(LOG_DEBUG,"No rset");
#endif

		break;

	}


	return res;
}

spocp_result_t
skip_sexp(octet_t * sexp)
{
	spocp_result_t  r;
	element_t      *ep = 0;

	if ((r = parse_canonsexp(sexp,&ep)) == SPOCP_SUCCESS)
		element_free(ep);

	return r;
}

/*
 * returns -1 parsing error 0 access denied 1 access allowed 
 */

spocp_result_t
ss_allow(ruleset_t * rs, octet_t * sexp, resset_t **rset, int scope)
{
	spocp_result_t  res = SPOCP_DENIED;	/* this is the default */
	element_t      *ep = 0;
	octet_t         dec;

	if (rs == 0 || sexp == 0 || sexp->len == 0) {
		if (rs == 0) {
			LOG(SPOCP_EMERG)
			    traceLog(LOG_NOTICE,"Ain't got no rule database");
		} else {
			LOG(SPOCP_ERR)
				traceLog(LOG_ERR,
				    "Blamey no S-expression to check, oh well");
		}
		return SPOCP_DENIED;
	}

	/*
	 * ruledatabase but no rules ? 
	 */
	if (rules(rs->db) == 0 && rs->down == 0) {
		traceLog(LOG_NOTICE,"Rule database but no rules");
		return SPOCP_DENIED;
	}

	octln(&dec, sexp);

	if ((res = parse_canonsexp( &dec, &ep)) == SPOCP_SUCCESS) {
		res = rec_allow(rs, ep, scope, rset);
		element_free(ep);
	}

#ifdef AVLUS
	/*DEBUG(SPOCP_DSRV)*/ if (rset) {
		traceLog(LOG_DEBUG,"ss_allow returned Result Set");
		resset_print(*rset);
		traceLog(LOG_DEBUG,"---------------------------");
	}
	else
		traceLog(LOG_DEBUG,"No rset from ss_allow");
#endif

	return res;
}

void
free_db(db_t * db)
{
	if (db) {
		free_all_rules(db->ri);
		junc_free(db->jp);
		Free(db);
	}
}

void
ss_del_db(ruleset_t * rs, int scope)
{
	ruleset_t      *trs;

	if (rs == 0)
		return;

	switch (scope) {
	case SUBTREE:
		if (rs->down) {
			for (trs = rs->down; trs->left; trs = trs->left);

			for (; trs; trs = trs->right)
				ss_del_db(trs, scope);
		}

	case BASE:
		free_db((db_t *) rs->db);
		rs->db = 0;
		break;


	case ONELEVEL:
		if (rs->down) {
			for (trs = rs->down; trs->left; trs = trs->left);

			for (; trs; trs = trs->right) {
				free_db((db_t *) trs->db);
				trs->db = 0;
			}
		}
		break;

	}
}

/****************************************************************/

static spocp_result_t
rec_del(ruleset_t * rs, dbackdef_t * dbc, octet_t *uid, size_t * nr)
{
	ruleset_t      *trs;
	db_t           *db;
	ruleinst_t     *rt;
	spocp_result_t  rc = SPOCP_SUCCESS;

	db = rs->db;

	if (db == 0)
		return 0;

	if ((rt = get_rule(db->ri, uid))) {
		char str[SHA1HASHLEN +1];

		if (oct2strcpy( uid, str, SHA1HASHLEN +1, 0) < 0 )
			return SPOCP_SYNTAXERROR;

		if (dbc) 
			dback_delete(dbc, str);

		if ((rc = rule_rm(db->jp, rt->rule, rt)) != SPOCP_SUCCESS)
			return rc;

		if (free_rule(db->ri, str) == 0) {
			traceLog(LOG_ERR
			    ,"Hmm, something fishy here, couldn't delete rule");
		}
		(*nr)++;
	}

	if (rs->down) {

		for (trs = rs->down; trs->left; trs = trs->left);

		for (; trs; trs = trs->right)
			if ((rc = rec_del(trs, dbc, uid, nr)) != SPOCP_SUCCESS)
				return rc;
	}

	return rc;
}

spocp_result_t
ss_del_rule(ruleset_t * rs, dbackdef_t * dbc, octet_t * op)
{
	size_t		n;
	spocp_result_t	r;

	if (rs == 0)
		return SPOCP_SUCCESS;

	if (( r = check_uid( op )) != SPOCP_SUCCESS ) return r;

	/*
	 * first check that the rule is there 
	 */

	n = 0;
	if (rec_del(rs, dbc, op, &n) != SPOCP_SUCCESS)
		traceLog(LOG_ERR,"Error while deleting rules");

	if (n > 1)
		traceLog(LOG_ERR,"%d rules successfully deleted", n);
	else if (n == 1)
		traceLog(LOG_ERR,"1 rule successfully deleted");
	else
		traceLog(LOG_ERR,"No rules deleted");

	return SPOCP_SUCCESS;
}

/*
 * --------------------------------------------------------------------------
 */

static int
rec_rules(ruleset_t * rs, int scope)
{
	int             n = 0;
	ruleset_t      *trs;

	switch (scope) {
	case SUBTREE:
		if (rs->down) {
			for (trs = rs->down; trs->left; trs = trs->left);

			for (; trs; trs = trs->right)
				n += rec_rules(trs, scope);
		}

	case BASE:
		n += nrules(rs->db->ri);
		break;

	case ONELEVEL:
		if (rs->down) {
			for (trs = rs->down; trs->left; trs = trs->left);

			for (; trs; trs = trs->right)
				n += nrules(trs->db->ri);
		}
		break;
	}

	return n;
}

int
ss_rules(ruleset_t * rs, int scope)
{
	return rec_rules(rs, scope);
}

/*
 * --------------------------------------------------------------------------
 */
/*
 * almost the same as spocp_dup, so I shold only have one of them 
 */

static db_t    *
db_dup(db_t * db)
{
	db_t           *new;

	if (db == 0)
		return 0;

	new = (db_t *) Calloc(1, sizeof(db_t *));

	if (new == 0) {
		LOG(SPOCP_ERR) traceLog(LOG_ERR,"Memory allocation problem");
		return 0;
	}

	new->ri = ruleinfo_dup(db->ri);
	new->jp = junc_dup(db->jp, new->ri);

	return new;
}

/*
 * regexp_t *regexp_dup( regexp_t *rp ) { return new_pattern( rp->regex ) ; }
 * 
 * aci_t *aci_dup( aci_t *ap ) { aci_t *new ;
 * 
 * if( ap == 0 ) return 0 ;
 * 
 * new = (aci_t *) Calloc( 1, sizeof( aci_t )) ;
 * 
 * new->access = ap->access ; new->string = Strdup(ap->string) ; new->net =
 * Strdup(ap->net) ; new->mask = Strdup(ap->mask) ; new->cert = regexp_dup(
 * ap->cert ) ;
 * 
 * if( ap->next ) new->next = aci_dup( ap->next ) ;
 * 
 * return new ; } 
 */

static ruleset_t *
ruleset_dup(ruleset_t * rs)
{
	ruleset_t      *new;
	octet_t         loc;

	if (rs == 0)
		return 0;

	loc.val = rs->name;
	loc.len = strlen(rs->name);
	new = ruleset_new(&loc);

	new->db = db_dup(rs->db);

	return new;
}

static ruleset_t *
rec_dup_rules(ruleset_t * rs, int scope)
{
	ruleset_t      *trs = 0, *drs = 0, *tmp = 0, *lrs = 0;

	switch (scope) {
	case SUBTREE:
		if (rs->down) {
			for (trs = rs->down; trs->left; trs = trs->left);

			for (; trs; trs = trs->right) {
				tmp = rec_dup_rules(trs, scope);
				if (lrs == 0)
					lrs = drs = tmp;
				else {
					drs->right = tmp;
					tmp->left = drs;
					drs = tmp;
				}
			}
			drs = lrs;
		}

	case BASE:
		drs = ruleset_dup(rs);
		if (lrs) {
			drs->down = lrs;
			for (; lrs; lrs = lrs->right)
				lrs->up = drs;
		}
		break;

	case ONELEVEL:
		if (rs->down) {
			for (trs = rs->down; trs->left; trs = trs->left);

			for (; trs; trs = trs->right) {
				drs = ruleset_dup(trs);
				if (lrs == 0)
					lrs = drs = tmp;
				else {
					drs->right = tmp;
					tmp->left = drs;
					drs = tmp;
				}
			}
			drs = lrs;
		}
		break;
	}

	return drs;
}

void           *
ss_dup(ruleset_t * rs, int scope)
{
	return rec_dup_rules(rs, scope);
}

/*
 * --------------------------------------------------------- 
 */

/*
static int
print_db(db_t * db)
{
	if (db == 0)
		return 0;

	if (db->ri)
		ruleinfo_print(db->ri);

	return 0;
}
*/

static int
ruleset_print_r(ruleset_t * rs)
{
	for (; rs->left; rs = rs->left);

	for (; rs; rs = rs->right)
		ruleset_print(rs);

	return 0;
}

int
ruleset_print(ruleset_t * rs)
{
	LOG(SPOCP_DEBUG) traceLog(LOG_DEBUG,"rulesetname: \"%s\"", rs->name);
	/* print_db(rs->db); */
	/*
	 * print_aci( rs->aci ) ; 
	 */
	if (rs->down)
		ruleset_print_r(rs->down);

	return 0;
}
