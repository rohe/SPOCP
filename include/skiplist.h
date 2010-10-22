/*
 *  skiplist.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/15/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef _SKIPLIST_H
#define _SKIPLIST_H

#include <boundary.h>
#include <varr.h>
#include <basefn.h>

typedef struct _slnode {
	int             sz;     /* The size of the next array */
	unsigned int	refc;   /* ref count */
	item_t          *value; /* The value */
	struct _slnode  **next;
    int             *width;
	varr_t          *varr;  /* varr of junc_t structs */
} slnode_t;

typedef struct _slist {
	slnode_t    *head;
	slnode_t    *tail;    
    int         typ;        /* type of data */
	int         size;       /* current size of the list */
	int         maxlev;     /* mac number of levels */
    cmpfn       *cmp ;      /* the compare function, should do a
                             * <, ==, > comparision */
    ffunc       *ff;        /* A function that should be used to free stored
                             * values */
    dfunc       *df;        /* A function to duplicate the value */
} slist_t;

typedef struct _localres {
    slnode_t    **chain;
    int         *distance;
} sl_res_t;

slnode_t    *slnode_new(item_t *value, int n, slnode_t *tail, int width);
slist_t     *slist_init(int max, int typ, cmpfn *lte, ffunc *ff, dfunc *df);
int         sl_len( slist_t *slp );
item_t      *sl_getitem(slist_t *slp, int i);
sl_res_t    *sl_find( slist_t *slp, item_t value, cmpfn *cmp);
slnode_t    *sl_find_node( slist_t *slp, item_t value, cmpfn *cmp);
void        free_result(sl_res_t *res);
int         height( int max );
slnode_t    *sl_insert(slist_t *slp, item_t value);
void        sl_remove(slist_t *slp, item_t value);
void        sl_free(slist_t *slp);
slist_t     *sl_dup(slist_t *slp);
void        snode_remove(slist_t *slp, slnode_t *snp);
void        free_slnode(slnode_t *node, ffunc *ff);
slnode_t    *slnode_dup( slnode_t *old);

#endif
