
/***************************************************************************
                          free.c  -  description
                             -------------------

    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <stdlib.h>

#include <struct.h>
#include <func.h>
#include <wrappers.h>

void	octet_free(octet_t * op);
void	boundary_free(boundary_t * bp);
void	branch_free(branch_t * bp);

/*
 * ======================================================== 
 */

void
octet_free(octet_t * op)
{
	if (op) {
		if (op->val)
			Free(op->val);
		Free(op);
	}
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

void
range_free(range_t * rp)
{
	if (rp) {
		switch (rp->lower.type) {
		case SPOC_DATE:
		case SPOC_ALPHA:
			if (rp->lower.v.val.len)
				Free(rp->lower.v.val.val);
			if (rp->upper.v.val.len)
				Free(rp->upper.v.val.val);
			break;

		case SPOC_TIME:
		case SPOC_NUMERIC:
		case SPOC_IPV4:
		case SPOC_IPV6:
			break;
		}

		Free(rp);
	}
}

/*
 * --------------------------------------------------------------- 
 */

void
list_free(list_t * lp)
{
	element_t      *ep, *nep;

	if (lp) {
		for (ep = lp->head; ep; ep = nep) {
			nep = ep->next;
			element_free(ep);
		}

		Free(lp);
	}
}

/*
 * --------------------------------------------------------------- 
 */

void
element_free(element_t * ep)
{
	element_t	*e;
	varr_t		*va;

	if (ep) {
		switch (ep->type) {
		case SPOC_ATOM:
		case SPOC_PREFIX:
		case SPOC_SUFFIX:
			atom_free(ep->e.atom);
			break;

		case SPOC_LIST:
			list_free(ep->e.list);
			break;

		case SPOC_SET:
			va = ep->e.set;
			if (va) {
				while ((e = varr_pop(va)))
					element_free(e);
				varr_free(va);
			}
			break;

		case SPOC_RANGE:
			range_free(ep->e.range);
			break;

		}

		Free(ep);
	}
}

/*
 * --------------------------------------------------------------- 
 */

void
boundary_free(boundary_t * bp)
{
	if (bp) {
		switch (bp->type & 0x07) {
		case SPOC_ALPHA:
		case SPOC_DATE:
			if (bp->v.val.len)
				Free(bp->v.val.val);
			break;

		case SPOC_NUMERIC:
		case SPOC_TIME:
		case SPOC_IPV4:
		case SPOC_IPV6:
			break;
		}

		Free(bp);
	}
}

/*
 * --------------------------------------------------------------- 
 */

static void
dset_free(dset_t * ds)
{
	if (ds) {
		if (ds->va) {
			varr_free(ds->va);
		}
		if (ds->next)
			dset_free(ds->next);
		Free(ds);
	}
}

/*
 * --------------------------------------------------------------- 
 */

void
branch_free(branch_t * bp)
{
	int	i;

	if (bp) {
		switch (bp->type) {
		case SPOC_ATOM:
			phash_free(bp->val.atom);
			break;

		case SPOC_LIST:
			junc_free(bp->val.list);
			break;

		case SPOC_SET:
			dset_free(bp->val.set);
			break;

		case SPOC_PREFIX:
			ssn_free(bp->val.prefix);
			break;

		case SPOC_RANGE:
			for (i = 0; i < DATATYPES; i++)
				sl_free_slist(bp->val.range[i]);
			break;

		}
	}
}

void
junc_free(junc_t * juncp)
{
	int	i;

	if (juncp) {
		for (i = 0; i < NTYPES; i++) {
			if (juncp->item[i]) {
				branch_free(juncp->item[i]);
			}
		}
		Free(juncp);
	}
}
