
/***************************************************************************
                          list.c  -  description
                             -------------------
    begin                : Tue Nov 05 2002
    copyright            : (C) 2002 by Umeå University, Sweden
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
#include <struct.h>
#include <func.h>
#include <proto.h>
#include <spocp.h>
#include <wrappers.h>
#include <macros.h>


varr_t         *get_rec_all_rules(junc_t * jp, varr_t * in);
varr_t         *subelem_lte_match(junc_t *, element_t *, varr_t *, varr_t *);
varr_t         *subelem_match_lte(junc_t *, element_t *, varr_t *, varr_t *,
				  spocp_result_t *);
varr_t         *get_rule_indexes(varr_t *, varr_t *);
varr_t         *get_all_to_next_listend(junc_t *, varr_t *, int);


/********************************************************************/

varr_t         *
get_rule_indexes(varr_t * pa, varr_t * in)
{
	unsigned int    i;
	void           *v;

	for (i = 0; (v = varr_nth(pa, i)); i++)
		in = get_rec_all_rules(v, in);

	varr_free(pa);
	return in;
}

/****************************************************************************/

void
junc_print(int lev, junc_t * jp)
{
	traceLog(LOG_DEBUG,"|---------------------------->");
	if (jp->item[0])
		traceLog(LOG_DEBUG,"Junction[%d]: ATOM", lev);
	if (jp->item[1])
		traceLog(LOG_DEBUG,"Junction[%d]: LIST", lev);
	if (jp->item[2])
		traceLog(LOG_DEBUG,"Junction[%d]: SET", lev);
	if (jp->item[3])
		traceLog(LOG_DEBUG,"Junction[%d]: PREFIX", lev);
	if (jp->item[4])
		traceLog(LOG_DEBUG,"Junction[%d]: SUFFIX", lev);
	if (jp->item[5])
		traceLog(LOG_DEBUG,"Junction[%d]: RANGE", lev);
	if (jp->item[6])
		traceLog(LOG_DEBUG,"Junction[%d]: ENDOFLIST", lev);
	if (jp->item[7]) {
		spocp_index_t *id;
		int i;

		traceLog(LOG_DEBUG,"Junction[%d]: ENDOFRULE", lev);
		id = jp->item[7]->val.id;
		for (i = 0; i < id->n; i++) 
			traceLog(LOG_DEBUG,"Rule: %p", id->arr[i]);
	}
	if (jp->item[8])
		traceLog(LOG_DEBUG,"Junction[%d]: ExTREF", lev);
	if (jp->item[9])
		traceLog(LOG_DEBUG,"Junction[%d]: ANY", lev);
	if (jp->item[10])
		traceLog(LOG_DEBUG,"Junction[%d]: REPEAT", lev);
	if (jp->item[11])
		traceLog(LOG_DEBUG,"Junction[%d]: REGEXP", lev);
	traceLog(LOG_DEBUG,">----------------------------|");
}

/*************************************************************************/
/*
 * Match rule elements where the subelement is LTE 
 */

/********************************************************************/
/*
 * get recursively all rules 
 */

/********************************************************************/

varr_t         *
get_rec_all_rules(junc_t * jp, varr_t * in)
{
	varr_t         *pa = 0;
	spocp_index_t	*id;
	int             i;

	if (jp->item[SPOC_ENDOFRULE]) {
		id = jp->item[SPOC_ENDOFRULE]->val.id;
		for (i = 0; i < id->n; i++)
			in = varr_ruleinst_add(in, id->arr[i]);
	}

	if (jp->item[SPOC_LIST])
		in = get_rec_all_rules(jp->item[SPOC_LIST]->val.list, in);

	if (jp->item[SPOC_ENDOFLIST])
		in = get_rec_all_rules(jp->item[SPOC_ENDOFLIST]->val.list, in);
	/*
	 * --- 
	 */

	if (jp->item[SPOC_ATOM])
		pa = get_all_atom_followers(jp->item[SPOC_ATOM], pa);

	if (jp->item[SPOC_PREFIX])
		pa = get_all_ssn_followers(jp->item[SPOC_PREFIX], SPOC_PREFIX,
					   pa);

	if (jp->item[SPOC_SUFFIX])
		pa = get_all_ssn_followers(jp->item[SPOC_SUFFIX], SPOC_SUFFIX,
					   pa);

	if (jp->item[SPOC_RANGE])
		pa = get_all_range_followers(jp->item[SPOC_RANGE], pa);

	/*
	 * --- 
	 */

	if (pa)
		in = get_rule_indexes(pa, in);

	return in;
}

/*************************************************************************/

varr_t         *
get_all_to_next_listend(junc_t * jp, varr_t * in, int lev)
{
	int             i;
	varr_t         *pa = 0;
	junc_t         *ju;
	void           *v;

	DEBUG(SPOCP_DMATCH) junc_print(lev, jp);

	if (jp->item[SPOC_ENDOFRULE]);	/* shouldn't get here */

	if (jp->item[SPOC_LIST])
		in = get_all_to_next_listend(jp->item[SPOC_LIST]->val.list, in,
					     lev + 1);

	if (jp->item[SPOC_ENDOFLIST]) {
		ju = jp->item[SPOC_ENDOFLIST]->val.list;
		lev--;
		if (lev >= 0)
			pa = varr_junc_add(pa, ju);
		else
			in = varr_junc_add(in, ju);
	}

	/*
	 * --- 
	 */

	if (jp->item[SPOC_ATOM])
		pa = get_all_atom_followers(jp->item[SPOC_ATOM], pa);

	if (jp->item[SPOC_PREFIX])
		pa = get_all_ssn_followers(jp->item[SPOC_PREFIX], SPOC_PREFIX,
					   pa);

	if (jp->item[SPOC_SUFFIX])
		pa = get_all_ssn_followers(jp->item[SPOC_SUFFIX], SPOC_SUFFIX,
					   pa);

	if (jp->item[SPOC_RANGE])
		pa = get_all_range_followers(jp->item[SPOC_RANGE], pa);

	/*
	 * --- 
	 */

	if (pa) {
		for (i = 0; (v = varr_nth(pa, i)); i++)
			in = get_all_to_next_listend(v, in, lev);
	}

	return in;
}

/*************************************************************************/

static varr_t  *
range2range_match_lte(range_t * rp, slist_t ** sarr, varr_t * pap)
{
	int             dtype = rp->lower.type & 0x07;

	pap = sl_range_lte_match(sarr[dtype], rp, pap);

	return pap;
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

varr_t         *
subelem_lte_match(junc_t * jp, element_t * ep, varr_t * pap, varr_t * id)
{
	branch_t       *bp;
	junc_t         *np;
	varr_t         *nap, *arr;
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
			pap =
			    prefix2atoms_match(ep->e.atom->val.val,
					       bp->val.atom, pap);
		}

		if ((bp = ARRFIND(jp, SPOC_PREFIX)) != 0) {
			pap =
			    prefix2prefix_match_lte(ep->e.atom, bp->val.prefix,
						    pap);
		}
		break;

	case SPOC_SUFFIX:	/* suffixes or atoms */
		if ((bp = ARRFIND(jp, SPOC_ATOM)) != 0) {
			pap =
			    suffix2atoms_match(ep->e.atom->val.val,
					       bp->val.atom, pap);
		}

		if ((bp = ARRFIND(jp, SPOC_SUFFIX)) != 0) {
			pap =
			    suffix2suffix_match_lte(ep->e.atom, bp->val.suffix,
						    pap);
		}
		break;

	case SPOC_RANGE:	/* ranges or atoms */
		if ((bp = ARRFIND(jp, SPOC_ATOM)) != 0) {
			pap =
			    range2atoms_match(ep->e.range, bp->val.atom, pap);
		}

		if ((bp = ARRFIND(jp, SPOC_RANGE)) != 0) {
			pap =
			    range2range_match_lte(ep->e.range, bp->val.range,
						  pap);
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
			nap = subelem_lte_match(jp, elem, nap, id);
		}
		pap = varr_or(pap, nap, 0);
		break;

	case SPOC_LIST:	/* other lists */
		arr = varr_junc_add(0, jp);

		for (ep = ep->e.list->head; ep; ep = ep->next) {
			nap = 0;
			for (i = 0; (v = varr_nth(arr, i)); i++) {
				nap = subelem_lte_match(v, ep, nap, id);
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
 * Match rule elements where the subelement is LTE 
 */

varr_t         *
subelem_match_lte(junc_t * jp, element_t * ep, varr_t * pap, varr_t * id,
		  spocp_result_t * rc)
{
	branch_t       *bp;
	junc_t         *np;
	varr_t         *nap, *arr;
	int             i;
	slist_t       **slpp;
	varr_t         *set;
	void           *v;

	/*
	 * DEBUG( SPOCP_DMATCH ) traceLog( LOG_DEBUG,"subelem_match_lte") ; 
	 */

	/*
	 * DEBUG( SPOCP_DMATCH ) junc_print( 0, jp ) ; 
	 */

	if ((bp = ARRFIND(jp, SPOC_ENDOFRULE))) {	/* Can't get much
							 * further */
		id = varr_or(id, (void *) bp->val.id, 1);
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
					nap =
					    atom2range_match(ep->e.atom,
							     slpp[i], i, rc);
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
					nap =
					    range2range_match(ep->e.range,
							      slpp[i]);
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
			    subelem_match_lte(jp, (element_t *) v, nap, id,
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
				for (i = 0; (v = varr_nth(arr, i)); i++) {
					nap =
					    subelem_match_lte(v, ep, nap, id,
							      rc);
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

			/*
			 * traceLog(LOG_INFO,"Gather until end of list") ; 
			 */

			for (i = 0; (v = varr_nth(arr, i)); i++)
				pap = get_all_to_next_listend(v, pap, 0);
		}
		break;

	}

	return pap;
}

/****************************************************************************/

static varr_t  *
adm_list(junc_t * jp, subelem_t * sub, spocp_result_t * rc)
{
	varr_t         *nap = 0, *res, *pap = 0;
	int             i;
	varr_t         *id = 0;
	void           *v;

	if (jp == 0)
		return 0;

	/*
	 * The rules must be lists and I only have subelements so I have to
	 * skip the list tag part 
	 */

	/*
	 * just the single list tag should actually match everything 
	 */
	if (jp->item[1] == 0)
		return get_rec_all_rules(jp, 0);

	jp = jp->item[1]->val.list;

	pap = varr_junc_add(0, jp);

	for (; sub; sub = sub->next) {
		res = 0;

		for (i = 0; (v = varr_nth(pap, i)); i++) {
			if (sub->direction == LT)
				nap = subelem_lte_match(v, sub->ep, nap, id);
			else
				nap =
				    subelem_match_lte(v, sub->ep, nap, id, rc);
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
	 * traceLog(LOG_INFO, "Done all subelements, %d junctions to follow through",
	 * pap->n ) ; 
	 */

	/*
	 * After going through all the subelement I should have a list of
	 * junctions which have to be followed to the end 
	 */

	for (i = 0; (v = varr_nth(pap, i)); i++) {
		id = get_rec_all_rules(v, id);
	}

	return id;
}

/********************************************************************/

static subelem_t *
pattern_parse(octarr_t * pattern)
{
	subelem_t      *res = 0, *sub, *pres = 0;
	element_t      *ep;
	octet_t         oct, **arr;
	int             i;

	for (i = 0, arr = pattern->arr; i < pattern->n; i++, arr++) {
		sub = subelem_new();

		if (*((*arr)->val) == '+')
			sub->direction = GT;
		else if (*((*arr)->val) == '-')
			sub->direction = LT;
		else
			return 0;

		if (*((*arr)->val + 1) == '(')
			sub->list = 1;

		octln(&oct, *arr);
		/*
		 * skip the +/- sign 
		 */
		oct.val++;
		oct.len--;

		if (element_get(&oct, &ep) != SPOCP_SUCCESS) {
			subelem_free(sub);
			subelem_free(res);
			return 0;
		}

		sub->ep = ep;
		if (res == 0)
			res = pres = sub;
		else {
			pres->next = sub;
			pres = sub;
		}
	}

	return res;
}

/********************************************************************/

spocp_result_t
get_matching_rules(db_t * db, octarr_t * pat, octarr_t * oa, char *rs)
{
	spocp_result_t  rc = SPOCP_SUCCESS;
	subelem_t      *sub;
	varr_t         *ip;
	octet_t        *oct, *arr[2];
	int             i;
	ruleinst_t     *r;
	unsigned int    ui;

	if ((sub = pattern_parse(pat)) == 0) {
		return SPOCP_SYNTAXERROR;
	}

	/*
	 * traceLog(LOG_INFO, "Parsed the subelements OK" ) ; 
	 */

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
