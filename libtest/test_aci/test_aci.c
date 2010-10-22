/*
 *  test_cachetime.c
 *
 *  Created by Roland Hedberg on 12/3/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

/* test program for lib/cachetime.c */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <spocp.h>
#include <proto.h>
#include <element.h>
#include <db0.h>
#include <hash.h>
#include <match.h>

#include <wrappers.h>
#include <testRunner.h>

void *bsdrb_init();

/*
 * test function 
 */
static unsigned int test_allowed(void);

test_conf_t tc[] = {
    {"test_allowed", test_allowed},
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

/* ------------------------------------------------------------------------ */
typedef struct _test_info {
    char    *rule;
    char    *blob;
} test_info_t;

test_info_t INFO[] = {
    { "(4:rule3:one)", "BLOB" },
    { "(4:rule3:two)", NULL },
    { "(4:rule5:three)", NULL },
    { NULL, NULL }
};

test_info_t PREFIX[] = {
    { "(4:rule(1:*6:prefix3:ref)3:foo)", NULL },
    { "(4:rule(1:*6:prefix4:berg))", NULL },
    { NULL, NULL }
};

test_info_t SUFFIX[] = {
    { "(4:rule(1:*6:suffix4:chef)3:xxx)", NULL },
    { "(4:rule(1:*6:suffix4:berg))", NULL },
    { NULL, NULL }
};

db_t * _add_to_db(db_t *dbp, test_info_t *ti)
{
    int             i;
    octet_t         rule, blob;
    octarr_t        *oa = NULL;
    spocp_result_t  sr;
    ruleinst_t      *ri;

    if (dbp == NULL)
        dbp = db_new();

    for (i = 0; ti[i].rule; i++) {
        oa = octarr_new(2);
        oct_assign(&rule, ti[i].rule);
        octarr_add(oa, &rule);
        if (ti[i].blob) {
            oct_assign(&blob, ti[i].blob);
            octarr_add(oa, &blob);
        }
        
        sr = add_right(&dbp, NULL, oa, &ri, NULL);
        Free(oa);        
    }
    
    return dbp;
}

/* ------------------------------------------------------------------------ */


static unsigned int test_allowed(void)
{    
    db_t            *dbp;
    resset_t        *res;
    comparam_t      comp;
    element_t       *ep=NULL;
    octet_t         oct;
	checked_t       *cr=0;
    octarr_t        *on= 0;
    spocp_result_t  sr;


    dbp = _add_to_db(NULL, INFO);
    dbp = _add_to_db(dbp, SUFFIX);
    dbp = _add_to_db(dbp, PREFIX);
    
    memset(&comp, 0, sizeof(comparam_t));
    oct_assign(&oct, INFO[1].rule);
    get_element(&oct, &ep);

    comp.rc = SPOCP_SUCCESS;
	comp.head = ep;
    comp.blob = &on;
    comp.cr = &cr;
    
    sr = allowed(dbp->jp, &comp, &res);
    TEST_ASSERT_EQUALS(sr, SPOCP_SUCCESS);
    TEST_ASSERT( res != NULL);
    
    oct_assign(&oct, "(4:rule4:four)");
    get_element(&oct, &ep);
    cr = 0;
    on = 0;
    comp.rc = SPOCP_SUCCESS;
	comp.head = ep;

    sr = allowed(dbp->jp, &comp, &res);
    TEST_ASSERT_EQUALS(sr, SPOCP_DENIED);

    return 0;
}
