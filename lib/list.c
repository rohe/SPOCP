
/***************************************************************************
                          list.c  -  description
                             -------------------
    begin                : Tue Nov 05 2002
    copyright            : (C) 2010 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

/*
 * Implements the ADMIN LIST command 
 */

#include <stdlib.h>
#include <string.h>

#include <db0.h>
#include <element.h>
#include <branch.h>
#include <proto.h>
#include <spocp.h>
#include <wrappers.h>
#include <macros.h>
#include <log.h>
#include <ruleinst.h>
#include <atommatch.h>
#include <ssn.h>
#include <range.h>
#include <list.h>
#include <match.h>

/*
varr_t         *get_rec_all_rules(junc_t * jp, varr_t * in);
varr_t         *subelem_inv_match(junc_t *, element_t *, varr_t *, varr_t *);
varr_t         *subelem_match(junc_t *, element_t *, varr_t *, varr_t *,
				  spocp_result_t *);
varr_t         *get_rule_indexes(varr_t *, varr_t *);
varr_t         *get_all_to_next_listend(junc_t *, varr_t *, int);
*/
static varr_t   *get_rec_all_rules(junc_t * jp, varr_t * in);


/********************************************************************/

static varr_t         *
get_rule_indexes(varr_t * pa, varr_t * in)
{
	void           *v;

	for (v = varr_first(pa); v; v = varr_next(pa, v)) 
		in = get_rec_all_rules(v, in);

	varr_free(pa);
	return in;
}

/*************************************************************************/
/*
 * Get recursively all rules below a certain junction.
 */
/********************************************************************/

static varr_t         *
get_rec_all_rules(junc_t * jp, varr_t * in)
{
	varr_t         *pa = 0;

	if (jp->branch[SPOC_ENDOFRULE]) {
        in = varr_ruleinst_add(in, jp->branch[SPOC_ENDOFRULE]->val.ri);
	}

	if (jp->branch[SPOC_LIST])
		in = get_rec_all_rules(jp->branch[SPOC_LIST]->val.list, in);

	if (jp->branch[SPOC_ENDOFLIST])
		in = get_rec_all_rules(jp->branch[SPOC_ENDOFLIST]->val.list, in);
	/*
	 * --- 
	 */

	if (jp->branch[SPOC_ATOM])
		pa = get_all_atom_followers(jp->branch[SPOC_ATOM], pa);

	if (jp->branch[SPOC_PREFIX])
		pa = get_all_ssn_followers(jp->branch[SPOC_PREFIX], SPOC_PREFIX,
					   pa);

	if (jp->branch[SPOC_SUFFIX])
		pa = get_all_ssn_followers(jp->branch[SPOC_SUFFIX], SPOC_SUFFIX,
					   pa);

	if (jp->branch[SPOC_RANGE])
		pa = get_all_range_followers(jp->branch[SPOC_RANGE], pa);

	/*
	 * --- 
	 */

	if (pa)
		in = get_rule_indexes(pa, in);

	return in;
}

/*************************************************************************/

static varr_t         *
get_all_to_next_listend(junc_t * jp, varr_t * in, int lev)
{
	varr_t         *pa = 0;
	junc_t         *ju;
	void           *v;

	DEBUG(SPOCP_DMATCH) junc_print(lev, jp);

	if (jp->branch[SPOC_ENDOFRULE]);	/* shouldn't get here */

	if (jp->branch[SPOC_LIST])
		in = get_all_to_next_listend(jp->branch[SPOC_LIST]->val.list, in,
					     lev + 1);

	if (jp->branch[SPOC_ENDOFLIST]) {
		ju = jp->branch[SPOC_ENDOFLIST]->val.list;
		lev--;
		if (lev >= 0)
			pa = varr_junc_add(pa, ju);
		else
			in = varr_junc_add(in, ju);
	}

	/*
	 * --- 
	 */

	if (jp->branch[SPOC_ATOM])
		pa = get_all_atom_followers(jp->branch[SPOC_ATOM], pa);

	if (jp->branch[SPOC_PREFIX])
		pa = get_all_ssn_followers(jp->branch[SPOC_PREFIX], SPOC_PREFIX,
					   pa);

	if (jp->branch[SPOC_SUFFIX])
		pa = get_all_ssn_followers(jp->branch[SPOC_SUFFIX], SPOC_SUFFIX,
					   pa);

	if (jp->branch[SPOC_RANGE])
		pa = get_all_range_followers(jp->branch[SPOC_RANGE], pa);

	/*
	 * --- 
	 */

	if (pa) {
        for (v = varr_first(pa); v; v = varr_next(pa, v)) 
			in = get_all_to_next_listend(v, in, lev);
	}

	return in;
}

/*************************************************************************/

static varr_t  *
prefix2prefix_match_lte(atom_t * ap, ssn_t * pssn, varr_t * pap)
{
	return ssn_lte_match(pssn, ap->val.val, FORWARD, pap);
}

/*************************************************************************/

static varr_t  *
suffix2suffix_match_lte(atom_t * ap, ssn_t * pssn, varr_t * pap)
{
	return ssn_lte_match(pssn, ap->val.val, BACKWARD, pap);
}

/*************************************************************************/
/* This is the inverse search. This should give you all the items in the
 * database that are more specific than the given.
 */
static varr_t         *
subelem_inv_match(junc_t * jp, element_t * ep, varr_t * pap, varr_t * id)
{
	branch_t       *bp;
	junc_t         *np;
	varr_t         *nap, *arr, *tmp;
	int             i;
	varr_t         *set;
	void           *v;
	element_t      *elem;

	switch (ep->type) {
	case SPOC_ATOM:	/* Can only be other atoms */
		if ((bp = ARRFIND(jp, SPOC_ATOM)) != 0) {
			np = atom2atom_match(ep->e.atom, bp->val.atom);
			if (np)
				pap = varr_junc_add(pap, np);
		}
		break;

	case SPOC_PREFIX:	/* prefixes or atoms */
		if ((bp = ARRFIND(jp, SPOC_ATOM)) != 0) {
			pap = prefix2atoms_match(ep->e.atom->val.val, bp->val.atom, pap);
		}

		if ((bp = ARRFIND(jp, SPOC_PREFIX)) != 0) {
			pap = prefix2prefix_match_lte(ep->e.atom, bp->val.prefix, pap);
		}
		break;

	case SPOC_SUFFIX:	/* suffixes or atoms */
		if ((bp = ARRFIND(jp, SPOC_ATOM)) != 0) {
			pap = suffix2atoms_match(ep->e.atom->val.val, bp->val.atom, pap);
		}

		if ((bp = ARRFIND(jp, SPOC_SUFFIX)) != 0) {
			pap = suffix2suffix_match_lte(ep->e.atom, bp->val.suffix, pap);
		}
		break;

	case SPOC_RANGE:	/* ranges or atoms */
		if ((bp = ARRFIND(jp, SPOC_ATOM)) != 0) {
			pap = range2atoms_match(ep->e.range, bp->val.atom, pap);
		}

		if ((bp = ARRFIND(jp, SPOC_RANGE)) != 0) {
			tmp = bsl_inv_match_ranges(bp->val.range, ep->e.range);
            pap = varr_or(pap, tmp, 1);
            varr_free(tmp);
		}
		break;

	case SPOC_SET:
		nap = 0;
		set = ep->e.set;

		for (elem = varr_first(set); elem; elem = varr_next(set, elem)) {
			/*
			 * Since this is in effect parallell path, I have to
			 * do it over and over again 
			 */
			nap = subelem_inv_match(jp, elem, nap, id);
		}
		pap = varr_or(pap, nap, 0);
		break;

	case SPOC_LIST:	/* other lists */
		arr = varr_junc_add(0, jp);

		for (ep = ep->e.list->head; ep; ep = ep->next) {
			nap = 0;
			for (i = 0; (v = varr_nth(arr, i)); i++) {
				nap = subelem_inv_match(v, ep, nap, id);
			}
			varr_free(arr);
			/*
			 * start the next transversal, where the last left off 
			 */
			if (nap)
				arr = nap;
			else
				return 0;
		}
		break;

	}

	return pap;
}

/****************************************************************************/

/*
 * This is the normal case, list all rules where the query are more specific
 */

static varr_t         *
subelem_match(junc_t * jp, element_t * ep, varr_t * pap, varr_t * id,
		  spocp_result_t * rc)
{
	branch_t    *bp;
	junc_t      *np;
	varr_t      *nap, *arr;
	int         i;
	slist_t     **slpp;
	varr_t      *set;
	void        *v;

	/*
	 * DEBUG( SPOCP_DMATCH ) traceLog( LOG_DEBUG,"subelem_match") ; 
	 */

	/*
	 * DEBUG( SPOCP_DMATCH ) junc_print( 0, jp ) ; 
	 */

	if ((bp = ARRFIND(jp, SPOC_ENDOFRULE))) {	/* Can't get much
							 * further */
		id = varr_or(id, (void *) bp->val.ri, 1);
	}

	switch (ep->type) {
	case SPOC_ATOM:	/* Atoms, prefixes, suffixes or ranges */
		/*
		 * traceLog(LOG_INFO, "Matching Atom: %s", ep->e.atom->val.val ) ; 
		 */
		if ((bp = ARRFIND(jp, SPOC_ATOM)) != 0) {
			/*
			 * traceLog(LOG_INFO, "against Atom" ) ; 
			 */
			np = atom2atom_match(ep->e.atom, bp->val.atom);
			if (np)
				pap = varr_junc_add(pap, np);
		}

		if ((bp = ARRFIND(jp, SPOC_PREFIX)) != 0) {
			/*
			 * traceLog(LOG_INFO, "against prefix" ) ; 
			 */
			arr = atom2prefix_match(ep->e.atom, bp->val.prefix);
			if (arr) {
				pap = varr_or(pap, arr, 0);
				varr_free(arr);
			}
		}

		if ((bp = ARRFIND(jp, SPOC_SUFFIX)) != 0) {
			/*
			 * traceLog(LOG_INFO, "against suffix" ) ; 
			 */
			arr = atom2suffix_match(ep->e.atom, bp->val.suffix);
			if (arr) {
				pap = varr_or(pap, arr, 0);
				varr_free(arr);
			}
		}

		if ((bp = ARRFIND(jp, SPOC_RANGE)) != 0) {
			/*
			 * traceLog(LOG_INFO, "against range" ) ; 
			 */
			slpp = bp->val.range;

			for (i = 0; i < DATATYPES; i++) {
				if (slpp[i]) {
					nap = bsl_match_atom(slpp[i], ep->e.atom, rc);
					if (nap) {
						pap = varr_or(pap, nap, 0);
						varr_free(nap);
					}
				}
			}
		}

		break;

	case SPOC_PREFIX:	/* only with other prefixes */
		if ((bp = ARRFIND(jp, SPOC_PREFIX)) != 0) {
			nap = prefix2prefix_match(ep->e.atom, bp->val.prefix);
			if (nap) {
				pap = varr_or(pap, nap, 0);
				varr_free(nap);
			}
		}
		break;

	case SPOC_SUFFIX:	/* only with other suffixes */
		if ((bp = ARRFIND(jp, SPOC_SUFFIX)) != 0) {
			nap = suffix2suffix_match(ep->e.atom, bp->val.suffix);
			if (nap) {
				pap = varr_or(pap, nap, 0);
				varr_free(nap);
			}
		}
		break;

	case SPOC_RANGE:	/* only with ranges */
		if ((bp = ARRFIND(jp, SPOC_RANGE)) != 0) {
			slpp = bp->val.range;

			for (i = 0; i < DATATYPES; i++) {
				if (slpp[i]) {
					nap = bsl_match_range(slpp[i], ep->e.range);
					if (nap) {
						pap = varr_or(pap, nap, 0);
						varr_free(nap);
					}
				}
			}
		}
		break;

	case SPOC_SET:
		nap = 0;
		set = ep->e.set;
		for (v = varr_first(set); v; v = varr_next(set, v)) {
			nap =
			    subelem_match(jp, (element_t *) v, nap, id,
					      rc);
		}
		break;

	case SPOC_LIST:	/* other lists */
		/*
		 * traceLog(LOG_INFO, "Matching List:" ) ; 
		 */
		if ((bp = ARRFIND(jp, SPOC_LIST)) != 0) {
			arr = varr_junc_add(0, bp->val.list);

			for (ep = ep->e.list->head; ep; ep = ep->next) {
				nap = 0;
                for (v = varr_first(arr); v; v = varr_next(arr, v)) {
					nap = subelem_match(v, ep, nap, id, rc);
				}
				varr_free(arr);
				/*
				 * start the next transversal, where the last
				 * left off 
				 */
				if (nap)
					arr = nap;
				else
					return 0;
			}
			/*
			 * if the number of elements in the ep list is fewer
			 * collect all alternative until the end of the arr
			 * list is found 
             */
			 /* traceLog(LOG_INFO,"Gather until end of list") ; */

            for (v = varr_first(arr); v; v = varr_next(arr, v))
				pap = get_all_to_next_listend(v, pap, 0);
		}
		break;

	}

	return pap;
}

/****************************************************************************/

varr_t  *
adm_list(junc_t * jp, subelem_t * sub, spocp_result_t * rc)
{
	varr_t  *nap = 0, *res, *pap = 0;
	varr_t  *id = 0;
	void    *v;

	if (jp == 0)
		return 0;

	/*
	 * The rules must be lists and I only have subelements so I have to
	 * skip the list tag part 
	 */

	/*
	 * just the single list tag should actually match everything 
	 */
	if (jp->branch[1] == 0)
		return get_rec_all_rules(jp, 0);

	jp = jp->branch[1]->val.list;

	pap = varr_junc_add(0, jp);

	for (; sub; sub = sub->next) {
		res = 0;

        for (v = varr_first(pap); v; v = varr_next(pap, v)) {
			if (sub->direction == LT)
				nap = subelem_inv_match(v, sub->ep, nap, id);
			else
				nap = subelem_match(v, sub->ep, nap, id, rc);
		}

		varr_free(pap);
		if (nap == 0)
			return 0;

		/*
		 * traceLog(LOG_INFO,"%d roads to follow", nap->n ) ; 
		 */
		pap = nap;
		nap = 0;
	}

	/*
	 * traceLog(LOG_INFO, 
     *      "Done all subelements, %d junctions to follow through",
	 *      pap->n ) ; 
	 */

	/*
	 * After going through all the subelement I should have a list of
	 * junctions which have to be followed to the end 
	 */

	for (v = varr_first(pap); v; v = varr_next(pap, v)) {
		id = get_rec_all_rules(v, id);
	}

	return id;
}

/********************************************************************/

/********************************************************************/

spocp_result_t
get_matching_rules(db_t * db, octarr_t *pat, octarr_t **oap, char *rs)
{
	spocp_result_t  rc = SPOCP_SUCCESS;
	subelem_t       *sub;
	varr_t          *ip;
	octet_t         *oct, *arr[2];
	int             i;
	ruleinst_t      *r;
	unsigned int    ui;
    octarr_t        *oa;

	if ((sub = pattern_parse(pat)) == 0) {
		return SPOCP_SYNTAXERROR;
	}

	/*
	 * get the IDs for the matching rules 
	 */
	ip = adm_list(db->jp, sub, &rc);

	if (ip == 0 && rc != SPOCP_SUCCESS)
		return rc;

	arr[1] = 0;

	/*
	 * produce the array of rules 
	 */
	if (ip) {
		ui = varr_len(ip);

        if (*oap == NULL)
            *oap = oa = octarr_new(ui+1);
        else
            oa = *oap;
        
		if ((unsigned int) (oa->size - oa->n) < ui)
			octarr_mr(oa, ui);

		for (i = 0;; i++) {
			if ((r = varr_ruleinst_nth(ip, i)) == 0)
				break;

			arr[0] = r->rule;

			oct = ruleinst_print(r, rs);

			octarr_add(oa, oct);
		}
	}

	return SPOCP_SUCCESS;
}
