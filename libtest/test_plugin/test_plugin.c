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
#include <proto.h>
#include <plugin.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_pdyn_new(void);
static unsigned int test_pdyn_free(void);
static unsigned int test_plugin_load(void);
static unsigned int test_plugin_match(void);
static unsigned int test_plugin_get(void);
static unsigned int test_plugin_unload(void);
static unsigned int test_plugin_unload_all(void);
static unsigned int test_plugin_add_cachedef(void);

test_conf_t tc[] = {
    {"test_pdyn_new", test_pdyn_new},
    {"test_pdyn_free", test_pdyn_free},
    {"test_plugin_load", test_plugin_load},
    {"test_plugin_match", test_plugin_match},
    {"test_plugin_get", test_plugin_get},
    {"test_plugin_unload", test_plugin_unload},
    {"test_plugin_unload_all", test_plugin_unload_all},
    {"test_plugin_add_cachedef", test_plugin_add_cachedef},
    {"",NULL}
};

#include <plugin.h>
#include <octet.h>

/* -------------------------------------------------------------------- */
spocp_result_t dummy1_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  r = SPOCP_SUCCESS;
    
    return r;
}

spocp_result_t dummy2_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  r = SPOCP_SUCCESS;
    
    return r;
}

plugin_t    dummy1_module = {
	SPOCP20_PLUGIN_STUFF,
	dummy1_test,
	NULL,
	NULL,
	NULL
};

plugin_t    dummy2_module = {
	SPOCP20_PLUGIN_STUFF,
	dummy2_test,
	NULL,
	NULL,
	NULL
};

/* -------------------------------------------------------------------- */

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

static unsigned int test_pdyn_new(void)
{
    
    pdyn_t  *pdyn;

    pdyn = pdyn_new(3);
    TEST_ASSERT(pdyn != NULL);
    TEST_ASSERT_EQUALS(pdyn->size, 3);
    
    return 0;
}

static unsigned int test_pdyn_free(void)
{
    
    pdyn_t  *pdyn;
    
    pdyn = pdyn_new(3);
    pdyn_free(pdyn);
    TEST_ASSERT(1);
    
    return 0;
}

static unsigned int test_plugin_load(void)
{
    
    plugin_t    *pp;

    pp = plugin_load(NULL, "first", NULL, &dummy1_module);
    TEST_ASSERT(pp->test == dummy1_test);
    
    return 0;
}

static unsigned int test_plugin_match(void)
{
    
    plugin_t    *top = NULL, *pp;
    octet_t     first;
    char        *name = "first";
    
    top = plugin_load(top, name, NULL, &dummy1_module);
    oct_assign(&first, name);
    
    pp = plugin_match(top, &first);
    TEST_ASSERT(pp != NULL);
    TEST_ASSERT(pp->test == dummy1_test);
    
    return 0;
}

static unsigned int test_plugin_get(void)
{
    
    plugin_t    *top = NULL, *pp;
    char        *first = "first", *second="second";
    
    top = plugin_load(top, first, NULL, &dummy1_module);
    top = plugin_load(top, second, NULL, &dummy2_module);
    pp = plugin_get(top, first);
    TEST_ASSERT(pp != NULL);
    TEST_ASSERT(pp->test == dummy1_test);
    pp = plugin_get(top, second);
    TEST_ASSERT(pp != NULL);
    TEST_ASSERT(pp->test == dummy2_test);
    
    return 0;
}

static unsigned int test_plugin_unload(void)
{
    
    plugin_t    *top = NULL, *pp;
    char        *first = "first", *second="second";
    
    top = plugin_load(top, first, NULL, &dummy1_module);
    top = plugin_load(top, second, NULL, &dummy2_module);
    pp = plugin_get(top, first);
    plugin_unload(pp);
    TEST_ASSERT(1);
        
    return 0;
}

static unsigned int test_plugin_unload_all(void)
{
    
    plugin_t    *top = NULL, *pp;
    char        *first = "first", *second="second";
    
    top = plugin_load(top, first, NULL, &dummy1_module);
    top = plugin_load(top, second, NULL, &dummy2_module);
    pp = plugin_get(top, first);
    plugin_unload_all(top);
    TEST_ASSERT(1);
    
    return 0;
}

static unsigned int test_plugin_add_cachedef(void)
{
    
    plugin_t    *top = NULL;
    octet_t     first ;
    char        *name = "first", *ctd="360 (3:foo";
    int         r;
    cachetime_t *ctp;
    
    top = plugin_load(top, name, NULL, &dummy1_module);
    oct_assign(&first, name);

    r = plugin_add_cachedef(top, ctd);
    TEST_ASSERT(r == TRUE);
    ctp = top->dyn->ct;
    TEST_ASSERT_EQUALS(ctp->limit, (time_t) 360);
    TEST_ASSERT(oct2strcmp(&ctp->pattern, "(3:foo") == 0);
                
    return 0;
}
