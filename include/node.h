/*
 *  node.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __NODE_H
#define __NODE_H

typedef struct _node {
	void            *payload;
	struct _node    *next;
	struct _node    *prev;
} node_t;

node_t	*node_new(void *vp);

#endif