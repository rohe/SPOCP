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

#include <octet.h>
#include <be.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_becon_new(void);
static unsigned int test_becpool_new(void);
static unsigned int test_becpool_push(void);
static unsigned int test_becon_return(void);
static unsigned int test_becon_get(void);
static unsigned int test_becpool_full(void);
static unsigned int test_becon_rm(void);
static unsigned int test_becpool_rm(void);

test_conf_t tc[] = {
    {"test_becon_new", test_becon_new},
    {"test_becpool_new", test_becpool_new},
    {"test_becpool_push", test_becpool_push},
    {"test_becon_return", test_becon_return},
    {"test_becon_get", test_becon_get},
    {"test_becpool_full", test_becpool_full},
    {"test_becon_rm", test_becon_rm},
    {"test_becpool_rm", test_becpool_rm},
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

static unsigned int test_becon_new(void)
{
    becon_t *bcp;
    
    bcp = becon_new(1);
    
    TEST_ASSERT( bcp != NULL);
    TEST_ASSERT( bcp->con == NULL);
    TEST_ASSERT_EQUALS(bcp->status, 0);
    
    return 0;
}

static unsigned int test_becpool_new(void)
{
    becpool_t *becp;
    
    becp = becpool_new(16);
    
    TEST_ASSERT(becp != NULL);
    TEST_ASSERT(becp->head == NULL);
    TEST_ASSERT(becp->tail == NULL);
    TEST_ASSERT_EQUALS(becp->size, 0);
    TEST_ASSERT_EQUALS(becp->max, 16);
    
    return 0;
}

int  dummy_close(void *vp)
{
    return 1;
}


static unsigned int test_becpool_push(void)
{
    becpool_t   *becp;
    octet_t     namn;
    becon_t     *bcp;
    void        *con = NULL;
    
    oct_assign(&namn, "Jello");
    becp = becpool_new(16);
    
    bcp = becon_push(&namn, dummy_close, con, becp);
    
    TEST_ASSERT(bcp != NULL);
    TEST_ASSERT(octcmp(bcp->arg,&namn) == 0);    
    TEST_ASSERT(bcp->close != NULL);
    TEST_ASSERT(bcp->con == con);
    TEST_ASSERT_EQUALS(bcp->status, CON_ACTIVE);
                       
    TEST_ASSERT(becp->tail == bcp);
    TEST_ASSERT(becp->head == bcp);
    
    return 0;
}

static unsigned int test_becon_return(void)
{
    becpool_t   *becp;
    octet_t     namn;
    becon_t     *bcp;
    void        *con = NULL;
    
    oct_assign(&namn, "Jello");
    becp = becpool_new(16);    
    bcp = becon_push(&namn, dummy_close, con, becp);
    
    becon_return(bcp);
    TEST_ASSERT_EQUALS(bcp->status, CON_FREE);
    TEST_ASSERT(bcp->last == time((time_t *) 0));
    
    return 0;
}

static unsigned int test_becon_get(void)
{
    becpool_t   *becp;
    octet_t     namn, unknown;
    becon_t     *bcp, *gbcp;
    void        *con = NULL;
    
    oct_assign(&namn, "Jello");
    becp = becpool_new(16);    
    bcp = becon_push(&namn, dummy_close, con, becp);
    
    gbcp = becon_get(&namn, becp);
    
    /* Since it's active */
    TEST_ASSERT(gbcp == NULL);

    /* make it unactive */
    becon_return(bcp);
    
    gbcp = becon_get(&namn, becp);
    
    TEST_ASSERT(octcmp(gbcp->arg,&namn) == 0);    
    TEST_ASSERT(gbcp->close != NULL);
    TEST_ASSERT(gbcp->con == con);
    TEST_ASSERT_EQUALS(gbcp->status, CON_ACTIVE);
        
    /* unknown connection type */
    oct_assign(&unknown, "Fellow");
    gbcp = becon_get(&unknown, becp);
    TEST_ASSERT(gbcp == NULL);

    return 0;
}


static unsigned int test_becpool_full(void)
{
    becpool_t   *becp;
    octet_t     first, second, third;
    becon_t     *bcp;
    void        *con = NULL;
    
    becp = becpool_new(2);    
    oct_assign(&first, "First");
    bcp = becon_push(&first, dummy_close, con, becp);
    TEST_ASSERT(bcp != NULL);
    oct_assign(&second, "Second");
    bcp = becon_push(&second, dummy_close, con, becp);
    TEST_ASSERT(bcp != NULL);
    
    oct_assign(&third, "Third");
    bcp = becon_push(&third, dummy_close, con, becp);
    TEST_ASSERT(bcp == NULL);

    return 0;
}

static unsigned int test_becon_rm(void)
{
    becpool_t   *becp;
    octet_t     first;
    becon_t     *bcp, *gbcp;
    void        *con = NULL;

    oct_assign(&first, "Jello");
    becp = becpool_new(4);    
    bcp = becon_push(&first, dummy_close, con, becp);

    becon_rm(becp, bcp);

    gbcp = becon_get(&first, becp);
    TEST_ASSERT(gbcp == NULL);

    return 0;
}

static unsigned int test_becpool_rm(void)
{
    becpool_t   *becp;
    octet_t     first;
    becon_t     *bcp;
    void        *con = NULL;
    
    oct_assign(&first, "Jello");
    becp = becpool_new(4);    
    bcp = becon_push(&first, dummy_close, con, becp);
    
    becpool_rm(becp, 1);
    TEST_ASSERT(1);
    
    return 0;
}
