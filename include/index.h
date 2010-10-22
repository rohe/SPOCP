/*
 *  index.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/8/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __INDEX_H
#define __INDEX_H

#include <ruleinst.h>
#include <ruleinfo.h>

typedef struct _index {
	int         size;
	int         n;
	ruleinst_t  **arr;
} spocp_index_t;

int             index_rm(spocp_index_t * id, ruleinst_t * ri);
void            index_free(spocp_index_t * id);
void            index_delete(spocp_index_t * id);
void            index_extend(spocp_index_t *, spocp_index_t *);
void            index_print(spocp_index_t *);
spocp_index_t   *index_new(int size);
spocp_index_t   *index_dup(spocp_index_t * id, ruleinfo_t * ri);
spocp_index_t   *index_cp(spocp_index_t *);
spocp_index_t   *index_add(spocp_index_t * id, ruleinst_t * ri);
spocp_index_t   *index_and(spocp_index_t *, spocp_index_t *);

#endif