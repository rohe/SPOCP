/*
 *  test_skiplist.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/15/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <element.h>
#include <octet.h>
#include <boundary.h>
#include <skiplist.h>
#include <boundarysl.h>

#include <wrappers.h>
#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_bsl_init(void);
static unsigned int test_bsl_find(void);
static unsigned int test_bsl_insert(void); 
static unsigned int test_bsl_remove(void); 

test_conf_t tc[] = {
    {"test_bsl_init", test_bsl_init},
    {"test_bsl_find", test_bsl_find},
    {"test_bsl_insert", test_bsl_insert},
    {"test_bsl_remove", test_bsl_remove},
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

static unsigned int test_bsl_init(void)
{    
    slist_t     *slp;
    slnode_t    *node;
    int         i;
    
    slp = bsl_init(8, 3);
    
    TEST_ASSERT(slp != NULL);
    TEST_ASSERT_EQUALS( slp->maxlev, 8);
    TEST_ASSERT_EQUALS( slp->size, 0);
    TEST_ASSERT( slp->head != 0);
    
    node = slp->head ;
    
    TEST_ASSERT_EQUALS(node->sz, 8);
    TEST_ASSERT_EQUALS(node->refc, 0);
    TEST_ASSERT(node->value == 0);
    TEST_ASSERT(node->varr == 0);

    for (i=1; i < node->sz; i++) {
        TEST_ASSERT(node->next[i] == slp->tail);
    }
    for (i=0; i < node->sz; i++) {
        TEST_ASSERT_EQUALS(node->width[i], 0);
    }
    
    return 0;
}

static unsigned int test_bsl_find(void)
{    
    slist_t         *slp;
    sl_res_t        *result;
    int             i, size;
    octet_t         oct;
    boundary_t      *bp;
    spocp_result_t  r;
    
    
    slp = bsl_init(8, 3);
    size = slp->head->sz;
    
    oct_assign(&oct, "ABC");
    bp = boundary_new(SPOC_ALPHA);
    r = set_limit(bp, &oct);
    
    result = bsl_find(slp, bp, NULL);
    TEST_ASSERT(result != 0);

    for (i = 0; i < size; i++) {
        TEST_ASSERT(result->chain[i] == slp->head);
        TEST_ASSERT_EQUALS(result->distance[i], 0);
    }
    
    return 0;
}

static unsigned int test_bsl_insert(void)
{    
    slist_t         *slp;
    slnode_t        *newnode;
    int             i;
    char            *str;
    octet_t         oct;
    boundary_t      *bp;
    spocp_result_t  r;
    
    slp = bsl_init(8, 3);

    oct_assign(&oct, "ABC");
    bp = boundary_new(SPOC_ALPHA);
    r = set_limit(bp, &oct);
    
    bsl_insert(slp, bp);
    
    TEST_ASSERT_EQUALS( slp->size, 1 ) ;
    newnode = slp->head->next[0] ;
    
    TEST_ASSERT(newnode->next[0] == slp->tail );
    for( i = 0; i < newnode->sz; i++) {
        TEST_ASSERT(slp->head->next[i] == newnode);
    }
    for (; i < slp->head->sz; i++) {
        TEST_ASSERT(slp->head->next[i] == slp->tail);
    }
    for (; i < slp->head->sz; i++) {
        TEST_ASSERT_EQUALS(slp->head->width[i], 1);
    }        
    
    oct_assign(&oct, "DEF");
    bp = boundary_new(SPOC_ALPHA);
    r = set_limit(bp, &oct);
    
    bsl_insert(slp, bp);
    
    str = boundary_prints((boundary_t *) slp->head->next[0]->value);
    TEST_ASSERT( strcmp(str, "- ABC") == 0);
    free(str);
    str = boundary_prints((boundary_t *) slp->head->next[0]->next[0]->value);
    TEST_ASSERT( strcmp(str, "- DEF") == 0);
    free(str);
    
    return 0;
}

static unsigned int test_bsl_remove(void)
{    
    slist_t         *slp;
    char            *str;
    octet_t         oct;
    boundary_t      *bp;
    spocp_result_t  r;
    
    slp = bsl_init(8, 3);
    
    oct_assign(&oct, "ABC");
    bp = boundary_new(SPOC_ALPHA);
    r = set_limit(bp, &oct);
    
    bsl_insert(slp, bp);
        
    oct_assign(&oct, "DEF");
    bp = boundary_new(SPOC_ALPHA);
    r = set_limit(bp, &oct);
    
    bsl_insert(slp, bp);
    
    str = boundary_prints((boundary_t *) slp->head->next[0]->value);
    TEST_ASSERT( strcmp(str, "- ABC") == 0);
    free(str);
    str = boundary_prints((boundary_t *) slp->head->next[0]->next[0]->value);
    TEST_ASSERT( strcmp(str, "- DEF") == 0);
    free(str);
    
    oct_assign(&oct, "ABC");
    bp = boundary_new(SPOC_ALPHA);
    r = set_limit(bp, &oct);

    bsl_remove(slp, bp);
    
    str = boundary_prints((boundary_t *) slp->head->next[0]->value);
    TEST_ASSERT( strcmp(str, "- DEF") == 0);
    free(str);
    
    return 0;
}
