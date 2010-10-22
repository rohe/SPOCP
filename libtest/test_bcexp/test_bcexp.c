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

spocp_result_t foo_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  r = SPOCP_SUCCESS;
    
    return r;
}

plugin_t    foo_module = {
	SPOCP20_PLUGIN_STUFF,
	foo_test,
	NULL,
	NULL,
	NULL
};

/* ==================================================================== */

/*
 * test function 
 */
static unsigned int test_parse_bcexp_bcondref(void);
static unsigned int test_parse_bcexp_bcondand(void);
static unsigned int test_parse_bcexp_bcondnot(void);
static unsigned int test_parse_bcexp_bcondspec(void);
static unsigned int test_parse_transv_stree(void);

test_conf_t tc[] = {
    {"test_parse_bcexp_bcondref", test_parse_bcexp_bcondref},
    {"test_parse_bcexp_bcondand", test_parse_bcexp_bcondand},
    {"test_parse_bcexp_bcondnot", test_parse_bcexp_bcondnot},
    {"test_parse_bcexp_bcondspec", test_parse_bcexp_bcondspec},
    {"test_parse_transv_stree", test_parse_transv_stree},
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

static unsigned int test_parse_bcexp_bcondref(void)
{
    octet_t     o;
    bcstree_t   *bcsp, *pp, *np;
    
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

static unsigned int test_parse_bcexp_bcondand(void)
{
    octet_t     o;
    bcstree_t   *bcsp, *pp, *np, *pnp;
    oct_assign(&o, "(3:and(3:ref3:foo)(3:ref3:bar))");
    bcsp = parse_bcexp(&o);

    TEST_ASSERT(bcsp != NULL);
    TEST_ASSERT_EQUALS(bcsp->list, 1);
    pp = bcsp->part;
    TEST_ASSERT(pp->part == NULL);
    TEST_ASSERT_EQUALS(pp->list, 0);
    TEST_ASSERT(oct2strcmp( &pp->val, "and") == 0);
    np = pp->next;
    /* np represents (3:ref3:foo) */
    TEST_ASSERT_EQUALS(np->list, 1);
    pnp = np->part;
    TEST_ASSERT(pnp->part == NULL);
    TEST_ASSERT_EQUALS(pnp->list, 0);
    TEST_ASSERT(oct2strcmp( &pnp->val, "ref") == 0);
    pnp = pnp->next;
    TEST_ASSERT_EQUALS(pnp->list, 0);
    TEST_ASSERT(oct2strcmp( &pnp->val, "foo") == 0);
    np = np->next;
    /* np represents (3:ref3:bar) */
    TEST_ASSERT_EQUALS(np->list, 1);
    pnp = np->part;
    TEST_ASSERT(pnp->part == NULL);
    TEST_ASSERT_EQUALS(pnp->list, 0);
    TEST_ASSERT(oct2strcmp( &pnp->val, "ref") == 0);
    pnp = pnp->next;
    TEST_ASSERT_EQUALS(pnp->list, 0);
    TEST_ASSERT(oct2strcmp( &pnp->val, "bar") == 0);
    
    return 0;
}

static unsigned int test_parse_bcexp_bcondnot(void)
{
    octet_t     o;
    bcstree_t   *bcsp, *pp, *np, *pnp;
    oct_assign(&o, "(3:not(3:ref3:bar))");
    bcsp = parse_bcexp(&o);
    
    TEST_ASSERT(bcsp != NULL);
    TEST_ASSERT_EQUALS(bcsp->list, 1);
    pp = bcsp->part;
    TEST_ASSERT(pp->part == NULL);
    TEST_ASSERT_EQUALS(pp->list, 0);
    TEST_ASSERT(oct2strcmp( &pp->val, "not") == 0);
    np = pp->next;
    /* np represents (3:ref3:foo) */
    TEST_ASSERT_EQUALS(np->list, 1);
    pnp = np->part;
    TEST_ASSERT(pnp->part == NULL);
    TEST_ASSERT_EQUALS(pnp->list, 0);
    TEST_ASSERT(oct2strcmp( &pnp->val, "ref") == 0);
    pnp = pnp->next;
    TEST_ASSERT_EQUALS(pnp->list, 0);
    TEST_ASSERT(oct2strcmp( &pnp->val, "bar") == 0);
    
    return 0;
}

static unsigned int test_parse_bcexp_bcondspec(void)
{
    octet_t     o;
    bcstree_t   *bcsp;
    
    oct_assign(&o, "3:foo1:6:xxxxxx");
    bcsp = parse_bcexp(&o);
    
    TEST_ASSERT(bcsp != NULL);
    TEST_ASSERT_EQUALS(bcsp->list, 0);
    TEST_ASSERT(oct2strcmp( &bcsp->val, "foo") == 0);
    TEST_ASSERT(bcsp->part == NULL);
    TEST_ASSERT(bcsp->next == NULL);
    
    return 0;
}

static unsigned int test_parse_transv_stree(void)
{
    octet_t     o;
    bcstree_t   *bcsp;
    bcexp_t     *bep;
    bcdef_t     *bclist = NULL;
    plugin_t    *plugp = NULL;
    bcspec_t    *bcs;

    oct_assign(&o, "(3:not(3:ref3:bar))");
    bcsp = parse_bcexp(&o);
    
    bep = transv_stree(plugp, bcsp, bclist, NULL);
    TEST_ASSERT( bep == NULL);
    
    plugp = plugin_load(plugp, "foo", NULL, &foo_module);
    
    /* Try again */
    oct_assign(&o, "27:foo:{//uid[1]}:foxtrot:${0}");
    bcsp = parse_bcexp(&o);
    bep = transv_stree(plugp, bcsp, bclist, NULL);
    TEST_ASSERT_EQUALS( bep->type, 5 );
    TEST_ASSERT( bep->parent == 0 );
    bcs = bep->val.spec;
    TEST_ASSERT( strcmp(bcs->name, "foo") == 0);
    TEST_ASSERT( oct2strcmp(bcs->spec, "{//uid[1]}:foxtrot:${0}") == 0 );
    
    return 0;
}
