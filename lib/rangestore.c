/*
 *  rangestore.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/16/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 * A rangestore uses a skiplist with boundaries as values.
 */

#include <config.h>

#include <basic.h>
#include <octet.h>
#include <verify.h>
#include <range.h>
#include <skiplist.h>
#include <branch.h>
#include <boundarysl.h>

varr_t *_left(slist_t *slp, slnode_t *pivot, slnode_t **pos);
varr_t *_right(slist_t *slp, slnode_t *sln, varr_t *right);
int _side(slnode_t *pivot, boundary_t *bp, spocp_result_t *rc);

/*!
 * \brief Stores a range in a skip-list
 * \param slp A pointer to the skip-list
 * \param rp A pointer to the range specification
 * \return A pointer to the junc_t structure that binds the two 
 *  values stored in the skip-list together.
 */
item_t bsl_add_range(slist_t *slp, range_t *rp)
{
    slnode_t    *low, *high;
    junc_t      *jp;

    if ((rp->lower.type & 0xF0) == 0) {	/* no value */
		low = slp->head;
		low->refc++;
	} else
		low = bsl_insert(slp, &rp->lower);
    

    if ((rp->upper.type & 0xF0) == 0) {
		high = slp->tail;
		high->refc++;
	} else
		high = bsl_insert(slp, &rp->upper);
    
    if ((jp = varr_junc_common(low->varr, high->varr)) == 0) {
		jp = junc_new();
        jp->refc = 1;
		low->varr = varr_junc_add(low->varr, jp);
		high->varr = varr_junc_add(high->varr, jp);
	}
    
	return (item_t) jp;    
}

/*!
 * \brief A compare functions that matches a single value against a 
 *  boundary specification
 * \param ap A pointer to an atom
 * \param bp A pointer to a boundary specification
 * \param rc A pointer to a spocp result item, this is where errors
 *  are reported. If everything went OK this must be set to SPOCP_SUCCESS.
 *  If something went wrong it contains an error code
 * \return <0, 0 or >0 depending on whether the arom is regarded to be 
 *  smaller equal och larger than the boundary value. The type of the 
 *  boundary then makes it clear whether the atom falls within or outside
 *  the boundary.
 */

int atom2boundary_cmp(atom_t *ap, boundary_t *bp, spocp_result_t *rc)
{
    int             v = 0;
    octet_t         val;
    long int        num;
    struct in_addr  v4;

#ifdef USE_IPV6
    struct in6_addr v6;
#endif
    
    *rc = SPOCP_SUCCESS;
    
    if (bp == NULL)
        return -1;
    
    switch (bp->type & RTYPE) {
        case SPOC_ALPHA:
            v = octcmp(&ap->val, &bp->v.val);
            break;

        case SPOC_DATE:
            if ((*rc = is_date(&ap->val)) == SPOCP_SUCCESS) {
                to_gmt(&ap->val, &val);
                v = octcmp(&val, &bp->v.val);
            }
            break;
            
        case SPOC_TIME:
            if ((*rc = is_time(&ap->val)) == SPOCP_SUCCESS) {
                hms2int(&ap->val, &num);
                v = num - bp->v.num;
            }
            break;
            
        case SPOC_NUMERIC:
            if ((*rc = is_numeric(&ap->val, &num)) == SPOCP_SUCCESS) 
                v = num - bp->v.num;
            break;
            
        case SPOC_IPV4:
            if ((*rc = is_ipv4(&ap->val, &v4)) == SPOCP_SUCCESS)
                v = ipv4cmp(&v4, &bp->v.v4);
            break;
            
#ifdef USE_IPV6
        case SPOC_IPV6:	/* Can be viewed as a array of unsigned ints */
            if ((*rc = is_ipv6(&ap->val, &v6)) == SPOCP_SUCCESS)
                v = ipv6cmp(&v6, &bp->v.v6);
            break;
#endif
    }
    
    return v;
}

static int _abc(item_t a, item_t b, spocp_result_t *rc)
{
    return atom2boundary_cmp((atom_t *)a, (boundary_t *)b, rc);
}

varr_t *_left(slist_t *slp, slnode_t *pivot, slnode_t **pos)
{
    varr_t      *left=NULL;
    slnode_t    *sln;
    int         typ;

    /* Now, check all nodes before this to see if they are the
     left hand side of a range */
    
    left = varr_dup(slp->head->varr, 0);
    
    if (pivot != slp->head) {
        for (sln = slp->head->next[0]; sln != pivot ; sln = sln->next[0]) {
            typ = ((boundary_t *) sln->value)->type & BTYPE;
            if ((typ == (GT|EQ)) || (typ == GT))
                left = varr_or(left, sln->varr, 1);
        }
        if (sln != slp->tail) /* step past the pivot node */
            sln = sln->next[0];
    }
    else {
        sln = slp->head->next[0];
    }
    
    *pos = sln;
    
    return left;
}

varr_t *_right(slist_t *slp, slnode_t *sln, varr_t *right)
{
    int         typ;

    for (; sln != slp->tail ; sln = sln->next[0]) {
        typ = ((boundary_t *) sln->value)->type & BTYPE;
        if ((typ == (LT|EQ)) || (typ == LT))
            right = varr_or(right, sln->varr, 1);
    }
    
    right = varr_or(right, sln->varr, 1);
    
    return right;
}

/*
varr_t         *
atom2range_match(atom_t * ap, slist_t * slp, int vtype, spocp_result_t * rc)
*/
/*
 * \brief Will find all ranges in which this atom falls
 * \param slp The skiplist that stores all the ranges
 * \param ap The atom.
 * \param rc Pointer to a spocp result, this is where errocodes will be
 *  placed if something went wrong.
 * \return Array of pointers to junctions
 */
varr_t *
bsl_match_atom(slist_t *slp, atom_t *ap, spocp_result_t * rc)
{
    slnode_t        *node, *sln = NULL;
    varr_t          *left=NULL, *right=NULL;
    int             typ, c;
    
    /* find the starting point */
    node = sl_find_node(slp, (item_t) ap, _abc);
    
    /* Now, check all nodes before this to see if they are the
     left hand side of a range */
    
    left = _left(slp, node, &sln);
    
    /* Now the pivot point, the value of the node is '<=' ap */
    
    if (node->value) {
        typ = ((boundary_t *) node->value)->type & BTYPE;
        if ((c = _abc(ap, node->value, rc)) == 0) { /* == */
            if (typ == (GT|EQ)) {
                left = varr_or(left, node->varr, 1);
            }
            if (typ == (LT|EQ)) {
                right = varr_or(right, node->varr, 1);
            }
        } else { /* < */
            if (typ == GT) {
                left = varr_or(left, node->varr, 1);
            }
        }
    }
    
    /* Now, check all nodes on the right hand side to see if they are the
     right hand side of a range */

    right = _right(slp, sln, right);
    
    return varr_and(left, right);
}

#define RS_UNDEFINED 0
#define RS_LEFT_SIDE 1
#define RS_RIGHT_SIDE 2

/*!
 * \brief Returns on which side the pivot value is in reference to the 
 *  test value. A value pointer of 0 is larger than anything else.
 */

int _side(slnode_t *pivot, boundary_t *bp, spocp_result_t *rc)
{    
    if (pivot->value == 0) 
        return 1;
    
    return boundarycmp((boundary_t *)pivot->value, bp, rc);
}

varr_t *bsl_match_range(slist_t *slp, range_t *rp)
{
    slnode_t        *pivot[2], *sln;
    varr_t          *left=NULL, *right=NULL;
    spocp_result_t  rc;
    
    /* find the starting points */
    if (rp->lower.type & 0xF0) {
        pivot[0] = sl_find_node(slp, (item_t) &rp->lower, 0);
        /* Now, check all nodes before pivot.1 to see if they are the
         left hand side of a range */
        
        left = _left(slp, pivot[0], &sln);

        if (_side(pivot[0], &rp->lower, &rc) <= 0) {
            left = varr_or(left, pivot[0]->varr, 1);
        }
    }
    else {
        left = varr_dup(slp->head->varr, 0);
    }
    if (rp->upper.type & 0xF0) {
        pivot[1] = sl_find_node(slp, (item_t) &rp->upper, 0);
    
        if (_side(pivot[1], &rp->upper, &rc) >= 0) {
            right = varr_or(right, pivot[1]->varr, 1);
        }

        right = _right(slp, pivot[1]->next[0], right);
    }
    else {
        right = varr_dup(slp->tail->varr, 0);
    }
    
    return varr_and(left, right);
}

varr_t *bsl_match_ranges(slist_t **slp, range_t *rp)
{
    int typ = rp->lower.type & BTYPE;

    if (slp[typ] != NULL)
        return bsl_match_range(slp[typ], rp);
    else
        return NULL;
}

/* ---------------------------------------------------------------------- */

varr_t         *
get_all_range_followers(branch_t * bp, varr_t * va)
{
	slnode_t    *node;
	slist_t     **slp = bp->val.range;
	int         i;
    
	for (i = 0; i < DATATYPES; i++) {
		if (slp[i] == 0)
			continue;
        
		for( node = slp[i]->head; node; node = node->next[0])
			va = varr_or(va, node->varr, 1);
	}
    
	return va;
}

/*!
 * \brief Find all ranges that are '<=' the given range
 * \param slp Pointer to a skiplist structure
 * \param rp Pointer to the range
 * \return Array of pointers to ruleinst structs
 */
varr_t *
bsl_inv_match_range(slist_t *slp, range_t *rp)
{
    slnode_t        *pivot[2], *sln;
    varr_t          *res=NULL, *tmp=NULL;
    item_t          *pp;
    spocp_result_t  rc;
    
    if (rp->lower.type & BTYPE) {
        pivot[0] = sl_find_node(slp, (item_t) &rp->lower, 0);
        if (pivot[0] == slp->head || _side(pivot[0], &rp->lower, &rc) < 0) { 
                pivot[0] = pivot[0]->next[0];
        }
    }
    else {
        pivot[0] = slp->head;
    }

    if (rp->upper.type & BTYPE) {
        pivot[1] = sl_find_node(slp, (item_t) &rp->upper, 0);
        if (pivot[1] == slp->tail || _side(pivot[1], &rp->upper, &rc) > 0) {
            for(sln=slp->head; sln->next[0] != pivot[1]; sln = sln->next[0]); 
            pivot[1] = sln;
        }
    }
    else {
        pivot[1] = slp->tail;
    }
        
    for( sln = pivot[0]; sln != pivot[1]->next[0]; sln = sln->next[0])
        tmp = varr_or(tmp, sln->varr, 1);
    
    /* Now find the pointers that there are two of */
    for (pp = varr_pop(tmp); pp; pp = varr_pop(tmp)){
        if (varr_find(tmp, pp) >= 0) {
            res = varr_add(res, pp);
            varr_rm(tmp, pp);
        }
    }
    
	return res;
}

varr_t *bsl_inv_match_ranges(slist_t **slp, range_t *rp)
{
    int typ = rp->lower.type & BTYPE;
    
    if (slp[typ] != NULL)
        return bsl_inv_match_range(slp[typ], rp);
    else
        return NULL;
}

junc_t  *bsl_rm_range(slist_t *slp, range_t *rp, spocp_result_t *sr)
{
    slnode_t    *right, *left;
    varr_t      *res;
    int         c;
    junc_t      *jp;
    
    *sr = SPOCP_SUCCESS;
    /* find the starting points */
    if (rp->lower.type & BTYPE) {
        left = sl_find_node(slp, (item_t) &rp->lower, 0);
        /* make sure it's exactly the same value */
        c = boundarycmp(&rp->lower, (boundary_t *)left->value, sr);
        if (c != 0) {
            *sr = SPOCP_CAN_NOT_PERFORM;
            return NULL;
        }
    }
    else {
        left = slp->head;
    }
    if (rp->upper.type & BTYPE) {
        right = sl_find_node(slp, (item_t) &rp->upper, 0);
        /* make sure it's exactly the same value */
        c = boundarycmp(&rp->upper, (boundary_t *)right->value, sr);
        if (c != 0) {
            *sr = SPOCP_CAN_NOT_PERFORM;
            return NULL;
        }
    }
    else {
        right = slp->tail;
    }

    /* any common junction ? */    
    res = varr_and(left->varr, right->varr);
    
    if (res != 0) {
        jp = res->arr[0];
        /* remove the nodes if refcount has reached 0 */
        if (left != slp->head)
            snode_remove(slp, left);
        if (right != slp->tail)
            snode_remove(slp, right);
        varr_free(res);
        return jp;
    }
    else {
        *sr = SPOCP_CAN_NOT_PERFORM;
        return NULL;
    }
}

