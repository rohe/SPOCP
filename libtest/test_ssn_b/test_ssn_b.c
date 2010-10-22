/*
 *  test_cachetime.c
 *
 *  Created by Roland Hedberg on 12/3/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <spocp.h>
#include <proto.h>
#include <wrappers.h>
#include <branch.h>
#include <ssn.h>

#include <testRunner.h>
#include <arpa/inet.h>

/*
 * test function 
 */
static unsigned int test_ssn_dup(void);
static unsigned int test_ssn_delete(void);

test_conf_t tc[] = {
    {"test_ssn_dup", test_ssn_dup},
    {"test_ssn_delete", test_ssn_delete},
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

static unsigned int test_ssn_dup(void)
{    
    ssn_t   *ssp = 0, *dup;

    ssn_insert(&ssp, "Fox", FORWARD);
    dup = ssn_dup(ssp, NULL);
    
    TEST_ASSERT_EQUALS(dup->ch, 'F');
    TEST_ASSERT_EQUALS(dup->down->ch, 'o');
    TEST_ASSERT_EQUALS(dup->down->down->ch, 'x');
    TEST_ASSERT(dup->down->down->down == 0);
    TEST_ASSERT(dup->down->down->next != NULL);
    
    return 0;
}

static unsigned int test_ssn_delete(void)
{    
    ssn_t   *ssp = 0;
    varr_t  *vp;
    
    ssn_insert(&ssp, "Fox", FORWARD);
    ssn_insert(&ssp, "Free", FORWARD);
    
    /* Checking what I have */
    TEST_ASSERT_EQUALS(ssp->ch, 'F');
    TEST_ASSERT_EQUALS(ssp->refc, 2);
    TEST_ASSERT_EQUALS(ssp->down->ch, 'o');
    TEST_ASSERT_EQUALS(ssp->down->refc, 1);
    TEST_ASSERT_EQUALS(ssp->down->down->ch, 'x');
    TEST_ASSERT_EQUALS(ssp->down->down->refc, 1);
    TEST_ASSERT_EQUALS(ssp->down->right->ch, 'r');
    TEST_ASSERT_EQUALS(ssp->down->right->refc, 1);

    vp = ssn_match(ssp, "Fox", FORWARD);
    TEST_ASSERT(vp != NULL);
    
    /* Now remove one of them, refc should drop along the 'removed' branch */    
    ssn_delete(&ssp, "Fox", FORWARD);
    TEST_ASSERT_EQUALS(ssp->ch, 'F');
    TEST_ASSERT_EQUALS(ssp->refc, 1);
    TEST_ASSERT_EQUALS(ssp->down->ch, 'o');
    TEST_ASSERT_EQUALS(ssp->down->refc, 0);
    TEST_ASSERT_EQUALS(ssp->down->down->ch, 'x');
    TEST_ASSERT_EQUALS(ssp->down->down->refc, 0);
    TEST_ASSERT_EQUALS(ssp->down->right->ch, 'r');
    TEST_ASSERT_EQUALS(ssp->down->right->refc, 1);
    TEST_ASSERT_EQUALS(ssp->down->right->down->ch, 'e');
    
    vp = ssn_match(ssp, "Fox", FORWARD);
    TEST_ASSERT(vp == NULL);

    ssn_insert(&ssp, "Fox", FORWARD);
    /* this shouldn't change anything */
    ssn_delete(&ssp, "Foy", FORWARD);
    TEST_ASSERT_EQUALS(ssp->ch, 'F');
    TEST_ASSERT_EQUALS(ssp->refc, 2);
    TEST_ASSERT_EQUALS(ssp->down->ch, 'o');
    TEST_ASSERT_EQUALS(ssp->down->refc, 1);
    TEST_ASSERT_EQUALS(ssp->down->down->ch, 'x');
    TEST_ASSERT_EQUALS(ssp->down->down->refc, 1);
    TEST_ASSERT_EQUALS(ssp->down->right->ch, 'r');
    TEST_ASSERT_EQUALS(ssp->down->right->refc, 1);
    
    return 0;
}

