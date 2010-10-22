/*
 *  test_dset.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 12/3/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

/* test program for lib/oct.c */

#include <config.h>

#include <dset.h>
#include <varr.h>

#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_dset_new(void);
static unsigned int test_dset_append(void);

test_conf_t tc[] = {
    {"test_dset_new", test_dset_new},
    {"test_dset_append", test_dset_append},
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

static unsigned int test_dset_new(void)
{
    dset_t *ds;
    
    ds = dset_new("uid", NULL);
    
    TEST_ASSERT( strcmp(ds->uid, "uid") == 0 );
    TEST_ASSERT( ds->va == NULL);
    
    return 0;
}

static unsigned int test_dset_append(void)
{
    dset_t *ds, *head=NULL;
    
    ds = dset_new("uid1", NULL);
    head = dset_append(head, ds);
    
    TEST_ASSERT( strcmp(head->uid, "uid1") == 0 );
    TEST_ASSERT( head->va == NULL);

    ds = dset_new("uid2", NULL);
    head = dset_append(head, ds);
    TEST_ASSERT( head->next == ds);

    return 0;
}

