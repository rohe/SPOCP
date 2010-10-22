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
static unsigned int test_ssn_new(void);
static unsigned int test_ssn_insert_forward(void);
static unsigned int test_ssn_insert_backward(void);
static unsigned int test_ssn_match_backward(void);
static unsigned int test_ssn_match_forward(void);
static unsigned int test_ssn_lte_match(void);

test_conf_t tc[] = {
    {"test_ssn_new", test_ssn_new},
    {"test_ssn_insert_forward", test_ssn_insert_forward},
    {"test_ssn_insert_backward", test_ssn_insert_backward},
    {"test_ssn_match_backward", test_ssn_match_backward},
    {"test_ssn_match_forward", test_ssn_match_forward},
    {"test_ssn_lte_match", test_ssn_lte_match},
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

static unsigned int test_ssn_new(void)
{    
    ssn_t   *ssp = 0;

    ssp = ssn_new('a');
    TEST_ASSERT_EQUALS(ssp->ch, 'a');
    TEST_ASSERT(ssp->next == 0 );
    TEST_ASSERT(ssp->left == 0 );
    TEST_ASSERT(ssp->right == 0 );
    TEST_ASSERT(ssp->down == 0 );
    TEST_ASSERT_EQUALS(ssp->refc, 1);
    TEST_ASSERT(ssp->next == 0 );
    return 0;
}

static unsigned int test_ssn_insert_forward(void)
{    
    junc_t  *jp = 0;
    ssn_t   *ssp = 0;
    
    jp = ssn_insert(&ssp, "Fox", FORWARD);

    TEST_ASSERT_EQUALS(ssp->ch, 'F');
    TEST_ASSERT(jp != 0);
    TEST_ASSERT_EQUALS(ssp->down->ch, 'o');
    TEST_ASSERT_EQUALS(ssp->down->down->ch, 'x');
    TEST_ASSERT(ssp->down->down->down == 0);
    TEST_ASSERT(ssp->down->down->next == jp);

    jp = ssn_insert(&ssp, "Foxy", FORWARD);
    TEST_ASSERT_EQUALS(ssp->down->down->down->ch, 'y');
    TEST_ASSERT_EQUALS(ssp->refc, 2);
    TEST_ASSERT_EQUALS(ssp->down->refc, 2);
    TEST_ASSERT_EQUALS(ssp->down->down->refc, 2);
    TEST_ASSERT_EQUALS(ssp->down->down->down->refc, 1);
    
    return 0;
}

static unsigned int test_ssn_insert_backward(void)
{    
    junc_t  *jp = 0;
    ssn_t   *ssp = 0;
    
    jp = ssn_insert(&ssp, "Fox", BACKWARD);
    
    TEST_ASSERT_EQUALS(ssp->ch, 'x');
    TEST_ASSERT(ssp->next == 0);
    TEST_ASSERT(jp != 0);
    TEST_ASSERT_EQUALS(ssp->down->ch, 'o');
    TEST_ASSERT(ssp->down->next == 0);
    TEST_ASSERT_EQUALS(ssp->down->down->ch, 'F');
    TEST_ASSERT(ssp->down->down->down == 0);
    TEST_ASSERT(ssp->down->down->next == jp);
    
    jp = ssn_insert(&ssp, "Foxy", BACKWARD);
    TEST_ASSERT_EQUALS(ssp->right->ch, 'y');
    TEST_ASSERT_EQUALS(ssp->right->down->ch, 'x');
    TEST_ASSERT_EQUALS(ssp->right->down->down->ch, 'o');
    TEST_ASSERT_EQUALS(ssp->right->down->down->down->ch, 'F');

    jp = ssn_insert(&ssp, "Box", BACKWARD);
    TEST_ASSERT_EQUALS(ssp->refc, 2);
    TEST_ASSERT_EQUALS(ssp->down->refc, 2);
    TEST_ASSERT_EQUALS(ssp->down->down->left->ch, 'B');

    return 0;
}

static unsigned int test_ssn_match_backward(void)
{    
    junc_t  *jp = 0;
    ssn_t   *ssp = 0;
    varr_t  *vap;
    
    jp = ssn_insert(&ssp, "Fox", BACKWARD);
    jp = ssn_insert(&ssp, "Foxy", BACKWARD);
    jp = ssn_insert(&ssp, "Box", BACKWARD);
    jp = ssn_insert(&ssp, "Sox", BACKWARD);
    
    TEST_ASSERT(ssp->next == 0 );
    TEST_ASSERT(ssp->down->next == 0 );
    TEST_ASSERT(ssp->down->down->next != 0 );
    vap = ssn_match(ssp, "Fox", BACKWARD);
    TEST_ASSERT_EQUALS(vap->n, 1);

    vap = ssn_match(ssp, "Foxy", BACKWARD);
    TEST_ASSERT_EQUALS(vap->n, 1);

    jp = ssn_insert(&ssp, "ox", BACKWARD);

    vap = ssn_match(ssp, "Fox", BACKWARD);
    TEST_ASSERT_EQUALS(vap->n, 2);

    jp = ssn_insert(&ssp, "ax", BACKWARD);
    jp = ssn_insert(&ssp, "sax", BACKWARD);
    vap = ssn_match(ssp, "sax", BACKWARD);
    TEST_ASSERT_EQUALS(vap->n, 2);
    
    
    return 0;
}

static unsigned int test_ssn_match_forward(void)
{    
    junc_t  *jp = 0;
    ssn_t   *ssp = 0;
    varr_t  *vap;
    
    jp = ssn_insert(&ssp, "Fox", FORWARD);
    jp = ssn_insert(&ssp, "Foxy", FORWARD);
    jp = ssn_insert(&ssp, "Box", FORWARD);
    
    TEST_ASSERT(ssp->next == 0 );
    TEST_ASSERT(ssp->down->next == 0 );
    TEST_ASSERT(ssp->down->down->next != 0 );
    vap = ssn_match(ssp, "Fox", FORWARD);
    TEST_ASSERT_EQUALS(vap->n, 1);
    
    vap = ssn_match(ssp, "Foxy", FORWARD);
    TEST_ASSERT_EQUALS(vap->n, 2);
    
    jp = ssn_insert(&ssp, "Fo", FORWARD);
    
    vap = ssn_match(ssp, "Fox", FORWARD);
    TEST_ASSERT_EQUALS(vap->n, 2);    
    vap = ssn_match(ssp, "Foxy", FORWARD);
    TEST_ASSERT_EQUALS(vap->n, 3);
    
    return 0;
}

static unsigned int test_ssn_lte_match(void)
{    
    junc_t  *jp = 0;
    ssn_t   *ssp = 0;
    varr_t  *res = NULL;
    
    jp = ssn_insert(&ssp, "Fox", FORWARD);
    jp = ssn_insert(&ssp, "Foxy", FORWARD);
    jp = ssn_insert(&ssp, "Box", FORWARD);

    res = ssn_lte_match(ssp, "Fox", FORWARD, res);
    TEST_ASSERT(res != NULL);
    TEST_ASSERT_EQUALS(res->n, 2);
    TEST_ASSERT(res->arr != NULL);
    jp = (junc_t *) res->arr[0];
    TEST_ASSERT_EQUALS(jp->dynamic, 0);
    TEST_ASSERT_EQUALS(jp->refc, 0);
    return 0;
}
