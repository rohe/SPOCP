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
static unsigned int test_element_type(void);
static unsigned int test_element_data(void);
static unsigned int test_element_size(void);
static unsigned int test_element_nth(void);
static unsigned int test_element_cmp(void);

test_conf_t tc[] = {
    {"test_element_type", test_element_type},
    {"test_element_data", test_element_data},
    {"test_element_size", test_element_size},
    {"test_element_nth", test_element_nth},
    {"test_element_cmp", test_element_cmp},
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

static unsigned int test_element_type(void)
{    
    element_t   *ep;
    octet_t     oct;

    ep = element_new();

    TEST_ASSERT(element_type(ep) == 0);
    
    ep->type = SPOC_ATOM;    
    TEST_ASSERT(element_type(ep) == SPOC_ATOM);
    element_free(ep);
    ep = NULL;

    /* --------------------------------- */
    oct_assign(&oct, "6:foobar");    
    get_element(&oct, &ep);    
    TEST_ASSERT(element_type(ep) == SPOC_ATOM);
    element_free(ep);
    ep = NULL;
    
    /* --------------------------------- */
    oct_assign(&oct, "(3:foo3:bar)");    
    get_element(&oct, &ep);
    TEST_ASSERT(element_type(ep) == SPOC_LIST);
    
    return 0;
}

static unsigned int test_element_data(void)
{    
    element_t   *ep, *elem;
    octet_t     oct;
    atom_t      *ap;

    oct_assign(&oct, "6:foobar");    
    get_element(&oct, &ep);    
    ap = (atom_t *) element_data(ep);
    TEST_ASSERT( oct2strcmp(&ap->val, "foobar") == 0 );
    element_free(ep);
    ep = NULL;
    
    /* --------------------------------- */
    oct_assign(&oct, "(3:foo3:bar)");    
    get_element(&oct, &ep);
    elem = (element_t *) element_data(ep);
    TEST_ASSERT(element_type(elem) == SPOC_ATOM);
    ap = (atom_t *) element_data(elem);
    TEST_ASSERT( oct2strcmp(&ap->val, "foo") == 0 );
    elem = elem->next;
    TEST_ASSERT(element_type(elem) == SPOC_ATOM);
    ap = (atom_t *) element_data(elem);
    TEST_ASSERT( oct2strcmp(&ap->val, "bar") == 0 );
    
    return 0;
}

static unsigned int test_element_size(void)
{    
    element_t   *ep=NULL;
    octet_t     oct;

    oct_assign(&oct, "6:foobar");    
    get_element(&oct, &ep);    
    TEST_ASSERT_EQUALS( element_size(ep), 1);
    element_free(ep);
    ep = NULL;

    oct_assign(&oct, "(3:foo3:bar)");    
    get_element(&oct, &ep);
    TEST_ASSERT_EQUALS( element_size(ep), 2);

    element_free(ep);
    ep = NULL;
    
    oct_assign(&oct, "(1:*3:set3:foo3:bar3:toe4:tick4:tack)");    
    get_element(&oct, &ep);
    TEST_ASSERT_EQUALS( element_size(ep), 5);
    
    return 0;
}

char *vals[] = { "foo", "bar", "toe", "tick", "tack", NULL };

static unsigned int test_element_nth(void)
{    
    element_t   *ep=NULL, *elem;
    octet_t     oct;
    atom_t      *ap;
    int         n, i, m;
    
    oct_assign(&oct, "6:foobar");    
    get_element(&oct, &ep);    
    TEST_ASSERT( element_nth(ep, 0) ==  ep);
    TEST_ASSERT( element_nth(ep, 1) ==  NULL);
    
    element_free(ep);
    ep = NULL;
    
    oct_assign(&oct, "(3:foo3:bar)");
    get_element(&oct, &ep);
    elem = element_nth(ep, 0);
    TEST_ASSERT(element_type(elem) == SPOC_ATOM);
    ap = (atom_t *) element_data(elem);
    TEST_ASSERT( oct2strcmp(&ap->val, "foo") == 0 );
    elem = element_nth(ep, 1);
    TEST_ASSERT(element_type(elem) == SPOC_ATOM);
    ap = (atom_t *) element_data(elem);
    TEST_ASSERT( oct2strcmp(&ap->val, "bar") == 0 );
    elem = element_nth(ep, 2);
    TEST_ASSERT( elem == NULL);
    
    element_free(ep);
    ep = NULL;
    
    oct_assign(&oct, "(1:*3:set3:foo3:bar3:toe4:tick4:tack)");    
    get_element(&oct, &ep);
    /* set is unordered so I don't know which is the first */
    for (n = 0, m = 0; 1; n++) {
        elem = element_nth(ep, n);
        if (elem == NULL)
            break;
        ap = (atom_t *) element_data(elem);
        for (i = 0; vals[i]; i++) {
            if (oct2strcmp(&ap->val, vals[i]) == 0) {
                m++;
            }
        }
    }
    TEST_ASSERT_EQUALS(m, 5);
    
    return 0;
}

static unsigned int test_element_cmp(void)
{    
    element_t       *ep[2];
    octet_t         octa, octb;
    spocp_result_t  rc;
    
    oct_assign(&octa, "6:foobar");    
    get_element(&octa, &ep[0]);    
    oct_assign(&octb, "6:foobar");
    get_element(&octb, &ep[1]);    

    TEST_ASSERT( element_cmp(ep[0], ep[1], &rc) == 0 );

    oct_assign(&octb, "3:foo");
    get_element(&octb, &ep[1]);    

    TEST_ASSERT( element_cmp(ep[0], ep[1], &rc) != 0 );

    oct_assign(&octa, "(3:foo3:bar)");
    get_element(&octa, &ep[0]);
    oct_assign(&octb, "(3:foo3:bar)");
    get_element(&octb, &ep[1]);

    TEST_ASSERT( element_cmp(ep[0], ep[1], &rc) == 0 );

    oct_assign(&octb, "(3:foo3:bax)");
    get_element(&octb, &ep[1]);

    TEST_ASSERT( element_cmp(ep[0], ep[1], &rc) != 0 );

    return 0;
}
