/*
 *  atom.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __ATOM_H
#define __ATOM_H

#include <octet.h>

/*! \brief Where atoms are places */
typedef struct _atom {
	/*! A hash sum calculated over the array of octets representing the atom */
	unsigned int    hash;
	/*! The octet array itself */
	octet_t         val;
} atom_t;

atom_t  *atom_new(octet_t * op);
void    atom_free(atom_t * ap);
atom_t  *get_atom(octet_t * op, spocp_result_t * rc);
char    *atom2str(atom_t *ap);

#endif /* __ATOM_H */