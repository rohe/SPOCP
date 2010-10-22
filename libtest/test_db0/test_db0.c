/*
 *  test_cachetime.c
 *
 *  Created by Roland Hedberg on 12/3/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

/* test program for lib/cachetime.c */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <spocp.h>
#include <proto.h>
#include <element.h>
#include <db0.h>
#include <hash.h>

#include <wrappers.h>
#include <testRunner.h>

void *bsdrb_init();

/*
 * test function 
 */
static unsigned int test_db_new(void);
static unsigned int test_save_rule(void);
static unsigned int test_rules(void);
static unsigned int test_get_all_rules(void);
static unsigned int test_store_right(void);
static unsigned int test_add_right(void);

test_conf_t tc[] = {
    {"test_db_new", test_db_new},
    {"test_save_rule", test_save_rule},
    {"test_rules", test_rules},
    {"test_get_all_rules", test_get_all_rules},
    {"test_store_right", test_store_right},
    {"test_add_right", test_add_right},
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

typedef struct _test_info {
    char    *rule;
    char    *blob;
} test_info_t;

test_info_t INFO[] = {
    { "(4:rule3:one)", "BLOB" },
    { "(4:rule3:two)", NULL },
    { "(4:rule5:three)", NULL },
    { NULL, NULL }
};

static unsigned int test_db_new(void)
{    
    db_t    *dbp;
    
    dbp = db_new();

    TEST_ASSERT(dbp != 0);

    return 0;
}

static unsigned int test_save_rule(void)
{    
    db_t        *dbp;
    octet_t     rule, blob;
    ruleinst_t  *rip;
    
    oct_assign(&rule, INFO[0].rule);
    oct_assign(&blob, INFO[0].blob);
    
    dbp = db_new();
    rip = save_rule(dbp, NULL, &rule, &blob, NULL);
    TEST_ASSERT(rip != NULL);
    TEST_ASSERT(octcmp(rip->rule, &rule) == 0);
    TEST_ASSERT(octcmp(rip->blob, &blob) == 0);
    return 0;
}

static unsigned int test_rules(void)
{    
    db_t    *dbp;
    octet_t rule, blob, *op;
    int     i;
    
    dbp = db_new();

    for (i=0; INFO[i].rule; i++) {
        oct_assign(&rule, INFO[i].rule);
        if (INFO[i].blob) {
            oct_assign(&blob, INFO[i].blob);
            op = &blob;
        }
        else {
            op = NULL;
        }

        save_rule(dbp, NULL, &rule, op, NULL);
        TEST_ASSERT_EQUALS( rules(dbp), 1);
        TEST_ASSERT_EQUALS( rdb_len(dbp->ri->rules), i+1);
    }

    return 0;
}

static unsigned int test_get_all_rules(void)
{    
    db_t            *dbp;
    octet_t         rule, blob, *op;
    spocp_result_t  sr;
    octarr_t        *oa = NULL;
    int             i;
    
    dbp = db_new();
    
    for (i=0; INFO[i].rule; i++) {
        oct_assign(&rule, INFO[i].rule);
        if (INFO[i].blob) {
            oct_assign(&blob, INFO[i].blob);
            op = &blob;
        }
        else {
            op = NULL;
        }
        
        save_rule(dbp, NULL, &rule, op, NULL);
    }

    sr = get_all_rules(dbp, &oa, NULL);
    TEST_ASSERT(sr == SPOCP_SUCCESS) ;
    TEST_ASSERT_EQUALS(oa->n, 3);
    
    return 0;
}

static unsigned int test_store_right(void)
{    
    db_t            *dbp;
    octet_t         rule, blob, *op;
    spocp_result_t  sr;
    ruleinst_t      *ri;
    element_t       *ep;
    int             i;
    
    dbp = db_new();

    for (i=0; INFO[i].rule; i++) {
        oct_assign(&rule, INFO[i].rule);
        if (INFO[i].blob) {
            oct_assign(&blob, INFO[i].blob);
            op = &blob;
        }
        else {
            op = NULL;
        }
        
        ri = save_rule(dbp, NULL, &rule, op, NULL);
        sr = get_element(&rule, &ep);
        sr = store_right(dbp, ep, ri);
        TEST_ASSERT( sr == SPOCP_SUCCESS );
    }
        
    return 0;
}


static unsigned int test_add_right(void)
{    
    db_t            *dbp;
    octet_t         rule, blob;
    octarr_t        *oa;
    spocp_result_t  sr;
    ruleinst_t      *ri;
    branch_t        *branch;
    phash_t         *hp;
    buck_t          *bucket;
    int             i, j;
    element_t       *ep;
    
    dbp = db_new();
    oa = octarr_new(2);

    for (i=0; INFO[i].rule; i++) {
        oct_assign(&rule, INFO[i].rule);
        octarr_add(oa, &rule);
        if (INFO[i].blob) {
            oct_assign(&blob, INFO[i].blob);
            octarr_add(oa, &blob);
        }
        
        sr = add_right(&dbp, NULL, oa, &ri, NULL);
        TEST_ASSERT( sr == SPOCP_SUCCESS );
        TEST_ASSERT( dbp->jp != NULL );
        TEST_ASSERT( dbp->ri != NULL );
        TEST_ASSERT( dbp->plugins == NULL );
        TEST_ASSERT( dbp->dback == NULL );
        for (j = 0; j < NTYPES; j++) {
            if (j == 1) {
                TEST_ASSERT(dbp->jp->branch[j] != NULL);
            } else {
                TEST_ASSERT(dbp->jp->branch[j] == NULL);
            }
        }
        branch = dbp->jp->branch[1];
        /* all of the lists starts with an atom */
        TEST_ASSERT( branch->val.list->branch[SPOC_ATOM] != NULL);
        for (j = 0; j < DATATYPES; j++) {
            if (j != SPOC_ATOM)
                TEST_ASSERT( branch->val.list->branch[j] == NULL);
        }
        hp = branch->val.list->branch[0]->val.atom;

        sr = get_element(&rule, &ep);        
        bucket = phash_search(hp, ep->e.list->head->e.atom);
        TEST_ASSERT( bucket != NULL);
        element_free(ep);
    }
        
    return 0;
}


