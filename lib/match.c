
/***************************************************************************
                          match.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>
#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include <struct.h>
#include <func.h>
#include <spocp.h>
#include <macros.h>
#include <wrappers.h>


/*
 * =============================================================== 
 */
/*
 * This routines tries to match a S-expression, broken down into it's parts as 
 * a linked structure of elements, with a tree. Important is to realize that
 * at every branching point in the tree there might easily be more than one
 * branch that originates there. Every branch at a branching point is typed
 * (atom, prefix, suffix, range, boundary condition, list, endOfList,
 * endOfRule) and they are therefore checked in a preferred order
 * (fastest->slowest). Each branch is in turn checked all the way down to a
 * leaf(endOFRule) is found or until no sub branch matches. 
 */

/*
 * =============================================================== 
 */


junc_t         *
atom2atom_match(atom_t * ap, phash_t * pp)
{
	buck_t         *bp;

	DEBUG(SPOCP_DMATCH) {
		traceLog("<<<<<<<<<<<<<<<<<<<");
		phash_print(pp);
		traceLog(">>>>>>>>>>>>>>>>>>>");
	}

	bp = phash_search(pp, ap, ap->hash);

	if (bp)
		return bp->next;
	else
		return 0;
}

varr_t         *
atom2prefix_match(atom_t * ap, ssn_t * pp)
{
	varr_t         *avp = 0;

	DEBUG(SPOCP_DMATCH) {
		traceLog("----%s*-----", ap->val.val);
	}

	avp = ssn_match(pp, ap->val.val, FORWARD);

	return avp;
}

varr_t         *
atom2suffix_match(atom_t * ap, ssn_t * pp)
{
	varr_t         *avp = 0;

	avp = ssn_match(pp, ap->val.val, BACKWARD);

	return avp;
}

varr_t         *
atom2range_match(atom_t * ap, slist_t * slp, int vtype, spocp_result_t * rc)
{
	boundary_t      value;
	octet_t        *vo;
	int             r = 1;

	value.type = vtype | GLE;
	vo = &value.v.val;

	DEBUG(SPOCP_DMATCH) traceLog("-ATOM->RANGE: %s", ap->val.val);

	/*
	 * make certain that the atom is of the right type 
	 */
	switch (vtype) {
	case SPOC_DATE:
		if (is_date(&ap->val) == SPOCP_SUCCESS) {
			vo->val = ap->val.val;
			vo->len = ap->val.len - 1;	/* without the Z at
							 * the end */
		} else
			r = 0;
		break;

	case SPOC_TIME:
		if (is_time(&ap->val) == SPOCP_SUCCESS)
			hms2int(&ap->val, &value.v.num);
		else
			r = 0;
		break;

	case SPOC_NUMERIC:
		if (is_numeric(&ap->val, &value.v.num) != SPOCP_SUCCESS)
			r = 0;
		break;

	case SPOC_IPV4:
		if (is_ipv4(&ap->val, &value.v.v4) != SPOCP_SUCCESS)
			r = 0;
		break;

#ifdef USE_IPV6
	case SPOC_IPV6:
		if (is_ipv6(&ap->val, &value.v.v6) != SPOCP_SUCCESS)
			r = 0;
		break;
#endif

	case SPOC_ALPHA:
		vo->val = ap->val.val;
		vo->len = ap->val.len;
		break;
	}

	if (!r) {
		*rc = SPOCP_SYNTAXERROR;
		DEBUG(SPOCP_DMATCH) traceLog("Wrong format [%s]", ap->val.val);
		return 0;
	} else {
		DEBUG(SPOCP_DMATCH) {
			traceLog("skiplist match");
		}

		return sl_match(slp, &value);
	}
}

varr_t         *
prefix2prefix_match(atom_t * prefa, ssn_t * prefix)
{
	return atom2prefix_match(prefa, prefix);
}

varr_t         *
suffix2suffix_match(atom_t * prefa, ssn_t * suffix)
{
	return atom2suffix_match(prefa, suffix);
}

varr_t         *
range2range_match(range_t * ra, slist_t * slp)
{
	return sl_range_match(slp, ra);
}

/*
 * 
 * This is the matrix over comparisions, that I'm using
 * 
 * X = makes sense O = can make sense in very special cases, should be handled 
 * while storing rules. Which means I don't care about them here
 * 
 * | atom | prefix | range | set | list | suffix | any |
 * --------+------+--------+-------+-----+------|--------|-------| atom | X |
 * X | X | X | | X | X |
 * --------+------+--------+-------+-----+------|--------|-------| prefix | |
 * X | | | | | X |
 * --------|------+--------+-------+-----+------|--------|-------| range | 0 | 
 * | X | 0 | | | X |
 * --------+------+--------+-------+-----+------|--------|-------| set | 0 | 0 
 * | 0 | X | 0 | 0 | X |
 * --------+------+--------+-------+-----+------|--------|-------| list | | |
 * | | X | | X |
 * ---------------------------------------------+--------|-------| suffix | |
 * | | | | X | X |
 * ---------------------------------------------+--------|-------| any | | | | 
 * X | | | X | ---------------------------------------------+--------|-------|
 * 
 */

static junc_t  *
ending(junc_t * jp, element_t * ep, comparam_t * comp)
{
	branch_t       *bp;
	element_t      *nep;
	junc_t         *vl = 0;
	spocp_result_t  r;

	if (!jp)
		return 0;

	if (jp->item[SPOC_ENDOFRULE]) {
		/*
		 * THIS IS WHERE BCOND IS CHECKED 
		 */
		r = bcond_check(comp->head, jp->item[SPOC_ENDOFRULE]->val.id,
				comp->blob);
		if (r != SPOCP_SUCCESS)
			return 0;
		else
			return jp;
	}

	if (jp->item[SPOC_ENDOFLIST]) {

		bp = jp->item[SPOC_ENDOFLIST];

		if (ep) {
			/*
			 * better safe then sorry, this should not be
			 * necessary FIX 
			 */
			if (ep->next == 0 && ep->memberof == 0) {
				vl = bp->val.list;

				if (vl && vl->item[SPOC_ENDOFRULE]) {
					/*
					 * THIS IS WHERE BCOND IS CHECKED 
					 */
					r = bcond_check(comp->head,
							vl->
							item[SPOC_ENDOFRULE]->
							val.id, comp->blob);
					if (r != SPOCP_SUCCESS)
						return 0;
					else
						return vl;
				} else
					return 0;
			}

			nep = ep->memberof;

			DEBUG(SPOCP_DMATCH) {
				if (nep->type == SPOC_LIST) {
					char           *tmp;
					tmp =
					    oct2strdup(&nep->e.list->head->e.
						       atom->val, '\\');
					traceLog("found end of list [%s]",
						 tmp);
					free(tmp);
				}
			}

			vl = bp->val.list;

			while (nep->memberof) {

				if (vl->item[SPOC_ENDOFLIST])
					bp = vl->item[SPOC_ENDOFLIST];
				else
					break;

				nep = nep->memberof;

				DEBUG(SPOCP_DMATCH) {
					if (nep->type == SPOC_LIST) {
						char           *tmp;
						tmp =
						    oct2strdup(&nep->e.list->
							       head->e.atom->
							       val, '\\');
						traceLog
						    ("found end of list [%s]",
						     tmp);
						free(tmp);
					}
				}

				vl = bp->val.list;
			}

			if (nep->next) {
				jp = element_match_r(vl, nep->next, comp);

				if (jp && jp->item[SPOC_ENDOFRULE])
					return jp;

			} else if (vl->item[SPOC_ENDOFRULE]) {
				/*
				 * THIS IS WHERE BCOND IS CHECKED 
				 */
				r = bcond_check(comp->head,
						vl->item[SPOC_ENDOFRULE]->val.
						id, comp->blob);
				if (r != SPOCP_SUCCESS)
					return 0;
				else
					return vl;
			}
		} else {	/* ep == 0, can't do a boundary check that
				 * uses variable substitution */
			jp = bp->val.list;
			if (jp->item[SPOC_ENDOFRULE]) {
				/*
				 * THIS IS WHERE BCOND IS CHECKED 
				 */
				r = bcond_check(comp->head,
						jp->item[SPOC_ENDOFRULE]->val.
						id, comp->blob);
				if (r != SPOCP_SUCCESS)
					return 0;
				else
					return jp;
			} else
				return jp;
		}
	} else if (jp->item[SPOC_ENDOFRULE]) {
		/*
		 * THIS IS WHERE BCOND IS CHECKED 
		 */
		r = bcond_check(comp->head, jp->item[SPOC_ENDOFRULE]->val.id,
				comp->blob);
		if (r != SPOCP_SUCCESS)
			return 0;
		else
			return jp;
	}

	return 0;
}

/*****************************************************************/

/*
 * static junc_t * do_branches( varr_t *avp, element_t *ep, spocp_result_t
 * *rc, octarr_t **on) { int i ; junc_t *jp = 0, *ea = 0 ;
 * 
 * * DEBUG( SPOCP_DMATCH ) traceLog( "do_branches: %d", varr_items( avp )) ; *
 * 
 * for( i = 0 ; ; i++ ) { if(( jp = varr_junc_nth( avp, i)) == 0 ) break ;
 * DEBUG( SPOCP_DMATCH ) traceLog( "ending called from do_branches" ) ; if((
 * ea = ending( jp, ep, rc, on ))) break ; }
 * 
 * if( jp != 0 ) jp = ea ;
 * 
 * * clean up * varr_free( avp ) ;
 * 
 * return jp ; } 
 */

/*****************************************************************/

static junc_t  *
do_arr(varr_t * a, element_t * e, comparam_t * comp)
{
	junc_t         *jp = 0;

	while ((jp = varr_junc_pop(a))) {
		if ((jp = element_match_r(jp, e, comp)) != 0)
			break;
	}

	varr_free(a);

	return jp;
}

/*****************************************************************/

static junc_t  *
next(junc_t * ju, element_t * ep, comparam_t * comp)
{
	junc_t         *jp;
	branch_t       *bp;

	if (ep->next == 0) {	/* end of list */
		do {
			if (!ep->memberof)
				break;
			if ((bp = ju->item[SPOC_ENDOFLIST]) == 0)
				break;
			ju = bp->val.list;
			ep = ep->memberof;
		} while (ep->next == 0 && ju->item[SPOC_ENDOFLIST]);

		if (!ep->memberof) {	/* reached the end */
			if ((jp = ending(ju, ep, comp)))
				return jp;
		}
	}

	jp = element_match_r(ju, ep->next, comp);

	return jp;
}

/*****************************************************************/

static junc_t  *
atom_match(junc_t * db, element_t * ep, comparam_t * comp)
{
	branch_t       *bp;
	junc_t         *jp = 0, *ju;
	varr_t         *avp = 0;
	element_t      *nep;
	slist_t       **slp;
	int             i;
	atom_t         *atom;
	char           *tmp;

	if (ep == 0)
		return 0;

	atom = ep->e.atom;
	nep = ep->next;

	if ((bp = ARRFIND(db, SPOC_ATOM)) != 0) {
		if ((ju = atom2atom_match(atom, bp->val.atom))) {
			DEBUG(SPOCP_DMATCH) {
				tmp = oct2strdup(&atom->val, '%');
				traceLog("Matched atom %s", tmp);
				free(tmp);
			}
			jp = next(ju, ep, comp);

			if (jp)
				return jp;
		} else {
			DEBUG(SPOCP_DMATCH) {
				tmp = oct2strdup(&atom->val, '%');
				traceLog("Failed to matched atom %s", tmp);
				free(tmp);
			}
		}
	}

	if ((bp = ARRFIND(db, SPOC_PREFIX)) != 0) {

		avp = atom2prefix_match(atom, bp->val.prefix);

		if (avp) {
			DEBUG(SPOCP_DMATCH) traceLog("matched prefix");
			jp = do_arr(avp, nep, comp);
		}
		if (jp)
			return jp;
	}

	if ((bp = ARRFIND(db, SPOC_SUFFIX)) != 0) {

		avp = atom2suffix_match(atom, bp->val.suffix);

		if (avp) {
			DEBUG(SPOCP_DMATCH) traceLog("matched suffix");
			jp = do_arr(avp, nep, comp);
		}

		if (jp)
			return jp;
	}

	if ((bp = ARRFIND(db, SPOC_RANGE)) != 0) {
		/*
		 * how do I get to know the type ? I don't so I have to test
		 * everyone 
		 */

		slp = bp->val.range;

		for (i = 0; i < DATATYPES; i++) {
			if (slp[i]) {
				avp =
				    atom2range_match(atom, slp[i], i,
						     &comp->rc);
				if (avp) {
					jp = do_arr(avp, nep, comp);
					if (jp)
						break;
				}
			}
		}
	}

	return jp;
}

/*****************************************************************/
/*
 * recursive matching of nodes 
 */

junc_t         *
element_match_r(junc_t * db, element_t * ep, comparam_t * comp)
{
	branch_t       *bp;
	junc_t         *jp = 0;
	varr_t         *avp = 0;
	int             i;
	slist_t       **slp;
	varr_t         *set;
	void           *v;

	if (db == 0)
		return 0;

	/*
	 * may have several roads to follow, this might just be one of them 
	 */
	DEBUG(SPOCP_DMATCH) traceLog("ending called from element_match_r");
	if ((jp = ending(db, ep, comp))) {
		return jp;
	}

	if (ep == 0) {
		DEBUG(SPOCP_DMATCH) traceLog("No more elements in input");
		return jp;
	}

	if ((bp = ARRFIND(db, SPOC_ANY)) != 0) {
		jp = element_match_r(bp->val.next, ep->next, comp);
		if (jp)
			return jp;
	}

	switch (ep->type) {
	case SPOC_ATOM:
		DEBUG(SPOCP_DMATCH) traceLog("Checking ATOM");
		jp = atom_match(db, ep, comp);
		break;

	case SPOC_SET:
		/*
		 */
		set = ep->e.set;
		for (v = varr_first(set); v; v = varr_next(set, v)) {
			jp = element_match_r(db, (element_t *) v, comp);
			if (jp && (ARRFIND(jp, SPOC_ENDOFRULE)))
				break;
		}

		break;

	case SPOC_PREFIX:
		if ((bp = ARRFIND(db, SPOC_PREFIX)) != 0) {
			avp = prefix2prefix_match(ep->e.atom, bp->val.prefix);

			/*
			 * got a set of plausible branches and more elements
			 */
			if (avp)
				jp = do_arr(avp, ep->next, comp);
			else
				jp = 0;
		}
		break;

	case SPOC_SUFFIX:
		if ((bp = ARRFIND(db, SPOC_SUFFIX)) != 0) {
			avp = suffix2suffix_match(ep->e.atom, bp->val.suffix);

			/*
			 * got a set of plausible branches and more elements
			 */
			if (avp)
				jp = do_arr(avp, ep->next, comp);
			else
				jp = 0;
		}
		break;

	case SPOC_RANGE:
		if ((bp = ARRFIND(db, SPOC_RANGE)) != 0) {
			slp = bp->val.range;

			i = ep->e.range->lower.type & 0x07;

			if (slp[i])	/* no use testing otherwise */
				avp = range2range_match(ep->e.range, slp[i]);

			/*
			 * got a set of plausible branches 
			 */
			if (avp)
				jp = do_arr(avp, ep->next, comp);
			else
				jp = 0;
		}
		break;

	case SPOC_LIST:
		DEBUG(SPOCP_DMATCH) traceLog("Checking LIST");
		if ((bp = ARRFIND(db, SPOC_LIST)) != 0) {
			jp = element_match_r(bp->val.list, ep->e.list->head,
					     comp);
		}
		break;

	case SPOC_ANY:
		/*
		 * jp = 0 ; implied 
		 */
		break;

	case SPOC_ENDOFLIST:
		bp = ARRFIND(db, SPOC_ENDOFLIST);
		if (bp)
			return bp->val.list;
		else
			return 0;
		break;		/* never reached */
	}

	return jp;
}
