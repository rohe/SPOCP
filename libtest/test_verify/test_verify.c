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
#include <arpa/inet.h>

/* #include <spocp.h> */
#include <proto.h>
#include <wrappers.h>
#include <result.h>
#include <verify.h>
#include <log.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_is_numeric(void);
static unsigned int test_is_date(void);
static unsigned int test_is_ipv4(void);
static unsigned int test_is_time(void);

test_conf_t tc[] = {
    {"test_is_numeric", test_is_numeric},
    {"test_is_date", test_is_date},
    {"test_is_ipv4", test_is_ipv4},
    {"test_is_time", test_is_time},
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

static unsigned int test_is_numeric(void)
{    
    octet_t oct;
    long    l;
    
    oct_assign(&oct, "123456");
    TEST_ASSERT(is_numeric(&oct, &l) == SPOCP_SUCCESS);
    oct_assign(&oct, "1234a");
    TEST_ASSERT(is_numeric(&oct, &l) == SPOCP_SYNTAXERROR);
    
    return 0;
}

static unsigned int test_is_date(void)
{    
    octet_t oct;
    
    oct_assign(&oct, "2002-12-31T23:59:59Z");
    TEST_ASSERT(is_date(&oct) == SPOCP_SUCCESS);
    
    oct_assign(&oct, "2002-12-31T23:59:59+01:30");
    TEST_ASSERT(is_date(&oct) == SPOCP_SUCCESS);

    oct_assign(&oct, "2002-12-31T23:59:59.123Z");
    TEST_ASSERT(is_date(&oct) == SPOCP_SUCCESS);

    oct_assign(&oct, "2002-12-31T23:59:59.01+03:15");
    TEST_ASSERT(is_date(&oct) == SPOCP_SUCCESS);

    return 0;
}

static unsigned int test_is_ipv4(void)
{    
    octet_t         oct;
    struct in_addr  ia;

    oct_assign(&oct, "193.16.81.2");
    TEST_ASSERT(is_ipv4(&oct, &ia) == SPOCP_SUCCESS);

    oct_assign(&oct, "193.16.81.256");
    TEST_ASSERT(is_ipv4(&oct, &ia) == SPOCP_SYNTAXERROR);

    TEST_ASSERT(is_ipv4_s("193.16.81.2", &ia) == SPOCP_SUCCESS);
    TEST_ASSERT(is_ipv4_s("193.16.81.256", &ia) == SPOCP_SYNTAXERROR);

    return 0;
}

static unsigned int test_is_time(void)
{    
    octet_t         oct;
    
    oct_assign(&oct, "08:12:53");
    TEST_ASSERT(is_time(&oct) == SPOCP_SUCCESS);
    
    oct_assign(&oct, "32:32:32");
    TEST_ASSERT(is_time(&oct) == SPOCP_SYNTAXERROR);

    oct_assign(&oct, "13:65:32");
    TEST_ASSERT(is_time(&oct) == SPOCP_SYNTAXERROR);

    oct_assign(&oct, "13:23:64");
    TEST_ASSERT(is_time(&oct) == SPOCP_SYNTAXERROR);
    
    return 0;
}
