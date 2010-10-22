/*
 *  test_cachetime.c
 *
 *  Created by Roland Hedberg on 12/3/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

/* test program for lib/cachetime.c */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <bsdrb.h>
#include <ruleinst.h>
#include <spocp.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>

void *bsdrb_init();

/*
 * test function 
 */
static unsigned int test_bsdrb_init(void);
static unsigned int test_bsdrb_insert(void);

test_conf_t tc[] = {
    {"test_bsdrb_init", test_bsdrb_init},
    {"test_bsdrb_insert", test_bsdrb_insert},
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

static unsigned int test_bsdrb_init(void)
{    
    void     *rb=0;
    
    rb = bsdrb_init();

    TEST_ASSERT(rb != 0);

    return 0;
}

static unsigned int test_bsdrb_insert(void)
{    
    void        *rh=0;
    ruleinst_t  *ri;
    
    rh = bsdrb_init();
    bsdrb_insert( rh, ri );
    
    return 0;
}

