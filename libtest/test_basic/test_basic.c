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

/* #include <atom.h> */
#include <octet.h>
#include <element.h>
#include <proto.h>
#include <wrappers.h>
#include <basic.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_ipv4cmp(void);
static unsigned int test_to_gmt(void);
static unsigned int test_hms2int(void);

test_conf_t tc[] = {
    {"test_ipv4cmp", test_ipv4cmp},
    {"test_to_gmt", test_to_gmt},
    {"test_hms2int", test_hms2int},
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

/*
static unsigned int test_element_new(void)
{    
    element_t *ep;

    ep = element_new();

    TEST_ASSERT(ep->next == 0);
    TEST_ASSERT(ep->memberof == 0);
    TEST_ASSERT( ep->e.atom == (atom_t *) 0);
    
    return 0;
}
*/

static unsigned int test_ipv4cmp(void)
{
    struct in_addr ia1, ia2;

    inet_aton("192.168.0.1",&ia1);
    inet_aton("192.168.0.2",&ia2);
    
    TEST_ASSERT_NOT_EQUALS(ipv4cmp(&ia1, &ia2), 0);

    inet_aton("192.168.0.1",&ia2);
    TEST_ASSERT_EQUALS( ipv4cmp(&ia1, &ia2), 0);

    inet_aton("192.168.0.0",&ia2);
    TEST_ASSERT_NOT_EQUALS( ipv4cmp(&ia1, &ia2), 0);

    return 0;
}

static unsigned int test_to_gmt(void)
{
    octet_t *loc, gmt;
    int     r;
    
    loc = oct_new(0, "2009-12-07T21:48:00-06:00");
    r = to_gmt(loc, &gmt);
    printf("\n=>: %d\n", r);
    printf("gmt: %s (%d)\n", gmt.val, (int) gmt.len);
    TEST_ASSERT(strcmp(gmt.val, "2009-12-07T15:48:00") == 0);

    oct_free(loc);
    loc = oct_new(0, "2009-12-07T21:48:00+04:30");
    to_gmt(loc, &gmt);
    /*printf("gmt: %s\n", gmt.val);*/
    TEST_ASSERT(strcmp(gmt.val, "2009-12-08T02:18:00") == 0);
    
    return 0;
}

static unsigned int test_hms2int(void)
{
    octet_t *oct;
    long    l;
    
    oct = oct_new(0, "00:05:23");
    hms2int(oct, &l);
    /* printf("long: %ld\n", l); */
    TEST_ASSERT_EQUALS(l, 323L);

    oct_free(oct);
    oct = oct_new(0, "12:20:44");
    hms2int(oct, &l);
    /*printf("long: %ld\n", l);*/
    TEST_ASSERT_EQUALS(l, 44444L);
    
    return 0;
}