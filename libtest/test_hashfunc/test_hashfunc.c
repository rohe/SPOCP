/*
 *  check_oct.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 12/3/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */


#include <string.h>

#include <hashfunc.h>

#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_lhash(void);

test_conf_t tc[] = {
    {"test_lhash", test_lhash},
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

static unsigned int test_lhash(void)
{    
    unsigned int    ui=13;
    char            *str;
    
    str = (char *) "String to calculate hash on";
    
    ui = lhash((unsigned char *) str, strlen(str), ui);    
    TEST_ASSERT(ui != 13);
    TEST_ASSERT_EQUALS(ui, 2683864018);

    ui = lhash((unsigned char *) str, strlen(str), ui);
    TEST_ASSERT(ui != 2683864018);
    TEST_ASSERT_EQUALS(ui, 1939761559);
    
    return 0;
}
