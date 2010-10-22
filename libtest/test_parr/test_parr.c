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
#include <log.h>
#include <parr.h>

#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_parr_new(void);
static unsigned int test_parr_add(void);
static unsigned int test_parr_add_nondup(void);
static unsigned int test_parr_dup(void);
static unsigned int test_parr_join(void);
static unsigned int test_parr_xor(void);
static unsigned int test_parr_common(void);
static unsigned int test_parr_or(void);
static unsigned int test_parr_and(void);
static unsigned int test_parr_find_index(void);
static unsigned int test_parr_find(void);
static unsigned int test_parr_get_index(void);
static unsigned int test_parr_rm_item(void);
static unsigned int test_parr_dec_refcount(void);

test_conf_t tc[] = {
    {"test_parr_new", test_parr_new},
    {"test_parr_add", test_parr_add},
    {"test_parr_add_nondup", test_parr_add_nondup},
    {"test_parr_dup", test_parr_dup},
    {"test_parr_join", test_parr_join},
    {"test_parr_xor", test_parr_xor},
    {"test_parr_common", test_parr_common},
    {"test_parr_or", test_parr_or},
    {"test_parr_and", test_parr_and},
    {"test_parr_find_index", test_parr_find_index},
    {"test_parr_find", test_parr_find},
    {"test_parr_get_index", test_parr_get_index},
    {"test_parr_rm_item", test_parr_rm_item},
    {"test_parr_dec_refcount", test_parr_dec_refcount},
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

static unsigned int test_parr_new(void)
{    
    parr_t *parr;

    parr = parr_new(4, P_strcmp, P_free, 0, 0);

    /* parr = parr_new(4, P_strcmp, P_free, P_strdup, 0);*/
    TEST_ASSERT_EQUALS(parr->n, 0);
    TEST_ASSERT_EQUALS(parr->size, 4);
    TEST_ASSERT(parr->vect != NULL);
    TEST_ASSERT(parr->refc != NULL);
    TEST_ASSERT(parr->cf == P_strcmp);
    TEST_ASSERT(parr->ff == P_free);
    TEST_ASSERT(parr->df == 0);
    TEST_ASSERT(parr->pf == 0);
    
    parr_free(parr);

    parr = parr_new(4, P_strcmp, P_free, P_strdup, 0);
    TEST_ASSERT_EQUALS(parr->n, 0);
    TEST_ASSERT_EQUALS(parr->size, 4);
    TEST_ASSERT(parr->vect != NULL);
    TEST_ASSERT(parr->refc != NULL);
    TEST_ASSERT(parr->cf == P_strcmp);
    TEST_ASSERT(parr->ff == P_free);
    TEST_ASSERT(parr->df == P_strdup);
    TEST_ASSERT(parr->pf == 0);
    
    return 0;
}

static unsigned int test_parr_add(void)
{    
    parr_t  *parr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    
    parr = parr_add(parr, (item_t) a);
    
    TEST_ASSERT_EQUALS(parr->n, 1);
    TEST_ASSERT_EQUALS(parr->size, 4);
    TEST_ASSERT_EQUALS(parr->refc[0], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[0], a) == 0);

    parr = parr_add(parr, (item_t) b);

    TEST_ASSERT_EQUALS(parr->n, 2);
    TEST_ASSERT_EQUALS(parr->size, 4);
    TEST_ASSERT_EQUALS(parr->refc[1], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[1], b) == 0);

    parr = parr_add(parr, (item_t) c);
    
    TEST_ASSERT_EQUALS(parr->n, 3);
    TEST_ASSERT_EQUALS(parr->size, 4);
    TEST_ASSERT_EQUALS(parr->refc[2], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[2], c) == 0);

    parr = parr_add(parr, (item_t) a);
    
    TEST_ASSERT_EQUALS(parr->n, 4);
    TEST_ASSERT_EQUALS(parr->size, 4);

    parr = parr_add(parr, (item_t) b);
    
    TEST_ASSERT_EQUALS(parr->n, 5);
    TEST_ASSERT_EQUALS(parr->size, 8);
    TEST_ASSERT_EQUALS(parr->refc[0], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[0], a) == 0);
    TEST_ASSERT_EQUALS(parr->refc[1], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[1], b) == 0);
    TEST_ASSERT_EQUALS(parr->refc[2], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[2], c) == 0);
    TEST_ASSERT_EQUALS(parr->refc[3], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[3], a) == 0);
    TEST_ASSERT_EQUALS(parr->refc[4], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[4], b) == 0);
    
    return 0;
}

void _print_refc(parr_t *parr)
{
    int     i;

    for (i = 0; i < parr->n; i++)
        printf("refc[%d]: %d\n", i, parr->refc[i]);
    
}

static unsigned int test_parr_add_nondup(void)
{    
    parr_t  *parr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    
    parr = parr_new(4, P_strcmp, P_free, P_strdup, 0);
    parr = parr_add_nondup(parr, (item_t) a);
    parr = parr_add_nondup(parr, (item_t) b);
    parr = parr_add_nondup(parr, (item_t) b);
    parr = parr_add_nondup(parr, (item_t) a);
    parr = parr_add_nondup(parr, (item_t) c);

    TEST_ASSERT_EQUALS(parr->refc[0], 2);
    TEST_ASSERT(strcmp((char *) parr->vect[0], a) == 0);
    TEST_ASSERT_EQUALS(parr->refc[1], 2);
    TEST_ASSERT(strcmp((char *) parr->vect[1], b) == 0);
    TEST_ASSERT_EQUALS(parr->refc[2], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[2], c) == 0);
    
    
    return 0;
}

static unsigned int test_parr_dup(void)
{    
    parr_t  *parr = 0, *darr;
    char    *a = "Tripp";
    char    *b = "Trapp";

    parr = parr_new(4, P_strcmp, P_free, P_strdup, 0);
    parr = parr_add_nondup(parr, (item_t) a);
    parr = parr_add_nondup(parr, (item_t) b);
    parr = parr_add_nondup(parr, (item_t) b);
    
    darr = parr_dup(parr);

    /* _print_refc(darr);*/
    TEST_ASSERT_EQUALS(parr->refc[0], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[0], a) == 0);
    TEST_ASSERT_EQUALS(parr->refc[1], 2);
    TEST_ASSERT(strcmp((char *) parr->vect[1], b) == 0);
    
    return 0;
}

static unsigned int test_parr_join(void)
{    
    parr_t  *aarr = 0;
    parr_t  *barr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    
    aarr = parr_new(4, P_strcmp, P_free, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) b);
    aarr = parr_add_nondup(aarr, (item_t) b);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) c);

    barr = parr_new(4, P_strcmp, P_free, P_strdup, 0);
    barr = parr_add_nondup(barr, (item_t) a);
    barr = parr_add_nondup(barr, (item_t) b);
    
    aarr = parr_join(aarr, barr, 0);
    /* _print_refc(aarr); */
    TEST_ASSERT_EQUALS(aarr->n, 3);
    TEST_ASSERT_EQUALS(aarr->refc[0], 3);
    TEST_ASSERT(strcmp((char *) aarr->vect[0], a) == 0);
    TEST_ASSERT_EQUALS(aarr->refc[1], 3);
    TEST_ASSERT(strcmp((char *) aarr->vect[1], b) == 0);
    TEST_ASSERT_EQUALS(aarr->refc[2], 1);
    TEST_ASSERT(strcmp((char *) aarr->vect[2], c) == 0);
    
    return 0;
}

static unsigned int test_parr_xor(void)
{    
    parr_t  *aarr = 0;
    parr_t  *barr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    
    aarr = parr_new(4, P_strcmp, P_free, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) b);
    aarr = parr_add_nondup(aarr, (item_t) b);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) c);
    
    barr = parr_new(4, P_strcmp, P_free, P_strdup, 0);
    barr = parr_add_nondup(barr, (item_t) a);
    barr = parr_add_nondup(barr, (item_t) b);
    
    aarr = parr_xor(aarr, barr, 0);
    /*
    _print_refc(aarr);
    printf("arr len: %d\n", aarr->n);
     */
    TEST_ASSERT_EQUALS(aarr->n, 1);
    TEST_ASSERT_EQUALS(aarr->refc[0], 1);
    TEST_ASSERT(strcmp((char *) aarr->vect[0], c) == 0);
    
    return 0;
}

static unsigned int test_parr_common(void)
{    
    parr_t  *aarr = 0;
    parr_t  *barr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    item_t  *ite;
    
    aarr = parr_new(4, P_strcmp, P_free, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) b);
    aarr = parr_add_nondup(aarr, (item_t) c);
    
    barr = parr_new(4, P_strcmp, P_free, P_strdup, 0);
    barr = parr_add_nondup(barr, (item_t) b);
    
    ite = parr_common(aarr, barr);
    
    TEST_ASSERT(strcmp((char *) ite, b) == 0);
    
    return 0;
}

static unsigned int test_parr_or(void)
{    
    parr_t  *aarr = 0;
    parr_t  *barr = 0;
    parr_t  *parr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    
    aarr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) b);
    aarr = parr_add_nondup(aarr, (item_t) c);
    
    barr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    barr = parr_add_nondup(barr, (item_t) b);
    
    parr = parr_or(aarr, barr);
    
    /*_print_refc(parr);*/
    TEST_ASSERT_EQUALS(parr->n, 1);
    TEST_ASSERT_EQUALS(parr->refc[0], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[0], b) == 0);
    
    parr_free(parr);
    parr_free(barr);
    parr_free(aarr);
    
    aarr = parr_new(4, P_strcmp, P_free, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) b);
    barr = parr_new(4, P_strcmp, P_free, P_strdup, 0);
    barr = parr_add_nondup(barr, (item_t) c);

    parr = parr_or(aarr, barr);
    TEST_ASSERT(parr == 0);
    
    return 0;
}

static unsigned int test_parr_and(void)
{    
    parr_t  *aarr = 0;
    parr_t  *barr = 0;
    parr_t  *parr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    
    aarr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) b);
    aarr = parr_add_nondup(aarr, (item_t) c);
    
    barr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    barr = parr_add_nondup(barr, (item_t) b);
    
    parr = parr_and(aarr, barr);
    
    /*_print_refc(parr);*/
    TEST_ASSERT_EQUALS(parr->n, 3);
    TEST_ASSERT_EQUALS(parr->refc[0], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[0], a) == 0);
    TEST_ASSERT_EQUALS(parr->refc[1], 2);
    TEST_ASSERT(strcmp((char *) parr->vect[1], b) == 0);
    TEST_ASSERT_EQUALS(parr->refc[2], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[2], c) == 0);
    
    parr_free(parr);
    parr_free(barr);
    parr_free(aarr);
    
    aarr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) b);
    barr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    barr = parr_add_nondup(barr, (item_t) c);
    
    parr = parr_and(aarr, barr);
    TEST_ASSERT_EQUALS(parr->n, 3);
    TEST_ASSERT_EQUALS(parr->refc[0], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[0], a) == 0);
    TEST_ASSERT_EQUALS(parr->refc[1], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[1], b) == 0);
    TEST_ASSERT_EQUALS(parr->refc[2], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[2], c) == 0);

    parr_free(parr);
    parr_free(barr);
    parr_free(aarr);
    
    aarr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    barr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    barr = parr_add_nondup(barr, (item_t) c);
    
    parr = parr_and(aarr, barr);
    TEST_ASSERT_EQUALS(parr->n, 2);
    TEST_ASSERT_EQUALS(parr->refc[0], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[0], a) == 0);
    TEST_ASSERT_EQUALS(parr->refc[1], 1);
    TEST_ASSERT(strcmp((char *) parr->vect[1], c) == 0);
    
    return 0;
}

static unsigned int test_parr_find_index(void)
{    
    parr_t  *aarr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    int     i;
    
    aarr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) b);
    aarr = parr_add_nondup(aarr, (item_t) c);
    
    i = parr_find_index(aarr, "Trapp");
    TEST_ASSERT_NOT_EQUALS(i, -1);

    i = parr_find_index(aarr, "Trupp");
    TEST_ASSERT_EQUALS(i, -1);

    return 0;
}

static unsigned int test_parr_find(void)
{    
    parr_t  *aarr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    item_t  i;
    
    aarr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) b);
    aarr = parr_add_nondup(aarr, (item_t) c);
    
    i = parr_find(aarr, "Trapp");
    TEST_ASSERT(i == b);
    
    i = parr_find(aarr, "Trupp");
    TEST_ASSERT(i == 0);
    
    return 0;
}

static unsigned int test_parr_get_index(void)
{    
    parr_t  *aarr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    item_t  i;

    aarr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) b);
    aarr = parr_add_nondup(aarr, (item_t) c);
    
    TEST_ASSERT_EQUALS(parr_get_index(aarr, b), 1);
    i = parr_find(aarr, "Trupp");
    TEST_ASSERT(i == 0);
    
    return 0;
}

static unsigned int test_parr_rm_item(void)
{    
    parr_t  *aarr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    
    aarr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) b);
    aarr = parr_add_nondup(aarr, (item_t) c);
    
    parr_rm_item(&aarr, (item_t) b);

    TEST_ASSERT_EQUALS(aarr->n, 2);
    TEST_ASSERT_NOT_EQUALS(parr_get_index(aarr, a), -1);
    TEST_ASSERT_NOT_EQUALS(parr_get_index(aarr, c), -1);
    return 0;
}

static unsigned int test_parr_dec_refcount(void)
{    
    parr_t  *aarr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    
    aarr = parr_new(4, P_strcmp, 0, P_strdup, 0);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) b);
    aarr = parr_add_nondup(aarr, (item_t) c);
    aarr = parr_add_nondup(aarr, (item_t) a);
    aarr = parr_add_nondup(aarr, (item_t) c);
    
    parr_dec_refcount(&aarr, 1);
    
    TEST_ASSERT_EQUALS(aarr->n, 2);
    TEST_ASSERT_NOT_EQUALS(parr_get_index(aarr, a), -1);
    TEST_ASSERT_NOT_EQUALS(parr_get_index(aarr, c), -1);

    parr_dec_refcount(&aarr, 1);
    
    TEST_ASSERT(aarr == 0);
    
    return 0;
}
