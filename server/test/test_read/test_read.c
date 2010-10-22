/*
 *  test_read.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/12/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <octet.h>
#include <bcond.h>
#include <proto.h>
#include <wrappers.h>
#include <log.h>
#include <srv.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_read_rules_0(void);
static unsigned int test_read_rules_1(void);

test_conf_t tc[] = {
    {"test_read_rules_0", test_read_rules_0},
    {"test_read_rules_1", test_read_rules_1},
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

static unsigned int test_read_rules_0(void)
{
    srv_t       srv;
    int         rc;
    char        localhost[MAXNAMLEN];
    octet_t     namn;
    ruleinfo_t  *ri;
    ruleset_t   *rs;
    
    memset(&srv,0, sizeof(srv_t));
    
    gethostname(localhost, MAXNAMLEN);
	srv.hostname = Strdup(localhost);

    rc = read_rules( &srv, "rules.0", NULL);
    TEST_ASSERT_EQUALS(rc, 0);
    TEST_ASSERT(srv.root != NULL);
    
    ri = srv.root->db->ri ;
    TEST_ASSERT_EQUALS(rules_len(ri), 4);
    
    oct_assign(&namn, "//hypatia/operation/");
    rs = ruleset_find(&namn, srv.root);
    TEST_ASSERT( rs != NULL);
    ri = rs->db->ri ;
    TEST_ASSERT_EQUALS(rules_len(ri), 3);

    oct_assign(&namn, "//hypatia/server/");
    rs = ruleset_find(&namn, srv.root);
    TEST_ASSERT( rs != NULL);
    ri = rs->db->ri ;
    TEST_ASSERT_EQUALS(rules_len(ri), 1);
    
    return 0;
}

static unsigned int test_read_rules_1(void)
{
    srv_t       srv;
    int         rc;
    char        localhost[MAXNAMLEN];
    octet_t     namn;
    ruleinfo_t  *ri;
    ruleset_t   *rs;
    
    memset(&srv,0, sizeof(srv_t));
    
    gethostname(localhost, MAXNAMLEN);
	srv.hostname = Strdup(localhost);
    
    rc = read_rules( &srv, "rules.1", NULL);
    TEST_ASSERT_EQUALS(rc, 0);
    TEST_ASSERT(srv.root != NULL);
    
    ri = srv.root->db->ri ;
    TEST_ASSERT_EQUALS(rules_len(ri), 1);
    
    oct_assign(&namn, "//*/operation/");
    rs = ruleset_find(&namn, srv.root);
    TEST_ASSERT( rs != NULL);
    ri = rs->db->ri ;
    TEST_ASSERT_EQUALS(rules_len(ri), 2);
    
    oct_assign(&namn, "//*/server/");
    rs = ruleset_find(&namn, srv.root);
    TEST_ASSERT( rs != NULL);
    ri = rs->db->ri ;
    TEST_ASSERT_EQUALS(rules_len(ri), 1);
    
    return 0;
}
