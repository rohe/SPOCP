/*
 *  atom.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <config.h>

#ifdef GLIBC2
#define _XOPEN_SOURCE
#endif

#include <atom.h>
#include <spocp.h>
#include <log.h>
#include <hashfunc.h>
#include <wrappers.h>

atom_t         *
atom_new(octet_t * op)
{
    atom_t         *ap;
    
    ap = (atom_t *) Calloc(1,sizeof(atom_t));
    
    if (op) {
        octcpy(&ap->val, op);
        ap->hash = lhash((unsigned char *) op->val, op->len, 0);
    } 
    
    return ap;
}

/*
 * --------------------------------------------------------------- 
 */

void
atom_free(atom_t * ap)
{
    if (ap) {
        octclr(&ap->val);
        
        Free(ap);
    }
}

/*
 * --------------------------------------------------------------- 
 */

atom_t  *
get_atom(octet_t * op, spocp_result_t * rc)
{
    octet_t         oct;
    
    if ((*rc = get_str(op, &oct)) != SPOCP_SUCCESS) {
        return 0;
    }
    
    return atom_new(&oct);
}

char *
atom2str(atom_t *ap) {
    char str[1024];
    
    snprintf(str, 1024, "%s[%u]", oct2strdup(&ap->val, 0), ap->hash);
    return Strdup(str);
}
