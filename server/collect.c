/***************************************************************************
                           collect.c  -  description
                             -------------------
    begin                : Tue Nov 29 2005
    copyright            : (C) 2005 by Umeå University, Sweden
    email                : roland.hedberg@adm.umu.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 **************************************************************************/

#include <stdarg.h>
#include "locl.h"

varr_t *collect_indexes(ruleset_t *rs)
{
	ruleset_t	*cr;
	varr_t      *pa = NULL, *pb;
	
	pa = rdb_all(rs->db->ri->rules);
				
	/* If children do those too */
	if (rs->down){
		for(cr = rs->down; cr->left; cr = cr->left);
		/* One at the time */
		for( ; cr ; cr = cr->right){
			pb = collect_indexes(cr);
			pa = varr_or(pa,pb,0);
		}
	}
	
	return pa;
}
