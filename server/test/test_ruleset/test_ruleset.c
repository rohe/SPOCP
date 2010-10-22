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
static unsigned int test_ruleset_new(void);
static unsigned int test_path_split(void);
static unsigned int test_ruleset_create(void);
static unsigned int test_ruleset_add(void);
static unsigned int test_ruleset_pathname(void);

test_conf_t tc[] = {
    {"test_ruleset_new", test_ruleset_new},
    {"test_path_split", test_path_split},
    {"test_ruleset_create", test_ruleset_create},
    {"test_ruleset_add", test_ruleset_add},
    {"test_ruleset_pathname", test_ruleset_pathname},
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

static unsigned int test_ruleset_new(void)
{
    ruleset_t   *rs;
    octet_t     namn;
    
    oct_assign(&namn, "foo");
    rs = ruleset_new(&namn);
    TEST_ASSERT(rs != NULL);
    TEST_ASSERT(strcmp(rs->name,"foo") == 0);
    
    return 0;
}

static unsigned int test_path_split(void)
{
    octarr_t    *oa;
    octet_t     path;
    int         len;
    
    oct_assign(&path, "/foo/bar");
    oa = path_split(&path, &len);
    
    TEST_ASSERT(oa != NULL);
    TEST_ASSERT_EQUALS(len, 8);
    TEST_ASSERT_EQUALS(oa->n, 2);
    TEST_ASSERT(oct2strcmp(oa->arr[0], "foo") == 0);
    TEST_ASSERT(oct2strcmp(oa->arr[1], "bar") == 0);
    
    octarr_free(oa);

    oct_assign(&path, "//foo/bar(3:foo)");
    oa = path_split(&path, &len);
    TEST_ASSERT(oa != NULL);
    TEST_ASSERT_EQUALS(len, 9);
    TEST_ASSERT_EQUALS(oa->n, 2);
    TEST_ASSERT(oct2strcmp(oa->arr[0], "foo") == 0);
    TEST_ASSERT(oct2strcmp(oa->arr[1], "bar") == 0);
    
    return 0;
}

static unsigned int test_ruleset_create(void)
{
    ruleset_t   *rs, *root=NULL;
    octet_t     path;
    
    oct_assign(&path, "/foo/bar");
    rs = ruleset_create(&path, &root);
    TEST_ASSERT(rs != NULL);
    TEST_ASSERT(strcmp(rs->name, "bar") == 0);
    TEST_ASSERT(strcmp(root->name, "") == 0);
    TEST_ASSERT(strcmp(root->down->name, "foo") == 0);
    TEST_ASSERT(strcmp(root->down->down->name, "bar") == 0);
    TEST_ASSERT(root->down->up == root );
    TEST_ASSERT(rs->up->up == root);
    
    oct_assign(&path, "/fox/base");
    rs = ruleset_create(&path, &root);
    TEST_ASSERT(rs != NULL);
    TEST_ASSERT(strcmp(rs->name, "base") == 0);
    TEST_ASSERT(strcmp(root->down->right->name, "fox") == 0);
    TEST_ASSERT(strcmp(root->down->right->down->name, "base") == 0);
    TEST_ASSERT(rs->up->up == root);

    remove_ruleset_tree(root);
    
    return 0;
}

static unsigned int test_ruleset_add(void)
{
    ruleset_t   *rs, *root=NULL;
    octet_t     path;
    
    rs = ruleset_add(&root, "/foo/bar", NULL);
    TEST_ASSERT(rs != NULL);
    TEST_ASSERT(strcmp(rs->name, "bar") == 0);
    TEST_ASSERT(strcmp(root->name, "") == 0);
    TEST_ASSERT(strcmp(root->down->name, "foo") == 0);
    TEST_ASSERT(strcmp(root->down->down->name, "bar") == 0);
    TEST_ASSERT(root->down->up == root );
    TEST_ASSERT(rs->up->up == root);
    
    oct_assign(&path, "/fox/base");
    rs = ruleset_add(&root, NULL, &path);
    TEST_ASSERT(rs != NULL);
    TEST_ASSERT(strcmp(rs->name, "base") == 0);
    TEST_ASSERT(strcmp(root->down->right->name, "fox") == 0);
    TEST_ASSERT(strcmp(root->down->right->down->name, "base") == 0);
    TEST_ASSERT(rs->up->up == root);
    
    oct_assign(&path, "//mlb/nyy");
    rs = ruleset_add(&root, NULL, &path);
    TEST_ASSERT(rs != NULL);
    TEST_ASSERT(strcmp(root->left->name, "//") == 0);
    TEST_ASSERT(strcmp(root->left->down->name, "mlb") == 0);
    
    return 0;
}

static unsigned int test_ruleset_pathname(void)
{
    ruleset_t       *rs, *root=NULL;
    octet_t         path;
    char            *namn;
    spocp_result_t  rc;
    
    namn = (char *)Calloc(128, sizeof(char));
    
    rs = ruleset_add(&root, "/foo/bar", NULL);
    rc = ruleset_pathname(rs, namn, 128);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT( strcmp(namn, "/foo/bar") == 0 );
    
    oct_assign(&path, "/fox/base");
    rs = ruleset_add(&root, NULL, &path);
    rc = ruleset_pathname(rs, namn, 128);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT( strcmp(namn, "/fox/base") == 0 );

    rs = ruleset_add(&root, "//mlb/nyy", NULL);
    rc = ruleset_pathname(rs, namn, 128);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT( strcmp(namn, "//mlb/nyy") == 0 );

    
    return 0;
}
