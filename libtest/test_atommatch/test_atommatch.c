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

#include <spocp.h>
#include <proto.h>
#include <wrappers.h>
#include <atom.h>
#include <db0.h>
#include <testRunner.h>
#include <atommatch.h>
#include <varr.h>
#include <match.h>

/*
 * test function 
 */
static unsigned int test_prefix2atoms_match(void);
static unsigned int test_suffix2atoms_match(void);
static unsigned int test_range2atoms_match(void);
static unsigned int test_get_all_atom_followers(void);

test_conf_t tc[] = {
    {"test_prefix2atoms_match", test_prefix2atoms_match},
    {"test_suffix2atoms_match", test_suffix2atoms_match},
    {"test_range2atoms_match", test_range2atoms_match},
    {"test_get_all_atom_followers", test_get_all_atom_followers},
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

db_t * _add_to_db(db_t *dbp, char **namn)
{
    octet_t         rule;
    octarr_t        *oa = NULL;
    spocp_result_t  sr;
    ruleinst_t      *ri;
    
    if (dbp == NULL)
        dbp = db_new();
    
    for (; *namn; namn++) {
        oa = octarr_new(2);
        oct_assign(&rule, *namn);
        octarr_add(oa, &rule);
        
        sr = add_right(&dbp, NULL, oa, &ri, NULL);
        Free(oa);        
    }
    
    return dbp;
}

/* ======================================================================= */

void varr_clear( varr_t *va)
{
    int i;
    
	for( i = 0; i < va->n; i++ )
        va->arr[i] = 0;
    
    va->n = 0;
}

/* ----------------------------------------------------------------*/

static unsigned int test_prefix2atoms_match(void)
{    
    db_t    *dbp;
    varr_t  *varr;
    phash_t *php;
    
    varr = varr_new(4);
    dbp = _add_to_db(NULL, PITCHER);
    /* lists and the first atom in those */
    php = dbp->jp->branch[1]->val.list->branch[0]->val.atom;
    
    varr = prefix2atoms_match("Ch", php, varr);
    TEST_ASSERT(varr->n == 2);
    /* both of them pointing further on to sets of one name */
    TEST_ASSERT_EQUALS( ((junc_t *) varr->arr[0])->branch[0]->val.atom->n, 1)
    TEST_ASSERT_EQUALS( ((junc_t *) varr->arr[1])->branch[0]->val.atom->n, 1)

    varr_clear(varr);
    varr = prefix2atoms_match("A", php, varr);
    TEST_ASSERT(varr->n == 4);

    varr_clear(varr);
    varr = prefix2atoms_match("F", php, varr);
    TEST_ASSERT(varr->n == 0);
    
    return 0;
}

static unsigned int test_suffix2atoms_match(void)
{    
    db_t    *dbp;
    varr_t  *varr;
    phash_t *php;
    
    varr = varr_new(4);
    dbp = _add_to_db(NULL, PITCHER);
    dbp = _add_to_db(dbp, INFIELD);
    
    /* lists and the first atom in those */
    php = dbp->jp->branch[1]->val.list->branch[0]->val.atom;
    
    varr = suffix2atoms_match("in", php, varr);
    TEST_ASSERT(varr->n == 2);
    /* both of them pointing further on to sets of one name */
    TEST_ASSERT_EQUALS( ((junc_t *) varr->arr[0])->branch[0]->val.atom->n, 1)
    TEST_ASSERT_EQUALS( ((junc_t *) varr->arr[1])->branch[0]->val.atom->n, 1)
    
    varr_clear(varr);
    varr = suffix2atoms_match("ark", php, varr);
    TEST_ASSERT(varr->n == 1);
    TEST_ASSERT_EQUALS( ((junc_t *) varr->arr[0])->branch[0]->val.atom->n, 2)
    
    varr_clear(varr);
    varr = suffix2atoms_match("o", php, varr);
    TEST_ASSERT(varr->n == 7);

    varr_clear(varr);
    varr = suffix2atoms_match("do", php, varr);
    TEST_ASSERT(varr->n == 2);

    varr_clear(varr);
    varr = suffix2atoms_match("h", php, varr);
    TEST_ASSERT(varr->n == 0);

    varr_clear(varr);
    varr = suffix2atoms_match("ido", php, varr);
    TEST_ASSERT(varr->n == 0);
    
    return 0;
}

char *A[] = { "Alfredo", "Andrew", "A.J.", "Andy", "Alex", NULL };
char *DE[] = { "David", "Derek", "Edwar", "Eduardo", NULL };

static unsigned int test_range2atoms_match(void)
{    
    db_t    *dbp;
    varr_t  *varr;
    phash_t *php;
    range_t *rp;
    octet_t *op;
    char    **str;
    atom_t  *ap;
    junc_t  *jp;
    
    varr = varr_new(4);
    dbp = _add_to_db(NULL, PITCHER);
    dbp = _add_to_db(dbp, INFIELD);

    /* lists and the first atom in those */
    php = dbp->jp->branch[1]->val.list->branch[0]->val.atom;
    
    op = oct_new(0, "5:alpha2:lt2:Bo)");
    rp = set_range(op);

    varr = range2atoms_match(rp, php, varr);

    for (str = A; *str; str++) {
        op = oct_new(0, *str);
        ap = atom_new(op);
        jp = atom2atom_match(ap, php);
        TEST_ASSERT( varr_find(varr, jp) >= 0);
        atom_free(ap);
    }
    
    TEST_ASSERT(varr->n == 5);

    op = oct_new(0, "5:alpha2:ge5:David2:le4:Eric)");
    rp = set_range(op);

    varr_clear(varr);
    varr = range2atoms_match(rp, php, varr);

    for (str = DE; *str; str++) {
        op = oct_new(0, *str);
        ap = atom_new(op);
        jp = atom2atom_match(ap, php);
        TEST_ASSERT( varr_find(varr, jp) >= 0);
        atom_free(ap);
    }
    
    TEST_ASSERT(varr->n == 4);

    return 0;
}

static unsigned int test_get_all_atom_followers(void)
{    
    db_t        *dbp;
    varr_t      *varr;
    branch_t    *bp;
    int         n;
    char        **str;
    
    varr = varr_new(4);
    dbp = _add_to_db(NULL, PITCHER);
    for (n = 0, str = PITCHER; *str; str++, n++) ;
    
    /* lists and the first atom in those */
    bp = dbp->jp->branch[1]->val.list->branch[0];
    
    varr = get_all_atom_followers(bp, varr);
    
    TEST_ASSERT(varr->n == n);

    dbp = _add_to_db(dbp, INFIELD);
    for (str = INFIELD; *str; str++, n++) ;
    n--; /* two persons has the same name */
    
    varr_clear(varr);
    varr = get_all_atom_followers(bp, varr);
        
    TEST_ASSERT(varr->n == n);
    
    return 0;
}

