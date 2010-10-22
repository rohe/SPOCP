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
#include <varr.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_varr_new(void);
static unsigned int test_varr_add(void);
static unsigned int test_varr_or(void);
static unsigned int test_varr_and(void);
static unsigned int test_varr_find(void);
static unsigned int test_varr_rm_dup(void);
static unsigned int test_varr_first(void);
static unsigned int test_varr_next(void);
static unsigned int test_varr_pop(void);
static unsigned int test_varr_fifo_pop(void);
static unsigned int test_varr_dup(void);
static unsigned int test_varr_first_common(void);
static unsigned int test_varr_rm(void);
static unsigned int test_varr_nth(void);

test_conf_t tc[] = {
    {"test_varr_new", test_varr_new},
    {"test_varr_add", test_varr_add},
    {"test_varr_or", test_varr_or},
    {"test_varr_and", test_varr_and},
    {"test_varr_find", test_varr_find},
    {"test_varr_rm_dup", test_varr_rm_dup},
    {"test_varr_first", test_varr_first},
    {"test_varr_next", test_varr_next},
    {"test_varr_pop", test_varr_pop},
    {"test_varr_fifo_pop", test_varr_fifo_pop},
    {"test_varr_dup", test_varr_dup},
    {"test_varr_first_common", test_varr_first_common},
    {"test_varr_rm", test_varr_rm},
    {"test_varr_nth", test_varr_nth},
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

static unsigned int test_varr_new(void)
{    
    varr_t *varr;

    varr = varr_new(4);

    TEST_ASSERT_EQUALS(varr->n, 0);
    TEST_ASSERT_EQUALS(varr->size, 4);
    
    return 0;
}

static unsigned int test_varr_add(void)
{    
    varr_t  *varr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    
    varr = varr_add(varr, (item_t) a);
    
    TEST_ASSERT_EQUALS(varr->n, 1);
    TEST_ASSERT_EQUALS(varr->size, 4);
    TEST_ASSERT(strcmp((char *) varr->arr[0], a) == 0);

    varr = varr_add(varr, (item_t) b);

    TEST_ASSERT_EQUALS(varr->n, 2);
    TEST_ASSERT_EQUALS(varr->size, 4);
    TEST_ASSERT(strcmp((char *) varr->arr[1], b) == 0);

    varr = varr_add(varr, (item_t) c);
    
    TEST_ASSERT_EQUALS(varr->n, 3);
    TEST_ASSERT_EQUALS(varr->size, 4);
    TEST_ASSERT(strcmp((char *) varr->arr[2], c) == 0);

    varr = varr_add(varr, (item_t) a);
    
    TEST_ASSERT_EQUALS(varr->n, 4);
    TEST_ASSERT_EQUALS(varr->size, 4);

    varr = varr_add(varr, (item_t) b);
    
    TEST_ASSERT_EQUALS(varr->n, 5);
    TEST_ASSERT_EQUALS(varr->size, 8);
    TEST_ASSERT(strcmp((char *) varr->arr[0], a) == 0);
    TEST_ASSERT(strcmp((char *) varr->arr[1], b) == 0);
    TEST_ASSERT(strcmp((char *) varr->arr[2], c) == 0);
    TEST_ASSERT(strcmp((char *) varr->arr[3], a) == 0);
    TEST_ASSERT(strcmp((char *) varr->arr[4], b) == 0);
    
    return 0;
}

static unsigned int test_varr_or(void)
{    
    varr_t  *aarr = 0;
    varr_t  *barr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    
    aarr = varr_new(4);

    aarr = varr_add(aarr, (item_t) a);
    aarr = varr_add(aarr, (item_t) b);
    aarr = varr_add(aarr, (item_t) c);
    
    barr = varr_new(4);
    barr = varr_add(barr, (item_t) b);
    
    aarr = varr_or(aarr, barr, 0);

    TEST_ASSERT_EQUALS(aarr->n, 3);
    
    TEST_ASSERT_EQUALS(strcmp((char *) aarr->arr[0], a), 0);
    TEST_ASSERT(strcmp((char *) aarr->arr[1], b) == 0);
    TEST_ASSERT(strcmp((char *) aarr->arr[2], c) == 0);

    varr_free(aarr);

    aarr = varr_new(4);
    aarr = varr_add(aarr, (item_t) a);
    aarr = varr_add(aarr, (item_t) b);
    barr = varr_new(4);
    barr = varr_add(barr, (item_t) c);

    aarr = varr_or(aarr, barr, 0);
    TEST_ASSERT_EQUALS(aarr->n, 3);

    return 0;
}

static unsigned int test_varr_and(void)
{    
    varr_t  *aarr = 0;
    varr_t  *barr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    
    aarr = varr_new(4);
    aarr = varr_add(aarr, (item_t) a);
    aarr = varr_add(aarr, (item_t) b);
    aarr = varr_add(aarr, (item_t) c);
    
    barr = varr_new(4);
    barr = varr_add(barr, (item_t) b);
    
    aarr = varr_and(aarr, barr);
    
    TEST_ASSERT_EQUALS(aarr->n, 1);
    TEST_ASSERT(strcmp((char *) aarr->arr[0], b) == 0);
    
    varr_free(barr);
    varr_free(aarr);
    
    aarr = varr_new(4);
    aarr = varr_add(aarr, (item_t) a);
    aarr = varr_add(aarr, (item_t) b);
    barr = varr_new(4);
    barr = varr_add(barr, (item_t) c);
    
    aarr = varr_and(aarr, barr);
    TEST_ASSERT(aarr ==  0);
    
    return 0;
}

static unsigned int test_varr_find(void)
{    
    varr_t  *aarr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    int     i;
    
    aarr = varr_new(4);
    aarr = varr_add(aarr, (item_t) a);
    aarr = varr_add(aarr, (item_t) b);
    aarr = varr_add(aarr, (item_t) c);
    
    i = varr_find(aarr, "Trapp");
    TEST_ASSERT(i == 1);
    
    i = varr_find(aarr, "Trupp");
    TEST_ASSERT(i == -1);
    
    return 0;
}

int
_strcmp(item_t a, item_t b, int *rc)
{
    *rc = SPOCP_SUCCESS;
	return strcmp((char *) a, (char *) b);
}

static unsigned int test_varr_rm_dup(void)
{    
    varr_t  *varr = 0, *warr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    
    varr = varr_new(4);
    varr = varr_add(varr, (item_t) a);
    varr = varr_add(varr, (item_t) b);
    varr = varr_add(varr, (item_t) a);
    
    varr_rm_dup(varr, (cmpfn *) _strcmp, 0);
    TEST_ASSERT_EQUALS(varr->n, 2);

    TEST_ASSERT_EQUALS(varr_find(varr, "Tripp"), 0);
    TEST_ASSERT_EQUALS(varr_find(varr, "Trapp"), 1);

    /* Can't test except that it's runable */
    varr_rm_dup(warr, (cmpfn *) _strcmp, 0);
    warr = varr_new(4);
    varr_rm_dup(warr, (cmpfn *) _strcmp, 0);

    return 0;
}

static unsigned int test_varr_first(void)
{    
    varr_t  *varr = 0, *warr=0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    void    *vp;
    
    varr = varr_new(4);
    varr = varr_add(varr, (item_t) a);
    varr = varr_add(varr, (item_t) b);
    varr = varr_add(varr, (item_t) a);
    
    vp = varr_first(varr);
    TEST_ASSERT(strcmp(vp, a) == 0);
    
    vp = varr_first(warr);
    TEST_ASSERT(vp == 0);
    
    return 0;
}

static unsigned int test_varr_next(void)
{    
    varr_t  *varr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    void    *vp;
    
    varr = varr_new(4);
    varr = varr_add(varr, (item_t) a);
    varr = varr_add(varr, (item_t) b);
    
    vp = varr_first(varr);
    TEST_ASSERT(strcmp((char *) vp, a) == 0);
    vp = varr_next(varr, vp);
    TEST_ASSERT(strcmp((char *) vp, b) == 0);
    vp = varr_next(varr, vp);
    TEST_ASSERT(vp == 0);
    
    return 0;
}

static unsigned int test_varr_pop(void)
{    
    varr_t  *varr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    void    *vp;
    
    varr = varr_new(4);
    varr = varr_add(varr, (item_t) a);
    varr = varr_add(varr, (item_t) b);
    varr = varr_add(varr, (item_t) c);
    
    vp = varr_pop(varr);
    TEST_ASSERT_EQUALS(varr->n, 2);
    TEST_ASSERT(strcmp((char *) vp, c) == 0);

    vp = varr_pop(varr);
    TEST_ASSERT_EQUALS(varr->n, 1);
    TEST_ASSERT(strcmp((char *) vp, b) == 0);

    vp = varr_pop(varr);
    TEST_ASSERT_EQUALS(varr->n, 0);
    TEST_ASSERT(strcmp((char *) vp, a) == 0);
    
    vp = varr_pop(varr);
    TEST_ASSERT(vp == 0);
    
    return 0;
}

static unsigned int test_varr_fifo_pop(void)
{    
    varr_t  *varr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    void    *vp;
    
    varr = varr_new(4);
    varr = varr_add(varr, (item_t) a);
    varr = varr_add(varr, (item_t) b);
    varr = varr_add(varr, (item_t) c);
    
    vp = varr_fifo_pop(varr);
    TEST_ASSERT_EQUALS(varr->n, 2);
    TEST_ASSERT(strcmp((char *) vp, a) == 0);
    
    vp = varr_fifo_pop(varr);
    TEST_ASSERT_EQUALS(varr->n, 1);
    TEST_ASSERT(strcmp((char *) vp, b) == 0);
    
    vp = varr_fifo_pop(varr);
    TEST_ASSERT_EQUALS(varr->n, 0);
    TEST_ASSERT(strcmp((char *) vp, c) == 0);
    
    vp = varr_fifo_pop(varr);
    TEST_ASSERT(vp == 0);
    
    return 0;
}

item_t _strdup( item_t i)
{
    return (item_t) Strdup((char *) i);
}

static unsigned int test_varr_dup(void)
{    
    varr_t  *varr = 0, *warr;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";

    varr = varr_new(4);
    varr = varr_add(varr, (item_t) a);
    varr = varr_add(varr, (item_t) b);
    varr = varr_add(varr, (item_t) c);
    
    warr = varr_dup(varr, (dfunc *)_strdup);
    
    TEST_ASSERT_EQUALS(warr->n, 3);
    TEST_ASSERT_EQUALS(varr_find(varr, "Tripp"), 0);
    TEST_ASSERT_EQUALS(varr_find(varr, "Trapp"), 1);
    TEST_ASSERT_EQUALS(varr_find(varr, "Trull"), 2);

    return 0;
}

static unsigned int test_varr_first_common(void)
{    
    varr_t  *varr = 0, *warr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    void    *vp;
    
    varr = varr_new(4);
    varr = varr_add(varr, (item_t) a);
    varr = varr_add(varr, (item_t) b);
    varr = varr_add(varr, (item_t) c);

    warr = varr_new(4);
    warr = varr_add(warr, (item_t) c);
    warr = varr_add(warr, (item_t) b);
    
    vp = varr_first_common(varr, warr);
    
    TEST_ASSERT(strcmp((char *) vp, b) == 0);

    /* order matters */
    vp = varr_first_common(warr, varr);
    
    TEST_ASSERT(strcmp((char *) vp, c) == 0);
    
    return 0;
}

static unsigned int test_varr_rm(void)
{    
    varr_t  *varr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    void    *vp;
    
    varr = varr_new(4);
    varr = varr_add(varr, (item_t) a);
    varr = varr_add(varr, (item_t) b);
    varr = varr_add(varr, (item_t) c);
    
    vp = varr_rm(varr, a);    
    TEST_ASSERT_EQUALS(varr->n, 2);
    TEST_ASSERT(strcmp((char *) varr->arr[0], b) == 0);

    vp = varr_rm(varr, c);    
    TEST_ASSERT_EQUALS(varr->n, 1);
    TEST_ASSERT(strcmp((char *) varr->arr[0], b) == 0);
    
    vp = varr_rm(varr, c);    
    TEST_ASSERT(vp == 0);
    
    return 0;
}

static unsigned int test_varr_nth(void)
{    
    varr_t  *varr = 0;
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    
    varr = varr_new(4);
    varr = varr_add(varr, (item_t) a);
    varr = varr_add(varr, (item_t) b);
    varr = varr_add(varr, (item_t) c);
    
    TEST_ASSERT(strcmp((char *) varr_nth(varr, 0), a) == 0);
    TEST_ASSERT(strcmp((char *) varr_nth(varr, 1), b) == 0);
    TEST_ASSERT(strcmp((char *) varr_nth(varr, 2), c) == 0);
    TEST_ASSERT(varr_nth(varr, 3) == 0);
    
    return 0;
}
