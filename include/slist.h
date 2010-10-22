/*
 *  slist.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/8/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef _SLIST_H
#define _SLIST_H

#include <varr.h>
#include <boundary.h>
#include <range.h>
#include <ruleinfo.h>
/* #include <branch.h> */


typedef struct _slnode {
	boundary_t      *item;  /* The value */
	unsigned int	refc;   /* ref count */
	struct _slnode  **next;
	int             sz;     /* The size of the next array */
	varr_t          *varr;  /* varr of junc_t structs */
} slnode_t;

typedef struct _slist {
	slnode_t	*head;
	slnode_t	*tail;
	int         n;
	int         lgn;
	int         lgnmax;
	int         type;
} slist_t;

slist_t     *sl_init(int max);
slnode_t    *sl_new(boundary_t * item, int n, slnode_t * tail);
slnode_t    *sl_find(slist_t * slp, boundary_t * item);
varr_t      *sl_match(slist_t * slp, boundary_t * item);
varr_t      *sl_range_match(slist_t * slp, range_t * rp);
void        *sl_add_range(slist_t * slp, range_t * rp);
void        *sl_range_rm(slist_t * slp, range_t * rp, int *rc);
varr_t      *sl_range_lte_match(slist_t * slp, range_t * rp, varr_t * pa);
void		sl_free_slist(slist_t * slp);
slist_t     *sl_dup(slist_t * old, ruleinfo_t * ri);
int         sl_rand(slist_t * slp);

#endif