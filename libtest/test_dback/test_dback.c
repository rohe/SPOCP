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

#include <octet.h>
#include <proto.h>
#include <dback.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_datum_make(void);
static unsigned int test_datum_parse(void);
static unsigned int test_dback_init(void);
static unsigned int test_dback_save(void);
static unsigned int test_dback_read(void);
static unsigned int test_dback_replace(void);
static unsigned int test_dback_all_keys(void);

test_conf_t tc[] = {
    {"test_datum_make", test_datum_make},
    {"test_datum_parse", test_datum_parse},
    {"test_dback_init", test_dback_init},
    {"test_dback_save", test_dback_save},
    {"test_dback_read", test_dback_read},
    {"test_dback_replace", test_dback_replace},
    {"test_dback_all_keys", test_dback_all_keys},
    {"",NULL}
};

#include <plugin.h>
#include <octet.h>

/* -------------------------------------------------------------------- */

typedef struct _ava {
    char *key;
    char *value;
    struct _ava *next;
} ava_t ;

ava_t *new_ava( char *key, octet_t *value )
{
    ava_t   *ap;
    
    ap = (ava_t *) Calloc(1, sizeof(ava_t));
    ap->key = strdup(key);
    ap->value = oct2strdup(value, '0');
    
    return ap;
}

void free_ava( ava_t *ap )
{
    ava_t *nap;
    
    if (ap) {
        for (; ap; ap = nap) {
            nap = ap->next;
            Free(ap->key);
            Free(ap->value);
            Free(ap);
        }
    }
}

void *ava_put(dbackdef_t * dbc, void *vkey, void *vdat, spocp_result_t * rc)
{
    ava_t *ava = dbc->dback->conf;
    
    *rc = SPOCP_SUCCESS;
    
    if (ava == NULL) {
        dbc->dback->conf = new_ava((char *) vkey, (octet_t *) vdat);
    }
    else {
        for( ; ava->next ; ava = ava->next);
        ava->next = new_ava((char *) vkey, (octet_t *) vdat);
    }
    return NULL;
}

void *ava_get(dbackdef_t * dbc, void *vkey, void *vdat, spocp_result_t * rc)
{
    ava_t           *ava = dbc->dback->conf;
    
    *rc = SPOCP_SUCCESS;
    
    if (ava != NULL) {
        for( ; ava ; ava = ava->next) {
            if (strcmp(ava->key, (char *)vkey) == 0) {
                return str2oct(ava->value, 0);
            }
        }
    }
    return NULL;
}

void *ava_replace(dbackdef_t * dbc, void *vkey, void *vdat, spocp_result_t * rc)
{
    ava_t *ava = dbc->dback->conf;
    
    *rc = SPOCP_SUCCESS;
    
    if (ava != NULL) {
        for( ; ava ; ava = ava->next) {
            if (strcmp(ava->key, (char *)vkey) == 0) {
                Free(ava->value);
                ava->value = oct2strdup((octet_t *) vdat, '0');;
                break;
            }
        }
    }
    return NULL;
}

void *ava_delete(dbackdef_t * dbc, void *vkey, void *vdat, spocp_result_t * rc)
{
    ava_t   *ava = dbc->dback->conf, *prev;
    void    *res = NULL;
    
    *rc = SPOCP_SUCCESS;
    
    if (ava != NULL) {
        if (strcmp(ava->key, (char *)vkey) == 0) {
            dbc->dback->conf = ava->next;
            /* free_ava(ava); */
        }
        else {
            prev = ava;
            for(ava = ava->next ; ava ; ava = ava->next) {
                if (strcmp(ava->key, (char *)vkey) == 0) {
                    prev->next = ava->next;
                    res = ava->value;
                    free_ava(ava);
                    break;
                }
            }
        }
    }
    return res;
}

void *ava_allkeys(dbackdef_t * dbc, void *vkey, void *vdat, spocp_result_t * rc)
{
    ava_t       *ava = dbc->dback->conf;
    octarr_t    *arr = NULL;
    int         n;
    
    *rc = SPOCP_SUCCESS;
    
    if (ava != NULL) {        
        for( ; ava ; ava = ava->next, n++) {            
            arr = octarr_add(arr, str2oct(ava->key, 0));
        }
    }
    return arr;
}

void *ava_init(dbackdef_t * dbc, void *vkey, void *vdat, spocp_result_t * rc)
{
    ava_t   *ava = dbc->dback->conf;
    
    if (ava != NULL) {
        free_ava(ava);
        dbc->dback->conf = NULL;
    }
    
    return NULL;
}

struct dback_struct ava_dback = {
	SPOCP20_DBACK_STUFF,
	ava_get,
	ava_put,
	ava_replace,
	ava_delete,
	ava_allkeys,
    
	NULL,           /* No support for transactions */
	NULL,
	NULL,
	NULL,
    
	ava_init,			/* No init function */
    
	NULL,
};

/* -------------------------------------------------------------------- */

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

static unsigned int test_datum_make(void)
{
    
    octet_t     *datum, *rule, *blob;
    char        *bcname;
    
    rule = oct_new(0, "(3:one4:step)");
    blob = oct_new(0, "some text");
    bcname = "condition";
    
    datum = datum_make(rule, blob, bcname);
    TEST_ASSERT(oct2strcmp(datum, 
                           "13:(3:one4:step):9:some text:9:condition") == 0);
    
    return 0;
}

static unsigned int test_datum_parse(void)
{
    
    octet_t         *datum, *rule, *blob, res[2];
    char            *bcname, *namn;
    spocp_result_t  rc;
    
    rule = oct_new(0, "(3:one4:step)");
    blob = oct_new(0, "some text");
    bcname = "condition";
    
    datum = datum_make(rule, blob, bcname);
    rc = datum_parse(datum, &res[0], &res[1], &namn);
    
    TEST_ASSERT_EQUALS( rc, SPOCP_SUCCESS);
    TEST_ASSERT( octcmp(rule, &res[0]) == 0 );
    TEST_ASSERT( octcmp(blob, &res[1]) == 0 );
    TEST_ASSERT( strcmp(bcname, namn) == 0 );
    
    return 0;
}

static unsigned int test_dback_init(void)
{
    dbackdef_t         dbc;
    spocp_result_t  rc;
    
    dbc.dback = &ava_dback;
    rc = dback_init(&dbc);
    TEST_ASSERT( rc == SPOCP_UNAVAILABLE );
    
    return 0;
}

static unsigned int test_dback_save(void)
{
    dbackdef_t         dbc;
    spocp_result_t  rc;
    char            *key = "KEY";
    octet_t         *rule, *blob;
    char            *bcname;

    dbc.dback = &ava_dback;
    rc = dback_init(&dbc);

    rule = oct_new(0, "(3:one4:step)");
    blob = oct_new(0, "some text");
    bcname = "condition";
    
    rc = dback_save(&dbc, key, rule, blob, bcname);
    TEST_ASSERT( rc == SPOCP_SUCCESS );
    
    return 0;
}

static unsigned int test_dback_read(void)
{
    dbackdef_t         dbc;
    spocp_result_t  rc;
    char            *key = "KEY";
    octet_t         *rule, *blob, res[2];
    char            *bcname, *namn;
    
    dbc.dback = &ava_dback;
    rc = dback_init(&dbc);

    rule = oct_new(0, "(3:one4:step)");
    blob = oct_new(0, "some text");
    bcname = "condition";
    
    rc = dback_save(&dbc, key, rule, blob, bcname);
    rc = dback_read(&dbc, key, &res[0], &res[1], &namn);

    TEST_ASSERT_EQUALS( rc, SPOCP_SUCCESS);
    TEST_ASSERT( octcmp(rule, &res[0]) == 0 );
    TEST_ASSERT( octcmp(blob, &res[1]) == 0 );
    TEST_ASSERT( strcmp(bcname, namn) == 0 );
    
    return 0;
}

static unsigned int test_dback_replace(void)
{
    dbackdef_t         dbc;
    spocp_result_t  rc;
    char            *key = "KEY";
    octet_t         *rule, *blob, *op[2], res[2];
    char            *bcname, *namn;
    
    dbc.dback = &ava_dback;
    rc = dback_init(&dbc);

    rule = oct_new(0, "(3:one4:step)");
    blob = oct_new(0, "some text");
    bcname = "condition";
    
    rc = dback_save(&dbc, key, rule, blob, bcname);

    op[0] = oct_new(0, "(3:two6:things)");
    op[1] = oct_new(0, "extra text");

    rc = dback_replace(&dbc, key, op[0], op[1], NULL);    
    rc = dback_read(&dbc, key, &res[0], &res[1], &namn);
    
    TEST_ASSERT_EQUALS( rc, SPOCP_SUCCESS);
    TEST_ASSERT(octcmp(op[0], &res[0]) == 0);
    TEST_ASSERT(octcmp(op[1], &res[1]) == 0);
    TEST_ASSERT(namn == 0);
    
    return 0;
}

static unsigned int test_dback_all_keys(void)
{
    dbackdef_t      dbc;
    spocp_result_t  rc;
    char            *key = "KEY";
    octet_t         *rule, *blob, *op[2], *pop;
    char            *bcname;
    octarr_t        *oap;
    
    dbc.dback = &ava_dback;
    rc = dback_init(&dbc);
    
    rule = oct_new(0, "(3:one4:step)");
    blob = oct_new(0, "some text");
    bcname = "condition";
    
    rc = dback_save(&dbc, key, rule, blob, bcname); 
    
    op[0] = oct_new(0, "(3:two6:things)");
    op[1] = oct_new(0, "extra text");
    
    rc = dback_save(&dbc, "2KEY2", op[0], op[1], NULL);    
    oap = dback_all_keys(&dbc, &rc);
    
    TEST_ASSERT_EQUALS(octarr_len(oap), 2);
    
    pop = octarr_pop(oap);
    TEST_ASSERT( oct2strcmp(pop, key) == 0 );
    pop = octarr_pop(oap);
    TEST_ASSERT( oct2strcmp(pop, "2KEY2") == 0 );

    return 0;
}
