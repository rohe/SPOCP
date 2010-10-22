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
#include <rvapi.h>
#include <spocp.h>
#include <proto.h>
#include <wrappers.h>
#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_element_new_atom(void);
static unsigned int test_atoms_join(void);
static unsigned int test_element_list_add(void);
static unsigned int test_element_first(void);
static unsigned int test_element_last(void);
static unsigned int test_element_reverse(void);
static unsigned int test_parse_path(void);
static unsigned int test_parse_intervall(void);
static unsigned int test_element_eval(void);
static unsigned int test_element_atom_sub(void);

test_conf_t tc[] = {
    {"test_element_new_atom", test_element_new_atom},
    {"test_atoms_join", test_atoms_join},
    {"test_element_list_add", test_element_list_add},
    {"test_element_first", test_element_first},
    {"test_element_last", test_element_last},
    {"test_element_reverse", test_element_reverse},
    {"test_parse_path", test_parse_path},
    {"test_parse_intervall", test_parse_intervall},
    {"test_element_eval", test_element_eval},
    {"test_element_atom_sub", test_element_atom_sub},
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

static unsigned int test_element_new_atom(void)
{    
    element_t   *ep;
    octet_t     oct;
    atom_t      *ap;
    
    oct_assign(&oct, "foo");
    ep = element_new_atom(&oct);

    TEST_ASSERT(ep->type == SPOC_ATOM);
    ap = ep->e.atom;
    TEST_ASSERT(octcmp(&ap->val, &oct) == 0);
    
    return 0;
}

static unsigned int test_atoms_join(void)
{    
    element_t   *ep=NULL;
    octet_t     oct, *op;
    
    oct_assign(&oct, "(1:*3:set5:alpha4:beta5:gamma)");
    get_element(&oct, &ep);

    op = atoms_join(ep, "-");
    TEST_ASSERT(oct2strcmp(op, "alpha-beta-gamma") == 0);
    oct_free(op);
    op = atoms_join(ep, "..");
    TEST_ASSERT(oct2strcmp(op, "alpha..beta..gamma") == 0);
    
    return 0;
}

static unsigned int test_element_list_add(void)
{    
    element_t   *ep, *elp=NULL;
    octet_t     oct;
    
    oct_assign(&oct, "3:foo");
    get_element(&oct, &ep);

    elp = element_list_add(elp, ep);
    TEST_ASSERT( elp != NULL );
    TEST_ASSERT_EQUALS( element_size(elp), 1);

    oct_assign(&oct, "4:bark");
    get_element(&oct, &ep);

    elp = element_list_add(elp, ep);
    TEST_ASSERT_EQUALS( element_size(elp), 2);
    
    return 0;
}

static unsigned int test_element_first(void)
{    
    element_t       *ep[2], *elp=NULL, *elem;
    octet_t         oct;
    spocp_result_t  rc;
    
    oct_assign(&oct, "3:foo");
    get_element(&oct, &ep[0]);    
    elp = element_list_add(elp, ep[0]);
    
    oct_assign(&oct, "4:bark");
    get_element(&oct, &ep[1]);
    elp = element_list_add(elp, ep[1]);
    
    elem = element_first(elp);
    TEST_ASSERT( element_cmp(elem, ep[0], &rc) == 0 ) ;
    TEST_ASSERT( element_cmp(elem, ep[1], &rc) != 0 ) ;
    
    return 0;
}

static unsigned int test_element_last(void)
{    
    element_t       *ep[2], *elp=NULL, *elem;
    octet_t         oct;
    spocp_result_t  rc;
    
    oct_assign(&oct, "3:foo");
    get_element(&oct, &ep[0]);    
    elp = element_list_add(elp, ep[0]);
    
    oct_assign(&oct, "4:bark");
    get_element(&oct, &ep[1]);
    elp = element_list_add(elp, ep[1]);
    
    elem = element_last(elp);
    TEST_ASSERT( element_cmp(elem, ep[0], &rc) != 0 ) ;
    TEST_ASSERT( element_cmp(elem, ep[1], &rc) == 0 ) ;
    
    return 0;
}

char *ATOM[] = {
    "3:one",
    "3:two",
    "5:three",
    "4:four",
    NULL
};

static unsigned int test_element_reverse(void)
{    
    element_t       *ep[5], *elp=NULL, *elem;
    octet_t         oct;
    spocp_result_t  rc;
    int             n;
    
    for (n = 0; ATOM[n] != NULL; n++) {
        oct_assign(&oct, ATOM[n]);
        get_element(&oct, &ep[n]);    
        elp = element_list_add(elp, ep[n]);
    }
        
    elem = element_first(elp);
    TEST_ASSERT( element_cmp(elem, ep[0], &rc) == 0 ) ;
    elem = element_last(elp);
    TEST_ASSERT( element_cmp(elem, ep[n-1], &rc) == 0 ) ;
    
    element_print(&oct, elp);
    TEST_ASSERT( oct2strcmp(&oct, "(3:one3:two5:three4:four)") == 0 );
    element_reverse(elp);
    oct.len = oct.size = 0;
    element_print(&oct, elp);
    TEST_ASSERT( oct2strcmp(&oct, "(4:four5:three3:two3:one)") == 0 );

    elem = element_first(elp);
    TEST_ASSERT( element_cmp(elem, ep[n-1], &rc) == 0 ) ;
    elem = element_last(elp);
    TEST_ASSERT( element_cmp(elem, ep[0], &rc) == 0 ) ;

    element_reverse(elp);

    elem = element_first(elp);
    TEST_ASSERT( element_cmp(elem, ep[0], &rc) == 0 ) ;
    elem = element_last(elp);
    TEST_ASSERT( element_cmp(elem, ep[n-1], &rc) == 0 ) ;
    
    return 0;
}

static unsigned int test_parse_path(void)
{    
    octet_t     spec;
    octarr_t    *oa;
    
    oct_assign(&spec, "tcm");
    oa = parse_path(&spec);
    TEST_ASSERT_EQUALS(octarr_len(oa), 1);
    TEST_ASSERT( oct2strcmp(oa->arr[0], "tcm") == 0);
    octarr_free(oa);
    
    oct_assign(&spec, "tcm/foo");
    oa = parse_path(&spec);
    TEST_ASSERT_EQUALS(octarr_len(oa), 2);
    TEST_ASSERT( oct2strcmp(oa->arr[0], "tcm") == 0);
    TEST_ASSERT( oct2strcmp(oa->arr[1], "foo") == 0);

    oct_assign(&spec, "tcm/foo/Dennis/menance");
    oa = parse_path(&spec);
    TEST_ASSERT_EQUALS(octarr_len(oa), 4);
    TEST_ASSERT( oct2strcmp(oa->arr[0], "tcm") == 0);
    TEST_ASSERT( oct2strcmp(oa->arr[1], "foo") == 0);
    TEST_ASSERT( oct2strcmp(oa->arr[2], "Dennis") == 0);
    TEST_ASSERT( oct2strcmp(oa->arr[3], "menance") == 0);

    oct_assign(&spec, "tcm/foo*");
    oa = parse_path(&spec);
    TEST_ASSERT_EQUALS(octarr_len(oa), 2);
    TEST_ASSERT( oct2strcmp(oa->arr[0], "tcm") == 0);
    TEST_ASSERT( oct2strcmp(oa->arr[1], "foo*") == 0);

    oct_assign(&spec, "tcm/foo[2-3]");
    oa = parse_path(&spec);
    TEST_ASSERT_EQUALS(octarr_len(oa), 2);
    TEST_ASSERT( oct2strcmp(oa->arr[0], "tcm") == 0);
    TEST_ASSERT( oct2strcmp(oa->arr[1], "foo") == 0);
    TEST_ASSERT( oct2strcmp(&spec, "[2-3]") == 0);

    oct_assign(&spec, "tcm/foo[2-last]");
    oa = parse_path(&spec);
    TEST_ASSERT_EQUALS(octarr_len(oa), 2);
    TEST_ASSERT( oct2strcmp(oa->arr[0], "tcm") == 0);
    TEST_ASSERT( oct2strcmp(oa->arr[1], "foo") == 0);
    TEST_ASSERT( oct2strcmp(&spec, "[2-last]") == 0);
    
    oct_assign(&spec, "tcm/foo//xyz");
    oa = parse_path(&spec);
    TEST_ASSERT_EQUALS(octarr_len(oa), 2);
    TEST_ASSERT( oct2strcmp(oa->arr[0], "tcm") == 0);
    TEST_ASSERT( oct2strcmp(oa->arr[1], "foo") == 0);
    TEST_ASSERT( oct2strcmp(&spec, "//xyz") == 0);
    
    return 0;
}

static unsigned int test_parse_intervall(void)
{    
    octet_t spec;
    char    *str;
    int     start, end;

    oct_assign(&spec, "2]");
    str = parse_intervall(&spec, &start, &end);
    
    TEST_ASSERT_EQUALS(start, 2);
    TEST_ASSERT_EQUALS(end, 2);
    
    oct_assign(&spec, "1-2]");
    str = parse_intervall(&spec, &start, &end);
    
    TEST_ASSERT_EQUALS(start, 1);
    TEST_ASSERT_EQUALS(end, 2);
    
    oct_assign(&spec, "-3]");
    str = parse_intervall(&spec, &start, &end);
    
    TEST_ASSERT_EQUALS(start, 0);
    TEST_ASSERT_EQUALS(end, 3);

    oct_assign(&spec, "last]");
    str = parse_intervall(&spec, &start, &end);
    
    TEST_ASSERT_EQUALS(start, -1);
    TEST_ASSERT_EQUALS(end, -1);

    oct_assign(&spec, "-last]");
    str = parse_intervall(&spec, &start, &end);
    
    TEST_ASSERT_EQUALS(start, 0);
    TEST_ASSERT_EQUALS(end, -1);
    
    oct_assign(&spec, "2-last]");
    str = parse_intervall(&spec, &start, &end);
    
    TEST_ASSERT_EQUALS(start, 2);
    TEST_ASSERT_EQUALS(end, -1);
    
    /* And these are faulty */
    
    oct_assign(&spec, "last-2]");
    str = parse_intervall(&spec, &start, &end);
    TEST_ASSERT( str == 0 );

    oct_assign(&spec, "4-2]");
    str = parse_intervall(&spec, &start, &end);
    TEST_ASSERT( str == 0 );

    oct_assign(&spec, "-]");
    str = parse_intervall(&spec, &start, &end);
    TEST_ASSERT( str == 0 );
    
    return 0;
}

static unsigned int test_element_eval(void)
{    
    element_t       *ep, *elem;
    octet_t         spec, oct;
    spocp_result_t  rc;

    oct_assign(&oct, "(4:http(1:*3:set(3:get3:foo)(3:put3:bar)))");
    get_element(&oct, &ep);    
    oct_assign(&spec, "/http/get[1]");
    elem = element_eval(&spec, ep, &rc);
    TEST_ASSERT(elem != NULL);
    TEST_ASSERT_EQUALS(elem->type, SPOC_ATOM);
    TEST_ASSERT( oct2strcmp(&elem->e.atom->val, "foo") == 0 );
    
    return 0;
}

static unsigned int test_element_atom_sub(void)
{    
    element_t   *ep, *elp=NULL, *eap;
    octet_t     spec, oct, *op;
    int         n;
    
    oct_assign(&oct, "3:foo");
    get_element(&oct, &ep);    
    oct_assign(&spec, "${0}bar");
    
    op = element_atom_sub(&spec, ep);
    
    TEST_ASSERT( oct2strcmp(op, "foobar") == 0 );

    for (n = 0; ATOM[n] != NULL; n++) {
        oct_assign(&oct, ATOM[n]);
        get_element(&oct, &eap);    
        elp = element_list_add(elp, eap);
    }

    oct_assign(&spec, "${0}:${2}:${1}");    
    op = element_atom_sub(&spec, elp);
    TEST_ASSERT( oct2strcmp(op, "one:three:two") == 0 );
    oct_free(op);

    oct_assign(&spec, "http://${0}.com/${2}/${1}");    
    op = element_atom_sub(&spec, elp);    
    TEST_ASSERT( oct2strcmp(op, "http://one.com/three/two") == 0 );
    oct_free(op);

    oct_assign(&spec, "www.${9}.org");    
    op = element_atom_sub(&spec, elp);
    TEST_ASSERT( op == 0 );
    
    return 0;
}
