/*
 *  test_config.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/12/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include "test_config.h"

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
static unsigned int test_read_config(void);

test_conf_t tc[] = {
    {"test_read_config", test_read_config},
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

static unsigned int test_read_config(void)
{
    srv_t   srv;
    int     rc;
	char    localhost[MAXNAMLEN + 1];
    
    memset(&srv,0, sizeof(srv_t));
    
    srv.root = ruleset_new(0);
    gethostname(localhost, MAXNAMLEN);
	srv.hostname = Strdup(localhost);

    rc = read_config("conf.0", &srv);
    TEST_ASSERT_EQUALS(rc, 1);
    TEST_ASSERT(1);
    
    return 0;
}
