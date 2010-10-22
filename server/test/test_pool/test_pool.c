/*
 *  test_read.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/12/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <octet.h>
#include <bcond.h>
#include <proto.h>
#include <wrappers.h>
#include <log.h>
#include <srv.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_pool_new(void);
static unsigned int test_pool_item_new(void);
static unsigned int test_pool_add(void);
static unsigned int test_pool_rm_item(void);
static unsigned int test_afpool_new(void);
static unsigned int test_afpool_make_item_active(void);
static unsigned int test_afpool_active2free(void);

test_conf_t tc[] = {
    {"test_pool_new", test_pool_new},
    {"test_pool_item_new", test_pool_item_new},
    {"test_pool_add", test_pool_add},
    {"test_pool_rm_item", test_pool_rm_item},
    {"test_afpool_new", test_afpool_new},
    {"test_afpool_make_item_active", test_afpool_make_item_active},
    {"test_afpool_active2free", test_afpool_active2free},
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

static unsigned int test_pool_new(void)
{
    pool_t  *pool;
    
    pool = pool_new();
    TEST_ASSERT( pool != NULL);
    
    return 0;
}

static unsigned int test_pool_item_new(void)
{
    pool_item_t *pi;
    
    pi = pool_item_new((void *) "Chan");
    TEST_ASSERT( pi != NULL);
    TEST_ASSERT( strcmp((char *)pi->info,"Chan") == 0);
    
    return 0;
}

static unsigned int test_pool_add(void)
{
    pool_item_t *pi;
    pool_t      *pool;
    
    pool = pool_new();
    pool_add(pool, pool_item_new((void *) "Jackie"));
    pool_add(pool, pool_item_new((void *) "Chan"));

    TEST_ASSERT_EQUALS( pool->size, 2);
    pi = pool->head;
    TEST_ASSERT( strcmp((char *)pi->info,"Jackie") == 0);
    TEST_ASSERT( strcmp((char *)pi->next->info,"Chan") == 0);
    TEST_ASSERT( pool->head->next == pool->tail );
    TEST_ASSERT( pool->tail->prev == pool->head);
    
    return 0;
}

static unsigned int test_pool_rm_item(void)
{
    pool_item_t *pi;
    pool_t      *pool;
    
    pool = pool_new();
    pool_add(pool, pool_item_new((void *) "Jackie"));
    pool_add(pool, pool_item_new((void *) "Chan"));
    pi = pool->head;
    
    pool_rm_item(pool, pi);
    
    TEST_ASSERT( pool->head == pool->tail );
    TEST_ASSERT_EQUALS(pool->size, 1);
    
    pool_add(pool, pool_item_new((void *) "Jackie"));
    pool_add(pool, pool_item_new((void *) "Spy"));

    /* remove something in the middle */
    pi = pool->head->next;
    pool_rm_item(pool, pi);
    TEST_ASSERT_EQUALS(pool->size, 2);
    TEST_ASSERT( pool->head->next == pool->tail );
    TEST_ASSERT( pool->tail->prev == pool->head);
    TEST_ASSERT( strcmp((char *)pool->head->info,"Chan") == 0);
    TEST_ASSERT( strcmp((char *)pool->tail->info,"Spy") == 0);
    
    pool_add(pool, pool_item_new((void *) "Accidental"));
    pi = pool->tail;
    pool_rm_item(pool, pi);
    TEST_ASSERT_EQUALS(pool->size, 2);
    TEST_ASSERT( pool->head->next == pool->tail );
    TEST_ASSERT( pool->tail->prev == pool->head);
    TEST_ASSERT( strcmp((char *)pool->head->info,"Chan") == 0);
    TEST_ASSERT( strcmp((char *)pool->tail->info,"Spy") == 0);
    
    return 0;
}

static unsigned int test_afpool_new(void)
{
    afpool_t  *afp;
    
    afp = afpool_new();
    TEST_ASSERT( afp != NULL);
    TEST_ASSERT( afp->free != NULL);
    TEST_ASSERT_EQUALS( afp->free->size, 0);
    TEST_ASSERT( afp->active != NULL);
    TEST_ASSERT_EQUALS( afp->active->size, 0);
    
    return 0;
}

static unsigned int test_afpool_make_item_active(void)
{
    afpool_t  *afp;
    
    afp = afpool_new();
    afpool_make_item_active(afp, pool_item_new((void *) "Jackie"));
    
    TEST_ASSERT( afp->active != NULL);
    TEST_ASSERT_EQUALS( afp->active->size, 1);

    afpool_make_item_active(afp, pool_item_new((void *) "Chan"));
    TEST_ASSERT_EQUALS( afp->active->size, 2);
    
    return 0;
}

static unsigned int test_afpool_active2free(void)
{
    afpool_t    *afp;
    pool_item_t *pi;
    
    afp = afpool_new();
    afpool_make_item_active(afp, pool_item_new((void *) "Jackie"));
    afpool_make_item_active(afp, pool_item_new((void *) "Chan"));
    
    pi = afpool_first_active(afp);
    afpool_active2free(afp, pi);
    
    TEST_ASSERT_EQUALS(afp->active->size, 1);
    TEST_ASSERT_EQUALS(afp->free->size, 1);
    
    pi = afpool_get_free(afp);
    TEST_ASSERT( strcmp((char *)pi->info,"Jackie") == 0);
    
    return 0;
}
