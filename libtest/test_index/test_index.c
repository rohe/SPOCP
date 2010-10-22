/*
 *  test_slist.c
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

#include <spocp.h>
#include <proto.h>
#include <wrappers.h>
#include <index.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_index_new(void);
static unsigned int test_index_add(void);
static unsigned int test_index_rm(void);
static unsigned int test_index_and(void);
static unsigned int test_index_extend(void);
static unsigned int test_index_cp(void);

test_conf_t tc[] = {
    {"test_index_new", test_index_new},
    {"test_index_add", test_index_add},
    {"test_index_rm", test_index_rm},
    {"test_index_and", test_index_and},
    {"test_index_extend", test_index_extend},
    {"test_index_cp", test_index_cp},
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

static unsigned int test_index_new(void)
{    
    spocp_index_t   *si;

    si = index_new(4);

    TEST_ASSERT_EQUALS( si->size, 4);
    TEST_ASSERT( si->arr !=  0);
    
    return 0;
}

char *rexp[] = {
    "(4:rule3:one)",
    "(4:rule3:two)",
    "(4:rule5:three)",
    "(4:rule4:four)",
    "(4:rule4:five)",
    NULL,
    NULL
};

static unsigned int test_index_add(void)
{    
    spocp_index_t   *si = NULL;
    ruleinst_t      *ri;
    octet_t         rule;
    int             i;
    
    for (i = 0; rexp[i]; i++) {
        oct_assign(&rule, rexp[i]);        
        ri = ruleinst_new(&rule, NULL, NULL);
        si = index_add(si, ri);
    }
    
    TEST_ASSERT_EQUALS( si->n, i);
    TEST_ASSERT( oct2strcmp(si->arr[0]->rule, rexp[0]) == 0 );


    return 0;
}

static unsigned int test_index_rm(void)
{    
    spocp_index_t   *si = NULL;
    ruleinst_t      *ri[8], *rip;
    octet_t         rule;
    int             i, rc;
    
    for (i = 0; rexp[i]; i++) {
        oct_assign(&rule, rexp[i]);        
        ri[i] = ruleinst_new(&rule, NULL, NULL);
        si = index_add(si, ri[i]);
    }
    
    /* one somewhere in the middle */
    oct_assign(&rule, rexp[2]); 
    rip = ruleinst_new(&rule, NULL, NULL);
    rc = index_rm(si, rip);
    /* will not work since the comparision is on the pointer */
    TEST_ASSERT_EQUALS(rc, FALSE);

    rc = index_rm(si, ri[2]);

    TEST_ASSERT_EQUALS( si->n, i-1);
    TEST_ASSERT( oct2strcmp(si->arr[0]->rule, rexp[0]) == 0 );
    TEST_ASSERT( oct2strcmp(si->arr[1]->rule, rexp[1]) == 0 );
    TEST_ASSERT( oct2strcmp(si->arr[2]->rule, rexp[3]) == 0 );
    TEST_ASSERT( oct2strcmp(si->arr[3]->rule, rexp[4]) == 0 );
    
    return 0;
}

static unsigned int test_index_and(void)
{    
    spocp_index_t   *sia=NULL, *sib=NULL, *sic;
    ruleinst_t      *ri[8];
    octet_t         rule;
    int             i;
    
    for (i = 0; rexp[i] ; i++) {
        oct_assign(&rule, rexp[i]);        
        ri[i] = ruleinst_new(&rule, NULL, NULL);
    }
    
    for (i = 0; rexp[i] ; i += 2) { /* 0,2,4 */
        sia = index_add(sia, ri[i]);
    }

    for (i = 0; i < 4 ; i++) { /* 0,1,2,3 */
        sib = index_add(sib, ri[i]);
    }
    
    sic = index_and(sia, sib) ;
    
    TEST_ASSERT_EQUALS( sic->n, 2);
    TEST_ASSERT( oct2strcmp(sic->arr[0]->rule, rexp[0]) == 0 );
    TEST_ASSERT( oct2strcmp(sic->arr[1]->rule, rexp[2]) == 0 );
    
    return 0;
}

static unsigned int test_index_extend(void)
{    
    spocp_index_t   *sia=NULL, *sib=NULL;
    ruleinst_t      *ri[8];
    octet_t         rule;
    int             i;
    
    for (i = 0; rexp[i] ; i++) {
        oct_assign(&rule, rexp[i]);        
        ri[i] = ruleinst_new(&rule, NULL, NULL);
    }
    
    for (i = 0; rexp[i] ; i += 2) { /* 0,2,4 */
        sia = index_add(sia, ri[i]);
    }
    
    for (i = 1; i < 4 ; i++) { /* 1,2,3 */
        sib = index_add(sib, ri[i]);
    }
    
    index_extend(sia, sib) ; /* 0,1,2,3,4 */
    
    TEST_ASSERT_EQUALS( sia->n, 5);
    /* The order is really deterministic and should be 0,2,4,1,3 */
    TEST_ASSERT( oct2strcmp(sia->arr[0]->rule, rexp[0]) == 0 );
    TEST_ASSERT( oct2strcmp(sia->arr[1]->rule, rexp[2]) == 0 );
    TEST_ASSERT( oct2strcmp(sia->arr[2]->rule, rexp[4]) == 0 );
    TEST_ASSERT( oct2strcmp(sia->arr[3]->rule, rexp[1]) == 0 );
    TEST_ASSERT( oct2strcmp(sia->arr[4]->rule, rexp[3]) == 0 );
    
    return 0;
}

static unsigned int test_index_cp(void)
{    
    spocp_index_t   *sia=NULL, *sic;
    ruleinst_t      *ri;
    octet_t         rule;
    int             i;
    
    for (i = 0; rexp[i] ; i++) {
        oct_assign(&rule, rexp[i]);
        ri = ruleinst_new(&rule, NULL, NULL);
        sia = index_add(sia, ri);
    }
    
    sic = index_cp(sia) ;
    
    TEST_ASSERT_EQUALS( sic->n, 5);
    TEST_ASSERT( oct2strcmp(sic->arr[0]->rule, rexp[0]) == 0 );
    TEST_ASSERT( oct2strcmp(sic->arr[1]->rule, rexp[1]) == 0 );
    TEST_ASSERT( oct2strcmp(sic->arr[2]->rule, rexp[2]) == 0 );
    TEST_ASSERT( oct2strcmp(sic->arr[3]->rule, rexp[3]) == 0 );
    TEST_ASSERT( oct2strcmp(sic->arr[4]->rule, rexp[4]) == 0 );
    
    return 0;
}
