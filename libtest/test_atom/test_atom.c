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

#include <spocp.h>
#include <proto.h>
#include <wrappers.h>
#include <atom.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_atom_new(void);
static unsigned int test_atom2str(void);

test_conf_t tc[] = {
    {"test_atom_new", test_atom_new},
    {"test_atom2str", test_atom2str},
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

static unsigned int test_atom_new(void)
{    
    atom_t  *ap;
    octet_t *op;
    
    op = oct_new(0, "Sickan");
    ap = atom_new(op);
    
    TEST_ASSERT( octcmp(op, (octet_t *) &ap->val) == 0);
    TEST_ASSERT( ap->hash == 883961139);
    
    return 0;
}

static unsigned int test_atom2str(void)
{    
    atom_t  *ap;
    octet_t *op;
    char    *s;
    
    op = oct_new(0, "Sickan");
    ap = atom_new(op);
    s = atom2str(ap);
    TEST_ASSERT( strcmp(s,"Sickan[883961139]") == 0 );
    
    return 0;
}


