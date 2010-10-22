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

#include <element.h>
#include <boundary.h>
#include <octet.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_set_limit(void);
static unsigned int test_boundarycmp(void);
static unsigned int test_boundary_cpy(void);
static unsigned int test_boundary_xcmp(void);

test_conf_t tc[] = {
    {"test_set_limit", test_set_limit},
    {"test_boundarycmp", test_boundarycmp},
    {"test_boundary_cpy", test_boundary_cpy},
    {"test_boundary_xcmp", test_boundary_xcmp},
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

static unsigned int test_set_limit(void)
{
    boundary_t      *bp;
    octet_t         oct;
    spocp_result_t  r;
    
    /* ALPHA */
    oct_assign(&oct, "ABC");
    bp = boundary_new(SPOC_ALPHA);
    r = set_limit(bp, &oct);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT( octcmp(&oct, &bp->v.val) == 0);
    boundary_free(bp);
    
    /* NUMERIC */
    oct_assign(&oct, "123456");
    bp = boundary_new(SPOC_NUMERIC);
    
    r = set_limit(bp, &oct);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT( bp->v.num == 123456L );
    boundary_free(bp);
    
    /* DATE */

    oct_assign(&oct, "2002-12-31T23:59:59Z");
    bp = boundary_new(SPOC_DATE);
    
    r = set_limit(bp, &oct);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT( oct2strcmp(&bp->v.val, "2002-12-31T23:59:59") == 0);
    boundary_free(bp);

    oct_assign(&oct, "2002-12-31T23:59:59+01:30");
    bp = boundary_new(SPOC_DATE);
    
    r = set_limit(bp, &oct);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    /* oct_print(0, "gmt", &bp->v.val); */
    TEST_ASSERT( oct2strcmp(&bp->v.val, "2003-01-01T01:29:59") == 0);
    
    /* TIME */

    oct_assign(&oct, "12:20:44");
    bp = boundary_new(SPOC_TIME);
    
    r = set_limit(bp, &oct);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS( bp->v.num, 44444L);
    boundary_free(bp);
    
    return 0;
}

static unsigned int test_boundarycmp(void)
{
    boundary_t      *bp[4];
    octet_t         oct;
    spocp_result_t  r;
    int             res;
    
    oct_assign(&oct, "2002-12-31T23:59:59Z");
    bp[0] = boundary_new(SPOC_DATE);
    r = set_limit(bp[0], &oct);

    oct_assign(&oct, "2003-01-01T00:00:10Z");
    bp[1] = boundary_new(SPOC_DATE);
    r = set_limit(bp[1], &oct);
    
    res = boundarycmp(bp[0], bp[1], &r);
    TEST_ASSERT_EQUALS(res, -1);
    res = boundarycmp(bp[1], bp[0], &r);
    TEST_ASSERT_EQUALS(res, 1);

    oct_assign(&oct, "59");
    bp[2] = boundary_new(SPOC_NUMERIC);
    r = set_limit(bp[2], &oct);
    
    oct_assign(&oct, "10");
    bp[3] = boundary_new(SPOC_NUMERIC);
    r = set_limit(bp[1], &oct);

    res = boundarycmp(bp[2], bp[3], &r);
    TEST_ASSERT_EQUALS(res, 1);
    res = boundarycmp(bp[3], bp[2], &r);
    TEST_ASSERT_EQUALS(res, -1);

    res = boundarycmp(bp[0], bp[2], &r);
    TEST_ASSERT( r != SPOCP_SUCCESS );
    TEST_ASSERT_EQUALS(res, -2);

    return 0;
}

static unsigned int test_boundary_cpy(void)
{
    boundary_t      *bp[4];
    octet_t         oct;
    spocp_result_t  r;
    int             res;

    oct_assign(&oct, "130.239.16.3");
    bp[0] = boundary_new(SPOC_IPV4);
    r = set_limit(bp[0], &oct);
    
    oct_assign(&oct, "130.239.8.10");
    bp[1] = boundary_new(SPOC_IPV4);
    r = set_limit(bp[1], &oct);
    
    bp[2] = boundary_new(0);
    res = boundary_cpy(bp[2], bp[0]);

    res = boundarycmp(bp[0], bp[1], &r);
    TEST_ASSERT_EQUALS(res, 1);
    res = boundarycmp(bp[2], bp[1], &r);
    TEST_ASSERT_EQUALS(res, 1);
    res = boundarycmp(bp[0], bp[2], &r);
    TEST_ASSERT_EQUALS(res, 0);

    return 0;
}

static unsigned int test_boundary_xcmp(void)
{
    boundary_t      *bp[4];
    octet_t         oct;
    spocp_result_t  r;
    int             res;
    
    oct_assign(&oct, "2002-12-31T23:59:59Z");
    bp[0] = boundary_new(SPOC_DATE);
    r = set_limit(bp[0], &oct);
    
    oct_assign(&oct, "2003-01-01T00:00:10Z");
    bp[1] = boundary_new(SPOC_DATE);
    r = set_limit(bp[1], &oct);
    
    res = boundary_xcmp(bp[0], bp[1]);
    TEST_ASSERT_EQUALS(res, -1);
    res = boundary_xcmp(bp[1], bp[0]);
    TEST_ASSERT_EQUALS(res, 1);
    
    oct_assign(&oct, "59");
    bp[2] = boundary_new(SPOC_NUMERIC);
    r = set_limit(bp[2], &oct);
    
    oct_assign(&oct, "10");
    bp[3] = boundary_new(SPOC_NUMERIC);
    r = set_limit(bp[1], &oct);
    
    res = boundary_xcmp(bp[2], bp[3]);
    TEST_ASSERT_EQUALS(res, 1);
    res = boundary_xcmp(bp[3], bp[2]);
    TEST_ASSERT_EQUALS(res, -1);
    
    res = boundary_xcmp(bp[0], bp[2]);
    TEST_ASSERT_EQUALS(res, -2);
    
    return 0;
}
