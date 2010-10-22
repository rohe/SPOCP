/*
 *  check_oct.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 12/3/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

/* test program for lib/oct.c */

#include <stdio.h>
#include <string.h>

#include <octet.h>
#include <ruleinfo.h>
#include <rdb.h>
/* #include <proto.h>*/
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_ruleinfo_new(void);
static unsigned int test_ruleinfo_dup(void);
static unsigned int test_ruleinfo_free(void);
static unsigned int test_add_rule(void);
static unsigned int test_ruleinst_find_by_uid(void);
static unsigned int test_get_rule(void);

test_conf_t tc[] = {
    {"test_ruleinfo_new", test_ruleinfo_new},
    {"test_ruleinfo_dup", test_ruleinfo_dup},
    {"test_ruleinfo_free", test_ruleinfo_free},
    {"test_add_rule", test_add_rule},
    {"test_ruleinst_find_by_uid", test_ruleinst_find_by_uid},
    {"test_get_rule", test_get_rule},
    {"",NULL}
};

int main( int argc, char **argv )
{
    test_conf_t *ptc;
    
    printf("------------------------------------------\n");
    for( ptc = tc; ptc->func ; ptc++) {
        run_test(ptc);
    }
    printf("------------------------------------------\n");
    return 0;
}

/* ----------------------------------------------------*/

char *RULE[] = { 
    "(4:rule3:one)",
    "(4:rule3:two)",
    "(4:rule5:three)",
    "(4:rule4:four)",
    "(4:rule4:five)",
    "(4:rule3:six)",
    NULL
};

/* ----------------------------------------------------*/

static unsigned int test_ruleinfo_new(void)
{
    ruleinfo_t *rip;
    
    rip = ruleinfo_new();
    TEST_ASSERT(rip != NULL);
    TEST_ASSERT_EQUALS(nrules(rip),0);
    return 0;
}

static unsigned int test_ruleinfo_free(void)
{
    ruleinfo_t *rip;
    
    ruleinfo_free(NULL);
    rip = ruleinfo_new();
    ruleinfo_free(rip);
    TEST_ASSERT(1);
    return 0;
}

static unsigned int test_add_rule(void)
{
    ruleinfo_t  *ri;
    ruleinst_t  *rt;
    octet_t     rule, blob;
    
    ri = ruleinfo_new();
    oct_assign(&rule, "(4:rule3:one)");
    oct_assign(&blob, "BLOB");
    rt = add_rule(ri, &rule, &blob, NULL);
    
    TEST_ASSERT(rt != NULL);
    TEST_ASSERT_EQUALS(nrules(ri), 1);

    oct_assign(&rule, "(4:rule3:two)");
    rt = add_rule(ri, &rule, NULL, NULL);
    
    TEST_ASSERT(rt != NULL);
    TEST_ASSERT_EQUALS(rules_len(ri), 2);

    oct_assign(&rule, "(4:rule5:three)");
    rt = add_rule(ri, &rule, NULL, NULL);
    
    TEST_ASSERT(rt != NULL);
    TEST_ASSERT_EQUALS(rules_len(ri), 3);
    
    return 0;
}

static unsigned int test_ruleinfo_dup(void)
{
    ruleinfo_t  *rip, *dup;
    int         i;
    ruleinst_t  *rt;
    octet_t     rule;
    varr_t      *all;
    void        *v;

    rip = ruleinfo_new();
    dup = ruleinfo_dup(NULL);
    TEST_ASSERT(dup == NULL);
    dup = ruleinfo_dup(rip);
    TEST_ASSERT(dup != NULL);

    ruleinfo_free(rip);
    ruleinfo_free(dup);
    
    rip = ruleinfo_new();
    for ( i = 0; RULE[i]; i++) {
        oct_assign(&rule, RULE[i]);
        rt = add_rule(rip, &rule, NULL, NULL);
    }

    dup = ruleinfo_dup(rip);
    TEST_ASSERT(dup != NULL);

    all = rdb_all(rip->rules);
    for(v = varr_first(all); v ; v = varr_next(all, v)) {
        TEST_ASSERT(varr_find(dup, v) >= 0 );
    }
    
    return 0;
}


static unsigned int test_ruleinst_find_by_uid(void)
{
    ruleinfo_t  *ri;
    ruleinst_t  *rtl[3], *rt;
    octet_t     rule, blob;
    
    ri = ruleinfo_new();
    oct_assign(&rule, "(4:rule3:one)");
    oct_assign(&blob, "BLOB");
    rtl[0] = add_rule(ri, &rule, &blob, NULL);
    oct_assign(&rule, "(4:rule3:two)");
    rtl[1] = add_rule(ri, &rule, NULL, NULL);
    oct_assign(&rule, "(4:rule5:three)");
    rtl[2] = add_rule(ri, &rule, NULL, NULL);
    
    rt = ruleinst_find_by_uid(ri->rules, rtl[1]->uid);
    TEST_ASSERT(rt != NULL);
    
    return 0;
}

static unsigned int test_get_rule(void)
{
    ruleinfo_t  *ri;
    ruleinst_t  *rtl[3], *rt;
    octet_t     rule, blob, ouid;
    
    ri = ruleinfo_new();
    oct_assign(&rule, "(4:rule3:one)");
    oct_assign(&blob, "BLOB");
    rtl[0] = add_rule(ri, &rule, &blob, NULL);
    oct_assign(&rule, "(4:rule3:two)");
    rtl[1] = add_rule(ri, &rule, NULL, NULL);
    oct_assign(&rule, "(4:rule5:three)");
    rtl[2] = add_rule(ri, &rule, NULL, NULL);
    
    oct_assign(&ouid, rtl[1]->uid);
    rt = get_rule(ri, &ouid);
    TEST_ASSERT(rt != NULL);
    
    return 0;
}
