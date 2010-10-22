
/***************************************************************************
                          subelem.c  -  description
                             -------------------
    begin                : Thr May 15 2003
    copyright            : (C) 2003 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <subelem.h>
#include <wrappers.h>

/********************************************************************/

subelem_t      *
subelem_new(void)
{
	subelem_t      *s;

	s = (subelem_t *) Calloc(1, sizeof(subelem_t));

	return s;
}

/********************************************************************/

void
subelem_free(subelem_t * sep)
{
	if (sep) {
		if (sep->ep)
			element_free(sep->ep);
		if (sep->next)
			subelem_free(sep->next);
		Free(sep);
	}
}

/********************************************************************/
/*!
 * Syntax of the LIST command 
 * list = "4:LIST" [l-path] *( pattern_length ":" "+"/"-" s-expr ) 
 * \brief Takes a array of list patterns and makes it into a subelem_t
 *  struct
 * \param pattern The octet array of patterns
 * \return The subelem_t struct
 */

subelem_t *
pattern_parse(octarr_t * pattern)
{
	subelem_t      *res = 0, *sub, *pres = 0;
	element_t      *ep;
	octet_t         oct, **arr;
	int             i;
    
	for (i = 0, arr = pattern->arr; i < pattern->n; i++, arr++) {
		sub = subelem_new();
        
		if (*((*arr)->val) == '+')
			sub->direction = GT;
		else if (*((*arr)->val) == '-')
			sub->direction = LT;
		else
			return 0;
        
		if (*((*arr)->val + 1) == '(')
			sub->list = 1;
        
		octln(&oct, *arr);
		/*
		 * skip the +/- sign 
		 */
		oct.val++;
		oct.len--;
        
		if (get_element(&oct, &ep) != SPOCP_SUCCESS) {
			subelem_free(sub);
			subelem_free(res);
			return 0;
		}
        
		sub->ep = ep;
		if (res == 0)
			res = pres = sub;
		else {
			pres->next = sub;
			pres = sub;
		}
	}
    
	return res;
}
