
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

#include <basic.h>
/*#include <element.h>*/
#include <atom.h>
#include <resset.h>
#include <hash.h>
#include <element.h>
#include <ssn.h>
#include <branch.h>
#include <db0.h>
#include <bcondfunc.h>
#include <spocp.h>
#include <macros.h>
#include <wrappers.h>
#include <verify.h>
#include <log.h>
#include <match.h>

/*#define AVLUS 1*/

static resset_t *next(junc_t *, element_t *, comparam_t *) ;

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
		traceLog(LOG_DEBUG,"<<<<<<<<<<<<<<<<<<<");
		phash_print(pp);
		traceLog(LOG_DEBUG,">>>>>>>>>>>>>>>>>>>");
	}

	bp = phash_search(pp, ap);

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
		traceLog(LOG_DEBUG,"----%s*-----", ap->val.val);
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
prefix2prefix_match(atom_t * prefa, ssn_t * prefix)
{
	return atom2prefix_match(prefa, prefix);
}

varr_t         *
suffix2suffix_match(atom_t * prefa, ssn_t * suffix)
{
	return atom2suffix_match(prefa, suffix);
}

/*
 * 
 * This is the matrix over comparisions, that I'm using
 * 
 * X = makes sense O = can make sense in very special cases, should be handled 
 * while storing rules. Which means I don't care about them here
 * 
 *         | atom | prefix | range | set | list | suffix |
 * --------+------+--------+-------+-----+------|--------|
 * atom    | X    | X      | X     | X   |      | X      |
 * --------+------+--------+-------+-----+------|--------|
 * prefix  |      | X      |       |     |      |        |
 * --------|------+--------+-------+-----+------|--------|
 * range   | 0    |        | X     | 0   |      |        |
 * --------+------+--------+-------+-----+------|--------|
 * set     | 0    | 0      | 0     | X   |   0  |  0     |
 * --------+------+--------+-------+-----+------|--------|
 * list    |      |        |       |     | X    |        |
 * ---------------------------------------------+--------|
 * suffix  |      |        |       |     |      | X      |
 * ---------------------------------------------+--------|
 * 
 */

static resset_t	*
rss_add( resset_t *rs, ruleinst_t *ri, comparam_t *comp)
{
	if (comp->blob) { 
		rs = resset_add(rs, ri, *comp->blob);
		*comp->blob = NULL;
	}
	else
		rs = resset_add( rs, ri, 0);

	return rs;
}

static resset_t  *
ending(junc_t * jp, element_t *ep, comparam_t * comp)
{
	branch_t        *bp;
	element_t       *nep;
	junc_t          *vl = 0;
	spocp_result_t	r = SPOCP_DENIED;
	resset_t        *res = 0;
    ruleinst_t      *ri;

	if (!jp)
		return 0;

	if (jp->branch[SPOC_ENDOFRULE]) {
		DEBUG(SPOCP_DMATCH) 
			traceLog(LOG_DEBUG,"ENDOFRULE marker(0)");
		/*
		 */
		ri = jp->branch[SPOC_ENDOFRULE]->val.ri ;
		if (comp->nobe) {
			res = rss_add( res, ri, comp);
		}
		else if ((r = bcond_check(comp->head, ri, comp->blob, comp->cr))
				== SPOCP_SUCCESS) {
#ifdef AVLUS
			traceLog(LOG_DEBUG,"ENDOFRULE"); 
#endif
			res = rss_add( res, ri, comp);
			
			if (!comp->all)
				return res;
		}
	}

	if (jp->branch[SPOC_ENDOFLIST]) {

		bp = jp->branch[SPOC_ENDOFLIST];

		if (ep) {

#ifdef AVLUS
			octet_t *eop ;

			eop = oct_new( 512,NULL);
			element_print( eop, ep );
			oct_print( LOG_INFO, "Last element in list", eop);
			oct_free( eop);
#endif

			/*
			 * better safe then sorry, this should not be
			 * necessary FIX 
			 */
			if (ep->next == 0 && ep->memberof == 0) {
				vl = bp->val.list;

				if (vl && vl->branch[SPOC_ENDOFRULE]) {
					DEBUG(SPOCP_DMATCH) 
						traceLog(LOG_DEBUG,
							"ENDOFRULE marker(1)");
					/*
					 * THIS IS WHERE BCOND IS CHECKED 
					 */
					ri = vl->branch[SPOC_ENDOFRULE]->val.ri;
					r = bcond_check(comp->head, ri, comp->blob,
						comp->cr);
					if (r == SPOCP_SUCCESS) {
						res = rss_add( res, ri, comp);

						if(!comp->all)
							return res;
					}
				} 

				return res;
			}

			nep = ep->memberof;

			DEBUG(SPOCP_DMATCH) {
				if (nep->type == SPOC_LIST) {
					oct_print(LOG_DEBUG,
					    "found end of list of",
					    &nep->e.list->head->e.atom->val);
				}
			}

			vl = bp->val.list;

			while (nep->memberof) {

				if (vl->branch[SPOC_ENDOFLIST])
					bp = vl->branch[SPOC_ENDOFLIST];
				else
					break;

				nep = nep->memberof;

				DEBUG(SPOCP_DMATCH) {
					if (nep->type == SPOC_LIST) {
						oct_print(LOG_DEBUG,
					    	   "found end of list of",
					   	   &nep->e.list->head->e.atom->val);
					}
				}

				vl = bp->val.list;
			}

			if (nep->next) {
				res = resset_extend(res,element_match_r(vl, nep->next, comp));

			} else if (vl->branch[SPOC_ENDOFRULE]) {
				DEBUG(SPOCP_DMATCH) 
					traceLog(LOG_DEBUG,
						"ENDOFRULE marker(3)",
						comp->nobe);
				/*
				 * THIS IS WHERE BCOND IS CHECKED 
				 */
				ri = vl->branch[SPOC_ENDOFRULE]->val.ri;
				if (comp->nobe) {
#ifdef AVLUS
					ruleinst_print( ri );
#endif
					res = rss_add( res, ri, comp);
				}
				else {
					r = bcond_check(comp->head, ri, comp->blob, comp->cr);
					if (r == SPOCP_SUCCESS) {
						res = rss_add( res, ri, comp);
					}
				}
			}
			else
				DEBUG(SPOCP_DMATCH) {
					traceLog(LOG_DEBUG,"Weird !?");
					traceLog(LOG_DEBUG,"memberof %p",
						nep->memberof);
					junc_print( 0, vl );
				}

		} else {
			vl = bp->val.list;
			while(1) {
				if (vl->branch[SPOC_ENDOFRULE]) {
					DEBUG(SPOCP_DMATCH) 
						traceLog(LOG_DEBUG, "ENDOFRULE marker(4)");
					/*
					 * THIS IS WHERE BCOND IS CHECKED 
					 */
					ri = vl->branch[SPOC_ENDOFRULE]->val.ri;
					if (comp->nobe) {
						res = rss_add( res, ri, comp);
					}
					else {
						r = bcond_check(comp->head, ri, comp->blob, comp->cr);
						if (r == SPOCP_SUCCESS) { 
							res = rss_add(res, ri, comp);
						}
					}
					break;
				} 
				else {
					bp = vl->branch[SPOC_ENDOFLIST];
					if (bp) {
						DEBUG(SPOCP_DMATCH) 
							traceLog(LOG_DEBUG, "ENDOFLIST marker");
						vl = bp->val.list;
					}
					else
						break;
				} 
			}
		}
	} 

#ifdef AVLUS
	traceLog(LOG_DEBUG,"--Ending %p--", res);
#endif

	return res;
}

/*****************************************************************/

static resset_t  *
do_arr(varr_t * a, element_t * e, comparam_t * comp)
{
	junc_t		*jp = 0;
	resset_t	*rs = 0, *r;

	while ((jp = varr_junc_pop(a))) {
#ifdef AVLUS
		octet_t *eop;

		eop = oct_new( 512,NULL);
		element_print( eop, e );
		oct_print(LOG_INFO, "do_arr", eop);
		oct_free( eop);

#endif
		r = next( jp, e, comp );

		if (!r || comp->all)
			r = resset_extend(r,element_match_r(jp, e, comp));
#ifdef AVLUS
		DEBUG(SPOCP_DMATCH)
			resset_print(r);
#endif
		if (r) {
			if (!comp->all) {
				rs = r;
				break;
			}
			else
				rs = resset_extend(rs,r);
		}
	}

	varr_free(a);

	return rs;
}

/*****************************************************************/

static resset_t  *
next(junc_t * ju, element_t * ep, comparam_t * comp)
{
	resset_t	*rs = 0;
	branch_t	*bp;
	element_t	*sep;

	DEBUG(SPOCP_DMATCH)
		traceLog(LOG_DEBUG,"Next ?");

	rs = ending(ju, ep, comp);
	sep = ep;
	if (rs) {
	       	if(!comp->all)
			return rs;
	}

	if (ep->next == 0) {	/* end of list */
		DEBUG(SPOCP_DMATCH) traceLog(LOG_DEBUG,"End of list");
		do {
			/* reached the top list */
			if (ep->memberof == 0) {
				DEBUG(SPOCP_DMATCH)
				       traceLog(LOG_DEBUG,"Top of the s-exp");
				break;
			}

			bp = ju->branch[SPOC_ENDOFLIST];

			if (bp)
				ju = bp->val.list;
			else
				break;

			ep = ep->memberof;

		} while (ep->next == 0 && ju->branch[SPOC_ENDOFLIST]);

		/*
		if (ep->memberof == 0) {
			DEBUG(SPOCP_DMATCH)
			    traceLog(LOG_DEBUG,"ending called from next");

			if ( sep == ep )
				return rs;

			if ((rs = resset_extend(rs,ending(ju, ep, comp))) &&
				!comp->all) 
				return rs;
		}
		*/
	}

	DEBUG(SPOCP_DMATCH)
		traceLog(LOG_DEBUG,"Do Next");

	if (ep->next)
		rs = resset_extend(rs,element_match_r(ju, ep->next, comp));

#ifdef AVLUS
	traceLog(LOG_DEBUG,"Next() => ");
	resset_print( rs );
#endif

	return rs;
}

/*****************************************************************/

static resset_t  *
atom_match(junc_t * db, element_t * ep, comparam_t * comp)
{
	branch_t	*bp;
	junc_t		*ju;
	varr_t		*avp = 0;
	slist_t		**slp;
	int		i;
	atom_t		*atom;
	resset_t	*rs = 0;

	if (ep == 0)
		return 0;

	atom = ep->e.atom;

	if ((bp = ARRFIND(db, SPOC_ATOM)) != 0) {
		if ((ju = atom2atom_match(atom, bp->val.atom))) {
			DEBUG(SPOCP_DMATCH) {
				oct_print(LOG_DEBUG,"Matched atom",&atom->val);
			}
			rs = next(ju, ep, comp);

			if (rs && !comp->all)
				return rs;
		} else {
			DEBUG(SPOCP_DMATCH) {
				oct_print(LOG_DEBUG,"Failed to matched atom",
					&atom->val);
			}
		}
	}

	if ((bp = ARRFIND(db, SPOC_PREFIX)) != 0) {

		avp = atom2prefix_match(atom, bp->val.prefix);

		if (avp) {
			DEBUG(SPOCP_DMATCH) traceLog(LOG_DEBUG,"matched prefix");
			rs = resset_extend(rs, do_arr(avp, ep, comp));
			if (rs && !comp->all)
				return rs;
		}
	}

	if ((bp = ARRFIND(db, SPOC_SUFFIX)) != 0) {

		avp = atom2suffix_match(atom, bp->val.suffix);

		if (avp) {

			DEBUG(SPOCP_DMATCH) traceLog(LOG_DEBUG,"matched suffix");

#ifdef AVLUS
			{
				octet_t *eop ;

				eop = oct_new( 512,NULL);
				element_print( eop, ep );
				oct_print(LOG_INFO, "ep", eop);
				oct_free( eop);
			}
#endif

			rs = resset_extend(rs,do_arr(avp, ep, comp));
			if (rs && !comp->all)
				return rs;
		}
	}

	if ((bp = ARRFIND(db, SPOC_RANGE)) != 0) {
		/*
		 * how do I get to know the type ? I don't so I have to test
		 * everyone 
		 */

		slp = bp->val.range;

		for (i = 0; i < DATATYPES; i++) {
			if (slp[i]) {
				avp = bsl_match_atom(slp[i], atom, &comp->rc);
				if (avp) {
					rs = resset_extend(rs, do_arr(avp, ep, comp));
					if (rs && !comp->all)
						break;
				}
			}
		}
	}

	return rs;
}

/*****************************************************************/
/*
 * recursive matching of nodes 
 */

resset_t         *
element_match_r(junc_t * db, element_t * ep, comparam_t * comp)
{
	branch_t	*bp;
	varr_t		*avp = 0;
	int		i, old;
	slist_t		**slp;
	varr_t		*set;
	void		*v;
	resset_t	*res = 0, *rs, *setrs=0;

	if (db == 0)
		return 0;

#ifdef AVLUS
	{
		octet_t		*eop;

		eop = oct_new( 512,NULL);
		element_print( eop, ep );
		oct_print(LOG_INFO, "element_match", eop);
		oct_free(eop);
	}
#endif

	/*
	 * may have several roads to follow, this might just be one of them 
	DEBUG(SPOCP_DMATCH) traceLog(LOG_DEBUG,"ending called from element_match_r");
	if ((res = ending(db, ep, comp)) && !comp->all) {
		return res;
	}
	 */

	if (ep == 0) {
		DEBUG(SPOCP_DMATCH) traceLog(LOG_DEBUG, "No more elements in input");
		return NULL;
	}

	if ((bp = ARRFIND(db, SPOC_ANY)) != 0) {
		res = resset_extend(res,
			  element_match_r(bp->val.next, ep->next, comp));
		if (!comp->all)
			return res;
	}

	switch (ep->type) {
	case SPOC_ATOM:
		DEBUG(SPOCP_DMATCH) {
			oct_print(LOG_INFO, "Checking ATOM", &ep->e.atom->val);
		}
		res = resset_extend(res, atom_match(db, ep, comp));
		break;

	case SPOC_SET:
		/*
		 */
		DEBUG(SPOCP_DMATCH) traceLog(LOG_DEBUG, "Doing SET");
		set = ep->e.set;
		/* have to remember what it was so I can reset it afterwards */
		old = comp->all;
		comp->all = 1;
		comp->nobe = 1;
		for (v = varr_first(set), i = 0; v; v = varr_next(set, v), i++) {

			rs = element_match_r(db, (element_t *) v, comp);
            setrs = resset_extend(setrs, rs);
			
			if (setrs == 0)
				break;
			else {
				DEBUG(SPOCP_DMATCH) {
					traceLog(LOG_DEBUG,"Result set join gave:");
					resset_print(setrs);
				}
			}
		}
		comp->all = old;
		comp->nobe = 0;
        for (rs = setrs; rs; rs = rs->next) {
			if( bcond_check(comp->head, rs->ri, comp->blob, comp->cr)
			    == SPOCP_SUCCESS) {
				res = rss_add( res, rs->ri, comp);
			}
		}
        resset_free(setrs);
		break;

	case SPOC_PREFIX:
		if ((bp = ARRFIND(db, SPOC_PREFIX)) != 0) {
			avp = prefix2prefix_match(ep->e.atom, bp->val.prefix);

			/*
			 * got a set of plausible branches and more elements
			 */
			if (avp) 
				res = resset_extend(res, do_arr(avp, ep->next, comp));
		}
		break;

	case SPOC_SUFFIX:
		if ((bp = ARRFIND(db, SPOC_SUFFIX)) != 0) {
			avp = suffix2suffix_match(ep->e.atom, bp->val.suffix);

			/*
			 * got a set of plausible branches and more elements
			 */
			if (avp) 
				res = resset_extend(res, do_arr(avp, ep->next, comp));
		}
		break;

	case SPOC_RANGE:
		if ((bp = ARRFIND(db, SPOC_RANGE)) != 0) {
			slp = bp->val.range;

			i = ep->e.range->lower.type & 0x07;

			if (slp[i])	/* no use testing otherwise */
				avp = bsl_match_range(slp[i], ep->e.range);

			/*
			 * got a set of plausible branches 
			 */
			if (avp) 
				res = resset_extend(res, do_arr(avp, ep->next, comp));
		}
		break;

	case SPOC_LIST:
		DEBUG(SPOCP_DMATCH) traceLog(LOG_DEBUG,"Checking LIST");
		if ((bp = ARRFIND(db, SPOC_LIST)) != 0) {
			rs = element_match_r(bp->val.list, ep->e.list->head, comp);
			res = resset_extend(res, rs);
		}
		break;

	case SPOC_ENDOFLIST:
		bp = ARRFIND(db, SPOC_ENDOFLIST);
		if (bp)
			res = resset_extend( res, next(bp->val.list, ep, comp));
		else
			return 0;
		break;		/* never reached */
	}

#ifdef AVLUS
	traceLog( LOG_INFO, "element_match_r returns %p", res);
#endif
	return res;
}

/****************************************************************************/

void
junc_print(int lev, junc_t * jp)
{
    ruleinst_t  *ri;
	char        indent[32];
	int         i;
    
	for ( i = 0 ; i < lev ; i++)
		indent[i] = ' ';
	indent[i] = '\0' ;
    
	traceLog(LOG_DEBUG,"|---------------------------->");
	if (jp->branch[SPOC_ATOM])
		traceLog(LOG_DEBUG,"%sATOM", indent);
	if (jp->branch[SPOC_LIST])
		traceLog(LOG_DEBUG,"%sLIST", indent);
	if (jp->branch[SPOC_SET])
		traceLog(LOG_DEBUG,"%sSET", indent);
	if (jp->branch[SPOC_PREFIX])
		traceLog(LOG_DEBUG,"%sPREFIX", indent);
	if (jp->branch[SPOC_SUFFIX])
		traceLog(LOG_DEBUG,"%sSUFFIX", indent);
	if (jp->branch[SPOC_RANGE])
		traceLog(LOG_DEBUG,"%sRANGE", indent);
	if (jp->branch[SPOC_ENDOFLIST])
		traceLog(LOG_DEBUG,"%sENDOFLIST", indent);
	if (jp->branch[SPOC_ENDOFRULE]) {
		traceLog(LOG_DEBUG,"%sENDOFRULE", indent);
		ri = jp->branch[7]->val.ri;
        traceLog(LOG_DEBUG,"%s Rule: %d", indent, ri->uid);
	}
	if (jp->branch[SPOC_ANY])
		traceLog(LOG_DEBUG,"%sANY", indent);
	traceLog(LOG_DEBUG,">----------------------------|");
}

