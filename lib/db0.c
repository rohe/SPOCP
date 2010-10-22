
/***************************************************************************
			  db0.c  -  description
			     -------------------
    begin		: Sat Oct 12 2002
    copyright	    : (C) 2002 by Umeå University, Sweden
    email		: roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <string.h>

#include <macros.h>
#include <element.h>
#include <rvapi.h>
#include <plugin.h>
#include <spocp.h>
#include <dback.h>
#include <wrappers.h>
#include <log.h>
#include <db0.h>

/*#define AVLUS 1*/

/*
 * ------------------------------------------------------------ 
 */

char	    item[NTYPES + 1];

/*
 * ---------------------------------------- 
 */

/* 
 * Store the rule as such in a database of its own
 * This information is not used when evaluating rights
 */

ruleinst_t *
save_rule(db_t * db, dbackdef_t * dbc, octet_t * rule, octet_t * blob,
	  char *bcondname)
{
	ruleinfo_t     *ri;
	ruleinst_t     *rt;

	if (db->ri == 0)
		db->ri = ri = ruleinfo_new();
	else
		ri = db->ri;

	rt = add_rule(ri, rule, blob, bcondname);

	if (rt && dbc && dbc->dback) {
		traceLog(LOG_DEBUG, "Got persistent store");
		dback_save(dbc, rt->uid, rule, blob, bcondname);
	}

	return rt;
}


/*!
 * \brief Checks if there are any rules at all in the database
 * \param db Pointer to the database
 * \return 1 if there are rules otherwise 0
 */
int
rules(db_t * db)
{
	if (db == 0 || nrules(db->ri) == 0 )
		return 0;
	else
		return 1;
}

/*!
 * \brief returns an array of octets each octet representing a rule
 * \param db Pointer to the rule database
 * \param oa The octet array where the result will be placed
 * \param rs The name of the ruleset (== the pathname )
 * \return A spocp operation result
 */
spocp_result_t
get_all_rules(db_t * db, octarr_t **oa, char *rs)
{
	int             i, n;
	ruleinst_t      *r;
	varr_t          *pa = 0;
	octet_t         *oct;
	spocp_result_t  rc = SPOCP_SUCCESS;
    octarr_t        *loa = *oa;

	n = rules_len(db->ri);

	if (n == 0)
		return rc;

	/* resize if too small */
    if (!loa) 
        loa = *oa = octarr_new(n+1);
	else if ((loa->size - loa->n) < n)
		octarr_mr(loa, n);

	pa = rdb_all(db->ri->rules);

	for (i = 0; (r = (ruleinst_t *) varr_nth(pa, i)); i++) {

		if ((oct = ruleinst_print(r, rs)) == 0) {
			rc = SPOCP_OPERATIONSERROR;
			octarr_free(loa);
			break;
		}

		octarr_add(loa, oct);
	}

	/*
	 * dont't remove the items since I've only used pointers 
	 */
	varr_free(pa);

	return rc;
}


db_t	   *
db_new()
{
	db_t	   *db;

	db = (db_t *) Calloc(1, sizeof(db_t));
	db->jp = junc_new();
	db->ri = ruleinfo_new();

	return db;
}

void
db_clr( db_t *db)
{
	if (db) {
		junc_free( db->jp );
		ruleinfo_free( db->ri );
/*		bcdef_free( db->bcdef ); */
		/* keep the plugins */
	}
}

void
db_free( db_t *db)
{
	if (db) {
		junc_free( db->jp );
		ruleinfo_free( db->ri );
/*		bcdef_free( db->bcdef ); */
		plugin_unload_all( db->plugins );

		Free(db);
	}
}

/************************************************************
*    	   *
************************************************************/
/*!\brief Store a rights description in the rules database
 * \parameter db Pointer to the incore representation of the rules database 
 * \parameter ep Pointer to a parsed S-expression 
 * \parameter rt Pointer to the rule instance
 * 
 * \return TRUE if OK 
 */

spocp_result_t
store_right(db_t * db, element_t * ep, ruleinst_t * rt)
{
	int	     r;
	element_t	*ec;

	if (db->jp == 0)
		db->jp = junc_new();

	ec = element_dup( ep );

#ifdef AVLUS
	{
		octet_t *eop ;

		eop = oct_new( 512,NULL);
		element_struct( eop, ec );
		oct_print( LOG_INFO, "struct", eop);
		oct_free( eop);
	}
#endif

	if (add_element(db->plugins, db->jp, ep, rt, 1) == 0)
		r = SPOCP_OPERATIONSERROR;
	else {
		rt->ep = ec;
		r = SPOCP_SUCCESS;
	}

	return r;
}

/*************************************************************/
/*!
 * \brief Add a rights description to the rules database
 * \param db Pointer to the incore representation of the rules database
 * \param dbc The persistent database
 * \param oa An octet array consisting of rule and possibly a blob 
 * \param ri On return, holds a pointer to the created ruleinst_t struct
 * \param bcd The collection of boundary conditions
 * \return A Spocp return code
 */

spocp_result_t
add_right(db_t ** db, dbackdef_t * dbc, octarr_t * oa, ruleinst_t ** ri,
	  bcdef_t * bcd)
{
	element_t       *ep;
	octet_t         rule, blob, oct;
	ruleinst_t      *rt;
	spocp_result_t  rc = SPOCP_SUCCESS;

	/*
	 * LOG( SPOCP_WARNING )
		traceLog(LOG_WARNING,"Adding new rule: \"%s\"", rp->val) ; 
	 */

    memset(&rule,0, sizeof(octet_t));
    memset(&blob,0, sizeof(octet_t));

	octln(&oct, oa->arr[0]);
	octln(&rule, oa->arr[0]);

	if (oa->n > 1) {
		octln(&blob, oa->arr[1]);
	} else
		blob.len = 0;

	if ((rc = get_element(&oct, &ep)) == SPOCP_SUCCESS) {

		/*
		 * stuff left ?? 
		 * just ignore it 
		 */
		if (oct.len) {
			rule.len -= oct.len;
		}

		if ((rt =
            save_rule(*db, dbc, &rule, &blob, bcd ? bcd->name : NULL)) == 0) {
			element_free(ep);
			return SPOCP_EXISTS;
		}

		/*
		 * right == rule 
		 */
		if ((rc = store_right(*db, ep, rt)) != SPOCP_SUCCESS) {
			element_free(ep);

			/*
			 * remove ruleinstance 
			 */
			free_rule((*db)->ri, rt->uid);

			LOG(SPOCP_WARNING)
				traceLog(LOG_WARNING,"Error while adding rule");
		}

		if (bcd) {
			rt->bcond = bcd;
			bcd->rules = varr_add(bcd->rules, (void *) rt);
		}

		*ri = rt;
#ifdef AVLUS
		junc_print_r(0, (*db)->jp);
#endif
	}

	return rc;
}
