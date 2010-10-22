/*
 *  dset.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/5/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __DSET_H
#define __DSET_H

#include <varr.h>

typedef struct _dset {
	char            *uid;
	varr_t          *va;
	struct _dset	*next;
} dset_t;

void    dset_free(dset_t * ds);
dset_t  *dset_new(char *uid, varr_t *val);
dset_t  *dset_append(dset_t *head, dset_t *ds);

#endif