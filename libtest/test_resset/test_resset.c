/* test program for lib/sexp.c */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <resset.h>
#include <spocp.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>
/* #include <libtest.h> */

/*
 * test function 
 */

static unsigned int test_resset_new(void);
static unsigned int test_resset_free(void);
static unsigned int test_resset_len(void);
static unsigned int test_resset_add(void);
static unsigned int test_resset_extend(void);

test_conf_t tc[] = {
    {"test_resset_new", test_resset_new},
    {"test_resset_free", test_resset_free},
    {"test_resset_len", test_resset_len},
    {"test_resset_add", test_resset_add},
    {"test_resset_extend", test_resset_extend},
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

typedef struct _test {
    char *rule;
    char *blob;
} _test_t;

_test_t EXAMPLE[] = {
    { "(4:rule3:one)", NULL },
    { "(4:rule3:two)", "Tripp" },
    { NULL, NULL }
};

static unsigned int test_resset_new(void)
{
    octarr_t        *blobarr=NULL;
    resset_t        *rsp;
    octet_t         rule, *op;
    ruleinst_t      *rip;
    _test_t         *ex;
    
    ex = &EXAMPLE[0];
    oct_assign(&rule, ex->rule);
    rip = ruleinst_new(&rule, NULL, NULL);
    
    rsp = resset_new(rip, blobarr);
    
    TEST_ASSERT(rsp != NULL);
    TEST_ASSERT(rsp->blob == blobarr);
    TEST_ASSERT(rsp->next == 0);
    TEST_ASSERT( oct2strcmp(rsp->ri->rule, ex->rule) == 0 );

    /* -------------- */
    
    ex = &EXAMPLE[0];
    oct_assign(&rule, ex->rule);
    rip = ruleinst_new(&rule, NULL, NULL);
    
    op = oct_new(0, ex->blob);
    blobarr = octarr_add(blobarr, op);

    rsp = resset_new(rip, blobarr);
    
    TEST_ASSERT(rsp != NULL);
    TEST_ASSERT(rsp->blob == blobarr);
    TEST_ASSERT(rsp->next == 0);
    TEST_ASSERT( oct2strcmp(rsp->ri->rule, ex->rule) == 0 );
    
    return 0;
}

static unsigned int test_resset_free(void)
{
    octarr_t        *blobarr=NULL;
    resset_t        *rsp;
    octet_t         rule, *op;
    ruleinst_t      *rip;
    _test_t         *ex;

    ex = &EXAMPLE[0];
    oct_assign(&rule, ex->rule);
    rip = ruleinst_new(&rule, NULL, NULL);
    
    rsp = resset_new(rip, blobarr);
    resset_free(rsp);
    
    TEST_ASSERT(1);

    ex = &EXAMPLE[1];
    oct_assign(&rule, ex->rule);
    rip = ruleinst_new(&rule, NULL, NULL);
    
    op = oct_new(0, ex->blob);
    blobarr = octarr_add(blobarr, op);
    
    rsp = resset_new(rip, blobarr);
    resset_free(rsp);

    TEST_ASSERT(1);
    
    return 0;
}

static unsigned int test_resset_add(void)
{
    octarr_t        *blobarr=NULL;
    resset_t        *rsp = NULL;
    octet_t         rule, *op;
    ruleinst_t      *rip;
    _test_t         *tt;

    for (tt=&EXAMPLE[0]; tt->rule; tt++ ) {
        oct_assign(&rule, tt->rule);
        rip = ruleinst_new(&rule, NULL, NULL);
        
        if (tt->blob) {
            op = oct_new(0, tt->blob);
            blobarr = octarr_add(blobarr, op);
        }
        else {
            blobarr = NULL;
        }

        rsp = resset_add(rsp, rip, blobarr);
    }

    TEST_ASSERT(rsp != NULL);
    TEST_ASSERT(rsp->next != NULL);
    
    return 0;
}

static unsigned int test_resset_len(void)
{
    octarr_t        *blobarr=NULL;
    resset_t        *rsp = NULL;
    octet_t         rule, *op;
    ruleinst_t      *rip;
    _test_t         *tt;
    int             i;
    
    for (i=0, tt=&EXAMPLE[0]; tt->rule; tt++,i++ ) {
        
        TEST_ASSERT_EQUALS( resset_len(rsp), i);

        oct_assign(&rule, tt->rule);
        rip = ruleinst_new(&rule, NULL, NULL);
        
        if (tt->blob) {
            op = oct_new(0, tt->blob);
            blobarr = octarr_add(blobarr, op);
        }
        else {
            blobarr = NULL;
        }
        
        rsp = resset_add(rsp, rip, blobarr);
    }
    
    TEST_ASSERT_EQUALS( resset_len(rsp), 2);
    
    return 0;
}

static unsigned int test_resset_extend(void)
{
    octarr_t        *blobarr=NULL;
    resset_t        *rsp[] = { NULL, NULL };
    octet_t         rule, *op;
    ruleinst_t      *rip;
    _test_t         *tt;
    int             i;
    
    for (i=0,tt=&EXAMPLE[0]; tt->rule; tt++,i++ ) {
        oct_assign(&rule, tt->rule);
        rip = ruleinst_new(&rule, NULL, NULL);
        
        if (tt->blob) {
            op = oct_new(0, tt->blob);
            blobarr = octarr_add(blobarr, op);
        }
        else {
            blobarr = NULL;
        }
        
        rsp[i] = resset_add(rsp[i], rip, blobarr);
    }
    
    rsp[0] = resset_extend(rsp[0], rsp[1]);
    
    tt=&EXAMPLE[0];
    TEST_ASSERT_EQUALS( resset_len(rsp[0]), 2);
    TEST_ASSERT( oct2strcmp(rsp[0]->ri->rule, tt->rule) == 0 );
    ++tt; 
    TEST_ASSERT( oct2strcmp(rsp[0]->next->ri->rule, tt->rule) == 0 );
    TEST_ASSERT( oct2strcmp(rsp[0]->next->blob->arr[0], tt->blob) == 0 );
    
    return 0;
}

