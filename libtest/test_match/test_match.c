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
#include <match.h>

#include <wrappers.h>
#include <testRunner.h>

void *bsdrb_init();

/*
 * test function 
 */
static unsigned int test_atom2atom_match(void);
static unsigned int test_atom2prefix_match(void);
static unsigned int test_atom2suffix_match(void);
static unsigned int test_prefix2prefix_match(void);
static unsigned int test_suffix2suffix_match(void);
static unsigned int test_element_match_r(void);

test_conf_t tc[] = {
    {"test_atom2atom_match", test_atom2atom_match},
    {"test_atom2prefix_match", test_atom2prefix_match},
    {"test_atom2suffix_match", test_atom2suffix_match},
    {"test_prefix2prefix_match", test_prefix2prefix_match},
    {"test_suffix2suffix_match", test_suffix2suffix_match},
    {"test_element_match_r", test_element_match_r},
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

/* ------------------------------------------------------------------------ */
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

test_info_t PREFIX[] = {
    { "(4:rule(1:*6:prefix3:ref)3:foo)", NULL },
    { "(4:rule(1:*6:prefix4:berg))", NULL },
    { NULL, NULL }
};

test_info_t SUFFIX[] = {
    { "(4:rule(1:*6:suffix4:chef)3:xxx)", NULL },
    { "(4:rule(1:*6:suffix4:berg))", NULL },
    { NULL, NULL }
};

db_t * _add_to_db(db_t *dbp, test_info_t *ti)
{
    int             i;
    octet_t         rule, blob;
    octarr_t        *oa = NULL;
    spocp_result_t  sr;
    ruleinst_t      *ri;

    if (dbp == NULL)
        dbp = db_new();

    for (i = 0; ti[i].rule; i++) {
        oa = octarr_new(2);
        oct_assign(&rule, ti[i].rule);
        octarr_add(oa, &rule);
        if (ti[i].blob) {
            oct_assign(&blob, ti[i].blob);
            octarr_add(oa, &blob);
        }
        
        sr = add_right(&dbp, NULL, oa, &ri, NULL);
        Free(oa);        
    }
    
    return dbp;
}

/* ------------------------------------------------------------------------ */

static unsigned int test_atom2atom_match(void)
{    
    atom_t          *ap=NULL;
    phash_t         *php=NULL;
    buck_t          *bp;
    octet_t         *op;
    db_t            *dbp;
    junc_t          *jp;
    
    php = phash_new(4, 70);
    
    op = oct_new(0, "Sickan");
    ap = atom_new(op);
    bp = phash_insert(php, ap);
    atom_free(ap);
    
    op = oct_new(0, "Bananen");
    ap = atom_new(op);
    jp = atom2atom_match(ap, php);
    TEST_ASSERT(jp == 0);

    bp = phash_insert(php, ap);
    TEST_ASSERT( phash_search(php, ap) != NULL);
    jp = atom2atom_match(ap, php);
    TEST_ASSERT(jp == 0);

    atom_free(ap);
    phash_free(php);
    
    /* +++++++++++++++++++++++++++++++++ */
    
    dbp = _add_to_db(NULL, INFO);
    
    op = oct_new(0, "rule");
    ap = atom_new(op);

    php = dbp->jp->branch[1]->val.list->branch[SPOC_ATOM]->val.atom;
    TEST_ASSERT(atom2atom_match(ap, php) != NULL);

    return 0;
}


static unsigned int test_atom2prefix_match(void)
{    
    db_t            *dbp;
    octet_t         *op;
    atom_t          *ap=NULL;
    phash_t         *php=NULL;
    junc_t          *jp;
    varr_t          *varr;
    buck_t          *bp;
    int             i;
    
    dbp = _add_to_db(NULL, PREFIX);

    op = oct_new(0, "rule");
    ap = atom_new(op);
    
    php = dbp->jp->branch[1]->val.list->branch[SPOC_ATOM]->val.atom;
    jp = atom2atom_match(ap, php);
    TEST_ASSERT( jp != NULL);

    op = oct_new(0, "reference");
    ap = atom_new(op);

    varr = atom2prefix_match(ap, jp->branch[SPOC_PREFIX]->val.prefix);
    TEST_ASSERT( varr != NULL);
    php = ((junc_t *) varr->arr[0])->branch[0]->val.atom;
    op = oct_new(0, "foo");
    ap = atom_new(op);
    bp = phash_search(php, ap);
    TEST_ASSERT( bp != NULL );
    TEST_ASSERT( oct2strcmp(&bp->val, "foo") == 0 );

    op = oct_new(0, "bergfors");
    ap = atom_new(op);
    
    varr = atom2prefix_match(ap, jp->branch[SPOC_PREFIX]->val.prefix);
    TEST_ASSERT( varr != NULL);
    TEST_ASSERT_EQUALS(varr->n, 1);
    jp = varr_first(varr);
    /* Only only one branch and that the ENDOFLIST branch */
    for (i = 0; i < NTYPES; i++) {
        if (i == SPOC_ENDOFLIST) {
            TEST_ASSERT( jp->branch[i] != NULL);
        } else {
            TEST_ASSERT( jp->branch[i] == NULL);
        }
    }
    
    return 0;
}

static unsigned int test_atom2suffix_match(void)
{    
    db_t            *dbp;
    octet_t         *op;
    atom_t          *ap=NULL;
    phash_t         *php=NULL;
    junc_t          *jp;
    varr_t          *varr;
    buck_t          *bp;
    int             i;
    
    dbp = _add_to_db(NULL, SUFFIX);
    
    op = oct_new(0, "rule");
    ap = atom_new(op);
    
    php = dbp->jp->branch[1]->val.list->branch[SPOC_ATOM]->val.atom;
    jp = atom2atom_match(ap, php);
    TEST_ASSERT( jp != NULL);
    
    op = oct_new(0, "enhetschef");
    ap = atom_new(op);
    
    varr = atom2suffix_match(ap, jp->branch[SPOC_SUFFIX]->val.suffix);
    TEST_ASSERT( varr != NULL);
    php = ((junc_t *) varr->arr[0])->branch[0]->val.atom;
    op = oct_new(0, "xxx");
    ap = atom_new(op);
    bp = phash_search(php, ap);
    TEST_ASSERT( bp != NULL );
    TEST_ASSERT( oct2strcmp(&bp->val, "xxx") == 0 );
    
    op = oct_new(0, "hedberg");
    ap = atom_new(op);
    
    varr = atom2suffix_match(ap, jp->branch[SPOC_SUFFIX]->val.suffix);
    TEST_ASSERT( varr != NULL);
    TEST_ASSERT_EQUALS(varr->n, 1);
    jp = varr_first(varr);
    /* Only only one branch and that the ENDOFLIST branch */
    for (i = 0; i < NTYPES; i++) {
        if (i == SPOC_ENDOFLIST) {
            TEST_ASSERT( jp->branch[i] != NULL);
        } else {
            TEST_ASSERT( jp->branch[i] == NULL);
        }
    }
    
    return 0;
}

static unsigned int test_prefix2prefix_match(void)
{    
    db_t            *dbp;
    octet_t         *op;
    atom_t          *ap=NULL;
    phash_t         *php=NULL;
    junc_t          *jp;
    varr_t          *varr;
    buck_t          *bp;
    int             i;
    
    dbp = _add_to_db(NULL, PREFIX);
    
    op = oct_new(0, "rule");
    ap = atom_new(op);
    
    php = dbp->jp->branch[1]->val.list->branch[SPOC_ATOM]->val.atom;
    jp = atom2atom_match(ap, php);
    TEST_ASSERT( jp != NULL);
    
    op = oct_new(0, "reference");
    ap = atom_new(op);
    
    varr = prefix2prefix_match(ap, jp->branch[SPOC_PREFIX]->val.prefix);
    TEST_ASSERT( varr != NULL);
    php = ((junc_t *) varr->arr[0])->branch[0]->val.atom;
    op = oct_new(0, "foo");
    ap = atom_new(op);
    bp = phash_search(php, ap);
    TEST_ASSERT( bp != NULL );
    TEST_ASSERT( oct2strcmp(&bp->val, "foo") == 0 );
        
    op = oct_new(0, "bergfors");
    ap = atom_new(op);
    
    varr = prefix2prefix_match(ap, jp->branch[SPOC_PREFIX]->val.prefix);
    TEST_ASSERT( varr != NULL);
    TEST_ASSERT_EQUALS(varr->n, 1);
    jp = varr_first(varr);
    /* Only only one branch and that the ENDOFLIST branch */
    for (i = 0; i < NTYPES; i++) {
        if (i == SPOC_ENDOFLIST) {
            TEST_ASSERT( jp->branch[i] != NULL);
        } else {
            TEST_ASSERT( jp->branch[i] == NULL);
        }
    }
    
    return 0;
}

static unsigned int test_suffix2suffix_match(void)
{    
    db_t            *dbp;
    octet_t         *op;
    atom_t          *ap=NULL;
    phash_t         *php=NULL;
    junc_t          *jp;
    varr_t          *varr;
    buck_t          *bp;
    int             i;
    
    dbp = _add_to_db(NULL, SUFFIX);
    
    op = oct_new(0, "rule");
    ap = atom_new(op);
    
    php = dbp->jp->branch[1]->val.list->branch[SPOC_ATOM]->val.atom;
    jp = atom2atom_match(ap, php);
    TEST_ASSERT( jp != NULL);
    
    op = oct_new(0, "enhetschef");
    ap = atom_new(op);
    
    varr = suffix2suffix_match(ap, jp->branch[SPOC_SUFFIX]->val.suffix);
    TEST_ASSERT( varr != NULL);
    php = ((junc_t *) varr->arr[0])->branch[0]->val.atom;
    op = oct_new(0, "xxx");
    ap = atom_new(op);
    bp = phash_search(php, ap);
    TEST_ASSERT( bp != NULL );
    TEST_ASSERT( oct2strcmp(&bp->val, "xxx") == 0 );
        
    op = oct_new(0, "hedberg");
    ap = atom_new(op);
    
    varr = suffix2suffix_match(ap, jp->branch[SPOC_SUFFIX]->val.suffix);
    TEST_ASSERT( varr != NULL);
    TEST_ASSERT_EQUALS(varr->n, 1);
    jp = varr_first(varr);
    /* Only only one branch and that the ENDOFLIST branch */
    for (i = 0; i < NTYPES; i++) {
        if (i == SPOC_ENDOFLIST) {
            TEST_ASSERT( jp->branch[i] != NULL);
        } else {
            TEST_ASSERT( jp->branch[i] == NULL);
        }
    }
    
    return 0;
}

static unsigned int test_element_match_r(void)
{    
    db_t        *dbp;
    resset_t    *res;
    comparam_t  comp;
    element_t   *ep=NULL;
    octet_t     oct;
	checked_t   *cr=0;
    octarr_t    *on = 0;


    dbp = _add_to_db(NULL, INFO);
    dbp = _add_to_db(dbp, SUFFIX);
    dbp = _add_to_db(dbp, PREFIX);
    
    memset(&comp, 0, sizeof(comparam_t));
    oct_assign(&oct, INFO[1].rule);
    get_element(&oct, &ep);

    comp.rc = SPOCP_SUCCESS;
	comp.head = ep;
    comp.blob = &on;
    comp.cr = &cr;
    
    res = element_match_r(dbp->jp, ep, &comp);
    TEST_ASSERT( res != NULL);

    
    oct_assign(&oct, "(4:rule4:four)");
    get_element(&oct, &ep);
    cr = 0;
    on = 0;
    comp.rc = SPOCP_SUCCESS;
	comp.head = ep;

    res = element_match_r(dbp->jp, ep, &comp);
    TEST_ASSERT(res == NULL);

    return 0;
}
