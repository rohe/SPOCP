/*
 *  skiplist.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/15/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <skiplist.h>
#include <wrappers.h>
#include <result.h>
#include <basefn.h>
#include <result.h>

static slnode_t *_prev(slist_t *slp, slnode_t *node, int level);

/************************************************************************/

slnode_t       *
slnode_new(item_t *value, int n, slnode_t *tail, int width)
{
	slnode_t       *sln;
	int             i;
    
	sln = (slnode_t *) Calloc(1, sizeof(slnode_t));
    
	/*
	 * set the number of levels for this node 
	 */
	if (n) {
		sln->sz = n;
		sln->next = (slnode_t **) Calloc(n, sizeof(slnode_t *));
        if (tail) {
            for( i = 0; i < n; i++)
                sln->next[i] = tail;
        }

		sln->width = (int *) Calloc(n, sizeof(int));
        if (width){
            for (i = 0; i < n; i++) 
                sln->width[i] = width;
        }
	} else {
		sln->next = 0;
        sln->width = 0;
		sln->sz = 0;
	}
    
	sln->value = value;	/* the data */
	sln->refc = 0;      /* reference counter to know if I should still 
                         * keep it around. If a request to free it is
                         * issued and refc == 0 then it is removed */
	sln->varr = 0;		/* joins the boundaries of a range */
    
	return sln;
}

void free_slnode(slnode_t *node, ffunc *ff)
{
    if(node) {
        Free(node->next);
        Free(node->width);
        
        if (ff)
            ff(node->value);
        
        Free(node);
    }
}

slnode_t *
slnode_dup( slnode_t *old)
{    
    slnode_t    *new;
    int         i;
    
    new = slnode_new(NULL, old->sz, NULL, 0);
    
    for (i = 0; i < old->sz; i++) {
        new->width[i] = old->width[i];
    }
    
    new->refc = old->refc;
    
    return new;
}

/************************************************************************
 Initializing the list, starts of with only a head and a tail, representing
 the absolute minumum and maximum values in a range 
 ************************************************************************/

slist_t        *
slist_init(int max, int typ, cmpfn *cmp, ffunc *ff, dfunc *df)
{
	slist_t     *slp;
    
	slp = (slist_t *) Malloc(sizeof(slist_t));
    
	slp->maxlev = max;	/* max number of levels */
    
    slp->tail = slnode_new(0, max, 0, 0);
	slp->head = slnode_new(0, max, slp->tail, 0);

    slp->typ = typ;         /* type of values */
    slp->size = 0;          /* size of the list */
    slp->cmp = cmp;         /* The compare function */
    slp->ff = ff;           /* A free function for the values stored */
    slp->df = df;           /* A duplicate function for the values stored */
    
	return slp;
}


/*
 *
 */

int sl_len( slist_t *slp )
{
    return slp->size;
}

/* 
 * Lookup value by rank 
 */

item_t *sl_getitem( slist_t *slp, int i)
{
    slnode_t    *node = slp->head;
    int         level = node->sz;
    
    i += 1;
    while (level >= 0) {
        if ( i <= node->width[level]) {
            i -= node->width[level];
            node = node->next[level];
        }
        else {
            level -= 1;
        }
    }
    
    return (item_t *) node->value ;
}

/*
 * ---------------------------------------------------------------------------
 */

void free_result(sl_res_t *res)
{
    if (res) {
        Free(res->chain);
        Free(res->distance);
        Free(res);
    }
}

/*
 * ---------------------------------------------------------------------------
 * Return first node on each level such that node.next[levels].value >= value
 * ---------------------------------------------------------------------------
 */

sl_res_t *sl_find( slist_t *slp, item_t value, cmpfn *cmp)
{
    slnode_t        *node = slp->head;
    int             level = node->sz - 1;
    int             *steps_at_level; 
    int             s, i;
    spocp_result_t  rc;
    sl_res_t        *result;
    cmpfn           *localcmp;
    
    if (cmp) {
        localcmp = cmp;
    }
    else {
        localcmp = slp->cmp;
    }
    
    result = Calloc(1, sizeof(sl_res_t));
    result->chain = Calloc(node->sz, sizeof(slnode_t *));
    result->distance = Calloc(node->sz, sizeof(int));
    
    steps_at_level = Calloc(node->sz, sizeof(int));
    
    while (level >= 0) {
        if (node == slp->tail) { /* tail node */
            result->chain[level] = node;
            level -= 1;
        }
        else {
            if (localcmp(value, node->next[level]->value, &rc) >= 0) {
                if (rc != SPOCP_SUCCESS)
                    return NULL;
                steps_at_level[level] += node->width[level];
                node = node->next[level];
            }
            else {
                result->chain[level] = node;
                level -= 1;
            }
            
            if (rc != SPOCP_SUCCESS) {
                /* bail out */
                Free(result->chain);
                Free(result->distance);
                Free(result);
                Free(steps_at_level);
                return NULL;
            }
        }
    }
    
    s = 0;
    for (i=0; i < slp->head->sz; i++) {
        result->distance[i] = s;
        s += steps_at_level[i];
    }
    
    Free(steps_at_level);
    
    return result;
}

slnode_t *sl_find_node( slist_t *slp, item_t value, cmpfn *cmp)
{
    slnode_t        *node = slp->head;
    int             level = node->sz - 1;
    int             *steps_at_level; 
    spocp_result_t  rc;
    cmpfn           *localcmp;
    
    if (cmp) {
        localcmp = cmp;
    }
    else {
        localcmp = slp->cmp;
    }
    
    steps_at_level = Calloc(node->sz, sizeof(int));
    
    while (level >= 0) {
        if (node == slp->tail) { /* tail node */
            level -= 1;
        }
        else {
            if (localcmp(value, node->next[level]->value, &rc) >= 0) {
                if (rc != SPOCP_SUCCESS) {
                    return NULL;
                }
                node = node->next[level];
            }
            else {
                level -= 1;
            }
            
            if (rc != SPOCP_SUCCESS) {
                /* bail out */
                return NULL;
            }
        }
    }
        
    return node;
}

/* returns a value between 0 and max, the probablity for higher values
 * lower than for lower values.
 * The probability of getting the value N is 1/(2**N) 
 */

int height( int max )
{
    int randmax = RAND_MAX;
    int i, m;
    
    srand(1);
    
    m = randmax/2;
    for (i = 1; i < max-1; i++) {
        if (rand() > m) 
            break;
    }
    return i;
}

slnode_t *sl_insert(slist_t *slp, item_t value)
{
    sl_res_t        *res;
    int             d, i;
    spocp_result_t  rc = SPOCP_SUCCESS;
    slnode_t        *newnode;
    
    res = sl_find(slp, value, NULL);
    d = height(slp->head->sz);
    
    if (res->chain[0]->value && 
        slp->cmp(res->chain[0]->value, value, &rc) == 0) { 
        if (rc != SPOCP_SUCCESS) {
            free_result(res);
            return NULL;
        }
        /* not a new value */
        res->chain[0]->refc += 1;
        newnode = res->chain[0];
    }
    else {
        newnode = slnode_new(value, d, 0, 0);
        
        for (i = 0; i < d; i++) {
            newnode->next[i] = res->chain[i]->next[i];
            res->chain[i]->next[i] = newnode;
            newnode->width[i] = res->chain[i]->width[i] - res->distance[i];
            res->chain[i]->width[i] = res->distance[i] +1;
        }
        for (; i < slp->head->sz; i++) {
            res->chain[i]->width[i] += 1;
        }
        slp->size += 1;
    }
    
    if (rc != SPOCP_SUCCESS)
        return NULL;

    free_result(res);
    return newnode;
}

static slnode_t *_prev(slist_t *slp, slnode_t *node, int level)
{
    slnode_t *prev;
    
    for (prev = slp->head; prev->next[level] != node; 
         prev = prev->next[level]) ;

    return prev;
}

void snode_remove(slist_t *slp, slnode_t *snp)
{
    int         d, i;
    slnode_t    *prev;
    
    if (snp->refc) {
        snp->refc -= 1;
    }
    else {
        d = snp->sz;
        for (i = 0; i < d; i++) {
            prev = _prev(slp, snp, i);
            prev->width[i] += snp->width[i] -1;
            prev->next[i] = snp->next[i];
        }
        for (; i < slp->head->sz; i++) {
            slp->head->width[i] -= 1;
        }
        slp->size -= 1;
    }
}

void sl_remove(slist_t *slp, item_t value)
{
    sl_res_t        *res;
    spocp_result_t  rc = SPOCP_SUCCESS;
    
    res = sl_find(slp, value, NULL);
    if (res->chain[0] == slp->tail || 
        slp->cmp(value, res->chain[0]->value, &rc) == 0) {        
        if (rc == SPOCP_SUCCESS) {
            snode_remove(slp, res->chain[0]);
            /* free_slnode(res->chain[0], slp->ff); */
        }
    }    
}

void sl_free(slist_t *slp)
{
    slnode_t *next = NULL, *node;
    
    if (slp == NULL)
        return ;
    
    for (node = slp->head; node; ) {
        next = node->next[0];
        free_slnode(node, slp->ff);
        node = next;
    }
    
    Free(slp);
}

slist_t *sl_dup(slist_t *old)
{
    slnode_t    *sln, *slc=NULL, *anode;
    slist_t     *dup;
    int         i, n ;
    
    dup = slist_init(old->maxlev, old->typ, old->cmp, old->ff, old->df);

    dup->size = old->size;
    
    for (sln = old->head; 1; sln = sln->next[0]) {
        if (slc) {
            anode = slnode_dup(sln);
            slc->next[0] = anode;
            slc = anode;
            if (sln->value)
                anode->value = old->df(sln->value);
            else
                break;
        } else {
            slc = slnode_dup(sln);
            dup->head = slc;
        }
 
    }

    dup->tail = slc;
    
    /* ------- */

    for (slc = dup->head; slc->next[0]; slc = slc->next[0]) {
        for (i = 0; i < old->maxlev; i++) {
            /* zero width denotes that we have passed the height of this
             node */
            if (!slc->width[i])
                break;
            anode = slc;
            for (n = slc->width[i]; n ; n--, anode = anode->next[0]) ;
            slc->next[i] = anode; 
        }
    }
    
    return dup;
}

