
/***************************************************************************
                          del.c  -  description
                             -------------------
    begin                : Sun Oct 19 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <string.h>

#include <macros.h>
#include <db0.h>
#include <struct.h>
#include <func.h>
#include <wrappers.h>
#include <spocp.h>

int             atom_rm(branch_t * bp, element_t * ep, ruleinst_t * rt);
int             range_rm(branch_t * bp, element_t * ep, ruleinst_t * rt);
int             prefix_rm(branch_t * bp, element_t * ep, ruleinst_t * rt);
int             suffix_rm(branch_t * bp, element_t * ep, ruleinst_t * rt);
int             list_rm(branch_t * bp, element_t * ep, ruleinst_t * rt);
int             element_rm(junc_t * ap, element_t * ep, ruleinst_t * rt);

int             rm_endoflist(junc_t * jp, element_t * ep, ruleinst_t * rt);
int             rm_endofrule(junc_t * jp, element_t * ep, ruleinst_t * rt);

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

static int
junction_index(junc_t * jp)
{
	int             i, r;

	for (i = 0, r = 0; i < NTYPES; i++)
		if (jp->item[i])
			r++;

	return r;
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

static int
rm_next(junc_t * jp, element_t * elemp, ruleinst_t * rt)
{
	int             r;

	if (elemp->next)
		r = element_rm(jp, elemp->next, rt);
	else if (elemp->next == 0 && elemp->memberof)
		r = rm_endoflist(jp, elemp->memberof, rt);
	else if (elemp->memberof == 0 && elemp->next == 0)
		r = rm_endofrule(jp, elemp, rt);

	return r;
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

int
atom_rm(branch_t * bp, element_t * elemp, ruleinst_t * rt)
{
	buck_t         *bucket = 0;
	junc_t         *jp;
	atom_t         *ap = elemp->e.atom;
	int             r;

	if (bp->val.atom == 0)
		return 0;

	bucket = phash_search(bp->val.atom, ap, ap->hash);
	/*
	 * bucket == 0 should be impossible, should I still check ? 
	 */
	if (bucket == 0)
		return 0;

	bucket->refc--;
	DEBUG(SPOCP_DSTORE) {
		char           *tmp;
		tmp = oct2strdup(&ap->val, '\\');
		traceLog(LOG_DEBUG,"atom_rm: bucket [%s] [%d]", tmp, bucket->refc);
		free(tmp);
	}

	jp = bucket->next;

	if (bucket->refc == 0) {
		bucket_rm(bp->val.atom, bucket);

		/*
		 * last in hashtable ? 
		 */
		if (phash_index(bp->val.atom) == 0) {
			phash_free(bp->val.atom);
			bp->val.atom = 0;
			DEBUG(SPOCP_DSTORE)
			    traceLog(LOG_DEBUG,"Get rid of the rest of this branch");
			branch_free(bp);
			return 0;
		} else {
			/*
			 * remove remaining references 
			 */
			junc_free(jp);
			return 1;
		}
	}

	r = rm_next(jp, elemp, rt);

	if (r == 0) {		/* one branch gone from the junction */

		if (junction_index(jp) == 0) {
			DEBUG(SPOCP_DSTORE)
			    traceLog(LOG_DEBUG,"Junction without any branches");
			junc_free(jp);
			bucket->next = 0;
		}
	} else if (r == -1)	/* the junctions is gone */
		bucket->next = 0;

	return 1;
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

int
range_rm(branch_t * bp, element_t * ep, ruleinst_t * rt)
{
	junc_t         *jp;
	int             rc, dtype = ep->e.range->lower.type & 0x07;

	jp = sl_range_rm(bp->val.range[dtype], ep->e.range, &rc);

	if (rc == 1) {
		junc_free(jp);
		branch_free(bp);
		return 0;
	}

	rc = rm_next(jp, ep, rt);

	if (rc == 0) {		/* one branch gone from the junction */

		/*
		 * could this really happen ?? 
		 */
		if (junction_index(jp) == 0) {
			DEBUG(SPOCP_DSTORE)
			    traceLog(LOG_DEBUG,"Junction without any branches");
			junc_free(jp);
			return -1;
		}
	}

	return 1;
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

static int
ssn_rm(branch_t * bp, ssn_t * ssn, char *sp, int direction, element_t * ep,
       ruleinst_t * rt)
{
	junc_t         *jp;
	int             r;

	jp = ssn_delete(&ssn, sp, direction);

	if (ssn == 0) {
		branch_free(bp);
		return 0;
	}

	if (jp) {
		r = rm_next(jp, ep, rt);

		if (r == 0) {	/* one branch gone from the junction */

			/*
			 * could this really happen ?? 
			 */
			if (junction_index(jp) == 0) {
				DEBUG(SPOCP_DSTORE)
					traceLog(LOG_DEBUG,
					    "Junction without any branches");
				junc_free(jp);
				return -1;
			}
		}
	}

	return 1;
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

int
prefix_rm(branch_t * bp, element_t * elemp, ruleinst_t * rt)
{
	atom_t         *ap = elemp->e.atom;

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"rm prefix \"%s\"", ap->val.val);

	return ssn_rm(bp, bp->val.prefix, ap->val.val, FORWARD, elemp, rt);
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

int
suffix_rm(branch_t * bp, element_t * elemp, ruleinst_t * rt)
{
	atom_t         *ap = elemp->e.atom;

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"rm suffix \"%s\"", ap->val.val);

	return ssn_rm(bp, bp->val.suffix, ap->val.val, BACKWARD, elemp, rt);
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

int
rm_endoflist(junc_t * jp, element_t * ep, ruleinst_t * rt)
{
	branch_t       *bp;
	junc_t         *rjp;
	int             r;

	bp = ARRFIND(jp, SPOC_ENDOFLIST);

	bp->count--;

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"EOL Branch [%d]", bp->count);

	rjp = bp->val.list;

	if (bp->count == 0) {
		DEBUG(SPOCP_DSTORE)
		    traceLog(LOG_DEBUG,"Get rid of the rest of this branch");

		branch_free(bp);

		DEBUG(SPOCP_DSTORE)
		    traceLog(LOG_DEBUG,"No type %d branch at this junction any more",
			     SPOC_ENDOFLIST);

		jp->item[SPOC_ENDOFLIST] = 0;

		return 0;
	}

	r = rm_next(rjp, ep, rt);

	if (r == 0) {		/* one branch gone from the junction */

		if (junction_index(rjp) == 0) {
			DEBUG(SPOCP_DSTORE)
			    traceLog(LOG_DEBUG,"Junction without any branches");
			junc_free(rjp);
			bp->val.list = 0;
		}
	}

	return 1;
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */
/*
 * void rm_index( index_t *ip, ruleinst_t *rt ) { int i, j ;
 * 
 * for( i = 0 ; i < ip->n ; i++ ) { if( ip->arr[i] == rt ) { if( i + 1 ==
 * ip->n ) * the last one * ip->arr[i] = 0 ; else { for( j = i+1 ; j < ip->n
 * ; i++, j++ ) { ip->arr[i] = ip->arr[j] ; } }
 * 
 * ip->n-- ; ip->arr[ip->n] = 0 ; } } } 
 */

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

int
rm_endofrule(junc_t * jp, element_t * ep, ruleinst_t * rt)
{
	branch_t       *bp;
	index_t        *inx;

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"rm end_of_rule");
	bp = ARRFIND(jp, SPOC_ENDOFRULE);

	bp->count--;

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"Branch count=%d", bp->count);
	inx = bp->val.id;

	index_rm(inx, rt);

	if (inx->n == 0) {
		branch_free(bp);
		return 0;
	}

	/*
	 * there is no next element 
	 */

	return 1;
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

int
list_rm(branch_t * bp, element_t * ep, ruleinst_t * rt)
{
	element_t      *elemp;
	list_t         *lp = ep->e.list;
	junc_t         *njp = bp->val.list;
	int             r;

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"rm list");

	elemp = lp->head;

	r = element_rm(njp, elemp, rt);

	/*
	 * DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG, "rm list; is there anything more?") ;
	 * 
	 * if( r == 0 ) { njp->item[ elemp->type ] = 0 ;
	 * 
	 * if( junction_index( njp ) == 0 ) { junc_free( njp ) ; bp->val.list
	 * = 0 ; } } 
	 */

	return 1;
}

/************************************************************
*                                                           *
************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

int
element_rm(junc_t * jp, element_t * ep, ruleinst_t * rt)
{
	branch_t       *bp;
	int             r = 0, n;

	bp = ARRFIND(jp, ep->type);

	if (bp == 0) {		/* Ooops, how did that happen, can't delete
				 * something that isn't there */
		DEBUG(SPOCP_DSTORE)
		    traceLog(LOG_DEBUG,"missing branch where there shold be one");
		return -2;
	}

	bp->count--;

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"Branch: [%d]", bp->count);

	if (bp->count == 0) {
		DEBUG(SPOCP_DSTORE)
		    traceLog(LOG_DEBUG,"element_rm; branch counter down to zero");
		branch_free(bp);

		/*
		 * have to do the junction handling here since the type is
		 * unknown further up 
		 */

		jp->item[ep->type] = 0;

		return 0;
	}

	/*
	 * type dependent part 
	 */
	switch (ep->type) {
	case SPOC_ATOM:
		r = atom_rm(bp, ep, rt);
		break;


	case SPOC_PREFIX:
		r = prefix_rm(bp, ep, rt);
		break;

	case SPOC_SUFFIX:
		r = suffix_rm(bp, ep, rt);
		break;

	case SPOC_RANGE:
		break;

	case SPOC_LIST:
		r = list_rm(bp, ep, rt);
		break;

	case SPOC_SET:
		break;

	}

	if (r == 0) {
		/*
		 * that branch is gone 
		 */
		DEBUG(SPOCP_DSTORE)
		    traceLog(LOG_DEBUG,"rm element; no type %d branch anymore",
			     ep->type);

		jp->item[ep->type] = 0;

		n = junction_index(jp);
		DEBUG(SPOCP_DSTORE)
			traceLog(LOG_DEBUG,"rm element; junction index %d", n);
		if (n == 0)
			junc_free(jp);
	}

	return 1;
}

spocp_result_t
rule_rm(junc_t * jp, octet_t * rule, ruleinst_t * rt)
{
	element_t      *ep = 0;
	int             r = 1;	/* default SUCCESS? well sort of */
	spocp_result_t  rc;
	octet_t         loc;

	DEBUG(SPOCP_DSTORE) {
		char           *tmp;
		tmp = oct2strdup(rule, '\\');
		traceLog(LOG_DEBUG,"- rm rule [%s]", tmp);
		free(tmp);
	}

	/*
	 * decouple 
	 */
	loc.val = rule->val;
	loc.len = rule->len;

	if ((rc = element_get(&loc, &ep)) != SPOCP_SUCCESS)
		return rc;

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"---");

	if (ep) {
		r = element_rm(jp, ep, rt);
	}

	if (r == 1)
		return SPOCP_SUCCESS;
	else
		return SPOCP_OPERATIONSERROR;
}
