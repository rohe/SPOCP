/*
 *  ssn.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/8/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __SSN_H
#define __SSN_H

#include <varr.h>
#include <branch.h>

ssn_t   *ssn_new(char ch);

junc_t  *ssn_insert(ssn_t ** top, char *str, int direction);
varr_t  *ssn_match(ssn_t * pssn, char *sp, int direction);
junc_t  *ssn_delete(ssn_t ** top, char *sp, int direction);
ssn_t   *ssn_dup(ssn_t *, ruleinfo_t * ri);
varr_t  *ssn_lte_match(ssn_t * pssn, char *sp, int direction,
                        varr_t * res);
varr_t  *get_all_ssn_followers(branch_t * bp, int type, varr_t * pa);
void    ssn_free(ssn_t *);

#endif