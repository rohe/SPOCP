
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

#include <octet.h>
#include <range.h>
#include <element.h>
#include <dset.h>
#include <wrappers.h>

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

