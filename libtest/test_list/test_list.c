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
#include <list.h>

void *bsdrb_init();

/*
 * test function 
 */
static unsigned int test_get_matching_rules(void);

test_conf_t tc[] = {
    {"test_get_matching_rules", test_get_matching_rules},
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

/* ======================================================================= */


char *PITCHER[] = {
    "(7:Alfredo6:Aceves)",
    "(8:Jonathan10:Albaladejo)",
    "(6:Andrew8:Brackman)",
    "(4:A.J.7:Burnett)",
    "(4:Joba11:Chamberlain)",
    "(6:Wilkin10:De La Rosa)",
    "(9:Christian6:Garcia)",
    "(4:Chad6:Gaudin)",
    "(4:Phil6:Hughes)",
    "(5:Boone5:Logan)",
    "(6:Damaso5:Marte)",
    "(4:Mark8:Melancon)",
    "(6:Sergio5:Mitre)",
    "(6:Hector5:Noesi)",
    "(4:Ivan4:Nova)",
    "(4:Andy8:Pettitte)",
    "(5:Edwar7:Ramirez)",
    "(7:Mariano6:Rivera)",
    "(5:David9:Robertson)",
    "(2:CC8:Sabathia)",
    "(6:Romulo7:Sanchez)",
    "(6:Javier7:Vazquez)",
    NULL
};

char *INFIELD[] = {
	"(8:Robinson4:Cano)",
 	"(6:Reegie6:Corona)",
 	"(5:Derek5:Jeter)",
    "(4:Nick7:Johnson)",
 	"(4:Juan7:Miranda)",
    "(7:Eduardo5:Nunez)",
 	"(6:Ramiro4:Pena)",
    "(4:Alex9:Rodriguez)",
 	"(5:Kevin5:Russo)",
    "(4:Mark8:Teixeira)",
    NULL
};

db_t * _add_to_db(db_t *dbp, char **rule)
{
    octet_t         regel;
    octarr_t        *oa = NULL;
    spocp_result_t  sr;
    ruleinst_t      *ri;
    
    if (dbp == NULL)
        dbp = db_new();
    
    for (; *rule; rule++) {
        oa = octarr_new(2);
        oct_assign(&regel, *rule);
        octarr_add(oa, &regel);
        
        sr = add_right(&dbp, NULL, oa, &ri, NULL);
        Free(oa);        
    }
    
    return dbp;
}

/* ======================================================================= */

static unsigned int test_get_matching_rules(void)
{    
    spocp_result_t  rc;
    db_t            *dbp=NULL;
    char            *pattern = "7:+4:Mark";
    octet_t         oct;
    octarr_t        *pat=NULL, *oarr=NULL, *oa=NULL;
    
    dbp = _add_to_db(dbp, INFIELD);

    oct_assign(&oct, pattern);
    octseq2octarr(&oct, &pat);
    
    rc = get_matching_rules(dbp, pat, &oarr, "/");
    TEST_ASSERT(rc == SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(oarr->n, 1);
    octseq2octarr(oarr->arr[0], &oa);
    TEST_ASSERT(oct2strcmp(oa->arr[2],"(4:Mark8:Teixeira)") == 0 );
    
    octarr_free(oarr);
    octarr_free(oa);
    octarr_free(pat);
    pat = NULL;
    oa = NULL ;
    
    dbp = _add_to_db(dbp, PITCHER);

    oct_assign(&oct, pattern);
    octseq2octarr(&oct, &pat);
    
    rc = get_matching_rules(dbp, pat, &oarr, "/");
    TEST_ASSERT(rc == SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(oarr->n, 2);
    
    return 0;
}

