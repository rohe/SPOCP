/*
 *  check_oct.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 12/3/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

/* test program for lib/oct.c */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <ruleinst.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_ruleinst_new(void);
static unsigned int test_ruleinst_dup(void);
static unsigned int test_ruleinst_free(void);
static unsigned int test_ruleinst_find_by_uid(void);

test_conf_t tc[] = {
    {"test_ruleinst_new", test_ruleinst_new},
    {"test_ruleinst_dup", test_ruleinst_dup},
    {"test_ruleinst_free", test_ruleinst_free},
    {"test_ruleinst_find_by_uid", test_ruleinst_find_by_uid},
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

static unsigned int test_ruleinst_new(void)
{
    octet_t     rule;
    ruleinst_t  *rip;
    
    oct_assign(&rule, "(4:rule3:one)");
    
    rip = ruleinst_new(&rule, NULL, NULL);
    
    TEST_ASSERT(rip != 0);
    TEST_ASSERT(rip->blob == 0);
    TEST_ASSERT(rip->ep == 0);
    TEST_ASSERT(rip->alias == 0);
    TEST_ASSERT(strcmp(rip->uid,
                       "0471df2f991b57a03dbfb21a60d89477598b55b5") ==0);
    return 0;
}

static unsigned int test_ruleinst_dup(void)
{
    octet_t     rule;
    ruleinst_t  *rip, *dup;
    
    oct_assign(&rule, "(4:rule3:one)");
    
    rip = ruleinst_new(&rule, NULL, NULL);
    dup = ruleinst_dup(rip);
    
    TEST_ASSERT(dup != 0);
    TEST_ASSERT(dup->blob == 0);
    TEST_ASSERT(dup->ep == 0);
    TEST_ASSERT(dup->alias == 0);
    TEST_ASSERT(strcmp(dup->uid,
                        "0471df2f991b57a03dbfb21a60d89477598b55b5") ==0);
    return 0;
}

static unsigned int test_ruleinst_free(void)
{
    octet_t     rule;
    ruleinst_t  *rip, *dup;
    
    oct_assign(&rule, "(4:rule3:one)");
    
    rip = ruleinst_new(&rule, NULL, NULL);
    dup = ruleinst_dup(rip);
    
    ruleinst_free(rip);
    
    TEST_ASSERT(dup != 0);
    TEST_ASSERT(dup->blob == 0);
    TEST_ASSERT(dup->ep == 0);
    TEST_ASSERT(dup->alias == 0);
    TEST_ASSERT(strcmp(dup->uid,
                       "0471df2f991b57a03dbfb21a60d89477598b55b5") ==0);
    return 0;
}

static unsigned int test_ruleinst_find_by_uid(void)
{
    octet_t     rule;
    ruleinst_t  *rip, *dup;
    
    oct_assign(&rule, "(4:rule3:one)");
    
    rip = ruleinst_new(&rule, NULL, NULL);
    dup = ruleinst_dup(rip);
    
    ruleinst_free(rip);
    
    TEST_ASSERT(dup != 0);
    TEST_ASSERT(dup->blob == 0);
    TEST_ASSERT(dup->ep == 0);
    TEST_ASSERT(dup->alias == 0);
    TEST_ASSERT(strcmp(dup->uid,
                       "0471df2f991b57a03dbfb21a60d89477598b55b5") ==0);
    return 0;
}

