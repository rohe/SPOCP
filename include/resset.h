/*
 *  resset.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/1/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __RESSET_H
#define __RESSET_H

#include <octet.h>
#include <index.h>

typedef struct _res_set {
	octarr_t        *blob;  /* Place for dynamic blobs */
	ruleinst_t      *ri;    /* Rule information, includes static blob */
	struct _res_set	*next;
} resset_t;

typedef struct _qresult {
	spocp_result_t	rc;
	resset_t        *part;
	struct _qresult	*next;
} qresult_t ;

/* ------------------------------------------------------------ */

resset_t    *resset_new(ruleinst_t *ri, octarr_t *blob );
void        resset_free(resset_t *rs);
resset_t    *resset_add(resset_t *rs, ruleinst_t *ri, octarr_t *blob);
resset_t    *resset_extend(resset_t *a, resset_t *b);
void        resset_print(resset_t *rs);
int         resset_len(resset_t *rs);

qresult_t   *qresult_new(spocp_result_t rc, resset_t *rs );
qresult_t   *qresult_add(qresult_t *res, spocp_result_t rc, resset_t *rs);
qresult_t   *qresult_join(qresult_t *a, qresult_t *b);
void        qresult_free(qresult_t *res);

#endif