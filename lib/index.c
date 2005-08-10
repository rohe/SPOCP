
/***************************************************************************
			index.c  -  description
			-------------------
	begin		: Wen Aug 27 2003
	copyright	: (C) 2003 by Umeå University, Sweden
	email		: roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <string.h>

#include <db0.h>
#include <func.h>
#include <wrappers.h>

#define MAX(a,b) ((a) < (b) ? b : a)

/* #define AVLUS 1 */

spocp_index_t	*
index_new(int size)
{
	spocp_index_t	*new;

	new = (spocp_index_t *) Malloc(sizeof(spocp_index_t));

	if (size) {
		new->size = size;
		new->arr = (ruleinst_t **) Calloc(size, sizeof(ruleinst_t *));
#ifdef AVLUS
		traceLog(LOG_DEBUG,"index_new size %d -> %p", size, new->arr);
#endif
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

spocp_index_t		*
index_dup(spocp_index_t * id, ruleinfo_t * ri)
{
	spocp_index_t		*new;
	int			 i;
	ruleinst_t	 *r;

	new = index_new(id->size);
#ifdef AVLUS
	traceLog(LOG_DEBUG, "index_dup %p",id);
#endif

	for (i = 0; i < id->n; i++) {

		r = (ruleinst_t *) id->arr[i];

		if (ri != 0) {
			new->arr[i] = rdb_search(ri->rules, r->uid);
		} else
			new->arr[i] = id->arr[i];	/* NO new
							 * ruleinstances ? */

		new->n++;
	}

	return new;
}

void
index_free(spocp_index_t * id)
{
	int			 i;

	if (id) {
		if (id->size) {
			for (i = 0; i < id->n; i++)
				ruleinst_free(id->arr[i]);
			Free(id->arr);
		}
		Free(id);
	}
}

/*!
 * \brief Shallow delete of a spocp_index, this means that the ruleinstance that
 * are pointed to will not be deleted. Only the basic structures will.
 * \param si The spocp_index
 */
void index_delete( spocp_index_t *si)
{
	if (si) {
		Free(si->arr);
		Free(si);
	}
}

spocp_index_t		*
index_add(spocp_index_t * id, ruleinst_t * ri)
{
	ruleinst_t	**ra;

	if (ri == 0)
		return id;

	if (id == 0) {
		DEBUG(SPOCP_DSTORE)
			traceLog( LOG_INFO, "First rule of it's kind");
		id = index_new(2);
	}
	else {
		DEBUG(SPOCP_DSTORE)
			traceLog(LOG_INFO, "Already got one of these (%d)",
				id->n);
	}

	if (id->n == id->size) {
		id->size *= 2;

		ra = Realloc(id->arr, id->size * sizeof(ruleinst_t *));
		id->arr = ra;
	}

	id->arr[id->n++] = ri;

#ifdef AVLUS
	traceLog( LOG_DEBUG, "index_add");
	index_print( id) ;
#endif

	return id;
}

/*!
 * \brief Tries to remove a ruleinst pointer from a spocp_index struct
 * \param id The spocp_index struct
 * \param ri The ruleinst
 * \return Returns TRUE if the pointer was remove, FALSE is it was part of the 
 * spocp_index
 */
int
index_rm(spocp_index_t * id, ruleinst_t * ri)
{
	int i, j;

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

/*!
 * \brief Creates a spocp_index that contains the ruleinst pointers that appears in 
 * both of the spocp_index'es given. Non-invasive, that is neither a not b is affected
 * by the and'ing.
 * \param a The first spocp_index
 * \param b The second spocp_index
 * \result A newly allocated spocp_index containing the the result, if there are no
 * common ruleinst pointer NULL will be returned.
 */
spocp_index_t *
index_and( spocp_index_t *a, spocp_index_t *b)
{
	spocp_index_t	*res;
	int		i,j;

	if (a == NULL || b == NULL ) 
		return NULL;

	res = index_new(MAX(a->n, b->n));
#ifdef AVLUS
	traceLog(LOG_DEBUG, "index_and %p",res);
#endif

	for (i = 0; i < a->n; i++)
		for( j = 0; j < b->n; j++)
			if (a->arr[i] == b->arr[j])
				res->arr[res->n++] = a->arr[i];
	
	/* traceLog(LOG_DEBUG,"index_and => %d rules", res->n);*/

	if (res->n == 0) {
		index_delete( res );
		return NULL;
	}
	else
		return res;
}

/*!
 * \brief Adds the set of ruleinst pointers that appears in a spocp_index struct
 * to another spocp_index struct
 * \param a The spocp_index struct to which the pointer should be added
 * \param b The spocp_index struct which pointer should be added. Duplicates will be
 * avoided
 */
void
index_extend( spocp_index_t *a, spocp_index_t *b)
{
	int	i,j=0;

	for (i = 0; i < b->n; i++) {
		for( j = 0; j < a->n; j++)
			if (a->arr[j] == b->arr[i])
				break;
		if (j == a->n)
			index_add(a,b->arr[i]);
	}
}

/*!
 * \brief Makes a shallow copy of a spocp_index struct. The shallowness
 * comes from the fact that the ruleinst structs that are pointed to are
 * not duplicated. The pointers are only copied. 
 * \param si The spocp_index struct to be copied
 * \result A pointer to a spocp_index that is a shallow copy of the old
 */
spocp_index_t *
index_cp( spocp_index_t *si )
{
	spocp_index_t	*res;
	int	i;

	res = index_new( si->size );
#ifdef AVLUS
	traceLog(LOG_DEBUG, "index_cp %p",res);
#endif
	for (i = 0; i < si->n; i++ )
		 res->arr[i] = si->arr[i];

	res->n = si->n;

	return res;
}

void index_print( spocp_index_t *si)
{
	int	i;

	for( i = 0; i < si->n ; i++)
		traceLog(LOG_DEBUG,"Rule: %s", si->arr[i]->uid);
}

