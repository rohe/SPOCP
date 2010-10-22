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
#include <bcond.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>

/* ==================================================================== */

spocp_result_t true_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  r = SPOCP_SUCCESS;
    
    return r;
}

plugin_t    true_module = {
	SPOCP20_PLUGIN_STUFF,
	true_test,
	NULL,
	NULL,
	NULL
};

spocp_result_t false_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  r = SPOCP_DENIED;
    
    return r;
}

plugin_t    false_module = {
	SPOCP20_PLUGIN_STUFF,
	false_test,
	NULL,
	NULL,
	NULL
};

/* ==================================================================== */

/*
 * test function 
 */
static unsigned int test_bcond_check(void);

test_conf_t tc[] = {
    {"test_bcond_check", test_bcond_check},
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

static unsigned int test_bcond_check(void)
{
    octet_t     o;
    bcstree_t   *bcsp, *pp, *np;
    plugin_t    *plug = NULL;
    octet_t     first;
    char        *name = "first";
    
    plug = plugin_load(plug, "true", NULL, &true_module);
    plug = plugin_load(plug, "false", NULL, &false_module);
    
    octet_t     rule;
    ruleinst_t  *rip;
    
    oct_assign(&rule, "(4:rule3:one)");    
    rip = ruleinst_new(&rule, &blob, bcname);
    
    oct_assign(&o, "(3:ref3:foo)");
    bcsp = parse_bcexp(&o);
               
    TEST_ASSERT(bcsp != NULL);
    TEST_ASSERT_EQUALS(bcsp->list, 1);
    pp = bcsp->part;
    TEST_ASSERT(pp->part == NULL);
    TEST_ASSERT_EQUALS(pp->list, 0);
    TEST_ASSERT(oct2strcmp( &pp->val, "ref") == 0);
    np = pp->next;
    TEST_ASSERT_EQUALS(np->list, 0);
    TEST_ASSERT(oct2strcmp( &np->val, "foo") == 0);

    return 0;
}
