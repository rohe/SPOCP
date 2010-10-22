
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

#include <wrappers.h>
#include <spocp.h>
#include <macros.h>
#include <log.h>

#include <octet.h>
#include <ruleinst.h>
#include <element.h>
#include <skiplist.h>
#include <ssn.h>
#include <db0.h>
#include <branch.h>
#include <hash.h>
#include <rm.h>

int	atom_rm(branch_t * bp, element_t * ep, ruleinst_t * rt);
int	range_rm(branch_t * bp, element_t * ep, ruleinst_t * rt);
int	prefix_rm(branch_t * bp, element_t * ep, ruleinst_t * rt);
int	suffix_rm(branch_t * bp, element_t * ep, ruleinst_t * rt);
int	list_rm(branch_t * bp, element_t * ep, ruleinst_t * rt);
int	element_rm(junc_t * ap, element_t * ep, ruleinst_t * rt);
int	set_rm(branch_t * bp, element_t * ep, ruleinst_t * rt);

int	rm_endoflist(junc_t * jp, element_t * ep, ruleinst_t * rt);
int	rm_endofrule(junc_t * jp, element_t * ep, ruleinst_t * rt);

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
		if (jp->branch[i])
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

static int _gone(int r, junc_t *jp)
{
    if (r == 0) {	/* one branch gone from the junction */
        
        if (junction_index(jp) == 0) {
            DEBUG(SPOCP_DSTORE)
                traceLog(LOG_DEBUG, "Junction without any branches");
            junc_free(jp);
        }
    }
    
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
		return -1;

	bucket = phash_search(bp->val.atom, ap);
	if (bucket == 0)
		return -1;

	DEBUG(SPOCP_DSTORE) {
		char           *tmp;
		tmp = oct2strdup(&ap->val, '\\');
		traceLog(LOG_DEBUG,"atom_rm: bucket [%s] [%d]", tmp, bucket->refc);
		Free(tmp);
	}

	jp = bucket->next;

	if ((r = _gone(rm_next(jp, elemp, rt), jp)) == 0) {
		bucket->next = 0;
    }

    if (r == 0 && bucket->refc == 1) {
		DEBUG(SPOCP_DSTORE)
        traceLog(LOG_DEBUG, "bucket reference down to zero");
        
		bucket_rm(bp->val.atom, bucket);
        
		/*
		 * last in hashtable ? 
		 */
		if (phash_empty(bp->val.atom)) {
			phash_free(bp->val.atom);
			bp->val.atom = 0;

			return 0;
		}
	}
    
    if (r >= 0)
        bucket->refc--;

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
range_rm(branch_t * bp, element_t * ep, ruleinst_t * rt)
{
	junc_t          *jp;
	int             value_type = ep->e.range->lower.type & RTYPE;
    int             r;
    spocp_result_t  rc;

	jp = bsl_rm_range(bp->val.range[value_type], ep->e.range, &rc);

	if (rc != SPOCP_SUCCESS)
        return -1;

    if ((r =_gone(rm_next(jp, ep, rt), jp)) == 0 )
        bp->val.range[value_type] = 0;
    
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
ssn_rm(branch_t * bp, ssn_t * ssn, char *sp, int direction, element_t * ep,
       ruleinst_t * rt)
{
	junc_t  *jp;
	int     r;

	jp = ssn_delete(&ssn, sp, direction);

    if (jp == 0)
        return -1;
    
	if (ssn == 0) {
		branch_free(bp);
		return 0;
	}

    r = _gone(rm_next(jp, ep, rt), jp);
    
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
	branch_t    *bp;
	junc_t      *rjp;
    int         r;

	bp = ARRFIND(jp, SPOC_ENDOFLIST);

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"EOL Branch [%d]", bp->count);

	rjp = bp->val.list;

    if ((r = _gone(rm_next(rjp, ep, rt), rjp)) == 0) {
        bp->val.list = 0;
    }

    if (r < 0)
        return -1;
    
	if (bp->count == 1) {
		DEBUG(SPOCP_DSTORE)
        traceLog(LOG_DEBUG,"Get rid of the rest of this branch");
        
		branch_free(bp);
        
		DEBUG(SPOCP_DSTORE)
        traceLog(LOG_DEBUG,"No type %d branch at this junction any more",
			     SPOC_ENDOFLIST);
        
		jp->branch[SPOC_ENDOFLIST] = 0;
        
		return 0;
	}
    else {
        bp->count--;
        return r;
    }
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
rm_endofrule(junc_t * jp, element_t * ep, ruleinst_t * rt)
{
	branch_t        *bp;

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"rm end_of_rule");
	bp = ARRFIND(jp, SPOC_ENDOFRULE);

	bp->count--;

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"Branch count=%d", bp->count);
    branch_free(bp);

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
	list_t         *lp = ep->e.list;
	junc_t         *njp = bp->val.list;

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"rm list");

	return element_rm(njp, lp->head, rt);
}


/************************************************************
*                                                           *
************************************************************/

int
set_rm(branch_t * bp, element_t * ep, ruleinst_t * rt)
{
	dset_t	*ds,*ts;

	if (bp->val.set == 0) 
		return 1;

	for( ds = bp->val.set; ds; ds=ds->next)
		if (strcmp(ds->uid, rt->uid) == 0) 
			break;

	if (ds) {
		varr_free(ds->va);
		if (ds == bp->val.set)
			bp->val.set = ds->next;
		else {
			for( ts = bp->val.set; ts; ts=ts->next)
				if (ts->next == ds)
					break;
			ts->next = ds->next;
		}

		Free(ds);
	}

	return 1;
}

/***********************************************************
*              PUBLIC INTERFACES                           *
************************************************************/

int
element_rm(junc_t * jp, element_t * ep, ruleinst_t * rt)
{
	branch_t       *bp;
	int             r = 0;

	bp = ARRFIND(jp, ep->type);

	if (bp == 0) {		/* Ooops, how did that happen, can't delete
                         * something that isn't there */
		DEBUG(SPOCP_DSTORE)
		    traceLog(LOG_DEBUG,"missing branch where there should be one");
		return -1;
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
            r = range_rm(bp, ep, rt);
            break;

        case SPOC_LIST:
            r = list_rm(bp, ep, rt);
            break;

        case SPOC_SET:
            break;

	}

    if (r < 0)
        return r;
    
    DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"Branch: [%d]", bp->count);
    

	if (r == 0 && bp->count == 1) {
		/*
		 * that branch is gone 
		 */
		DEBUG(SPOCP_DSTORE)
		    traceLog(LOG_DEBUG,"rm element; no type %d branch anymore",
			     ep->type);

		jp->branch[ep->type] = 0;

        /*
		if (junction_index(jp) == 0)
			junc_free(jp);
        */
        return 0;
	}
    else {
        bp->count--;
        return r;
    }
}

spocp_result_t
rule_rm(junc_t * jp, octet_t * rule, ruleinst_t * rt)
{
	element_t      *ep = 0;
	int             r = 1;	/* default SUCCESS? well sort of */
	spocp_result_t  rc;
	octet_t         loc;

	DEBUG(SPOCP_DSTORE) {
		oct_print(LOG_DEBUG,"- rm rule", rule);
	}

	/*
	 * decouple 
	 */
	octln(&loc, rule);

	if ((rc = get_element(&loc, &ep)) != SPOCP_SUCCESS)
		return rc;

	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"---");

	if (ep) {
		r = element_rm(jp, ep, rt);
	}

	if (r >= 0)
		return SPOCP_SUCCESS;
	else
		return SPOCP_OPERATIONSERROR;
}

