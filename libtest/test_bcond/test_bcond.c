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
#include <bcondfunc.h>
#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_bcdef_add(void);
static unsigned int test_bcdef_del(void);
static unsigned int test_bcdef_get(void);
static unsigned int test_make_hash(void);
/*static unsigned int test_bcexp_eval(void); */
static unsigned int test_bcond_users(void);

test_conf_t tc[] = {
    {"test_bcdef_add", test_bcdef_add},
    {"test_bcdef_del", test_bcdef_del},
    {"test_bcdef_get", test_bcdef_get},
    {"test_make_hash", test_make_hash},
/*    {"test_bcexp_eval", test_bcexp_eval},*/
    {"test_bcond_users", test_bcond_users},
    {"",NULL}
};

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

/* ==================================================================== */

int main( int argc, char **argv )
{
    test_conf_t *ptc;
    
    printf("---------------------------------------------------\n");
    for( ptc = tc; ptc->func ; ptc++)
        run_test(ptc);
    printf("---------------------------------------------------\n");

    return 0;
}

static unsigned int test_bcdef_add(void)
{
    octet_t     bcond, namn;
    bcdef_t     *top = NULL, *bcdp;
    bcexp_t     *bep;
    plugin_t    *pp = NULL;
    bcspec_t    *bsp;
    
    oct_assign(&bcond, "true:{//uid[1]}:foxtrot:${0}");
    bcdp = bcdef_add(&top, pp, NULL, NULL, &bcond);
    
    /* Since the plugin refered to isn't available */
    TEST_ASSERT(bcdp == NULL);
    
    pp = plugin_load(pp, "true", NULL, &true_module);

    /* Try again */
    oct_assign(&bcond, "true:{//uid[1]}:foxtrot:${0}");
    oct_assign(&namn, "foox");
    bcdp = bcdef_add(&top, pp, NULL, &namn, &bcond);
    
    /* Since the plugin refered to is available */
    TEST_ASSERT(bcdp != NULL);
    TEST_ASSERT( strcmp( bcdp->name, "foox") == 0 );
    bep = bcdp->exp;
    TEST_ASSERT_EQUALS(bep->type, 5); /* says it's a specification*/
    bsp = bep->val.spec;
    TEST_ASSERT( strcmp( bsp->name, "true") == 0);
    TEST_ASSERT( bsp->plugin->test == true_test);

    /* And another using the same plugin */
    oct_assign(&bcond, "true:{//time[1]}:tango:${0}");
    oct_assign(&namn, "slowpox");
    bcdp = bcdef_add(&top, pp, NULL, &namn, &bcond);

    TEST_ASSERT(bcdp != NULL);
    TEST_ASSERT(bcdp != top);
    TEST_ASSERT(bcdp->prev == top);
    TEST_ASSERT(bcdp == top->next);
    
    oct_assign(&bcond, "(3:ref7:slowpox)");
    bcdp = bcdef_add(&top, pp, NULL, NULL, &bcond);
    TEST_ASSERT_EQUALS( bcdp->exp->type, 4);
    TEST_ASSERT(strcmp(bcdp->exp->val.ref->name, "slowpox") == 0 );
    
    return 0;
}

static unsigned int test_bcdef_del(void)
{    
    octet_t     bcond, namn;
    bcdef_t     *top = NULL, *bcdp;
    plugin_t    *pp = NULL;
    
    pp = plugin_load(pp, "true", NULL, &true_module);
    
    oct_assign(&bcond, "true:{//uid[1]}:foxtrot:${0}");
    oct_assign(&namn, "foox");
    bcdp = bcdef_add(&top, pp, NULL, &namn, &bcond);

    oct_assign(&bcond, "true:{//time[1]}:tango:${0}");
    oct_assign(&namn, "slowpox");
    bcdp = bcdef_add(&top, pp, NULL, &namn, &bcond);

    oct_assign(&bcond, "true:{//date[1]}:pavan:${0}");
    oct_assign(&namn, "perky");
    bcdp = bcdef_add(&top, pp, NULL, &namn, &bcond);
    
    /* remove the one in the middle */
    oct_assign(&namn, "slowpox");
    bcdef_del(&top, NULL, &namn);
    
    TEST_ASSERT( strcmp( top->name, "foox") == 0 );
    TEST_ASSERT( strcmp( top->next->name, "perky") == 0 );
    TEST_ASSERT( strcmp( top->next->prev->name, "foox") == 0 );

    /* remove the first one */
    oct_assign(&namn, "foox");
    bcdef_del(&top, NULL, &namn);
    TEST_ASSERT( strcmp( top->name, "perky") == 0 );
    TEST_ASSERT( top->next == 0 );
    
    return 0;
}

static unsigned int test_bcdef_get(void)
{    
    octet_t         bcond, namn, ref;
    bcdef_t         *top = NULL, *bcdp;
    plugin_t        *pp = NULL;
    spocp_result_t  rc;
    
    pp = plugin_load(pp, "true", NULL, &true_module);
    
    oct_assign(&bcond, "true:{//uid[1]}:foxtrot:${0}");
    oct_assign(&namn, "foox");
    bcdp = bcdef_add(&top, pp, NULL, &namn, &bcond);
    
    oct_assign(&bcond, "true:{//time[1]}:tango:${0}");
    oct_assign(&namn, "slowpox");
    bcdp = bcdef_add(&top, pp, NULL, &namn, &bcond);
    
    oct_assign(&bcond, "true:{//date[1]}:pavan:${0}");
    oct_assign(&namn, "perky");
    bcdp = bcdef_add(&top, pp, NULL, &namn, &bcond);

    oct_assign(&ref, "(3:ref7:slowpox)");
    bcdp = bcdef_get(&top, pp, NULL, &ref, &rc);
    
    TEST_ASSERT( strcmp( bcdp->name, "slowpox") == 0 );
    TEST_ASSERT_EQUALS( bcdp->exp->type, 5 );
    TEST_ASSERT( strcmp( bcdp->exp->val.spec->name, "true") == 0 );
    TEST_ASSERT( oct2strcmp( bcdp->exp->val.spec->spec, 
                            "{//time[1]}:tango:${0}") == 0 );
    
    plugin_unload(pp);
    
    return 0;
}

/*
static unsigned int test_bcexp_eval(void)
{    
    plugin_t        *plug = NULL;
    octet_t         query, rule, bcond, namn;
    bcdef_t         *top = NULL, *bcdp;
    octarr_t        *oa=NULL;
    element_t       *eq, *er;
    spocp_result_t  rc;

    * load some plugins, actually the same though under different name *
    plug = plugin_load(plug, "true", NULL, &true_module);
    plug = plugin_load(plug, "false", NULL, &true_module);

    * A couple of boundary condition definitions *
    oct_assign(&bcond, "true:{//uid[1]}:foxtrot:${0}");
    oct_assign(&namn, "foxtrot");
    bcdp = bcdef_add(&top, plug, NULL, &namn, &bcond);
    
    oct_assign(&bcond, "true:{//time[1]}:tango:${0}");
    oct_assign(&namn, "slowpox");
    bcdp = bcdef_add(&top, plug, NULL, &namn, &bcond);
    
    oct_assign(&bcond, "false:{//date[1]}:pavan:${0}");
    oct_assign(&namn, "perky");
    bcdp = bcdef_add(&top, plug, NULL, &namn, &bcond);
    
    oct_assign(&query,"(4:rule(3:uid8:fire0001))"); 
    oct_assign(&rule,"(4:rule(3:uid))");     
    get_element(&query, &eq);
    get_element(&rule, &er);
    
    rc = bcexp_eval(eq, er, bcdp->exp, &oa);
    
    TEST_ASSERT(rc == SPOCP_SUCCESS);
    
    return 0;
}
*/

static unsigned int test_make_hash(void)
{    
    char    hash[43];
    octet_t oct;
    
    memset(hash,0,43);
    oct_assign(&oct, "TEST message");
    make_hash(hash, &oct);
    
    /* I don't know what the created hash will be, just that it's
     something */
    
    TEST_ASSERT(strcmp(hash,
                       "_a4253aace4b1d3d2b865bbb1bd4a163a32f57dd_") == 0);
    return 0;
}

static unsigned int test_bcond_users(void)
{    
    return 0;
}

