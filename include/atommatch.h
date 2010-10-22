/*
 *  atommatch.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/2/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __ATOMMATCH_H
#define __ATOMMATCH_H

#include <varr.h>
#include <range.h>
#include <branch.h>

varr_t  *get_all_atom_followers(branch_t * bp, varr_t * in);
varr_t  *prefix2atoms_match(char *prefix, phash_t * ht, varr_t * pa);
varr_t  *suffix2atoms_match(char *suffix, phash_t * ht, varr_t * pa);
varr_t  *range2atoms_match(range_t * rp, phash_t * ht, varr_t * pa);

#endif