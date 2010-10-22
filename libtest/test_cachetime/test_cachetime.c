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

#include <cache.h>
#include <spocp.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_cachetime_new(void);
static unsigned int test_cachetime_set(void);

test_conf_t tc[] = {
    {"test_cachetime_new", test_cachetime_new},
    {"test_cachetime_set", test_cachetime_set},
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

static unsigned int test_cachetime_new(void)
{    
    cachetime_t     *ctp=0;
    octet_t         *ctd;
    
    ctd = oct_new(0, "360 (3:foo");
    ctp = cachetime_new(ctd);

    TEST_ASSERT_EQUALS(ctp->limit, (time_t) 360);
    TEST_ASSERT(oct2strcmp(&ctp->pattern, "(3:foo") == 0);

    return 0;
}

static unsigned int test_cachetime_set(void)
{    
    cachetime_t     *ctp=0;
    octet_t         *ctd, *tmp;
    time_t          t;
    
    ctd = oct_new(0, "360 (3:foo");
    ctp = cachetime_new(ctd);
    
    tmp = oct_new(0, "(3:foo3:bar)");
    
    t = cachetime_set(tmp, ctp);
    TEST_ASSERT_EQUALS(t, (time_t) 360);
    oct_free(tmp);

    tmp = oct_new(0, "(4:http3:bar)");
    t = cachetime_set(tmp, ctp);
    TEST_ASSERT_EQUALS(t, (time_t) 0);

    return 0;
}

