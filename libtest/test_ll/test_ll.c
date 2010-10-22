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

#include <ll.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_ll_new(void);
/* static unsigned int test_new_node(void); */
static unsigned int test_ll_push(void);
static unsigned int test_ll_pop(void);
static unsigned int test_ll_first(void);
static unsigned int test_ll_next(void);
static unsigned int test_ll_find(void);
static unsigned int test_ll_rm_link(void);
static unsigned int test_ll_dup(void);

test_conf_t tc[] = {
    {"test_ll_new", test_ll_new},
/*    {"test_new_node", test_new_node}, */
    {"test_ll_push", test_ll_push},
    {"test_ll_pop", test_ll_pop},
    {"test_ll_first", test_ll_first},
    {"test_ll_next", test_ll_next},
    {"test_ll_find", test_ll_find},
    {"test_ll_rm_link", test_ll_rm_link},
    {"test_ll_dup", test_ll_dup},
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

static unsigned int test_ll_new(void)
{    
    ll_t *ll;

    ll = ll_new(0, 0, 0, 0);

    TEST_ASSERT(ll->n == 0);
    TEST_ASSERT(ll->head == 0);
    TEST_ASSERT(ll->tail == 0);
    TEST_ASSERT(ll->cf == 0);
    TEST_ASSERT(ll->ff == 0);
    TEST_ASSERT(ll->df == 0);
    TEST_ASSERT(ll->pf == 0);
        
    return 0;
}

/*
static unsigned int test_new_node(void)
{    
    char    *a = "Tripp";
    node_t  *np;

    np = node_new((void *) a);
    TEST_ASSERT(np->next == 0);
    TEST_ASSERT(np->prev == 0);
    TEST_ASSERT(np->payload == (void *) a);
    
    return 0;
}
*/

static unsigned int test_ll_push(void)
{    
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    ll_t    *llp = 0;

    llp = ll_new(0, 0, 0, 0);
    llp = ll_push(llp, (void *)a, 1);

    TEST_ASSERT(llp != 0);
    TEST_ASSERT(llp->n == 1);
    TEST_ASSERT(llp->head == llp->tail);

    llp = ll_push(llp, (void *)b, 1);
    TEST_ASSERT(llp->n == 2);
    TEST_ASSERT(llp->head->payload == (void *) a);
    TEST_ASSERT(llp->tail->payload == (void *) b);

    llp = ll_push(llp, (void *)c, 1);
    TEST_ASSERT(llp->n == 3);
    TEST_ASSERT(llp->head->payload == (void *) a);
    TEST_ASSERT(llp->tail->payload == (void *) c);

    return 0;
}

static unsigned int test_ll_pop(void)
{    
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    ll_t    *llp = 0;
    void    *vp;
    
    llp = ll_new(0, 0, 0, 0);
    llp = ll_push(llp, (void *)a, 1);
    llp = ll_push(llp, (void *)b, 1);
    llp = ll_push(llp, (void *)c, 1);
    
    vp = ll_pop(llp);
    TEST_ASSERT(llp->n == 2);
    TEST_ASSERT((char *)vp == a);
    TEST_ASSERT(llp->head->payload == (void *) b);
    TEST_ASSERT(llp->tail->payload == (void *) c);

    vp = ll_pop(llp);
    TEST_ASSERT(llp->n == 1);
    TEST_ASSERT(vp == (void *) b);
    TEST_ASSERT(llp->tail->payload == (void *) c);
    TEST_ASSERT(llp->head->payload == (void *) c);
    
    return 0;
}

void print_str(void *vp)
{
    printf("%s\n", (char *)vp);
}

static unsigned int test_ll_first(void)
{    
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    ll_t    *llp = 0;
    node_t  *np;
    int     dont_duplicate = 1;
    void    *vp;

    llp = ll_new(0, 0, 0, (pfunc *) print_str);
    llp = ll_push(llp, (void *)a, dont_duplicate);
    llp = ll_push(llp, (void *)b, dont_duplicate);
    llp = ll_push(llp, (void *)c, dont_duplicate);

    np = ll_first(llp);
    TEST_ASSERT(np->payload == (void *) a);
    vp = ll_pop(llp);
    np = ll_first(llp);
    TEST_ASSERT(np->payload == (void *) b);
    vp = ll_pop(llp);
    np = ll_first(llp);
    TEST_ASSERT(np->payload == (void *) c);
    vp = ll_pop(llp);
    np = ll_first(llp);
    TEST_ASSERT(np == 0);

    return 0;
}

static unsigned int test_ll_next(void)
{    
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    ll_t    *llp = 0;
    node_t  *np;
    int     dont_duplicate = 1;
    
    llp = ll_new(0, 0, 0, (pfunc *) print_str);
    llp = ll_push(llp, (void *)a, dont_duplicate);
    llp = ll_push(llp, (void *)b, dont_duplicate);
    llp = ll_push(llp, (void *)c, dont_duplicate);
    
    np = ll_first(llp);
    TEST_ASSERT(np->payload == (void *) a);
    np = ll_next(llp, np);
    TEST_ASSERT(np->payload == (void *) b);
    np = ll_next(llp, np);
    TEST_ASSERT(np->payload == (void *) c);
    np = ll_next(llp, np);
    TEST_ASSERT(np == 0);
    
    return 0;
}

int
_strcmp(item_t a, item_t b)
{
	return strcmp((char *) a, (char *) b);
}

static unsigned int test_ll_find(void)
{    
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    ll_t    *llp = 0;
    node_t  *np;
    int     dont_duplicate = 1;
    
    llp = ll_new((cmpfn *) _strcmp, 0, 0, (pfunc *) print_str);
    llp = ll_push(llp, (void *)a, dont_duplicate);
    llp = ll_push(llp, (void *)b, dont_duplicate);
    llp = ll_push(llp, (void *)c, dont_duplicate);
    
    np = ll_find(llp, "Tripp");
    TEST_ASSERT(strcmp((char *) np->payload,"Tripp") == 0);
    np = ll_find(llp, "Trapp");
    TEST_ASSERT(strcmp((char *) np->payload,"Trapp") == 0);
    np = ll_find(llp, "Trull");
    TEST_ASSERT(strcmp((char *) np->payload,"Trull") == 0);
    np = ll_find(llp, "Troll");
    TEST_ASSERT(np == 0);
    
    return 0;
}

static unsigned int test_ll_rm_link(void)
{    
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    ll_t    *llp = 0;
    node_t  *np;
    int     dont_duplicate = 1;
    
    llp = ll_new((cmpfn *) _strcmp, 0, 0, (pfunc *) print_str);
    llp = ll_push(llp, (void *)a, dont_duplicate);
    llp = ll_push(llp, (void *)b, dont_duplicate);
    llp = ll_push(llp, (void *)c, dont_duplicate);
    
    np = ll_find(llp, "Tripp");
    TEST_ASSERT(strcmp((char *) np->payload,"Tripp") == 0);
    ll_rm_link(llp, np);
    TEST_ASSERT_EQUALS( llp->n, 2);
    
    return 0;
}

static unsigned int test_ll_dup(void)
{
    char    *a = "Tripp";
    char    *b = "Trapp";
    char    *c = "Trull";
    ll_t    *llp = 0, *dup;
    node_t  *np;
    int     dont_duplicate = 1;
    
    llp = ll_new((cmpfn *) _strcmp, 0, 0, (pfunc *) print_str);
    llp = ll_push(llp, (void *)a, dont_duplicate);
    llp = ll_push(llp, (void *)b, dont_duplicate);
    llp = ll_push(llp, (void *)c, dont_duplicate);
    
    dup = ll_dup(llp);
    TEST_ASSERT_EQUALS( llp->n, 3);
    np = ll_find(dup, "Tripp");
    TEST_ASSERT(np != 0);
    np = ll_find(dup, "Trapp");
    TEST_ASSERT(np != 0);
    np = ll_find(dup, "Trull");
    TEST_ASSERT(np != 0);
    
    return 0;
}
