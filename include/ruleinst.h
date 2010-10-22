/*
 *  ruleinst.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/7/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __RULEINST_H_
#define __RULEINST_H_

#include <element.h>
#include <octet.h>
#include <ll.h>
#include <bcond.h>
#include <varr.h>

#define SHA1HASHLEN 40

typedef struct _ruleinstance {
	char		uid[SHA1HASHLEN + 1];	/* place for sha1 hash */
	octet_t		*rule;
	octet_t		*blob;
	octet_t		*bcexp;
	element_t	*ep;					/* only used for bcond checks */
	ll_t		*alias;
	bcdef_t		*bcond;
} ruleinst_t;

unsigned int    ruleinst_uid(unsigned char *sha1sum, octet_t * rule, 
                             octet_t * blob, char *bcname) ;
ruleinst_t      *ruleinst_new(octet_t * rule, octet_t * blob, char *bcname);
ruleinst_t      *ruleinst_dup(ruleinst_t * ri);
void            ruleinst_free(ruleinst_t * rt);
ruleinst_t      *ruleinst_find_by_uid(void * rules, char *uid);
octet_t         *ruleinst_print(ruleinst_t * r, char *rs);

varr_t          *varr_ruleinst_add(varr_t * va, ruleinst_t * ri);
ruleinst_t      *varr_ruleinst_nth(varr_t * va, int n);

#endif