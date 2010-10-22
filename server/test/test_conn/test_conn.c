/*
 *  test_ruleset.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/12/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <config.h>

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
static unsigned int test_conn_new(void);
static unsigned int test_conn_setup(void);

test_conf_t tc[] = {
    {"test_conn_new", test_conn_new},
    {"test_conn_setup", test_conn_setup},
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

static unsigned int test_conn_new(void)
{
    conn_t   *con;
    
    con = conn_new();
    TEST_ASSERT(con != NULL);
    
    return 0;
}

static unsigned int test_conn_setup(void)
{
    conn_t  *con;
    srv_t   srv;
    
    memset(&srv, 0, sizeof(srv_t));
    con = conn_new();
    conn_setup(con, &srv, STDIN_FILENO, "localhost", "127.0.0.1");
    TEST_ASSERT(con != NULL);
    TEST_ASSERT(con->srv == &srv);
    TEST_ASSERT_EQUALS(con->fd, STDIN_FILENO);
    TEST_ASSERT_EQUALS(con->status, CNST_SETUP);
    TEST_ASSERT_EQUALS(con->sslstatus, INACTIVE);
    TEST_ASSERT_EQUALS(con->con_type, NATIVE);
    TEST_ASSERT( strcmp(con->sri.hostaddr, "127.0.0.1") == 0 );
    TEST_ASSERT( strcmp(con->sri.hostname, "localhost") == 0 );
    TEST_ASSERT_EQUALS(con->layer, SPOCP_LAYER_NONE);
    TEST_ASSERT(con->last_event != 0 );
    
    return 0;
}


