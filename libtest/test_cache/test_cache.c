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
#include <arpa/inet.h>

#include <spocp.h>
#include <proto.h>
#include <wrappers.h>
#include <cache.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_cache_value(void);
static unsigned int test_cache_rm(void);

test_conf_t tc[] = {
    {"test_cache_value", test_cache_value},
    {"test_cache_rm", test_cache_rm},
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

static unsigned int test_cache_value(void)
{    
    cache_t         *cp=0;
    octet_t         *op[64], *blob=0, bl, *tmp;
    unsigned int    timeout = 60;
    int             res = 0, i;
    char            line[32];

    for ( i = 0; i < 8; i++) {
        sprintf(line, "(4:sexp2:%02d)",i); 
        op[i] = oct_new(0, line);
        res = random() & 01;
        /*
        printf("sexp: %s (%d)\n", line, res);
         */
        cp = cache_value(cp, op[i], timeout, res, blob);
    }
    
    memset(&bl, 0, sizeof(octet_t));
    
    TEST_ASSERT_NOT_EQUALS(cached(cp, op[3], &bl), 0);
    TEST_ASSERT_EQUALS( bl.len, 0);
    
    tmp = oct_new(0, "(4:sexp2:99)"); 
    TEST_ASSERT_EQUALS(cached(cp, tmp, &bl), 0);

    return 0;
}

static unsigned int test_cache_rm(void)
{    
    cache_t         *cp=0;
    octet_t         *op[64], *blob=0, bl, *tmp;
    unsigned int    timeout = 60;
    int             res = 0, i;
    char            line[32];
    
    for ( i = 0; i < 8; i++) {
        sprintf(line, "(4:sexp2:%02d)",i); 
        op[i] = oct_new(0, line);
        res = random() & 01;
        /*
         printf("sexp: %s (%d)\n", line, res);
         */
        cp = cache_value(cp, op[i], timeout, res, blob);
    }
    
    memset(&bl, 0, sizeof(octet_t));
    
    TEST_ASSERT_NOT_EQUALS(cached(cp, op[3], &bl), 0);
    TEST_ASSERT_EQUALS( bl.len, 0);
    
    cached_rm(cp, op[3]);
    /* printf("r: %d\n",cached(cp, op[3], &bl)); */
    TEST_ASSERT_EQUALS(cached(cp, op[3], &bl), 0);

    tmp = oct_new(0, "(4:sexp2:99)"); 
    cached_rm(cp, tmp);
    /* printf("r: %d\n",cached(cp, tmp, &bl)); */
    TEST_ASSERT_EQUALS(cached(cp, tmp, &bl), 0);
    
    return 0;
}

