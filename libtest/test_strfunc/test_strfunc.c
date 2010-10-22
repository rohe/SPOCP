/*
 *  check_string.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/31/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

/* test program for lib/oct.c */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <octet.h>
#include <proto.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_rmcrlf(void);
static unsigned int test_rmlt(void);
static unsigned int test_line_split(void);
static unsigned int test_find_balancing(void);

test_conf_t tc[] = {
    {"test_rmcrlf", test_rmcrlf},
    {"test_rmlt", test_rmlt},
    {"test_line_split", test_line_split},
    {"test_find_balancing", test_find_balancing},
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

static unsigned int test_rmcrlf(void)
{
    char    *sp, *str = "line";
    
    sp = strdup("line\r");
    rmcrlf(sp);
    TEST_ASSERT(strcmp(sp, str) == 0); 
    sp = strdup("line\r\n");
    rmcrlf(sp);
    TEST_ASSERT(strcmp(sp, str) == 0); 
    sp = strdup("line\n\r");
    rmcrlf(sp);
    TEST_ASSERT(strcmp(sp, str) == 0); 
    sp = strdup("line\r\n\r\n");
    rmcrlf(sp);
    TEST_ASSERT(strcmp(sp, str) == 0); 
    
    return 0;
}

static unsigned int test_rmlt(void)
{
    char    *sp, *str = "line";
    
    sp = strdup(" line ");
    sp = rmlt(sp);
    TEST_ASSERT(strcmp(sp, str) == 0); 
    sp = strdup("line  ");
    sp = rmlt(sp);
    TEST_ASSERT(strcmp(sp, str) == 0); 
    sp = strdup("\tline\t");
    sp = rmlt(sp);
    TEST_ASSERT(strcmp(sp, str) == 0); 
    sp = strdup("\t line \t ");
    sp = rmlt(sp);
    TEST_ASSERT(strcmp(sp, str) == 0); 
    
    return 0;
}

char *ARR[] = { "urn", "mace", "umu.se", "ed", "uid", NULL };

static unsigned int test_line_split(void)
{
    char    **arr;
    int     n, i;

    arr = line_split("urn:mace:umu.se:ed:uid", ':', '\\', 0, 0, &n); 
    TEST_ASSERT_EQUALS(n, 5);
    for (i=0; ARR[i]; i++) {
        TEST_ASSERT( strcmp(arr[i], ARR[i]) == 0 );
    }

    arr = line_split("urn:mace:umu.se:ed:uid", ':', '\\', 0, 3, &n); 
    TEST_ASSERT_EQUALS(n, 3);
    TEST_ASSERT( strcmp(arr[0], ARR[0]) == 0 );
    TEST_ASSERT( strcmp(arr[1], ARR[1]) == 0 );
    TEST_ASSERT( strcmp(arr[2], "umu.se:ed:uid") == 0 );

    arr = line_split("urn:mace:umu.se:ed\\:uid", ':', '\\', 0, 0, &n); 
    TEST_ASSERT_EQUALS(n, 4);
    TEST_ASSERT( strcmp(arr[0], ARR[0]) == 0 );
    TEST_ASSERT( strcmp(arr[1], ARR[1]) == 0 );
    TEST_ASSERT( strcmp(arr[2], ARR[2]) == 0 );
    TEST_ASSERT( strcmp(arr[3], "ed\\:uid") == 0 );

    arr = line_split(strdup("urn:mace:umu.se:ed:uid:"), ':', '\\', 1, 0, &n); 
    TEST_ASSERT_EQUALS(n, 5);
    for (i=0; ARR[i]; i++) {
        TEST_ASSERT( strcmp(arr[i], ARR[i]) == 0 );
    }
    
    return 0;
}

static unsigned int test_find_balancing(void)
{
    char    *rp, *str;
    
    str = "abc)";
    rp = find_balancing(str, '(', ')');
    TEST_ASSERT( rp != NULL);
    TEST_ASSERT( *rp == ')' );
    TEST_ASSERT( rp == str+3);

    str = "a(bc))";
    rp = find_balancing(str, '(', ')');
    TEST_ASSERT( rp != NULL);
    TEST_ASSERT( *rp == ')' );
    TEST_ASSERT( rp == str+5);

    str = "a((b)c))";
    rp = find_balancing(str, '(', ')');
    TEST_ASSERT( rp != NULL);
    TEST_ASSERT( *rp == ')' );
    TEST_ASSERT( rp == str+7);
    
    str = "a(bc)";
    rp = find_balancing(str, '(', ')');
    TEST_ASSERT( rp == NULL);
    
    return 0;
}
