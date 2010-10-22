/*
 *  branch.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/8/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */
#include <config.h>

#include <branch.h>
#include <atom.h>
#include <ruleinst.h>
#include <ruleinfo.h>
#include <hash.h>
#include <element.h>
#include <range.h>
#include <ssn.h>
#include <dset.h>
#include <boundarysl.h>

#include <wrappers.h>
#include <log.h>


/************************************************************ */

branch_t *branch_new(int type)
{
    branch_t    *bp;
    
    /*printf("[barnch_new]\n"); */
    bp = (branch_t *) Calloc(1, sizeof(branch_t));
    bp->type = type;
    bp->count = 1;
    
    return bp;
}

/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t   *
add_atom(branch_t * bp, atom_t * ap)
{
    buck_t   *bucket = 0;
    
    DEBUG(SPOCP_DSTORE) 
    oct_print(LOG_DEBUG,"atom_add", &ap->val);
    
    if (bp->val.atom == 0)
        bp->val.atom = phash_new(3, 50);
    
    bucket = phash_insert(bp->val.atom, ap);
    bucket->refc++;
    
    if (bucket->next == 0)
        bucket->next = junc_new();
    
    return bucket->next;
}


/************************************************************/

junc_t  *
add_any(branch_t * bp)
{
    if (bp->val.next == 0)
        bp->val.next = junc_new();
    
    return bp->val.next;
}

/************************************************************
 *                             *
 ************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t   *
add_list(plugin_t * pl, branch_t * bp, list_t * lp, ruleinst_t * ri)
{
    element_t      *elp;
    junc_t   *jp;
    
    if (bp->val.list == 0) {
        jp = bp->val.list = junc_new();
    } else
        jp = bp->val.list;
    
    elp = lp->head;
    
    /*
     * fails, means I should clean up 
     */
    if ((jp = add_element(pl, jp, elp, ri, 1)) == 0) {
        traceLog(LOG_WARNING,"List add failed");
        return 0;
    }
    
    return jp;
}

/************************************************************
 *                                                          *
 ************************************************************/

static void
list_clean(junc_t * jp, list_t * lp)
{
    element_t      *ep = lp->head, *next;
    buck_t   *buck;
    branch_t       *bp;
    
    for (; ep; ep = next) {
        next = ep->next;
        if (jp == 0 || (bp = ARRFIND(jp, ep->type)) == 0)
            return;
        
        switch (ep->type) {
            case SPOC_ATOM:
                buck =
                phash_search(bp->val.atom, ep->e.atom);
                jp = buck->next;
                buck->refc--;
                bp->count--;
                if (buck->refc == 0) {
                    buck->val.val = 0;
                    buck->hash = 0;
                    junc_free(buck->next);
                    buck->next = 0;
                    next = 0;
                }
                break;
                
            case SPOC_LIST:
                bp->count--;
                if (bp->count == 0) {
                    junc_free(bp->val.list);
                    bp->val.list = 0;
                } else
                    list_clean(bp->val.list, ep->e.list);
                break;
        }
    }
}


/************************************************************
 *                             *
 ************************************************************/
/*
 * 
 * Arguments:
 * 
 * Returns: 
 */

junc_t   *
add_range(branch_t * bp, range_t * rp)
{
    int      i = rp->lower.type & 0x07;
    junc_t   *jp;
    
    char *l, *u ;
    /* printf("[add_range] -boundaries-\n"); */
    u = boundary_prints( &rp->upper ) ; 
    /* printf("[add_range] upper: %s\n",u); */
    l = boundary_prints( &rp->lower ) ;
    /* printf("[add_range] lower: %s\n",l); */
    
    if (bp->val.range[i] == 0) {
        bp->val.range[i] = bsl_init(4, rp->upper.type & RTYPE);
    }

    jp = bsl_add_range(bp->val.range[i], rp);
    
    return jp;
}

/************************************************************
 *      adds a prefix to a prefix branch           *
 ************************************************************/
/*
 * 
 * Arguments: bp pointer to a branch ap pointer to a atom
 * 
 * Returns: pointer to next node in the tree 
 */

junc_t   *
add_prefix(branch_t * bp, atom_t * ap)
{
    /*
     * assumes that the string to add is '\0' terminated 
     */
    return ssn_insert(&bp->val.prefix, ap->val.val, FORWARD);
}


/************************************************************
 *      adds a suffix to a suffix branch           *
 ************************************************************/
/*
 * 
 * Arguments: bp pointer to a branch ap pointer to a atom
 * 
 * Returns: pointer to next node in the tree 
 */

junc_t  *
add_suffix(branch_t * bp, atom_t * ap)
{
    /*
     * assumes that the string to add is '\0' terminated 
     */
    return ssn_insert(&bp->val.suffix, ap->val.val, BACKWARD);
}

junc_t *
add_set( branch_t *bp, element_t *ep, plugin_t *pl, junc_t *jp, 
        ruleinst_t *rt)
{
    int         n,i;
    junc_t      *ap = 0/*, *rp=0*/;
    varr_t      *va, *dsva = NULL;
    element_t   *elem;
    dset_t      *ds;
    void        *v;
    
    /* printf("[add_set]\n"); */

    va = ep->e.set;
    n = varr_len(va);
    /* printf("[add_set] set size: %d\n", n); */
    
    for (v = varr_first(va), i=0; v; v = varr_next(va, v), i++) {
        DEBUG(SPOCP_DSTORE) traceLog(LOG_DEBUG, "- set element %d -", i);
        elem = (element_t *) v;
        if ((ap = add_element(pl, jp, elem, rt, 1)) == 0)
            break;
        
#ifdef AVLUS
        if (i==0) {
            junc_print_r( 2, jp);
        }
#endif
        /* printf("[add_set] varr_add"); */
        dsva = varr_add(dsva, ap);
    }
    
    ds = dset_new(rt->uid, dsva);
    bp->val.set = dset_append(bp->val.set, ds);
    
    return ap;
}

/************************************************************
 *     Adds end-of-rule marker to the tree       *
 ************************************************************/
/*
 * 
 * Arguments: ap node in the tree id ID for this rule
 * 
 * Returns: pointer to next node in the tree 
 */

junc_t  *
rule_end(junc_t * ap, ruleinst_t * ri)
{
    branch_t       *bp;
    
#ifdef AVLUS
    junc_print( 0, ap );
#endif
    
    if ((bp = ARRFIND(ap, SPOC_ENDOFRULE)) == 0) {
        DEBUG(SPOCP_DSTORE)
        traceLog(LOG_DEBUG,"New rule end");
        
        bp = branch_new(SPOC_ENDOFRULE);
        bp->val.ri = ri;
        
        ap = add_branch(ap, bp);
    } else {        /* If rules contains references this can
                     * happen, otherwise not */
        bp->count++;
        DEBUG(SPOCP_DSTORE)
        traceLog(LOG_DEBUG,"Old rule end: count [%d]",
                 bp->count);
        bp->val.ri = ri;
    }
    
    DEBUG(SPOCP_DSTORE)
    traceLog(LOG_DEBUG,"done rule %s", ri->uid);
    
    return ap;
}

void
branch_free(branch_t * bp)
{
    int i;
    
    /* printf("[branch_free] %p\n", bp); */
    if (bp) {
        /* printf("[branch_free] type %d\n", bp->type); */
        switch (bp->type) {
            case SPOC_ATOM:
                /* printf("phash_free\n"); */
                phash_free(bp->val.atom);
                break;
                
            case SPOC_LIST:
                /* printf("junc_free\n"); */
                junc_free(bp->val.list);
                break;
                
            case SPOC_SET:
                /* printf("set_free\n"); */
                dset_free(bp->val.set);
                break;
                
            case SPOC_PREFIX:
                /* printf("prefix_free\n"); */
                ssn_free(bp->val.prefix);
                break;

            case SPOC_SUFFIX:
                /* printf("suffix_free\n"); */
                ssn_free(bp->val.suffix);
                break;
                
            case SPOC_RANGE:
                /* printf("range_free\n"); */
                for (i = 0; i < DATATYPES; i++)
                    sl_free(bp->val.range[i]);
                /* printf("range_free - done\n"); */
                break;
                
        }
    }
}
