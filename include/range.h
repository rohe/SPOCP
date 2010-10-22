/*
 *  range.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/4/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef _RANGE_H
#define _RANGE_H

#include <atom.h>
#include <boundary.h>
#include <octet.h>
#include <skiplist.h>
#include <varr.h>
#include <result.h>

/*! \brief range definition */
typedef struct _range {
	/*! lower boundary */
	boundary_t      lower;
	/*! upper boundary */
	boundary_t      upper;
} range_t ;

/* In range.c */
boundary_t      *set_delimiter(range_t * range, octet_t oct);
spocp_result_t  range_limit(octet_t *op, range_t *rp);
void            range_free(range_t *rp);
range_t         *set_range(octet_t * op);
spocp_result_t  is_atom(range_t * rp);
atom_t          *get_atom_from_range_definition(octet_t *op);
int             range_print(octet_t *oct, range_t *r);

/* In rangestore.c */
item_t          bsl_add_range(slist_t *slp, range_t *rp);
int             atom2boundary_cmp(atom_t *ap, boundary_t *bp, 
                                  spocp_result_t *rc);

varr_t          *bsl_match_atom(slist_t *slp, atom_t *ap, 
                                spocp_result_t * rc);
varr_t          *bsl_match_range(slist_t *slp, range_t *rp);
varr_t          *bsl_match_ranges(slist_t **slp, range_t *rp);
varr_t          *bsl_inv_match_range(slist_t *slp, range_t *rp);
varr_t          *bsl_inv_match_ranges(slist_t **slp, range_t *rp);


#endif