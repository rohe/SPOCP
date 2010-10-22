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
#include <branch.h>
#include <range.h>

#include <wrappers.h>
#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_bsl_add_range(void);
static unsigned int test_atom2boundary_cmp(void);
static unsigned int test_bsl_match_atom(void);
static unsigned int test_bsl_match_range(void);
static unsigned int test_bsl_inv_match_range(void);
static unsigned int test_bsl_rm_range(void);

test_conf_t tc[] = {
    {"test_bsl_add_range", test_bsl_add_range},
    {"test_atom2boundary_cmp", test_atom2boundary_cmp},
    {"test_bsl_match_atom", test_bsl_match_atom},
    {"test_bsl_match_range", test_bsl_match_range},
    {"test_bsl_inv_match_range", test_bsl_inv_match_range},
    {"test_bsl_rm_range", test_bsl_rm_range},
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

/* returns strings of the form:
 * value '(' width ')
 */
char *_node_info(slnode_t *snp, int level)
{
    char    *str, *dval=0, *val;
    
    if (snp->value)
        val = dval = boundary_prints((boundary_t *) snp->value);
    else
        val = "*";
        
    str = Calloc(strlen(val)+10, sizeof(char));
    sprintf(str, "'%s'(%d) ", val, snp->width[level]);
    
    if (dval)
        Free(dval);
        
    return str;
}

char *_list_info(slist_t *slp, int level)
{
    int         len = 1;
    slnode_t    *node;
    char        *sp;
    char        **val;
    int         i, size;
    
    size = sl_len(slp) + 2;
    val = Calloc( size, sizeof(char *));
    
    for(node = slp->head, i = 0; node; node = node->next[level], i++) {
        val[i] = _node_info(node, level);
        len += strlen(val[i]) + 1;
    }
    
    sp = Calloc(len, sizeof(char));
    
    for(i = 0; i < size; i++) {
        sp = Strcat(sp, val[i], &len);
    }
    
    return sp;
}

char *ORDER[] = { "World", "add", "long", "pre", "suf", NULL };

static unsigned int test_bsl_add_range(void)
{    
    slist_t     *slp;
    slnode_t    *sln;
    range_t     *rp;
    octet_t     *op;
    junc_t      *jp;
    char        *s;
    int         i;
    
    op = oct_new(0, "5:alpha2:gt5:World)");
    rp = set_range(op);
    
    slp = bsl_init(8, SPOC_ALPHA);
    
    jp = bsl_add_range(slp, rp);
    
    s = _list_info(slp, 0);
    TEST_ASSERT( strcmp(s,"'*'(1) '> World'(0) '*'(0) ") == 0);
    Free(s);
    
    TEST_ASSERT( jp != NULL );

    op = oct_new(0, "5:alpha2:gt4:long2:le3:pre)");
    rp = set_range(op);
    TEST_ASSERT( rp != NULL);
    jp = bsl_add_range(slp, rp);

    op = oct_new(0, "5:alpha2:ge3:add2:lt3:suf)");
    rp = set_range(op);
    TEST_ASSERT( rp != NULL);
    jp = bsl_add_range(slp, rp);
    
    /* upper case are sorted before lower case */
    for( i = 0, sln = slp->head->next[0] ; ORDER[i]; i++, sln = sln->next[0])
        TEST_ASSERT( oct2strcmp(&((boundary_t *) sln->value)->v.val, 
                                ORDER[i]) == 0);

    return 0;
}

static unsigned int test_atom2boundary_cmp(void)
{    
    range_t         *rp;
    octet_t         *op;
    atom_t          *ap;
    int             r;
    spocp_result_t  rc;
    
    op = oct_new(0, "5:alpha2:gt5:World)");
    rp = set_range(op);
    oct_free(op);
    
    op = oct_new(0, "Sickan");
    ap = atom_new(op);
    oct_free(op);
    
    r = atom2boundary_cmp(ap, &rp->lower, &rc);

    TEST_ASSERT_EQUALS( rc, 1);
    TEST_ASSERT( r < 0 );

    range_free(rp);
    
    /* ------------------------------------------------*/
    
    op = oct_new(0, "7:numeric2:lt3:1002:ge2:17)");
    rp = set_range(op);
    oct_free(op);
    
    r = atom2boundary_cmp(ap, &rp->lower, &rc);
    TEST_ASSERT(rc != 1);
    TEST_ASSERT(r == 0);
    atom_free(ap);
    
    op = oct_new(0, "57");
    ap = atom_new(op);
    oct_free(op);

    r = atom2boundary_cmp(ap, &rp->lower, &rc);
    TEST_ASSERT_EQUALS( rc, 1);
    TEST_ASSERT( r > 0 );

    r = atom2boundary_cmp(ap, &rp->upper, &rc);
    TEST_ASSERT_EQUALS( rc, 1);
    TEST_ASSERT( r < 0 );

    range_free(rp);

    /* ------------------------------------------------*/

    op = oct_new(0, "4:ipv42:lt13:130.239.128.02:gt12:130.239.65.0)");
    rp = set_range(op);
    oct_free(op);

    r = atom2boundary_cmp(ap, &rp->lower, &rc);
    TEST_ASSERT(rc != 1);
    TEST_ASSERT(r == 0);
    atom_free(ap);

    op = oct_new(0, "130.239.93.57");
    ap = atom_new(op);
    oct_free(op);

    r = atom2boundary_cmp(ap, &rp->lower, &rc);
    TEST_ASSERT_EQUALS( rc, 1);
    TEST_ASSERT( r > 0 );
    
    r = atom2boundary_cmp(ap, &rp->upper, &rc);
    TEST_ASSERT_EQUALS( rc, 1);
    TEST_ASSERT( r < 0 );
    
    return 0;
}

char *RANGE[] = {
    "5:alpha2:gt7:rediris)",
    "5:alpha2:gt5:janet2:le7:surfnet)",
    "5:alpha2:lt5:sunet)",
    "5:alpha2:gt5:funet2:le7:uninett)",
    NULL
};

static unsigned int test_bsl_match_atom(void)
{    
    slist_t         *slp;
    range_t         *rp;
    octet_t         *op;
    varr_t          *var;
    junc_t          *jp[8];
    int             i;
    atom_t          *ap;
    spocp_result_t  rc;
    
    slp = bsl_init(8, SPOC_ALPHA);

    for (i = 0; RANGE[i]; i++) {
        op = oct_new(0, RANGE[i]);
        rp = set_range(op);        
        jp[i] = bsl_add_range(slp, rp);
    }
    
    op = oct_new(0, "dknet");
    ap = atom_new(op);
    var = bsl_match_atom(slp, ap, &rc);

    TEST_ASSERT( var != NULL );
    TEST_ASSERT( varr_find(var, jp[0]) == -1 );
    TEST_ASSERT( varr_find(var, jp[1]) == -1 );
    TEST_ASSERT( varr_find(var, jp[2]) >= 0 );
    TEST_ASSERT( varr_find(var, jp[3]) == -1 );

    op = oct_new(0, "uninett");
    ap = atom_new(op);
    var = bsl_match_atom(slp, ap, &rc);
    
    TEST_ASSERT( var != NULL );
    TEST_ASSERT( varr_find(var, jp[0]) >= 0 );
    TEST_ASSERT( varr_find(var, jp[1]) == -1 );
    TEST_ASSERT( varr_find(var, jp[2]) == -1 );
    TEST_ASSERT( varr_find(var, jp[3]) >= 0 );

    op = oct_new(0, "upnet");
    ap = atom_new(op);
    var = bsl_match_atom(slp, ap, &rc);
    
    TEST_ASSERT( var != NULL );
    TEST_ASSERT( varr_find(var, jp[0]) >= 0 );
    TEST_ASSERT( varr_find(var, jp[1]) == -1 );
    TEST_ASSERT( varr_find(var, jp[2]) == -1 );
    TEST_ASSERT( varr_find(var, jp[3]) == -1 );

    op = oct_new(0, "funet");
    ap = atom_new(op);
    var = bsl_match_atom(slp, ap, &rc);
    
    TEST_ASSERT( var != NULL );
    TEST_ASSERT( varr_find(var, jp[0]) == -1 );
    TEST_ASSERT( varr_find(var, jp[1]) == -1 );
    TEST_ASSERT( varr_find(var, jp[2]) >= 0 );
    TEST_ASSERT( varr_find(var, jp[3]) == -1 );
    
    return 0;
}

static unsigned int test_bsl_match_range(void)
{    
    slist_t         *slp;
    range_t         *rp;
    octet_t         *op;
    varr_t          *var;
    junc_t          *jp[8];
    int             i;
    
    slp = bsl_init(8, SPOC_ALPHA);
    
    for (i = 0; RANGE[i]; i++) {
        op = oct_new(0, RANGE[i]);
        rp = set_range(op);        
        jp[i] = bsl_add_range(slp, rp);
    }

    op = oct_new(0, "5:alpha2:lt5:janet)");
    rp = set_range(op);        
    
    var = bsl_match_range(slp, rp);
    TEST_ASSERT(var != NULL);
    TEST_ASSERT_EQUALS(var->n, 1);
    TEST_ASSERT( varr_find(var, jp[0]) == -1 );
    TEST_ASSERT( varr_find(var, jp[1]) == -1 );
    TEST_ASSERT( varr_find(var, jp[2]) >= 0 );
    TEST_ASSERT( varr_find(var, jp[3]) == -1 );

    op = oct_new(0, "5:alpha2:gt7:surfnet)");
    rp = set_range(op);        
    
    var = bsl_match_range(slp, rp);
    TEST_ASSERT(var != NULL);
    TEST_ASSERT_EQUALS(var->n, 1);
    TEST_ASSERT( varr_find(var, jp[0]) >= 0 );
    TEST_ASSERT( varr_find(var, jp[1]) == -1 );
    TEST_ASSERT( varr_find(var, jp[2]) == -1 );
    TEST_ASSERT( varr_find(var, jp[3]) == -1 );

    op = oct_new(0, "5:alpha2:gt5:henet2:le7:rediris)");
    rp = set_range(op);        
    
    var = bsl_match_range(slp, rp);
    TEST_ASSERT(var != NULL);
    TEST_ASSERT_EQUALS(var->n, 2);
    TEST_ASSERT( varr_find(var, jp[0]) == -1 );
    TEST_ASSERT( varr_find(var, jp[1]) == -1 );
    TEST_ASSERT( varr_find(var, jp[2]) >= 0 );
    TEST_ASSERT( varr_find(var, jp[3]) >= 0 );

    op = oct_new(0, "5:alpha2:gt5:arnet2:le7:uninett)");
    rp = set_range(op);        
    
    var = bsl_match_range(slp, rp);
    TEST_ASSERT(var == NULL);
    
    return 0;
}

static unsigned int test_bsl_inv_match_range(void)
{    
    slist_t         *slp;
    range_t         *rp;
    octet_t         *op;
    varr_t          *var;
    junc_t          *jp[8];
    int             i;
    
    slp = bsl_init(8, SPOC_ALPHA);
    
    for (i = 0; RANGE[i]; i++) {
        op = oct_new(0, RANGE[i]);
        rp = set_range(op);        
        jp[i] = bsl_add_range(slp, rp);
    }
    
    op = oct_new(0, "5:alpha2:lt5:janet)");
    rp = set_range(op);        
    
    var = bsl_inv_match_range(slp, rp);
    TEST_ASSERT(var == NULL);

    op = oct_new(0, "5:alpha2:gt5:janet)");
    rp = set_range(op);        
    
    var = bsl_inv_match_range(slp, rp);
    TEST_ASSERT(var != NULL);
    TEST_ASSERT_EQUALS(var->n, 2);
    TEST_ASSERT( varr_find(var, jp[0]) >= 0 );
    TEST_ASSERT( varr_find(var, jp[1]) >= 0 );
    TEST_ASSERT( varr_find(var, jp[2]) == -1 );
    TEST_ASSERT( varr_find(var, jp[3]) == -1 );
    
    op = oct_new(0, "5:alpha2:gt5:henet2:le7:uninett)");
    rp = set_range(op);        
    
    var = bsl_inv_match_range(slp, rp);
    TEST_ASSERT(var != NULL);
    TEST_ASSERT_EQUALS(var->n, 1);
    TEST_ASSERT( varr_find(var, jp[0]) == -1 );
    TEST_ASSERT( varr_find(var, jp[1]) >= 0 );
    TEST_ASSERT( varr_find(var, jp[2]) == -1 );
    TEST_ASSERT( varr_find(var, jp[3]) == -1 );
    
    op = oct_new(0, "5:alpha2:gt5:arnet2:le7:uninett)");
    rp = set_range(op);        
    
    var = bsl_inv_match_range(slp, rp);
    TEST_ASSERT(var != NULL);
    TEST_ASSERT_EQUALS(var->n, 2);
    TEST_ASSERT( varr_find(var, jp[0]) == -1 );
    TEST_ASSERT( varr_find(var, jp[1]) >= 0 );
    TEST_ASSERT( varr_find(var, jp[2]) == -1 );
    TEST_ASSERT( varr_find(var, jp[3]) >= 0 );
    
    return 0;
}

static unsigned int test_bsl_rm_range(void)
{    
    slist_t         *slp;
    range_t         *rp;
    octet_t         *op;
    varr_t          *var;
    junc_t          *jp[8], *rjp;
    int             i;
    spocp_result_t  rc;
    
    slp = bsl_init(8, SPOC_ALPHA);
    
    for (i = 0; RANGE[i]; i++) {
        op = oct_new(0, RANGE[i]);
        rp = set_range(op);        
        jp[i] = bsl_add_range(slp, rp);
    }

    op = oct_new(0, RANGE[1]);
    rp = set_range(op);        

    var = bsl_match_range(slp, rp);
    TEST_ASSERT(var->n == 2);
    TEST_ASSERT( varr_find(var, jp[0]) == -1 );
    TEST_ASSERT( varr_find(var, jp[1]) >= 0 );
    TEST_ASSERT( varr_find(var, jp[2]) == -1 );
    TEST_ASSERT( varr_find(var, jp[3]) >= 0 );
    
    /* remove the second range */
    rjp = bsl_rm_range(slp, rp, &rc);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT(rjp != NULL);

    var = bsl_match_range(slp, rp);
    TEST_ASSERT(var->n == 1);
    TEST_ASSERT( varr_find(var, jp[0]) == -1 );
    TEST_ASSERT( varr_find(var, jp[1]) == -1 );
    TEST_ASSERT( varr_find(var, jp[2]) == -1 );
    TEST_ASSERT( varr_find(var, jp[3]) >= 0 );

    /* try to remove a non existent range */
    
    op = oct_new(0, "5:alpha2:gt7:surfnet)");
    rp = set_range(op);
    
    rjp = bsl_rm_range(slp, rp, &rc);
    TEST_ASSERT_EQUALS(rc, SPOCP_CAN_NOT_PERFORM);
    TEST_ASSERT(rjp == NULL);
    
    return 0;
}
