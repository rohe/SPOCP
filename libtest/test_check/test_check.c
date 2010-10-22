/*
 *  check_oct.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 12/3/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <octet.h>
#include <ruleinst.h>
#include <result.h>

#include <check.h>
#include <octet.h>
#include <result.h>
#include <ruleinst.h>

#include <wrappers.h>
#include <log.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_checked_new(void);
static unsigned int test_checked_free(void);
static unsigned int test_add_checked(void);
static unsigned int test_checked_rule(void);
static unsigned int test_checked_res(void);

test_conf_t tc[] = {
    {"test_checked_new", test_checked_new},
    {"test_checked_free", test_checked_free},
    {"test_add_checked", test_add_checked},
    {"test_checked_rule", test_checked_rule},
    {"test_checked_res", test_checked_res},
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

static unsigned int test_checked_new(void)
{        
    checked_t       *cp;
    ruleinst_t      *ri;
    spocp_result_t  rc;
    octet_t         rule;
    
    oct_assign(&rule, "(4:rule3:one)");
    
    ri = ruleinst_new(&rule, NULL, NULL);
    rc = SPOCP_SUCCESS;
    
    cp = checked_new(ri, rc, NULL);
    
    TEST_ASSERT(cp != NULL);
    TEST_ASSERT(cp->ri == ri);
    TEST_ASSERT(cp->rc == rc);
    TEST_ASSERT(cp->blob == NULL);
    
    return 0;
}

static unsigned int test_checked_free(void)
{        
    checked_t       *cp;
    ruleinst_t      *ri;
    spocp_result_t  rc;
    octet_t         rule;
    
    oct_assign(&rule, "(4:rule3:one)");
    
    ri = ruleinst_new(&rule, NULL, NULL);
    rc = SPOCP_SUCCESS;
    
    cp = checked_new(ri, rc, NULL);
    checked_free(cp);
    TEST_ASSERT(1);
    checked_free(NULL);
    TEST_ASSERT(1);
    
    return 0;
}

char *exrules[] = {
    "(4:rule3:one)",
    "(4:rule3:two)",
    "(4:rule5:three)",
    "(4:rule4:four)",
    NULL
} ;

#define SIZE 10

static unsigned int test_add_checked(void)
{        
    checked_t       *cr = NULL, *nc;
    ruleinst_t      *ri[SIZE];
    spocp_result_t  rc;
    octet_t         *blob = NULL, *rule[SIZE];
    int             n;
    
    rc = SPOCP_SUCCESS;    
    for (n = 0; exrules[n]; n++) {
        rule[n] = oct_new(0, exrules[n]);    
        ri[n] = ruleinst_new(rule[n], NULL, NULL);
        add_checked(ri[n], rc, blob, &cr);
    }

    TEST_ASSERT( cr != NULL) ;
    
    for( nc = cr, n = 0; nc ; nc = nc->next ) n++;

    TEST_ASSERT_EQUALS(n, 4) ;

    return 0;
}

static unsigned int test_checked_rule(void)
{        
    checked_t       *cr = NULL, *nc;
    ruleinst_t      *ri[SIZE];
    spocp_result_t  rc;
    octet_t         *blob = NULL, *rule[SIZE];
    int             n;
    
    rc = SPOCP_SUCCESS;    
    for (n = 0; exrules[n]; n++) {
        rule[n] = oct_new(0, exrules[n]);    
        ri[n] = ruleinst_new(rule[n], NULL, NULL);
        add_checked(ri[n], rc, blob, &cr);
    }
    
    nc = checked_rule(ri[2], &cr);
    TEST_ASSERT(nc != NULL);
    
    return 0;
}

static unsigned int test_checked_res(void)
{        
    checked_t       *cr = NULL;
    ruleinst_t      *ri[SIZE];
    spocp_result_t  rc, sr;
    octet_t         *blob = NULL, *rule[SIZE];
    int             n;
    
    rc = SPOCP_SUCCESS;    
    for (n = 0; exrules[n]; n++) {
        rule[n] = oct_new(0, exrules[n]);    
        ri[n] = ruleinst_new(rule[n], NULL, NULL);
        add_checked(ri[n], rc, blob, &cr);
    }
    
    sr = checked_res(ri[2], &cr);
    TEST_ASSERT(sr == SPOCP_SUCCESS);

    rule[n] = oct_new(0, "(4:rule5:extra)" );
    ri[n] = ruleinst_new(rule[n], NULL, NULL);
    add_checked(ri[n], SPOCP_DENIED, blob, &cr);

    sr = checked_res(ri[n], &cr);
    TEST_ASSERT(sr == SPOCP_DENIED);

    return 0;
}
