/*
 *  test_element.c
 *
 *  Created by Roland Hedberg on 12/3/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

/* test program for lib/element.c */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <element.h>
#include <range.h>
#include <spocp.h>
#include <proto.h>
#include <wrappers.h>
#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_element_new(void);
static unsigned int test_do_prefix(void);
static unsigned int test_do_suffix(void);
static unsigned int test_do_set(void);
static unsigned int test_do_range(void);
static unsigned int test_element_get(void);



test_conf_t tc[] = {
    {"test_element_new", test_element_new},
    {"test_do_prefix", test_do_prefix},
    {"test_do_suffix", test_do_suffix},
    {"test_do_set", test_do_set},
    {"test_do_range", test_do_range},
    {"test_element_get", test_element_get},
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

static unsigned int test_element_new(void)
{    
    element_t *ep;

    ep = element_new();

    TEST_ASSERT(ep->next == 0);
    TEST_ASSERT(ep->memberof == 0);
    TEST_ASSERT(ep->e.atom == (atom_t *) 0);
    
    return 0;
}

static unsigned int test_do_prefix(void)
{    
    element_t       *ep;
    octet_t         oct;
    spocp_result_t  r;
    /*
    char            *s;
     */
    
    ep = Calloc(1, sizeof(element_t));
    
    oct_assign(&oct, "6:foobar)");
    r = do_prefix(&oct, ep);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(ep->type, SPOC_PREFIX);
    /*
     s = atom2str(ep->e.atom);
     printf("[test_do_prefix] atom: %s", s);
     Free(s);
     */
    TEST_ASSERT(oct2strcmp(&ep->e.atom->val, "foobar") == 0);
    
    return 0;
}

static unsigned int test_do_suffix(void)
{    
    element_t       *ep;
    octet_t         oct;
    spocp_result_t  r;
    /*
     char            *s;
     */
    
    ep = Calloc(1, sizeof(element_t));
    
    oct_assign(&oct, "6:foobar)");
    r = do_suffix(&oct, ep);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(ep->type, SPOC_SUFFIX);
    /*
     s = atom2str(ep->e.atom);
     printf("[test_do_prefix] atom: %s", s);
     Free(s);
     */
    TEST_ASSERT(oct2strcmp(&ep->e.atom->val, "foobar") == 0);
    
    return 0;
}

static unsigned int test_do_range(void)
{    
    element_t       *ep;
    octet_t         oct;
    spocp_result_t  r;
    range_t         *rp;
    
    ep = Calloc(1, sizeof(element_t));
    
    oct_assign(&oct, "5:alpha2:le3:ABC)");
    r = do_range(&oct, ep);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(ep->type, SPOC_RANGE);
    rp = ep->e.range;    
    TEST_ASSERT_EQUALS(rp->upper.type & 0xF0, LT|EQ);
    TEST_ASSERT_EQUALS(rp->upper.type & 0x0F, 2);
    TEST_ASSERT( oct2strcmp(&rp->upper.v.val, "ABC") == 0 );

    element_free(ep);
    ep = Calloc(1, sizeof(element_t));

    oct_assign(&oct, "7:numeric2:le3:4912:gt2:23)");
    r = do_range(&oct, ep);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(ep->type, SPOC_RANGE);
    rp = ep->e.range;    
    TEST_ASSERT_EQUALS(rp->upper.type & 0xF0, LT|EQ);
    TEST_ASSERT_EQUALS(rp->upper.type & 0x0F, SPOC_NUMERIC);
    TEST_ASSERT(rp->upper.v.num == 491);
    TEST_ASSERT_EQUALS(rp->lower.type & 0xF0, GT);
    TEST_ASSERT_EQUALS(rp->lower.type & 0x0F, SPOC_NUMERIC);
    TEST_ASSERT(rp->lower.v.num == 23);
    
    return 0;
}

static unsigned int test_do_set(void)
{    
    element_t       *ep, *nep;
    octet_t         oct;
    spocp_result_t  r;
    
    ep = Calloc(1, sizeof(element_t));
    
    oct_assign(&oct, "3:foo3:bar3:xyz)");
    r = do_set(&oct, ep);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(ep->type, SPOC_SET);
    TEST_ASSERT_EQUALS(varr_len(ep->e.set), 3);
    
    nep = varr_first(ep->e.set);
    TEST_ASSERT_EQUALS(nep->type, SPOC_ATOM);
    TEST_ASSERT(oct2strcmp(&nep->e.atom->val, "foo") == 0);
    
    nep = varr_next(ep->e.set, nep);
    TEST_ASSERT_EQUALS(nep->type, SPOC_ATOM);
    TEST_ASSERT(oct2strcmp(&nep->e.atom->val, "bar") == 0);
    
    nep = varr_next(ep->e.set, nep);
    TEST_ASSERT_EQUALS(nep->type, SPOC_ATOM);
    TEST_ASSERT(oct2strcmp(&nep->e.atom->val, "xyz") == 0);
    
    nep = varr_next(ep->e.set, nep);
    TEST_ASSERT(nep == 0);
    
    return 0;
}

static unsigned int test_element_get(void)
{    
    element_t       *ep = 0, *pep, *nep;
    octet_t         oct;
    range_t         *rp;
    spocp_result_t  r;
    
    oct_assign(&oct, "6:foobar");

    get_element(&oct, &ep);
    TEST_ASSERT_EQUALS(ep->type, SPOC_ATOM);
    TEST_ASSERT(oct2strcmp(&ep->e.atom->val, "foobar") == 0);
    
    element_free(ep);
    ep = NULL;
    
    oct_assign(&oct, "(3:foo3:bar)");
    
    get_element(&oct, &ep);
    TEST_ASSERT_EQUALS(ep->type, SPOC_LIST);

    pep = ep->e.list->head;
    TEST_ASSERT_EQUALS(pep->type, SPOC_ATOM);
    TEST_ASSERT(oct2strcmp(&pep->e.atom->val, "foo") == 0);
    
    pep = pep->next;
    TEST_ASSERT_EQUALS(pep->type, SPOC_ATOM);
    TEST_ASSERT(oct2strcmp(&pep->e.atom->val, "bar") == 0);
    TEST_ASSERT(pep->next == 0);
    
    element_free(ep);
    ep = NULL;
    oct_assign(&oct, "(1:*6:prefix5:alpha)");
    
    get_element(&oct, &ep);
    TEST_ASSERT_EQUALS(ep->type, SPOC_PREFIX);    
    TEST_ASSERT(oct2strcmp(&ep->e.atom->val, "alpha") == 0);

    element_free(ep);
    ep = NULL;
    oct_assign(&oct, "(1:*6:suffix4:beta)");
    
    get_element(&oct, &ep);
    TEST_ASSERT_EQUALS(ep->type, SPOC_SUFFIX);    
    TEST_ASSERT(oct2strcmp(&ep->e.atom->val, "beta") == 0);

    element_free(ep);
    ep = NULL;
    oct_assign(&oct, "(1:*5:range5:alpha2:le3:ABC)");    
    get_element(&oct, &ep);
    TEST_ASSERT_EQUALS(ep->type, SPOC_RANGE);
    rp = ep->e.range;    
    TEST_ASSERT_EQUALS(rp->upper.type & 0xF0, LT|EQ);
    TEST_ASSERT_EQUALS(rp->upper.type & 0x0F, 2);
    TEST_ASSERT( oct2strcmp(&rp->upper.v.val, "ABC") == 0 );
    
    element_free(ep);
    ep = NULL;
    oct_assign(&oct, "(1:*5:range7:numeric2:le3:4912:gt2:23)");
    get_element(&oct, &ep);
    TEST_ASSERT_EQUALS(ep->type, SPOC_RANGE);
    rp = ep->e.range;    
    TEST_ASSERT_EQUALS(rp->upper.type & 0xF0, LT|EQ);
    TEST_ASSERT_EQUALS(rp->upper.type & 0x0F, SPOC_NUMERIC);
    TEST_ASSERT(rp->upper.v.num == 491);
    TEST_ASSERT_EQUALS(rp->lower.type & 0xF0, GT);
    TEST_ASSERT_EQUALS(rp->lower.type & 0x0F, SPOC_NUMERIC);
    TEST_ASSERT(rp->lower.v.num == 23);
    
    /* Error handling */
    
    element_free(ep);
    ep = NULL;
    oct_assign(&oct, "(1:*4:range7:numeric2:le3:4912:gt2:23)");
    r = get_element(&oct, &ep);
    TEST_ASSERT( r != SPOCP_SUCCESS);

    oct_assign(&oct, "(1:*5:soffa3:bed)");
    r = get_element(&oct, &ep);
    TEST_ASSERT(r != SPOCP_SUCCESS);
    TEST_ASSERT(ep == NULL);

    /* this expression doesn't make any sense */
    
    oct_assign(&oct, "(1:*6:suffix(1:*3:set1:A1:B))");
    r = get_element(&oct, &ep);
    TEST_ASSERT(r != SPOCP_SUCCESS);
    TEST_ASSERT(ep == NULL);
    
    /* reasonably complex */

    oct_assign(&oct, "(1:*3:set(1:*6:suffix1:A)(1:*6:suffix1:B))");
    r = get_element(&oct, &ep);
    TEST_ASSERT(r == SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(ep->type, SPOC_SET);
    TEST_ASSERT_EQUALS(varr_len(ep->e.set), 2);
    
    nep = varr_first(ep->e.set);
    TEST_ASSERT_EQUALS(nep->type, SPOC_SUFFIX);
    TEST_ASSERT(oct2strcmp(&nep->e.atom->val, "A") == 0);

    nep = varr_next(ep->e.set, nep);
    TEST_ASSERT_EQUALS(nep->type, SPOC_SUFFIX);
    TEST_ASSERT(oct2strcmp(&nep->e.atom->val, "B") == 0);
    
    return 0;
}



