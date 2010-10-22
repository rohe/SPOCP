/*
 *  parr.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __PARR_H
#define __PARR_H

#include <basefn.h>

/* An array of pointers to things of the same kind */
typedef struct _parr {
	int		n;
	int		size;
	int		*refc;
	void    **vect;
	cmpfn	*cf;	/* compare function */
	ffunc	*ff;	/* free function */
	dfunc	*df;	/* duplicate function */
	pfunc	*pf;	/* print function */
} parr_t;

/* ----------------------------------------------------------------------- */

parr_t      *parr_new(int size, cmpfn * cf, ffunc * ff, dfunc * df,
                      pfunc * pf);
parr_t      *parr_add(parr_t * ap, item_t vp);
parr_t      *parr_add_nondup(parr_t * ap, item_t vp);
parr_t      *parr_dup(parr_t * ap);
parr_t      *parr_join(parr_t * to, parr_t * from, int dup);
item_t      parr_common(parr_t * avp1, parr_t * avp2);
parr_t      *parr_or(parr_t * a1, parr_t * a2);
parr_t      *parr_xor(parr_t * a1, parr_t * a2, int dup);
parr_t      *parr_and(parr_t * a1, parr_t * a2);
void		parr_free(parr_t * ap);
item_t      parr_nth(parr_t * gp, int i);
int         parr_get_index(parr_t * gp, item_t vp);
int         parr_items(parr_t * gp);
item_t      parr_find(parr_t * ap, item_t val);
int         parr_find_index(parr_t * ap, item_t val);
void		parr_rm_index(parr_t ** gpp, int i, int enforce);
void		parr_rm_item(parr_t ** gpp, item_t vp);
void		parr_dec_refcount(parr_t ** gpp, int delta);

void		P_null_free(item_t vp);

int         P_strcasecmp(item_t a, item_t b);
int         P_strcmp(item_t a, item_t b, spocp_result_t *rc);
int         P_pcmp(item_t a, item_t b);
item_t      P_strdup(item_t vp);
char        *P_strdup_char(item_t vp);
item_t      P_pcpy(item_t i, item_t j);
void		P_free(item_t vp);

#endif