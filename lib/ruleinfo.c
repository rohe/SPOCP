/*
 *  ruleinfo.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/7/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

/* ruleinfo is the structure that keeps information about rules.
 * This information is not used when matching requests against rules.
 * It's used when listing the rule database and when checking if a 
 * new rule is already present.
 */

#include <ruleinfo.h>
#include <ruleinst.h>
#include <wrappers.h>
#include <rdb.h>
#include <log.h>

ruleinfo_t     *
ruleinfo_new(void)
{
	return (ruleinfo_t *) Calloc(1, sizeof(ruleinfo_t));
}

ruleinfo_t     *
ruleinfo_dup(ruleinfo_t * ri)
{
	ruleinfo_t     *new;
    
	if (ri == 0)
		return 0;
    
	new = ruleinfo_new();
    if (ri->rules) {
        new->rules = rdb_dup(ri->rules, 1);
    }
    
	return new;
}

void
ruleinfo_free(ruleinfo_t * ri)
{
	if (ri) {
		rdb_free(ri->rules);
		Free(ri);
	}
}

int
ruleinfo_print(ruleinfo_t * r)
{
	if (r == 0)
		return 0;
    
	rdb_print(r->rules);
    
	return 0;
}

int
free_rule(ruleinfo_t * ri, char *uid)
{
    if (ri == 0) 
        return 0;
    else
        return rdb_remove(ri->rules, uid);
}

/* Just an alias */
void
free_all_rules(ruleinfo_t * ri)
{
	ruleinfo_free(ri);
}

int
nrules(ruleinfo_t * ri)
{
	if (ri == 0)
		return 0;
	else
		return rdb_len(ri->rules);
}

int
rules_len(ruleinfo_t * ri)
{
	if (ri == 0)
		return 0;
	else
		return rdb_len(ri->rules);
}

/************************************************************
 RULE INFO functions 
 ************************************************************/

static item_t
P_ruleinst_dup(item_t i)
{
	return (void *) ruleinst_dup((ruleinst_t *) i);
}

static void
P_ruleinst_free(void *vp)
{
	ruleinst_free((ruleinst_t *) vp);
}

static int
P_ruleinst_cmp(void *a, void *b, spocp_result_t *rc)
{
	ruleinst_t     *ra, *rb;
    
    *rc = SPOCP_SUCCESS;
    
	if (a == 0 && b == 0)
		return 0;
	if (a == 0 || b == 0)
		return 1;
    
	ra = (ruleinst_t *) a;
	rb = (ruleinst_t *) b;
    
	return strcmp(ra->uid, rb->uid);
}

/*
 * PLACEHOLDER 
 */
static char    *
P_ruleinst_print(void *vp)
{
	return Strdup((char *) vp);
}

static Key
P_ruleinst_key(item_t i)
{
	ruleinst_t     *ri = (ruleinst_t *) i;
    
	return ri->uid;
}

/*
 */

ruleinst_t *
add_rule(ruleinfo_t *ri, octet_t *rule, octet_t *blob, char *bcondname)
{    
	ruleinst_t      *rt;
    int             r;

	rt = ruleinst_new(rule, blob, bcondname);
    
	if (ri->rules == 0)
		ri->rules = rdb_new(&P_ruleinst_cmp, &P_ruleinst_free,
                            &P_ruleinst_key, &P_ruleinst_dup,
                            &P_ruleinst_print);
	else {
		if (ruleinst_find_by_uid(ri->rules, rt->uid) != 0) {
			ruleinst_free(rt);
			return 0;
		}
	}
        
	r = rdb_insert(ri->rules, (item_t) rt);

	DEBUG(SPOCP_DSTORE) {
		traceLog(SPOCP_DEBUG,"rdb_insert %d", r);
		rdb_print( ri->rules );
	}
    
	return rt;
}

ruleinst_t     *
get_rule(ruleinfo_t * ri, octet_t *oct)
{
	char	uid[41];
    
	if (ri == 0 || oct == 0 )
		return 0;
    
	if (oct2strcpy( oct, uid, 41, 0 ) < 0 ) 
		return NULL;
    
	return ruleinst_find_by_uid(ri->rules, uid);
}

ruleinst_t     *
ruleinst_find_by_uid(void * rules, char *uid)
{
	if (rules == NULL)
		return NULL;
    
	return (ruleinst_t *) rdb_search(rules, uid);
}

