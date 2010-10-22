/*
 *  test_cachetime.c
 *
 *  Created by Roland Hedberg on 12/3/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <spocp.h>
#include <proto.h>
#include <element.h>

#include <wrappers.h>
#include <testRunner.h>
#include <dbapi.h>

/*
 * test function 
 */
static unsigned int test_spocp_add_rule(void);
static unsigned int test_spocp_list_rules(void);
static unsigned int test_spocp_del_rule(void);
static unsigned int test_spocp_allowed(void);
static unsigned int test_spocp_dup(void);

test_conf_t tc[] = {
    {"test_spocp_add_rule", test_spocp_add_rule},
    {"test_spocp_list_rules", test_spocp_list_rules},
    {"test_spocp_del_rule", test_spocp_del_rule},
    {"test_spocp_allowed", test_spocp_allowed},
    {"test_spocp_dup", test_spocp_dup},
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

/* ======================================================================= */


char *PITCHER[] = {
    "(7:Alfredo6:Aceves)",
    "(8:Jonathan10:Albaladejo)",
    "(6:Andrew8:Brackman)",
    "(4:A.J.7:Burnett)",
    "(4:Joba11:Chamberlain)",
    "(6:Wilkin10:De La Rosa)",
    "(9:Christian6:Garcia)",
    "(4:Chad6:Gaudin)",
    "(4:Phil6:Hughes)",
    "(5:Boone5:Logan)",
    "(6:Damaso5:Marte)",
    "(4:Mark8:Melancon)",
    "(6:Sergio5:Mitre)",
    "(6:Hector5:Noesi)",
    "(4:Ivan4:Nova)",
    "(4:Andy8:Pettitte)",
    "(5:Edwar7:Ramirez)",
    "(7:Mariano6:Rivera)",
    "(5:David9:Robertson)",
    "(2:CC8:Sabathia)",
    "(6:Romulo7:Sanchez)",
    "(6:Javier7:Vazquez)",
    NULL
};

char *INFIELD[] = {
	"(8:Robinson4:Cano)",
 	"(6:Reegie6:Corona)",
 	"(5:Derek5:Jeter)",
    "(4:Nick7:Johnson)",
 	"(4:Juan7:Miranda)",
    "(7:Eduardo5:Nunez)",
 	"(6:Ramiro4:Pena)",
    "(4:Alex9:Rodriguez)",
 	"(5:Kevin5:Russo)",
    "(4:Mark8:Teixeira)",
    NULL
};

db_t * _add_to_db(db_t *dbp, char **rule)
{
    octet_t         regel;
    octarr_t        *oa = NULL;
    spocp_result_t  sr;
    ruleinst_t      *ri;
    
    if (dbp == NULL)
        dbp = db_new();
    
    for (; *rule; rule++) {
        oa = octarr_new(2);
        oct_assign(&regel, *rule);
        octarr_add(oa, &regel);
        
        sr = add_right(&dbp, NULL, oa, &ri, NULL);
        Free(oa);        
    }
    
    return dbp;
}

/* ======================================================================= */

static unsigned int test_spocp_add_rule(void)
{    
    spocp_result_t  rc;
    octet_t         oct;
    db_t            *dp=NULL;
    octarr_t        *oa=NULL;
    void            *rule;
    
    oct_assign(&oct, INFIELD[1]);
    oa = octarr_add(oa, &oct);
    rc = spocp_add_rule(&dp, oa);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);

    rc = spocp_add_rule(&dp, oa);
    TEST_ASSERT_EQUALS(rc, SPOCP_EXISTS);

    return 0;
}

static unsigned int test_spocp_list_rules(void)
{    
    spocp_result_t  rc;
    db_t            *dp=NULL;
    octarr_t        *oa=NULL;
    
    dp = _add_to_db(dp, INFIELD);
    
    rc = spocp_list_rules(dp, NULL, &oa, "/");
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(oa->n, 10);

    octarr_free(oa);
    oa = NULL;
    
    dp = _add_to_db(dp, PITCHER);
    rc = spocp_list_rules(dp, NULL, &oa, "/");
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(oa->n, 32);
    
    return 0;
}

static unsigned int test_spocp_del_rule(void)
{    
    spocp_result_t  rc;
    db_t            *dp=NULL;
    octet_t         oct;
    varr_t          *pa;
    ruleinst_t      *ri;
    
    dp = _add_to_db(dp, INFIELD);
    /* array with all the rules instances */
    pa = rdb_all(dp->ri->rules);   
    TEST_ASSERT_EQUALS(pa->n, 10);
    
    ri = (ruleinst_t *) pa->arr[0];
    oct_assign(&oct, ri->uid);
    rc = spocp_del_rule(dp, &oct);
    
    varr_free(pa);
    pa = rdb_all(dp->ri->rules);
    TEST_ASSERT_EQUALS(pa->n, 9);
    
    db_free(dp);
    
    return 0;
}

static unsigned int test_spocp_allowed(void)
{    
    spocp_result_t  rc;
    db_t            *dp=NULL;
    octet_t         sexp;
    resset_t        *rp= NULL;

    dp = _add_to_db(dp, INFIELD);

    oct_assign(&sexp, INFIELD[2]);
    rc = spocp_allowed(dp, &sexp, &rp);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    oct_assign(&sexp, "(5:Derek10:Sandersson5:Jeter)");
    rc = spocp_allowed(dp, &sexp, &rp);
    TEST_ASSERT_EQUALS(rc, SPOCP_DENIED);
    
    return 0;
}

static unsigned int test_spocp_dup(void)
{    
    spocp_result_t  rc;
    db_t            *dp=NULL, *copy;
    octet_t         sexp;
    resset_t        *rp= NULL;
    varr_t          *pa, *cpa;
    void            *v;
    
    dp = _add_to_db(dp, INFIELD);
    
    copy = spocp_dup(dp, &rc);
    
    oct_assign(&sexp, INFIELD[2]);
    rc = spocp_allowed(copy, &sexp, &rp);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    oct_assign(&sexp, "(5:Derek10:Sandersson5:Jeter)");
    rc = spocp_allowed(dp, &sexp, &rp);
    TEST_ASSERT_EQUALS(rc, SPOCP_DENIED);

    pa = rdb_all(dp->ri->rules);   
    cpa = rdb_all(copy->ri->rules);   

    for (v = varr_first(cpa); v ; v =varr_next(cpa, v))
        TEST_ASSERT( varr_find(pa, v) >= 0 );
    
    return 0;
}
