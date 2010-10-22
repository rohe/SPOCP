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
#include <rdb.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_rdb_new(void);
static unsigned int test_rdb_dup(void);

test_conf_t tc[] = {
    {"test_rdb_new", test_rdb_new},
    {"test_rdb_dup", test_rdb_dup},
    {"",NULL}
};

void _print_oct( octet_t *oct) 
{
    char *s;
    
    printf("len: %d\n", (int) oct->len);
    printf("size: %d\n", (int) oct->size);
    printf("base: %p\n", oct->base);
    printf("val: %p\n", oct->val);
    s = oct2strdup(oct, 0);
    printf("val: '%s'\n", s);
    Free(s);
}

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

static unsigned int test_rdb_new(void)
{
    void *vp;
    
    vp = rdb_new(NULL, NULL, NULL, NULL, NULL);
    
    TEST_ASSERT(vp != NULL);
    
    return 0;
}

char *VALUE[] = { "one", "two", "three", "four", "five", NULL };

static unsigned int test_rdb_dup(void)
{
    void    *vp, *dup, *v;
    int     i, r;
    varr_t  *va, *vb;
    
    vp = rdb_new(NULL, NULL, NULL, NULL, NULL);
    for (i = 0; VALUE[i]; i++) {
        r = rdb_insert(vp, VALUE[i]);
        TEST_ASSERT(r);
    }
    
    dup = rdb_dup(vp, 1);
    va = rdb_all(dup);
    TEST_ASSERT(va->n == 5);
    vb = rdb_all(vp);
    for (v=varr_first(va); v; v = varr_next(va,v)) {
        TEST_ASSERT(varr_find(vb, v) >= 0);
    }
    return 0;
}

