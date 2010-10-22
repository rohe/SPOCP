/*
 *  complex_free.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 12/9/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

#include <stdlib.h>

#include <hash.h>
#include <branch.h>
#include <dset.h>
#include <ssn.h>
#include <skiplist.h>
#include <wrappers.h>


/*
 * --------------------------------------------------------------- 
 */

void
branch_free(branch_t * bp)
{
	int	i;
    
	if (bp) {
		switch (bp->type) {
            case SPOC_ATOM:
                phash_free(bp->val.atom);
                break;
                
            case SPOC_LIST:
                junc_free(bp->val.list);
                break;
                
            case SPOC_SET:
                dset_free(bp->val.set);
                break;
                
            case SPOC_PREFIX:
                ssn_free(bp->val.prefix);
                break;
                
            case SPOC_RANGE:
                for (i = 0; i < DATATYPES; i++)
                    sl_free_slist(bp->val.range[i]);
                break;
                
		}
	}
}
