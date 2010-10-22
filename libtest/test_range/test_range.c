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

#include <element.h>
#include <range.h>
#include <octet.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_set_delimiter(void);
static unsigned int test_range_limit(void);
static unsigned int test_set_range(void);
static unsigned int test_is_atom(void);

test_conf_t tc[] = {
    {"test_set_delimiter", test_set_delimiter},
    {"test_range_limit", test_range_limit},
    {"test_set_range", test_set_range},
    {"test_is_atom", test_is_atom},
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

static unsigned int test_set_delimiter(void)
{
    range_t     *rp;
    octet_t     oct;
    boundary_t  *bp;
    
    rp = Calloc(1, sizeof(range_t));
    
    oct_assign(&oct, "gt");
    bp = set_delimiter(rp, oct);
    /* printf("%p %p\n",&rp->lower, bp); */
    TEST_ASSERT(&rp->lower == bp);
    /* printf("type: %d\n", rp->lower.type); */
    TEST_ASSERT_EQUALS(rp->lower.type, GT);
    
    oct_assign(&oct, "ge");
    bp = set_delimiter(rp, oct);
    TEST_ASSERT(bp == NULL);

    oct_assign(&oct, "le");
    bp = set_delimiter(rp, oct);
    TEST_ASSERT(&rp->upper == bp);
    TEST_ASSERT_EQUALS(rp->upper.type, LT|EQ);

    oct_assign(&oct, "lt");
    bp = set_delimiter(rp, oct);
    TEST_ASSERT(bp == NULL);
    
    Free(rp);
    rp = Calloc(1, sizeof(range_t));

    oct_assign(&oct, "lt");
    bp = set_delimiter(rp, oct);
    TEST_ASSERT(&rp->upper == bp);
    TEST_ASSERT_EQUALS(rp->upper.type, LT);
    
    oct_assign(&oct, "le");
    bp = set_delimiter(rp, oct);
    TEST_ASSERT(bp == NULL);

    oct_assign(&oct, "ge");
    bp = set_delimiter(rp, oct);
    /* printf("%p %p\n",&rp->lower, bp); */
    TEST_ASSERT(&rp->lower == bp);
    TEST_ASSERT_EQUALS(rp->lower.type, GT|EQ);
    
    oct_assign(&oct, "gt");
    bp = set_delimiter(rp, oct);
    TEST_ASSERT(bp == NULL);
    
    return 0;
}

static unsigned int test_range_limit(void)
{
    range_t         *rp;
    octet_t         *op;
    spocp_result_t  r;
    
    op = oct_new(0, "2:ge3:ABC");

    rp = Calloc(1, sizeof(range_t));
    rp->lower.type = rp->upper.type = SPOC_ALPHA;
    
    r = range_limit(op, rp);
    
    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    /* printf("type: %d\n", rp->lower.type); */
    TEST_ASSERT_EQUALS(rp->lower.type & BTYPE, GT|EQ);
    TEST_ASSERT_EQUALS(rp->lower.type & RTYPE, 2);
    TEST_ASSERT( oct2strcmp(&rp->lower.v.val, "ABC") == 0 );
    
    range_free(rp);
    oct_free(op);

    /* ----- */
    
    op = oct_new(0, "2:lt3:QQQ");
    rp = Calloc(1, sizeof(range_t));
    rp->lower.type = rp->upper.type = SPOC_ALPHA;

    r = range_limit(op, rp);

    TEST_ASSERT_EQUALS(r, SPOCP_SUCCESS);
    /*printf("type: %d\n", rp->upper.type);*/
    TEST_ASSERT_EQUALS(rp->upper.type & BTYPE, LT);
    TEST_ASSERT_EQUALS(rp->upper.type & RTYPE, 2);
    TEST_ASSERT( oct2strcmp(&rp->upper.v.val, "QQQ") == 0 );
    
    return 0;
}

static unsigned int test_set_range(void)
{
    range_t         *rp;
    octet_t         *op;

    op = oct_new(0, "5:alpha2:lt3:QQQ)");
    rp = set_range(op);
    
    TEST_ASSERT(rp != NULL);
    TEST_ASSERT_EQUALS(rp->upper.type & BTYPE, LT);
    TEST_ASSERT_EQUALS(rp->upper.type & RTYPE, 2);
    TEST_ASSERT( oct2strcmp(&rp->upper.v.val, "QQQ") == 0 );
    
    range_free(rp);
    oct_free(op);

    op = oct_new(0, "5:alpha2:lt3:QQQ2:ge3:ABC)");
    rp = set_range(op);
    TEST_ASSERT_EQUALS(rp->upper.type & BTYPE, LT);
    TEST_ASSERT_EQUALS(rp->upper.type & RTYPE, 2);
    TEST_ASSERT( oct2strcmp(&rp->upper.v.val, "QQQ") == 0 );
    TEST_ASSERT_EQUALS(rp->lower.type & BTYPE, GT|EQ);
    TEST_ASSERT_EQUALS(rp->lower.type & RTYPE, 2);
    TEST_ASSERT( oct2strcmp(&rp->lower.v.val, "ABC") == 0 );

    range_free(rp);
    oct_free(op);
    
    op = oct_new(0, "7:numeric2:lt3:1002:ge2:17)");
    rp = set_range(op);
    TEST_ASSERT_EQUALS(rp->upper.type & BTYPE, LT);
    TEST_ASSERT_EQUALS(rp->upper.type & RTYPE, SPOC_NUMERIC);
    TEST_ASSERT_EQUALS(rp->upper.v.num, 100);
    TEST_ASSERT_EQUALS(rp->lower.type & BTYPE, GT|EQ);
    TEST_ASSERT_EQUALS(rp->lower.type & RTYPE, SPOC_NUMERIC);
    TEST_ASSERT_EQUALS(rp->lower.v.num, 17);
    
    range_free(rp);
    oct_free(op);

    op = oct_new(0, "4:ipv42:le15:130.239.255.2552:ge11:130.239.0.0)");
    rp = set_range(op);
    TEST_ASSERT_EQUALS(rp->upper.type & BTYPE, LT|EQ);
    TEST_ASSERT_EQUALS(rp->lower.type & BTYPE, GT|EQ);

    range_free(rp);
    oct_free(op);
    
    op = oct_new(0, "4:date2:ge25:2009-12-07T21:48:00+04:30)");
    rp = set_range(op);
    TEST_ASSERT_EQUALS(rp->lower.type, SPOC_DATE|GT|EQ);
    
    /* And now some that should fail */
    
    range_free(rp);
    
    oct_free(op);    
    op = oct_new(0, "6:foobar2:lt3:1002:ge2:17)");
    rp = set_range(op);    
    TEST_ASSERT(rp == NULL);

    oct_free(op);    
    op = oct_new(0, "4:date2:lt3:100)");
    rp = set_range(op);    
    TEST_ASSERT(rp == NULL);

    oct_free(op);    
    op = oct_new(0, "6:alpha2:geq3:100)");
    rp = set_range(op);    
    TEST_ASSERT(rp == NULL);
    
    return 0;
}

static unsigned int test_is_atom(void)
{
    range_t *rp;
    octet_t *op, oct;
    atom_t  *ap;
    
    op = oct_new(0, "5:alpha2:le3:QQQ2:ge3:QQQ)");
    rp = set_range(op);
    TEST_ASSERT_EQUALS(is_atom(rp), SPOCP_SUCCESS);
    
    range_free(rp);
    oct_free(op);
    op = oct_new(0, "7:numeric2:ge3:6662:le3:666)");
    octln(&oct, op);
    rp = set_range(op);
    TEST_ASSERT_EQUALS(is_atom(rp), SPOCP_SUCCESS);
    ap = get_atom_from_range_definition(&oct);
    TEST_ASSERT( oct2strcmp(&ap->val, "666") == 0) ;
    
    return 0;
}
