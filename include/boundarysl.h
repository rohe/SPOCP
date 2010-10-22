/*
 *  boundarysl.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/16/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __BOUNDARYSL_H
#define __BOUNDARYSL_H

#include <basefn.h>
#include <boundary.h>
#include <skiplist.h>
#include <range.h>

slist_t     *bsl_init(int max, int typ);
sl_res_t    *bsl_find( slist_t *slp, boundary_t *value, cmpfn *cmp);
slnode_t    *bsl_insert(slist_t *slp, boundary_t *value);
void        bsl_remove(slist_t *slp, boundary_t *value);

item_t      bsl_add_range(slist_t *slp, range_t *rp);

#endif