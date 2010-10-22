/*
 *  check.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/8/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __CHECK_H_
#define __CHECK_H_

#include <result.h>
#include <octet.h>
#include <ruleinst.h>

typedef struct _checked {
	spocp_result_t  rc;
	ruleinst_t      *ri;
	octet_t         *blob;
	struct _checked *next;
} checked_t;

checked_t       *checked_new( ruleinst_t *ri, spocp_result_t rc, 
                             octet_t *blob);
void            checked_free( checked_t *c );
checked_t       *checked_rule( ruleinst_t *ri, checked_t **cr);
spocp_result_t	checked_res( ruleinst_t *ri, checked_t **cr);
void            add_checked( ruleinst_t *ri, spocp_result_t rc,
                            octet_t *blob, checked_t **cr);
void            checked_print( checked_t *c );

#endif