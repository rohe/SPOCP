/*
 *  subelem.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __SUBELEM_H
#define __SUBELEM_H

#include <element.h>
#include <octet.h>

typedef struct _subelem {
	int             direction;
	int             list;	/* yes = 1, no = 0 */
	element_t       *ep;
	struct _subelem *next;
} subelem_t;

subelem_t   *subelem_new(void);
void        subelem_free(subelem_t * sep);
subelem_t   *pattern_parse(octarr_t * pattern);

#endif
