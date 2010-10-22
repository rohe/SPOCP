/*
 *  boundarysl.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/16/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 * A specialised type of a skiplist, that carries boundary specifications
 * as values.
 */

#include <boundary.h>
#include <skiplist.h>
#include <boundarysl.h>

static int _bcmp( item_t ia, item_t ib, spocp_result_t *rcp)
{
    *rcp = SPOCP_SUCCESS;
    if (ib == 0)
        return -1;
    else
        return boundarycmp((boundary_t *) ia, (boundary_t *) ib, rcp);
}

static void _ffunc(item_t ia)
{
    boundary_free((boundary_t *) ia);
}

static item_t _dfunc(item_t ia)
{
    return boundary_dup((boundary_t *) ia);
}

slist_t *bsl_init(int max, int typ)
{
    return slist_init(max, typ, _bcmp, _ffunc, _dfunc);
}

sl_res_t *bsl_find( slist_t *slp, boundary_t *value, cmpfn *cmp)
{
    return sl_find(slp, (item_t) value, cmp);
}

slnode_t *bsl_insert(slist_t *slp, boundary_t *value)
{
    return sl_insert(slp, (item_t) value);
}

void bsl_remove(slist_t *slp, boundary_t *value)
{
    sl_remove(slp, (item_t) value);
}
