/*
 *  ruleinfo.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/7/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __RULEINFO_H_
#define __RULEINFO_H_

#include <stdlib.h>

#include <octet.h>
#include <ruleinst.h>

typedef struct _ruleinfo {
	void	*rules;						/* rule database */
} ruleinfo_t;

ruleinfo_t  *ruleinfo_new(void);
ruleinfo_t  *ruleinfo_dup(ruleinfo_t * ri);
void        ruleinfo_free(ruleinfo_t * ri);
int         ruleinfo_print(ruleinfo_t * ri);
int         free_rule(ruleinfo_t * ri, char *uid);
void        free_all_rules(ruleinfo_t * ri);
int         nrules(ruleinfo_t * ri);
int         rules_len(ruleinfo_t * ri);
ruleinst_t  *get_rule(ruleinfo_t * ri, octet_t *oct);
ruleinst_t  *ruleinst_find_by_uid(void * rules, char *uid);
ruleinst_t  *add_rule(ruleinfo_t *ri, octet_t *rule, octet_t *blob, 
                      char *bcondname);

#endif