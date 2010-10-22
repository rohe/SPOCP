/*
 *  junc.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 12/9/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

#include <varr.h>
#include <octet.h>
#include <element.h>
#include <ruleinst.h>
#include <ruleinfo.h>
#include <branch.h>
#include <hash.h>
#include <ssn.h>

#include <wrappers.h>
#include <log.h>

/*#define AVLUS 1 */

/************************************************************
 *							   *
 ************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t	 *
junc_new()
{
	junc_t	 *jp;
    
	jp = (junc_t *) Calloc(1, sizeof(junc_t));
    
	return jp;
}

/* -------------------------------------------------------------------- */

varr_t         *
varr_junc_add(varr_t * va, junc_t * ju)
{
	return varr_add(va, (void *) ju);
}

junc_t         *
varr_junc_pop(varr_t * va)
{
	return (junc_t *) varr_pop(va);
}

junc_t         *
varr_junc_nth(varr_t * va, int n)
{
	return (junc_t *) varr_nth(va, n);
}

junc_t         *
varr_junc_common(varr_t * a, varr_t * b)
{
	return (junc_t *) varr_first_common(a, b);
}

junc_t         *
varr_junc_rm(varr_t * a, junc_t * jp)
{
	return (junc_t *) varr_rm(a, (void *) jp);
}

/************************************************************
 *							   *
 ************************************************************/
/*
 * Adds a branch to a junction
 *
 * Arguments:
 * 
 * Returns: 
 */

junc_t	 *
add_branch(junc_t * jp, branch_t * bp)
{
    /*printf("[add_branch]\n"); */
	if (jp == 0)
		jp = junc_new();
    
	jp->branch[bp->type] = bp;
	bp->parent = jp;
    
	return jp;
}


/************************************************************
 *						   &P_match_uid	*
 ************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t	 *
junc_dup(junc_t * jp, ruleinfo_t * ri)
{
	junc_t      *new;
	branch_t    *nbp, *obp;
	int         i, j;
    
	if (jp == 0)
		return 0;
    
	new = junc_new();
    
	for (i = 0; i < NTYPES; i++) {
		if (jp->branch[i]) {
			nbp = new->branch[i] = branch_new(0);
			obp = (branch_t *) jp->branch[i];
            
			nbp->type = obp->type;
			nbp->count = obp->count;
			nbp->parent = new;
            
			switch (i) {
                case SPOC_ATOM:
                    nbp->val.atom = phash_dup(obp->val.atom, ri);
                    break;
                    
                case SPOC_LIST:
                    nbp->val.list = junc_dup(obp->val.list, ri);
                    break;
                    
                case SPOC_PREFIX:
                    nbp->val.prefix = ssn_dup(obp->val.prefix, ri);
                    break;
                    
                case SPOC_SUFFIX:
                    nbp->val.suffix = ssn_dup(obp->val.suffix, ri);
                    break;
                    
                case SPOC_RANGE:
                    for (j = 0; j < DATATYPES; j++) {
                        nbp->val.range[j] =
					    sl_dup(obp->val.range[j]);
                    }
                    break;
                    
                case SPOC_ENDOFLIST:
                    nbp->val.list = junc_dup(obp->val.list, ri);
                    break;
                    
                case SPOC_ENDOFRULE:
                    nbp->val.ri = obp->val.ri;
                    break;
                    
                    /*
                     * case SPOC_REPEAT : break ; 
                     */
                    
			}
		}
	}
    
	return new;
}

/************************************************************
 *							   *
 ************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

char	   *
set_item_list(junc_t * jp)
{
    char    *arr;
	int	    i;
    
    arr = Calloc(NTYPES+1, sizeof(char));
    
	for (i = 0; i < NTYPES; i++) {
		if (jp && jp->branch[i])
			arr[i] = '1';
		else
			arr[i] = '0';
	}
    
	arr[NTYPES] = '\0';
    
	return arr;
}

/************************************************************
 *							   *
 ************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t	 *
list_end(junc_t * arr)
{
	branch_t       *db;
    
	db = ARRFIND(arr, SPOC_ENDOFLIST);
    
	if (db == 0) {
		db = branch_new(SPOC_ENDOFLIST);
		db->val.list = junc_new();
		add_branch(arr, db);
	} else {
		db->count++;
	}
    
	DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG,"List end [%d]", db->count);
    
	arr = db->val.list;
    
	return arr;
}


/************************************************************
 *	   Adds end-of-list marker to the tree	     *
 ************************************************************/
/*
 * 
 * Arguments: ap node in the tree ep end of list element id ID for this rule
 * 
 * Returns: pointer to next node in the tree
 * 
 * Should mark in some way that end of rule has been reached 
 */

static junc_t  *
list_close(junc_t * ap, element_t **epp, ruleinst_t * ri, int *eor)
{
	element_t      *parent, *ep = *epp;
    
	do {
		parent = ep->memberof;
        
		if (parent->type == SPOC_LIST) 
			ap = list_end(ap);
        
		DEBUG(SPOCP_DSTORE) {
			if (parent->type == SPOC_LIST) {
				oct_print(LOG_DEBUG,
                          "end_list that started with",
                          &parent->e.list->head->e.atom->val);
			}
			else if(parent->type == SPOC_SET) 
				traceLog(LOG_DEBUG,"Set end");
			else
				traceLog(LOG_DEBUG,"end of type=%d", parent->type); 
		}
        
		ep = parent;
        
#ifdef AVLUS
		DEBUG(SPOCP_DSTORE)
        traceLog(LOG_DEBUG,"type:%d next:%p memberof:%p",
                 ep->type, ep->next, ep->memberof);
#endif
        
	} while ((ep->type == SPOC_LIST || ep->type == SPOC_SET)
             && ep->next == 0 && ep->memberof != 0);
    
	*epp = parent;
	if (parent->memberof == 0) {
		ap = rule_end(ap, ri);
		*eor = 1;
	} else
		*eor = 0;
    
	return ap;
}

static junc_t  *
add_next(plugin_t * plugin, junc_t * jp, element_t * ep, ruleinst_t * ri)
{
	int	     eor = 0;
    
#ifdef AVLUS
	{
		octet_t *eop ;
        
		eop = oct_new( 512,NULL);
		element_print( eop, ep );
		oct_print( LOG_INFO, "add_next() after", eop);
		oct_free( eop);
	}
#endif
    
	if (ep->next) {
		DEBUG(SPOCP_DSTORE)
        traceLog( LOG_DEBUG, "next exists");
		jp = add_element(plugin, jp, ep->next, ri, 1);
	}
	else if (ep->memberof) {
		DEBUG(SPOCP_DSTORE)
        traceLog( LOG_DEBUG, "No next exists");
		/*
		 * if( ep->memberof->type == SPOC_SET ) ; 
		 */
		
		if (ep->type != SPOC_LIST)	/* a list never closes itself */
			jp = list_close(jp, &ep, ri, &eor);
        
		/* ep is now one of the parents of the original ep */
		if ( !eor && ep->next) {
			/*traceLog( LOG_DEBUG, "next exists");*/
			jp = add_element(plugin, jp, ep->next, ri, 1);
		}
	}
    
	return jp;
}


/************************************************************
 *	   Add a s-expression element		      *
 ************************************************************/
/*
 * The representation of the S-expression representing rules are as a tree. At
 * every node in the tree there are a couple of different types of branches
 * that can appear. This routine chooses which type it should be and then
 * adds this element to that branch.
 * 
 * Arguments: ap node in the tree ep element to store rt pointer to the
 * ruleinstance
 * 
 * Returns: pointer to the next node in the tree 
 */


junc_t	 *
add_element(plugin_t * pl, junc_t * jp, element_t * ep, ruleinst_t * rt,
            int next)
{
	branch_t	*bp = 0;
	junc_t		*ap = 0;
    
	bp = ARRFIND(jp, ep->type);
    /* printf("[add_element] type branch: %p (%d)\n", bp, ep->type); */
    
#ifdef AVLUS
	{
		octet_t *eop ;
        
		eop = oct_new( 512,NULL);
		element_print( eop, ep );
		oct_print( LOG_INFO, "add_element", eop);
		oct_free( eop);
	}
#endif
    
	if (bp == 0) {
		bp = branch_new(ep->type);
		ap = add_branch(jp, bp);
	} else {
		bp->count++;
	}
    
    /* printf("[add_element] starting add_* (ep->type:%d)\n", ep->type); */
	if (ep->type == SPOC_SET){
        /* printf("[add_element] starting add_set\n"); */
		ap = add_set(bp, ep, pl, jp, rt);
	}
	else {
		switch (ep->type) {
            case SPOC_ATOM:
                ap = add_atom(bp, ep->e.atom);
                break;
                
            case SPOC_PREFIX:
                ap = add_prefix(bp, ep->e.atom);
                break;
                
            case SPOC_SUFFIX:
                ap = add_suffix(bp, ep->e.atom);
                break;
                
            case SPOC_RANGE:
                ap = add_range(bp, ep->e.range);
                break;
                
            case SPOC_LIST:
                ap = add_list(pl, bp, ep->e.list, rt);
                break;
                
            case SPOC_ANY:
                ap = add_any(bp);
                break;
		}
        
		if (ap && next)
			ap = add_next(pl, ap, ep, rt);
		
	}
    /* printf("[add_element] add_[%d] done\n", ep->type); */
    
	if (ap == 0 && bp != 0) {
		bp->count--;
        
		if (bp->count == 0) {
			DEBUG(SPOCP_DSTORE)
            traceLog( LOG_DEBUG, "Freeing type %d branch",
				     bp->type);
			branch_free(bp);
			jp->branch[ep->type] = 0;
		}
	}
    
    /* printf("[add_element] return: %p\n", ap); */
	return ap;
}

void
junc_free(junc_t * juncp)
{
	int	i;
    
	if (juncp) {        
        if (juncp->refc == 0) {
            for (i = 0; i < NTYPES; i++) {
                if (juncp->branch[i]) {
                    branch_free(juncp->branch[i]);
                }
            }
            Free(juncp);
        }
        juncp->refc--;
	}
}
