
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

#include <db0.h>
#include <struct.h>
#include <func.h>
#include <proto.h>
#include <spocp.h>
#include <wrappers.h>
#include <macros.h>

/********************************************************************/

subelem_t      *
subelem_new()
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
		free(sep);
	}
}

/********************************************************************/

int
to_subelements(octet_t * arg, octarr_t * oa)
{
	int             depth = 0;
	octet_t         oct, *op, str;

	/*
	 * first one should be a '(' 
	 */
	if (*arg->val == '(') {
		arg->val++;
		arg->len--;
	} else
		return -1;

	oct.val = arg->val;
	oct.len = 0;

	while (arg->len) {
		if (*arg->val == '(') {	/* start of list */
			depth++;
			if (depth == 1) {
				oct.val = arg->val;
			}

			arg->len--;
			arg->val++;
		} else if (*arg->val == ')') {	/* end of list */
			depth--;
			if (depth == 0) {
				oct.len = arg->val - oct.val + 1;
				op = octdup(&oct);
				octarr_add(oa, op);
				oct.val = arg->val + 1;
			} else if (depth < 0) {
				arg->len--;
				arg->val++;
				break;	/* done */
			}

			arg->len--;
			arg->val++;
		} else {
			if (get_str(arg, &str) != SPOCP_SUCCESS)
				return -1;

			if (depth == 0) {
				oct.len = arg->val - oct.val;
				op = octdup(&oct);
				octarr_add(oa, op);
				oct.val = arg->val;
				oct.len = 0;
			}
		}
	}

	return 1;
}

/********************************************************************/

subelem_t      *
octarr2subelem(octarr_t * oa, int dir)
{
	subelem_t      *head = 0, *pres = 0, *tmp;
	element_t      *ep;
	int             i;

	for (i = 0; i < oa->n; i++) {
		tmp = subelem_new();
		tmp->direction = dir;

		if (*(oa->arr[i]->val) == '(')
			tmp->list = 1;
		else
			tmp->list = 0;

		if (element_get(oa->arr[i], &ep) != SPOCP_SUCCESS) {
			subelem_free(tmp);
			subelem_free(head);
			return 0;
		}

		tmp->ep = ep;

		if (head == 0)
			head = pres = tmp;
		else {
			pres->next = tmp;
			pres = pres->next;
		}
	}

	return head;
}

/********************************************************************/

subelem_t      *
subelement_dup(subelem_t * se)
{
	subelem_t      *new = 0;

	if (se == 0)
		return 0;

	new = subelem_new();

	new->direction = se->direction;
	new->list = se->list;
	new->ep = element_dup(se->ep, 0);

	if (se->next)
		new->next = subelement_dup(se->next);

	return new;
}
