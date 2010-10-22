/*
 *  dset.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/9/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <dset.h>
#include <varr.h>
#include <wrappers.h>

dset_t *dset_new(char *uid, varr_t *val)
{
    dset_t *dp;
    
    dp = (dset_t *) Calloc(1, sizeof(dset_t));
    dp->uid = uid;
    dp->va = val;
    
    return dp;
}

void
dset_free(dset_t * ds)
{
	if (ds) {
		if (ds->va) {
			varr_free(ds->va);
		}
		if (ds->next)
			dset_free(ds->next);
		Free(ds);
	}
}

dset_t *dset_append(dset_t *head, dset_t *dp)
{
    dset_t *ds;
    
    if (head == NULL)
        return dp;
    else {
        for (ds = head; ds->next; ds = ds->next);
        ds->next = dp;
        return head;
    }
}
