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

#include <skiplist.h>
#include <result.h>
#include <wrappers.h>
#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_slnode_new(void);
static unsigned int test_slist_init(void);
static unsigned int test_sl_len(void);
static unsigned int test_sl_getitem(void);
static unsigned int test_sl_find(void); 
static unsigned int test_height(void); 
static unsigned int test_sl_insert(void); 
static unsigned int test_sl_remove(void); 
static unsigned int test_sl_dup(void); 
static unsigned int test_snode_remove(void); 

test_conf_t tc[] = {
    {"test_slnode_new", test_slnode_new},
    {"test_slist_init", test_slist_init},
    {"test_sl_cmp", test_sl_len},
    {"test_sl_getitem", test_sl_getitem},
    {"test_sl_find", test_sl_find},
    {"test_height", test_height},
    {"test_sl_insert", test_sl_insert},
    {"test_sl_remove", test_sl_remove},
    {"test_sl_dup", test_sl_dup},
    {"test_snode_remove", test_snode_remove},
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

static unsigned int test_slnode_new(void)
{    
    slnode_t *snp;
    
    snp = slnode_new(0, 0, 0, 0);
    
    TEST_ASSERT_EQUALS( snp->sz, 0);
    TEST_ASSERT_EQUALS( snp->refc, 0);
    TEST_ASSERT( snp->value == 0);
    TEST_ASSERT( snp->next == 0);
    TEST_ASSERT( snp->width == 0);
    TEST_ASSERT( snp->varr == 0);
    
    return 0;
}

static unsigned int test_slist_init(void)
{    
    slist_t     *slp;
    slnode_t    *node;
    int         i;
    
    slp = slist_init(8, 3, 0, 0, 0);
    
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

int
_cmp(item_t a, item_t b, spocp_result_t *rc)
{
    *rc = SPOCP_SUCCESS;
    if (b == NULL)
        return -1;
    else
        return strcmp((char *) a, (char *) b);
}

item_t
_dup(item_t a)
{
    return (item_t) strdup((char *) a);
}

static unsigned int test_sl_len(void)
{    
    slist_t *slp;
    
    slp = slist_init(8, 3, _cmp, 0, 0);
    
    TEST_ASSERT_EQUALS(sl_len(slp), 0);
    
    return 0;
}

static unsigned int test_sl_getitem(void)
{    
    slist_t *slp;
    item_t  *value;
    
    slp = slist_init(8, 3, _cmp, 0, 0);
    
    value = sl_getitem(slp, 3);
    TEST_ASSERT(value == 0);
    
    return 0;
}

static unsigned int test_sl_find(void)
{    
    slist_t     *slp;
    sl_res_t    *result;
    int         i, size;
    
    slp = slist_init(8, 3, _cmp, 0, 0);
    size = slp->head->sz;
    
    result = sl_find(slp, (void *) "foo", NULL);
    TEST_ASSERT(result != 0);
    /* all of them should point to the tail node */
    for (i = 0; i < size; i++) {
        TEST_ASSERT(result->chain[i] == slp->head);
        TEST_ASSERT_EQUALS(result->distance[i], 0);
    }
    
    return 0;
}

static unsigned int test_height(void)
{    
    int n, i;
    
    for( i = 0; i < 100; i++) {
        n = height(10);
        /*printf("%d ",n);*/
        TEST_ASSERT( (1 <= n) && (n <= 10) );
    }
    /*printf("\n"); */
    
    return 0;
}

char *_print( slist_t *slp, int level)
{
    int         len = 0;
    slnode_t    *node;
    char        *str, *sp, width[6];
    
    len = 7;
    for(node = slp->head->next[level]; node; node = node->next[level]) {
        if (node->value)
            len += strlen((char *) node->value);
        else
            len += 1;
        
        len += 6;
    }
    
    sp = str = Calloc(len+1, sizeof(char));

    for(node = slp->head; node; node = node->next[level]) {
        if (node->value)
            sp = stpcpy(sp, (char *) node->value);
        else
            sp = stpcpy(sp, "*");
            
        sprintf(width,"(%d)",node->width[level]);
        sp = stpcpy(sp, width);
        *sp = ' ';
        sp += 1;
        *sp = '\0';
    }
    
    return str;
}

void _print_list(slist_t *slp)
{
    int     i;
    char    *str;
    
    for(i = 0 ; i < slp->maxlev ; i++) {
        str = _print(slp, i);
        printf("[%d] '%s'\n", i, str);
        Free(str);
    }
}

static unsigned int test_sl_insert(void)
{    
    slist_t     *slp;
    slnode_t    *newnode;
    int         i;
    char        *str;
    
    slp = slist_init(8, 3, _cmp, 0, 0);

    sl_insert(slp, (void *) strdup("C"));

    /* _print_list(slp); */
    str = _print(slp, 0);
    TEST_ASSERT( strcmp(str,"*(1) C(0) *(0) ") == 0);
    Free(str);
    
    TEST_ASSERT_EQUALS( slp->size, 1 ) ;
    newnode = slp->head->next[0] ;
    
    TEST_ASSERT(newnode->next[0] == slp->tail );
    for( i = 0; i < newnode->sz; i++) {
        TEST_ASSERT(slp->head->next[i] == newnode);
    }
    for (; i < slp->head->sz; i++) {
        TEST_ASSERT(slp->head->next[i] == slp->tail);
    }
    for( i = 0; i < newnode->sz; i++) {
        TEST_ASSERT_EQUALS(slp->head->width[i], 1);
        /*printf("newnode->width[%d] = %d\n",i,newnode->width[i]); */
        TEST_ASSERT_EQUALS(newnode->width[i], 0);
    }
    for (; i < slp->head->sz; i++) {
        TEST_ASSERT_EQUALS(slp->head->width[i], 1);
    }
        
    /* --------------------------------------------------------------------*/
    
    /* should be sorted after 'C' */
    sl_insert(slp, (void *) strdup("E"));
    /*_print_list(slp);*/

    str = _print(slp, 0);
    TEST_ASSERT( strcmp(str,"*(1) C(1) E(0) *(0) ") == 0);
    Free(str);
        
    TEST_ASSERT_EQUALS(slp->size, 2);

    /* --------------------------------------------------------------------*/

    sl_insert(slp, (void *) strdup("A"));
    /* _print_list(slp); */
    
    str = _print(slp, 0);
    TEST_ASSERT( strcmp(str,"*(1) A(1) C(1) E(0) *(0) ") == 0);
    Free(str);
    
    TEST_ASSERT_EQUALS(slp->size, 3);

    /* --------------------------------------------------------------------*/
    
    sl_insert(slp, (void *) strdup("H"));
    /* _print_list(slp); */
    
    str = _print(slp, 0);
    TEST_ASSERT( strcmp(str,"*(1) A(1) C(1) E(1) H(0) *(0) ") == 0);
    Free(str);
    
    TEST_ASSERT_EQUALS(slp->size, 4);

    newnode = slp->head->next[0]->next[0]->next[0];
    TEST_ASSERT_EQUALS(newnode->refc, 0);

    sl_insert(slp, (void *) strdup("E"));
    
    str = _print(slp, 0);
    TEST_ASSERT( strcmp(str,"*(1) A(1) C(1) E(1) H(0) *(0) ") == 0);
    Free(str);
    
    TEST_ASSERT_EQUALS(slp->size, 4);
    TEST_ASSERT_EQUALS(newnode->refc, 1);
    
    /* _print_list(slp); */
    return 0;
}

static unsigned int test_sl_remove(void)
{    
    slist_t     *slp;
    int         i;
    char        *str;
    
    slp = slist_init(8, 3, _cmp, 0, 0);
    
    sl_insert(slp, (void *) strdup("C"));
    sl_insert(slp, (void *) strdup("E"));
    sl_insert(slp, (void *) strdup("A"));
    sl_insert(slp, (void *) strdup("H"));
    sl_insert(slp, (void *) strdup("E"));

    str = _print(slp, i);
    TEST_ASSERT( strcmp(str,"*(1) A(1) C(1) E(1) H(0) *(0) ") == 0);
    Free(str);

    /* _print_list(slp); */

    sl_remove(slp, "C");
    
    /* _print_list(slp);  */

    str = _print(slp, i);
    TEST_ASSERT( strcmp(str,"*(1) A(1) E(1) H(0) *(0) ") == 0);
    Free(str);

    sl_remove(slp, "E");
    /* _print_list(slp); */
    
    str = _print(slp, i);
    TEST_ASSERT( strcmp(str,"*(1) A(1) E(1) H(0) *(0) ") == 0);
    Free(str);

    sl_remove(slp, "Q");
    /* _print_list(slp); */
    str = _print(slp, i);
    TEST_ASSERT( strcmp(str,"*(1) A(1) E(1) H(0) *(0) ") == 0);
    Free(str);

    sl_remove(slp, "A");
    /* _print_list(slp); */
    str = _print(slp, i);
    TEST_ASSERT( strcmp(str,"*(1) E(1) H(0) *(0) ") == 0);
    Free(str);
    
    /* remove the remaining objects */
    sl_remove(slp, "E");
    sl_remove(slp, "H");

    /* should have an empty list now */
    str = _print(slp, i);
    TEST_ASSERT( strcmp(str,"*(0) *(0) ") == 0);
    Free(str);
    
    return 0;
}

static unsigned int test_sl_dup(void)
{    
    slist_t     *slp, *dup;
    char        *str;
    
    slp = slist_init(8, 3, _cmp, 0, _dup);
    
    sl_insert(slp, (void *) strdup("C"));
    sl_insert(slp, (void *) strdup("E"));
    sl_insert(slp, (void *) strdup("A"));
    sl_insert(slp, (void *) strdup("H"));
    sl_insert(slp, (void *) strdup("E"));

    /*printf("---1---\n");*/
    dup = sl_dup(slp);
    TEST_ASSERT( dup != NULL );

    str = _print(dup, 0);
    TEST_ASSERT( strcmp(str,"*(1) A(1) C(1) E(1) H(0) *(0) ") == 0);

    return 0;
}

static unsigned int test_snode_remove(void)
{    
    slist_t     *slp, *dup;
    char        *str;
    sl_res_t    *result;
    
    slp = slist_init(8, 3, _cmp, 0, _dup);
    
    sl_insert(slp, (void *) strdup("C"));
    sl_insert(slp, (void *) strdup("E"));
    sl_insert(slp, (void *) strdup("A"));
    sl_insert(slp, (void *) strdup("H"));
    sl_insert(slp, (void *) strdup("E"));
    
    result = sl_find(slp, (void *) "A", NULL);
    TEST_ASSERT(result != NULL);
    TEST_ASSERT(strcmp(result->chain[0]->value,"A") == 0 );
    
    snode_remove(slp, result->chain[0]);

    result = sl_find(slp, (void *) "A", NULL);
    TEST_ASSERT(result != NULL);
    /* head */
    TEST_ASSERT(result->chain[0]->value == 0 );
    /* next is 'C' since 'A' is gone */
    TEST_ASSERT(strcmp(result->chain[0]->next[0]->value,"C") == 0 );

    /* There are two 'E's, first check it's there */
    result = sl_find(slp, (void *) "E", NULL);
    TEST_ASSERT(result != NULL);
    TEST_ASSERT(strcmp(result->chain[0]->value,"E") == 0 );
    
    /* remove one of the E's */
    snode_remove(slp, result->chain[0]);
    
    /* There should be one left */
    result = sl_find(slp, (void *) "E", NULL);
    TEST_ASSERT(result != NULL);
    TEST_ASSERT(strcmp(result->chain[0]->value,"E") == 0 );
    
    /* remove the last */
    snode_remove(slp, result->chain[0]);

    /* And the 'E' should be gone */
    result = sl_find(slp, (void *) "E", NULL);
    TEST_ASSERT(strcmp(result->chain[0]->value,"C") == 0 );
    TEST_ASSERT(strcmp(result->chain[0]->next[0]->value,"H") == 0 );
    
    return 0;
}

