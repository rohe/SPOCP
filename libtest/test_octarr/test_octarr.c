/* test program for lib/sexp.c */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <spocp.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>
/* #include <libtest.h> */

/*
 * test function 
 */

static unsigned int test_octarr_new(void);
static unsigned int test_octarr_add(void);
static unsigned int test_octarr_dup(void);
static unsigned int test_octarr_mr(void);
static unsigned int test_octarr_half_free(void);
static unsigned int test_octarr_pop(void);
static unsigned int test_octarr_rpop(void);
static unsigned int test_octarr_join(void);
static unsigned int test_octarr_rm(void);
static unsigned int test_oct_split(void);

test_conf_t tc[] = {
    {"test_octarr_new", test_octarr_new},
    {"test_octarr_add", test_octarr_add},
    {"test_octarr_dup", test_octarr_dup},
    {"test_octarr_mr", test_octarr_mr},
    {"test_octarr_half_free", test_octarr_half_free},
    {"test_octarr_pop", test_octarr_pop},
    {"test_octarr_rpop", test_octarr_rpop},
    {"test_octarr_join", test_octarr_join},
    {"test_octarr_rm", test_octarr_rm},
    {"test_oct_split", test_oct_split},
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

static unsigned int test_octarr_new(void)
{
    octarr_t         *octarr;
    
    octarr = octarr_new(4);
    TEST_ASSERT_EQUALS(octarr->size, 4);
    TEST_ASSERT_EQUALS(octarr->n, 0);
    TEST_ASSERT(octarr->arr != NULL);
    
    return 0;
}

static unsigned int test_octarr_add(void)
{
    octarr_t    *octarr=NULL;
    octet_t     *oap, *obp, *ocp, *op;
    
    oap = oct_new(0, "Tripp");
    octarr = octarr_add(0, oap);
    
    TEST_ASSERT_EQUALS(octarr->size, 4);
    TEST_ASSERT_EQUALS(octarr->n, 1);
    op = octarr->arr[0];
    TEST_ASSERT(octcmp(op, oap) == 0);

    obp = oct_new(0, "Trapp");
    octarr = octarr_add(octarr, obp);

    TEST_ASSERT_EQUALS(octarr->size, 4);
    TEST_ASSERT_EQUALS(octarr->n, 2);
    op = octarr->arr[0];
    TEST_ASSERT(octcmp(op, oap) == 0);
    op = octarr->arr[1];
    TEST_ASSERT(octcmp(op, obp) == 0);
    
    ocp = oct_new(0, "Trull");
    octarr = octarr_add(octarr, ocp);
    
    TEST_ASSERT_EQUALS(octarr->size, 4);
    TEST_ASSERT_EQUALS(octarr->n, 3);
    op = octarr->arr[0];
    TEST_ASSERT(octcmp(op, oap) == 0);
    op = octarr->arr[1];
    TEST_ASSERT(octcmp(op, obp) == 0);
    op = octarr->arr[2];
    TEST_ASSERT(octcmp(op, ocp) == 0);

    octarr = octarr_add(octarr, oap);
    octarr = octarr_add(octarr, obp);
    octarr = octarr_add(octarr, ocp);
    TEST_ASSERT_EQUALS(octarr->size, 8);
    TEST_ASSERT_EQUALS(octarr->n, 6);

    return 0;
}

void print_arr(octarr_t *oa)
{
    int         i;
    char        *s;
    
    for (i=0; i < oa->n; i++) {
        s = oct2strdup(oa->arr[i], 0);
        printf("(%d) '%s'\n", i, s);
        Free(s);
    }
}

static unsigned int test_octarr_dup(void)
{
    octarr_t    *octarr=NULL, *oacpy;
    octet_t     *oap, *obp, *ocp;
    
    oap = oct_new(0, "Tripp");
    obp = oct_new(0, "Trapp");
    ocp = oct_new(0, "Trull");
    octarr = octarr_add(0, oap);
    octarr_add(octarr, obp);
    octarr_add(octarr, ocp);
    /*
    printf("==3==\n");
    print_arr(octarr);
     */
    
    oacpy = octarr_dup(octarr);
    
    TEST_ASSERT_EQUALS(oacpy->size, octarr->size);
    TEST_ASSERT_EQUALS(oacpy->n, octarr->n);
    TEST_ASSERT(octcmp(oap, oacpy->arr[0]) == 0);
    TEST_ASSERT(octcmp(obp, oacpy->arr[1]) == 0);
    TEST_ASSERT(octcmp(ocp, oacpy->arr[2]) == 0);

    return 0;
}

static unsigned int test_octarr_mr(void)
{
    octarr_t    *octarr;
    octet_t     *oap, *obp, *ocp;
        
    octarr = octarr_new(4);
    TEST_ASSERT_EQUALS(octarr->size, 4);
    TEST_ASSERT_EQUALS(octarr->n, 0);
    TEST_ASSERT(octarr->arr != NULL);

    oap = oct_new(0, "Tripp");
    obp = oct_new(0, "Trapp");
    ocp = oct_new(0, "Trull");
    octarr = octarr_add(0, oap);
    octarr_add(octarr, obp);
    octarr_add(octarr, ocp);
    TEST_ASSERT_EQUALS(octarr->n, 3);

    octarr_mr(octarr, 8);

    TEST_ASSERT_EQUALS(octarr->size, 8);
    TEST_ASSERT_EQUALS(octarr->n, 3);
    TEST_ASSERT(octcmp(oap, octarr->arr[0]) == 0);
    TEST_ASSERT(octcmp(obp, octarr->arr[1]) == 0);
    TEST_ASSERT(octcmp(ocp, octarr->arr[2]) == 0);
    
    return 0;
}

static unsigned int test_octarr_half_free(void)
{
    octarr_t    *octarr=NULL;
    octet_t     *oap, *obp, *ocp;
    char        *s;
    
    oap = oct_new(0, "Tripp");
    obp = oct_new(0, "Trapp");
    ocp = oct_new(0, "Trull");
    octarr = octarr_add(0, oap);
    octarr_add(octarr, obp);
    octarr_add(octarr, ocp);

    octarr_half_free(octarr);
    /* should still be able to access the octet structs */
    
    s = oct2strdup(oap, '%');
    TEST_ASSERT( strcmp(s, "Tripp") == 0 );
    s = oct2strdup(obp, '%');
    TEST_ASSERT( strcmp(s, "Trapp") == 0 );
    s = oct2strdup(ocp, '%');
    TEST_ASSERT( strcmp(s, "Trull") == 0 );
    
    return 0;
}

static unsigned int test_octarr_pop(void)
{
    octarr_t    *octarr=NULL;
    octet_t     *oap, *obp, *ocp, *op;
    
    oap = oct_new(0, "Tripp");
    obp = oct_new(0, "Trapp");
    ocp = oct_new(0, "Trull");
    octarr = octarr_add(0, oap);
    octarr_add(octarr, obp);
    octarr_add(octarr, ocp);
    
    op = octarr_pop(octarr);
    TEST_ASSERT(octcmp(op, oap) == 0);
    TEST_ASSERT_EQUALS(octarr->n, 2);

    op = octarr_pop(octarr);
    TEST_ASSERT(octcmp(op, obp) == 0);
    TEST_ASSERT_EQUALS(octarr->n, 1);

    op = octarr_pop(octarr);
    TEST_ASSERT(octcmp(op, ocp) == 0);
    TEST_ASSERT_EQUALS(octarr->n, 0);

    op = octarr_pop(octarr);
    TEST_ASSERT( op == 0);
    
    return 0;
}

static unsigned int test_octarr_rpop(void)
{
    octarr_t    *octarr=NULL;
    octet_t     *oap, *obp, *ocp, *op;
    
    oap = oct_new(0, "Tripp");
    obp = oct_new(0, "Trapp");
    ocp = oct_new(0, "Trull");
    octarr = octarr_add(0, oap);
    octarr_add(octarr, obp);
    octarr_add(octarr, ocp);
    
    op = octarr_rpop(octarr);
    TEST_ASSERT(octcmp(op, ocp) == 0);
    TEST_ASSERT_EQUALS(octarr->n, 2);
    
    op = octarr_rpop(octarr);
    TEST_ASSERT(octcmp(op, obp) == 0);
    TEST_ASSERT_EQUALS(octarr->n, 1);
    
    op = octarr_rpop(octarr);
    TEST_ASSERT(octcmp(op, oap) == 0);
    TEST_ASSERT_EQUALS(octarr->n, 0);
    
    op = octarr_pop(octarr);
    TEST_ASSERT( op == 0);
    
    return 0;
}

static unsigned int test_octarr_join(void)
{
    octarr_t    *oarr1=NULL, *oarr2=NULL;
    octet_t     *oap, *obp, *ocp;
    
    oap = oct_new(0, "Tripp");
    obp = oct_new(0, "Trapp");
    ocp = oct_new(0, "Trull");
    
    oarr1 = octarr_add(oarr1, oap);
    oarr2 = octarr_add(oarr2, obp);
    oarr2 = octarr_add(oarr2, ocp);

    oarr1 = octarr_extend(oarr1, oarr2);
    TEST_ASSERT_EQUALS(oarr1->n, 3);
    TEST_ASSERT(octcmp(oarr1->arr[0], oap) == 0);
    TEST_ASSERT(octcmp(oarr1->arr[1], obp) == 0);
    TEST_ASSERT(octcmp(oarr1->arr[2], ocp) == 0);

    return 0;
}

static unsigned int test_octarr_rm(void)
{
    octarr_t    *oarr1=NULL;
    octet_t     *oap, *obp, *ocp, *op;
    
    oap = oct_new(0, "Tripp");
    obp = oct_new(0, "Trapp");
    ocp = oct_new(0, "Trull");
    
    oarr1 = octarr_add(oarr1, oap);
    oarr1 = octarr_add(oarr1, obp);
    oarr1 = octarr_add(oarr1, ocp);
    
    op = octarr_rm(oarr1, 1);
    TEST_ASSERT_EQUALS(oarr1->n, 2);
    TEST_ASSERT(octcmp(op, obp) == 0);
    
    return 0;
}

static unsigned int test_oct_split(void)
{
    octarr_t    *octarr=NULL;
    octet_t     *oct;

    oct = oct_new(0, "abc:def:ghi:jkl:mno");
    octarr = oct_split(oct, ':', 0, 0, 0);
    
    TEST_ASSERT_EQUALS(octarr->n, 5);
    TEST_ASSERT(oct2strcmp(octarr->arr[0], "abc") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[1], "def") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[2], "ghi") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[3], "jkl") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[4], "mno") == 0);
    octarr_free(octarr);
    
    octarr = oct_split(oct, ':', 0, 0, 2);
    TEST_ASSERT_EQUALS(octarr->n, 2);
    TEST_ASSERT(oct2strcmp(octarr->arr[0], "abc") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[1], "def:ghi:jkl:mno") == 0);

    octarr_free(octarr);
    octarr = oct_split(oct, ':', 0, 0, 3);
    TEST_ASSERT_EQUALS(octarr->n, 3);
    TEST_ASSERT(oct2strcmp(octarr->arr[0], "abc") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[1], "def") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[2], "ghi:jkl:mno") == 0);

    octarr_free(octarr);
    oct_free(oct);
    oct = oct_new(0, "abc::ghi:jkl");
    octarr = oct_split(oct, ':', 0, 0, 0);
    
    TEST_ASSERT_EQUALS(octarr->n, 4);
    TEST_ASSERT(oct2strcmp(octarr->arr[0], "abc") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[1], "") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[2], "ghi") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[3], "jkl") == 0);
    octarr_free(octarr);

    octarr = oct_split(oct, ':', 0, 1, 0);    
    /* print_arr(octarr); */
    TEST_ASSERT_EQUALS(octarr->n, 3);
    TEST_ASSERT(oct2strcmp(octarr->arr[0], "abc") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[1], "ghi") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[2], "jkl") == 0);
    octarr_free(octarr);

    octarr = oct_split(oct, ':', 0, 1, 2);
    TEST_ASSERT_EQUALS(octarr->n, 2);
    TEST_ASSERT(oct2strcmp(octarr->arr[0], "abc") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[1], "ghi:jkl") == 0);
    
    octarr_free(octarr);
    octarr = oct_split(oct, ':', 0, 1, 3);
    /*print_arr(octarr); */
    TEST_ASSERT_EQUALS(octarr->n, 3);
    TEST_ASSERT(oct2strcmp(octarr->arr[0], "abc") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[1], "ghi") == 0);
    TEST_ASSERT(oct2strcmp(octarr->arr[2], "jkl") == 0);
    
    return 0;
}
