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
#include <branch.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_branch_new(void);
static unsigned int test_add_atom(void);
static unsigned int test_add_any(void);

test_conf_t tc[] = {
    {"test_branch_new", test_branch_new},
    {"test_add_atom", test_add_atom},
    {"test_add_any", test_add_any},
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

static unsigned int test_branch_new(void)
{
    branch_t *bp;
    
    bp = branch_new(0);
    TEST_ASSERT(bp != NULL);
    TEST_ASSERT_EQUALS(bp->type, 0);
    TEST_ASSERT_EQUALS(bp->count, 1);
    TEST_ASSERT(bp->val.atom == 0);
    return 0;
}

static unsigned int test_add_atom(void)
{
    branch_t    *bp;    
    atom_t      *ap;
    octet_t     *op;
    junc_t      *jp;

    op = oct_new(0, "Sickan");
    ap = atom_new(op);
    bp = branch_new(0);

    jp = add_atom(bp, ap);
    
    TEST_ASSERT(bp->val.atom != NULL);
    TEST_ASSERT(jp != NULL);
    
    return 0;
}

static unsigned int test_add_any(void)
{
    branch_t    *bp;    
    junc_t      *jp;
    
    bp = branch_new(0);
    
    jp = add_any(bp);
    
    TEST_ASSERT(bp->count == 1);
    TEST_ASSERT(bp->val.next == jp);
    TEST_ASSERT(jp != NULL);
    
    return 0;
}
