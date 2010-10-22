#include <stdlib.h>
#include <spocpcli.h>
#include <octet.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_fail_open(void);
/* static unsigned int test_atom2str(void); */

test_conf_t tc[] = {
    {"test_fail_open", test_fail_open},
/*    {"test_atom2str", test_atom2str}, */
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

static unsigned int test_fail_open(void)
{
    SPOCP   *spocp = NULL;
    char    *server = "localhost:5678";
    int     timeout = 1;
    
    spocp = spocpc_open( 0, server, timeout ) ;
    TEST_ASSERT( spocp == NULL);
    
    return 0;
}
