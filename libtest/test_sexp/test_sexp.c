/* test program for lib/sexp.c */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <spocp.h>
#include <proto.h>
#include <wrappers.h>

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <spocp.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */

static unsigned int test_get_len(void);
static unsigned int test_get_str(void);
static unsigned int test_sexp_len(void);
static unsigned int test_sexp_printa(void);
static unsigned int test_octseq2octarr(void);

test_conf_t tc[] = {
    {"test_get_len", test_get_len},
    {"test_get_str", test_get_str},
    {"test_sexp_len", test_sexp_len},
    {"test_sexp_printa", test_sexp_printa},
    {"test_octseq2octarr", test_octseq2octarr},
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

static unsigned int test_get_len(void)
{
    spocp_result_t  rc = 0;
    octet_t         *oct;
    int             n;
    
    oct = oct_new(0, "4:http");
    TEST_ASSERT_EQUALS(get_len(oct, &rc), 4);
    /* printf("rc: %d", rc); */
    TEST_ASSERT_EQUALS(rc, 0);

    oct_free(oct);
    /* printf("freed\n"); */
    oct = oct_new(0,"43:foo");
    n = get_len(oct, &rc);
    /* printf("len -> %d (%d)\n", n, rc); */
    TEST_ASSERT_EQUALS(n, 43);
    TEST_ASSERT_EQUALS(rc, 0);

    oct_free(oct);
    oct = oct_new(0,"43;foo");
    n = get_len(oct, &rc);
    TEST_ASSERT_EQUALS(n, -1);

    oct_free(oct);
    oct = oct_new(0,"4a:foo");
    n = get_len(oct, &rc);
    TEST_ASSERT_EQUALS(n, -1);
    
    return 0;
}

static unsigned int test_get_str(void)
{
    octet_t         *from, to;
    
    memset(&to, 0, sizeof(octet_t));
    
    from = oct_new(0, "4:http3:foo");
    get_str(from, &to);
    TEST_ASSERT_EQUALS(to.len,4);
    TEST_ASSERT_EQUALS(to.size,0);
    TEST_ASSERT( memcmp(to.val,"http",4) == 0);
    TEST_ASSERT_EQUALS(from->len,5);
    TEST_ASSERT_EQUALS(from->size,11);
    TEST_ASSERT( memcmp(from->val,"3:foo",5) == 0);
    /* get the second part */
    get_str(from, &to);
    TEST_ASSERT_EQUALS(to.len,3);
    TEST_ASSERT_EQUALS(to.size,0);
    TEST_ASSERT( memcmp(to.val,"foo",4) == 0);
    TEST_ASSERT_EQUALS(from->len,0);
    TEST_ASSERT_EQUALS(from->size,11);
    return 0;
}

static unsigned int test_sexp_len(void)
{
    octet_t         *sexp;
        
    sexp = oct_new(0, "(4:http3:foo)");    
    TEST_ASSERT_EQUALS(sexp_len(sexp),13);
    oct_free(sexp);

    sexp = oct_new(0, "(4:http3:foo)3:hex");
    TEST_ASSERT_EQUALS(sexp_len(sexp),13);
    oct_free(sexp);
    
    sexp = oct_new(0, "4:http3:foo)3:hex");
    TEST_ASSERT_EQUALS(sexp_len(sexp), -1);    
    oct_free(sexp);
    
    sexp = oct_new(0, "(4:http(3:foo)3:hex");
    TEST_ASSERT_EQUALS(sexp_len(sexp), 0);    
    
    return 0;
}


static unsigned int test_sexp_printa(void)
{
    char            sexp[1024];
    char            *a = "http", *b="method", *c="GET";
    void            *argv[16], *arr[3];
    unsigned int    size=1024;
    octet_t         *oap, *obp, *ocp;

    argv[0] = a;
    argv[1] = b;
    argv[2] = c;
    argv[3] = 0;
    sexp_printa(sexp, &size, "(aa)", argv);
    TEST_ASSERT( strcmp(sexp,"(4:http6:method)") == 0);
    TEST_ASSERT_EQUALS( size, 1006);

    size = 1024;
    sexp_printa(sexp, &size, "(a(aa))", argv);
    /*
    printf("Sexp: '%s'\n", sexp);
    printf("size: %d\n", size);
     */
    TEST_ASSERT( strcmp(sexp,"(4:http(6:method3:GET))") == 0);
    TEST_ASSERT_EQUALS( size, 999);
    
    oap = oct_new(0, a);
    obp = oct_new(0, b);
    ocp = oct_new(0, c);

    argv[0] = oap;
    argv[1] = obp;
    argv[2] = ocp;
    argv[3] = 0;
    size = 1024;
    sexp_printa(sexp, &size, "(oo)", argv);
    /*
     printf("Sexp: '%s'\n", sexp);
     printf("size: %d\n", size);
     */
    TEST_ASSERT( strcmp(sexp,"(4:http6:method)") == 0);
    TEST_ASSERT_EQUALS( size, 1006);

    size = 1024;
    sexp_printa(sexp, &size, "(o(oo))", argv);
    TEST_ASSERT( strcmp(sexp,"(4:http(6:method3:GET))") == 0);
    TEST_ASSERT_EQUALS( size, 999);

    arr[0] = obp;
    arr[1] = ocp;
    arr[2] = 0;
    argv[1] = arr;
    argv[2] = 0;
    
    size = 1024;
    sexp_printa(sexp, &size, "(o(O))", argv);
    TEST_ASSERT( strcmp(sexp,"(4:http(6:method3:GET))") == 0);
    TEST_ASSERT_EQUALS( size, 999);
    
    return 0;
}

static unsigned int test_octseq2octarr(void)
{
    octarr_t        *oarr=NULL;
    octet_t         oct;
    spocp_result_t  r;
    
    oct_assign(&oct,"4:rule5:Tripp");
    
    r = octseq2octarr(&oct, &oarr);
    TEST_ASSERT_EQUALS( r, SPOCP_SUCCESS );
    TEST_ASSERT_EQUALS(oarr->n, 2);
    TEST_ASSERT(oct2strcmp(oarr->arr[0], "rule") == 0);
    TEST_ASSERT(oct2strcmp(oarr->arr[1], "Tripp") == 0);
    
    return 0;
}
