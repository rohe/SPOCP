
/***************************************************************************
                          index.c  -  description
                             -------------------
    begin                : Wen Aug 27 2003
    copyright            : (C) 2003 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <string.h>

#include <db0.h>
#include <func.h>
#include <wrappers.h>

index_t        *
index_new(int size)
{
	index_t        *new;

	new = (index_t *) Malloc(sizeof(index_t));

	if (size) {
		new->size = size;
		new->arr = (ruleinst_t **) Calloc(size, sizeof(ruleinst_t *));
	} else {
		new->size = 0;
		new->arr = 0;
	}
	new->n = 0;

	return new;
}

/*
 * when duplicating an array of pointers, in this case to rule instances I
 * have to make sure the copy points to the new copy of the rule instance. No
 * good pointing to the old since that one will be deleted 
 */

index_t        *
index_dup(index_t * id, ruleinfo_t * ri)
{
	index_t        *new;
	int             i;
	ruleinst_t     *r;

	new = index_new(id->size);

	for (i = 0; i < id->n; i++) {

		r = (ruleinst_t *) id->arr[i];

		if (ri != 0) {
			new->arr[i] = rbt_search(ri->rules, r->uid);
		} else
			new->arr[i] = id->arr[i];	/* NO new
							 * ruleinstances ? */

		new->n++;
	}

	return new;
}

void
index_free(index_t * id)
{
	int             i;

	if (id) {
		if (id->size) {
			for (i = 0; i < id->n; i++)
				ruleinst_free(id->arr[i]);
			free(id->arr);
		}
		free(id);
	}
}

index_t        *
index_add(index_t * id, ruleinst_t * ri)
{
	ruleinst_t    **ra;

	if (ri == 0)
		return id;

	if (id == 0)
		id = index_new(2);

	if (id->n == id->size) {
		id->size *= 2;

		ra = Realloc(id->arr, id->size * sizeof(ruleinst_t *));
		id->arr = ra;
	}

	id->arr[id->n++] = ri;

	return id;
}

int
index_rm(index_t * id, ruleinst_t * ri)
{
	int             i, j;

	for (i = 0; i < id->n; i++) {
		if (id->arr[i] == ri) {
			id->n--;

			for (j = i; j < id->n; j++)	/* left shift */
				id->arr[j] = id->arr[j + 1];

			id->arr[j] = 0;
		}
	}

	if (i == id->n)
		return FALSE;
	else
		return TRUE;
}
