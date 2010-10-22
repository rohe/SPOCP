/* test program for lib/sexp.c */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <spocp.h>
#include <octet.h>
#include <proto.h>
#include <wrappers.h>
#include <subelem.h>

#include <testRunner.h>
/* #include <libtest.h> */

/*
 * test function 
 */

static unsigned int test_subelem_new(void);
static unsigned int test_subelem_free(void);
static unsigned int test_pattern_parse(void);

test_conf_t tc[] = {
    {"test_subelem_new", test_subelem_new},
    {"test_subelem_free", test_subelem_free},
    {"test_pattern_parse", test_pattern_parse},
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

static unsigned int test_subelem_new(void)
{
    subelem_t   *se;
    
    se = subelem_new();
    TEST_ASSERT(se != NULL);
    
    return 0;
}

static unsigned int test_subelem_free(void)
{
    subelem_t   *se;
    
    se = subelem_new();
    subelem_free(se);
    TEST_ASSERT(1);
    
    return 0;
}

char *PATTERN[] = { "7:+4:rule6:+3:one", "7:+4:rule13:+(3:one3:two)"};
static unsigned int test_pattern_parse(void)
{
    subelem_t   *se, *nse;
    octarr_t    *oarr=NULL;
    octet_t     oct;
    element_t   *ep;

    oct_assign(&oct, PATTERN[0]);
    octseq2octarr(&oct, &oarr);
    se = pattern_parse(oarr);

    TEST_ASSERT( se != NULL);
    TEST_ASSERT_EQUALS(se->direction, GT);
    TEST_ASSERT( se->list == 0 );
    TEST_ASSERT( se->ep != NULL);
    ep = se->ep;
    TEST_ASSERT_EQUALS(ep->type, SPOC_ATOM);
    TEST_ASSERT( oct2strcmp(&ep->e.atom->val, "rule") == 0 );
    TEST_ASSERT( se->next != NULL );
    nse = se->next;
    TEST_ASSERT_EQUALS(nse->direction, GT);
    TEST_ASSERT( nse->list == 0 );
    TEST_ASSERT( nse->ep != NULL);
    ep = nse->ep;
    TEST_ASSERT_EQUALS(ep->type, SPOC_ATOM);
    TEST_ASSERT( oct2strcmp(&ep->e.atom->val, "one") == 0 );
    TEST_ASSERT( nse->next == NULL );

    octarr_free(oarr);
    subelem_free(se);
    /* have to start with a blank slate */
    oarr = NULL;
    
    oct_assign(&oct, PATTERN[1]);
    octseq2octarr(&oct, &oarr);
    se = pattern_parse(oarr);

    TEST_ASSERT( se != NULL);
    TEST_ASSERT_EQUALS(se->direction, GT);
    TEST_ASSERT( se->list == 0 );
    TEST_ASSERT( se->ep != NULL);
    ep = se->ep;
    TEST_ASSERT_EQUALS(ep->type, SPOC_ATOM);
    TEST_ASSERT( oct2strcmp(&ep->e.atom->val, "rule") == 0 );
    TEST_ASSERT( se->next != NULL );

    se = se->next;
    TEST_ASSERT_EQUALS(se->direction, GT);
    TEST_ASSERT( se->list == 1 );
    ep = se->ep;
    TEST_ASSERT(ep->type == SPOC_LIST)
    TEST_ASSERT( oct2strcmp(&ep->e.list->head->e.atom->val, "one") == 0);
    TEST_ASSERT( oct2strcmp(&ep->e.list->head->next->e.atom->val, "two") == 0);

    return 0;
}

