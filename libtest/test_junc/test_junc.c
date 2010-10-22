/*
 *  test_junc.c
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

#include <branch.h>
#include <element.h>

#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_junc_new(void);
static unsigned int test_add_element(void);
static unsigned int test_add_atom_element(void);
static unsigned int test_add_list_element(void);
static unsigned int test_add_set_element(void);
static unsigned int test_add_range_element(void);
static unsigned int test_junc_free(void);

test_conf_t tc[] = {
    {"test_junc_new", test_junc_new},
    {"test_add_atom_element", test_add_atom_element},
    {"test_add_list_element", test_add_list_element},
    {"test_add_set_element", test_add_set_element},
    {"test_add_range_element", test_add_range_element},
    {"test_add_element", test_add_element},
    {"test_junc_free", test_junc_free},
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

static unsigned int test_junc_new(void)
{    
    junc_t *jp;

    jp = junc_new();

    TEST_ASSERT(jp != NULL);
    
    return 0;
}

char *typerule[] = {
    "5:style",
    "(3:foo3:bar5:queue)",
    "(1:*3:set1:A1:B1:C)",
    "(1:*6:prefix3:pro)",
    "(1:*6:suffix3:ing)",
    "(1:*5:range5:alpha2:le3:ABC)",
    "()",
    NULL
};

int sexptype[] = {
    SPOC_ATOM,
    SPOC_LIST,
    SPOC_SET,
    SPOC_PREFIX,
    SPOC_SUFFIX,
    SPOC_RANGE,
    SPOC_ANY
} ;

static unsigned int test_add_atom_element(void)
{    
    junc_t          *jp, *head;
    element_t       *ep = NULL;
    ruleinst_t      *ri;
    octet_t         rule;
    int             n = 0;
    spocp_result_t  r;
    
    head = junc_new();
    oct_assign(&rule, typerule[n]);
    ri = ruleinst_new(&rule, NULL, NULL);
    r = get_element(&rule, &ep);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT(ep->type == sexptype[n]);
    jp = add_element(NULL, head, ep, ri, 0);    
    TEST_ASSERT(jp != NULL);

    junc_free(head);

    return 0;
}

static unsigned int test_add_list_element(void)
{    
    junc_t          *jp, *head;
    element_t       *ep = NULL;
    ruleinst_t      *ri;
    octet_t         rule;
    int             n = 1;
    spocp_result_t  r;
    
    head = junc_new();
    oct_assign(&rule, typerule[n]);
    ri = ruleinst_new(&rule, NULL, NULL);
    r = get_element(&rule, &ep);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT(ep->type == sexptype[n]);
    jp = add_element(NULL, head, ep, ri, 0);    
    TEST_ASSERT(jp != NULL);
    
    junc_free(head);

    return 0;
}

static unsigned int test_add_set_element(void)
{    
    junc_t          *jp, *head;
    element_t       *ep = NULL;
    ruleinst_t      *ri;
    octet_t         rule;
    int             n = 2;
    spocp_result_t  r;
    
    /* printf("(test_add_set_element)\n"); */
    head = junc_new();
    oct_assign(&rule, typerule[n]);
    ri = ruleinst_new(&rule, NULL, NULL);
    r = get_element(&rule, &ep);
    /* printf("(test_add_set_element) get_element: %d\n", r); */
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT(ep->type == sexptype[n]);
    /* printf("(test_add_set_element) -> add_element\n"); */
    jp = add_element(NULL, head, ep, ri, 0);    
    TEST_ASSERT(jp != NULL);
    
    junc_free(head);

    return 0;
}

static unsigned int test_add_range_element(void)
{    
    junc_t          *jp, *head;
    element_t       *ep = NULL;
    ruleinst_t      *ri;
    octet_t         rule;
    int             n = 5;
    spocp_result_t  r;
    
    head = junc_new();
    oct_assign(&rule, typerule[n]);
    ri = ruleinst_new(&rule, NULL, NULL);
    r = get_element(&rule, &ep);
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    TEST_ASSERT(ep->type == sexptype[n]);
    jp = add_element(NULL, head, ep, ri, 0);    
    TEST_ASSERT(jp != NULL);
    
    junc_free(head);

    return 0;
}

static unsigned int test_add_element(void)
{    
    junc_t          *jp, *head;
    element_t       *ep = NULL;
    ruleinst_t      *ri;
    octet_t         rule;
    branch_t        *bp;
    int             n;
    spocp_result_t  r;
    
    head = junc_new();
    oct_assign(&rule, typerule[0]);
    ri = ruleinst_new(&rule, NULL, NULL);
    get_element(&rule, &ep);
    jp = add_element(NULL, head, ep, ri, 0);
    
    TEST_ASSERT(jp != NULL);
    
    bp = ARRFIND(head, SPOC_ATOM);
    TEST_ASSERT(bp != NULL);
    bp = ARRFIND(head, SPOC_PREFIX);
    TEST_ASSERT(bp == NULL);
    
    for (n = 1; typerule[n]; n++) {    
        /*printf("(test_add_element) typerule: %s\n", typerule[n]);*/
        oct_assign(&rule, typerule[n]);
        ri = ruleinst_new(&rule, NULL, NULL);
        ep = NULL;
        r = get_element(&rule, &ep);
        /* printf("(test_add_element) returned:%d\n", r); */
        TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
        TEST_ASSERT(ep->type == sexptype[n]);
        jp = add_element(NULL, head, ep, ri, 0);
        TEST_ASSERT(jp != NULL);
    }
    bp = ARRFIND(head, SPOC_ATOM);
    TEST_ASSERT(bp != NULL);
    bp = ARRFIND(head, SPOC_LIST);
    TEST_ASSERT(bp != NULL);
    bp = ARRFIND(head, SPOC_SET);
    TEST_ASSERT(bp != NULL);
    bp = ARRFIND(head, SPOC_PREFIX);
    TEST_ASSERT(bp != NULL);
    bp = ARRFIND(head, SPOC_SUFFIX);
    TEST_ASSERT(bp != NULL);
    bp = ARRFIND(head, SPOC_RANGE);
    TEST_ASSERT(bp != NULL);
    bp = ARRFIND(head, SPOC_ANY);
    TEST_ASSERT(bp != NULL);
    
    return 0;
}


static unsigned int test_junc_free(void)
{    
    junc_t          *jp, *head;
    element_t       *ep = NULL;
    ruleinst_t      *ri;
    octet_t         rule;
    int             n;
    spocp_result_t  r;
    
    head = junc_new();
    /* printf("(test_junc_free) %p\n", head); */
    
    for (n = 0; typerule[n]; n++) {    
        /* printf("n: %d\n", n); */
        oct_assign(&rule, typerule[n]);
        ri = ruleinst_new(&rule, NULL, NULL);
        ep = NULL;
        r = get_element(&rule, &ep);
        jp = add_element(NULL, head, ep, ri, 0);
    }

    /* printf("--- junc_free %p ---\n", head); */
    junc_free(head);
    /* printf("+++\n"); */
    TEST_ASSERT(1);
    return 0;
}

