/*
 *  ll.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef _LL_H
#define _LL_H

#include <basefn.h>
#include <parr.h>
#include <node.h>

typedef struct _ll {
	node_t  *head;
	node_t  *tail;
	cmpfn   *cf;
	ffunc   *ff;
	dfunc   *df;
	pfunc   *pf;
	int     n;
} ll_t;

/*
 * ll.c 
 */

ll_t    *ll_new(cmpfn * cf, ffunc * ff, dfunc * df, pfunc * pf);
ll_t    *ll_push(ll_t * lp, void *vp, int nodup);
void    *ll_pop(ll_t * lp);
void    ll_free(ll_t * lp);
ll_t    *ll_dup(ll_t * lp);
node_t  *ll_find(ll_t * lp, void *pattern);
int     ll_print(ll_t * lp);

ll_t    *parr2ll(parr_t * pp);
node_t  *ll_first(ll_t * lp);
node_t  *ll_next(ll_t * lp, node_t * np);
void    ll_rm_link(ll_t * lp, node_t * np);

#endif