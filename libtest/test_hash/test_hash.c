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

#include <hash.h>

#include <log.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_phash_new(void);
static unsigned int test_buck_new(void);
static unsigned int test_phash_insert(void);
static unsigned int test_phash_search(void);
static unsigned int test_bucket_rm(void);
static unsigned int test_phash_empty(void);

test_conf_t tc[] = {
    {"test_phash_new", test_phash_new},
    {"test_buck_new", test_buck_new},
    {"test_phash_insert", test_phash_insert},
    {"test_phash_search", test_phash_search},
    {"test_bucket_rm", test_bucket_rm},
    {"test_phash_empty", test_phash_empty},
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

static unsigned int test_phash_new(void)
{    
    phash_t *php;

    php = phash_new(4, 70);

    TEST_ASSERT_EQUALS( php->n, 0);
    TEST_ASSERT_EQUALS( php->pnr, 4);
    /* The 4th prime, a prime close to 2^(n+1) */
    TEST_ASSERT_EQUALS( php->size, 121);
    TEST_ASSERT_EQUALS( php->density, 70);
    TEST_ASSERT_EQUALS( php->limit, 84);

    return 0;
}

static unsigned int test_buck_new(void)
{    
    buck_t *bp;
    octet_t *op;
    
    op = oct_new(0, "Sickan");    
    bp = buck_new(12046, op);
    TEST_ASSERT_EQUALS(bp->hash, 12046);
    TEST_ASSERT( octcmp(op, &bp->val) == 0 );
    
    return 0;
}

static unsigned int test_phash_insert(void)
{    
    buck_t  *bp, *bp2;
    octet_t *op;
    atom_t  *ap;
    phash_t *php, *sphp;

    php = phash_new(4, 70);

    op = oct_new(0, "Sickan");
    ap = atom_new(op);
    bp = phash_insert(php, ap);

    TEST_ASSERT( octcmp(op, &bp->val) == 0 );
    TEST_ASSERT_EQUALS(php->n, 1);

    op = oct_new(0, "Carlsson");
    ap = atom_new(op);
    bp2 = phash_insert(php, ap);
    
    TEST_ASSERT( octcmp(op, &bp2->val) == 0 );
    TEST_ASSERT_EQUALS(php->n, 2);
    TEST_ASSERT(bp != bp2);

    /* more of the same */
    op = oct_new(0, "Carlsson");
    ap = atom_new(op);
    bp = phash_insert(php, ap);
    
    TEST_ASSERT( octcmp(op, &bp->val) == 0 );
    TEST_ASSERT_EQUALS(php->n, 2);
    TEST_ASSERT(bp == bp2);

    /* ================================= */
    
    sphp = phash_new(0, 50);
    TEST_ASSERT_EQUALS(sphp->size, 3);
    
    op = oct_new(0, "Sickan");
    ap = atom_new(op);
    bp = phash_insert(sphp, ap);
    
    TEST_ASSERT( octcmp(op, &bp->val) == 0 );
    TEST_ASSERT_EQUALS(sphp->n, 1);
    TEST_ASSERT_EQUALS(sphp->size, 3);

    /* the hashtable should be resized */
    op = oct_new(0, "Carlsson");
    ap = atom_new(op);
    bp = phash_insert(sphp, ap);
    
    TEST_ASSERT( octcmp(op, &bp->val) == 0 );
    TEST_ASSERT_EQUALS(sphp->n, 2);
    TEST_ASSERT_EQUALS(sphp->size, 7);

    return 0;
}
    
char *name[] = {
    "Derek",
    "Rodriguez",
    "Alonso",
    NULL
};

static unsigned int test_phash_search(void)
{    
    buck_t  *res;
    octet_t oct;
    atom_t  *ap[8], *nap;
    phash_t *php;
    int     n;
    
    php = phash_new(0, 50);

    for( n = 0; name[n] ; n++) { 
        oct_assign(&oct, name[n]);
        ap[n] = atom_new(&oct);
        phash_insert(php, ap[n]);
    }

    for( n = 0; name[n] ; n++) { 
        res = phash_search(php, ap[n]);
        TEST_ASSERT( oct2strcmp(&res->val, name[n]) == 0);
    }

    oct_assign(&oct, "Speed");
    nap = atom_new(&oct);
    res = phash_search(php, nap);
    TEST_ASSERT(res == NULL);
    
    return 0;
}

static unsigned int test_bucket_rm(void)
{    
    buck_t  *res, *bp;
    octet_t oct;
    atom_t  *ap[8], *nap;
    phash_t *php;
    int     n;
    
    php = phash_new(1, 50);
    
    for( n = 0; name[n] ; n++) { 
        oct_assign(&oct, name[n]);
        ap[n] = atom_new(&oct);
        phash_insert(php, ap[n]);
    }

    for( n = 0; name[n] ; n++) { 
        res = phash_search(php, ap[n]);
        TEST_ASSERT( oct2strcmp(&res->val, name[n]) == 0);
    }

    oct_assign(&oct, name[1]);
    nap = atom_new(&oct);
    bp = phash_search(php, nap);

    bucket_rm(php, bp);

    /* it was there, now it isn't */
    res = phash_search(php, nap);
    TEST_ASSERT(res == NULL);

    return 0;
}

static unsigned int test_phash_empty(void)
{    
    buck_t  *bp;
    octet_t oct;
    atom_t  *ap[8], *nap;
    phash_t *php;
    int     n;
    
    php = phash_new(1, 50);
    
    TEST_ASSERT(phash_empty(php));
    
    for( n = 0; name[n] ; n++) { 
        oct_assign(&oct, name[n]);
        ap[n] = atom_new(&oct);
        phash_insert(php, ap[n]);
    }

    TEST_ASSERT_FALSE(phash_empty(php));

    /* now remove them one by one */
    for( n = 0; name[n] ; n++) { 
        oct_assign(&oct, name[n]);
        nap = atom_new(&oct);
        bp = phash_search(php, nap);
        bucket_rm(php, bp);
    }
    
    TEST_ASSERT(phash_empty(php));
    
    return 0;
}
