/*!
 * \file lib/octarr.c
 * \author Roland Hedberg <roland@catalogix.se> 
 * \brief A set of functions working on or with octarr_t structs
 */

/***************************************************************************
                          octarr.c  -  description
                             -------------------
    begin                : Jant 12 2004
    copyright            : (C) 2004 by Stockholm university, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in tdup level directory
   of this package for details.

 ***************************************************************************/


#include <config.h>

#include <string.h>

#include <spocp.h>
#include <wrappers.h>

/*!
 * \brief Creates a new octarr struct
 * \param n The size of the array
 * \return A octarr struct
 */
octarr_t       *
octarr_new(size_t n)
{
	octarr_t       *oa;

	oa = (octarr_t *) Malloc(sizeof(octarr_t));
	oa->size = n;
	oa->n = 0;
	oa->arr = (octet_t **) Calloc(n, sizeof(octet_t *));

	return oa;
}

/*!
 * \brief Creates a copy of a octarr struct
 * \param old The octarr struct to be copied
 * \return A pointer to the copy
 */
octarr_t       *
octarr_dup(octarr_t * old)
{
	int             i;
	octarr_t       *new;

	new = octarr_new(old->size);

	for (i = 0; i < old->n; i++)
		new->arr[i] = octdup(old->arr[i]);
	new->n = old->n;

	return new;
}

/*!
 * \brief Add a octet struct to the octarr struct. If the octarr struct pointer is
 *   NULL a new will be created. If the storage array is to small it will be increased.
 * \param oa A pointer to the octarr struct in which the octet struct should be placed
 * \param op The octet struct that shgould be added
 * \return A pointer to the octarr strict
 */
octarr_t       *
octarr_add(octarr_t * oa, octet_t * op)
{
	octet_t       **arr;

	if (oa == 0) {
		oa = octarr_new(4);
	}

	if (oa->size == 0) {
		oa->size = 2;
		oa->arr = (octet_t **) Malloc(oa->size * sizeof(octet_t *));
		oa->n = 0;
	} else if (oa->n == oa->size) {
		oa->size *= 2;
		arr =
		    (octet_t **) Realloc(oa->arr,
					 oa->size * sizeof(octet_t *));
		oa->arr = arr;
	}

	oa->arr[oa->n++] = op;

	return oa;
}

/*!
 * \brief Will increase the size of the storage array in a octarr struct
 * \param oa The octarr which arr struct should be increased
 * \param n The size that the array should be, if it is smaller than the
 *    present size the array will not be resized.
 */
void
octarr_mr(octarr_t * oa, size_t n)
{
	octet_t       **arr;

	if (oa) {
		if (oa->size == 0){
			oa->size = n;
			oa->n = 0;
			oa->arr = (octet_t **) Calloc(n, sizeof(octet_t *));
		}
		else if (n > oa->size)
			oa->size = n;

			arr = (octet_t **)
				Realloc(oa->arr, oa->size * sizeof(octet_t *));
			oa->arr = arr;
		}
	}
}
/*! 
 * \brief Will free a octarr struct, but will not attempt to free the octet structs
 *     that are stored within the octarr struct.
 * \param oa A pointer to the octarr to be freed.
 */
void
octarr_half_free(octarr_t * oa)
{
	int             i;

	if (oa) {
		if (oa->size) {
			for (i = 0; i < oa->n; i++)
				free(oa->arr[i]);

			free(oa->arr);
		}
		free(oa);
	}
}

/*!
 * \brief Will free not just the octarr struct but also the octet structs stored
 *   within it.
 * \param oa A pointer to the octarr struct to be freed
 */
void
octarr_free(octarr_t * oa)
{
	int             i;

	if (oa) {
		if (oa->size) {
			for (i = 0; i < oa->n; i++)
				oct_free(oa->arr[i]);

			free(oa->arr);
		}
		free(oa);
	}
}

/*!
 * \brief Will remove the first octet stored in the octarr struct
 * \param oa A pointer to the octarr struct
 * \return A pointer to the octet struct removed from the octarr struct
 */
octet_t        *
octarr_pop(octarr_t * oa)
{
	octet_t        *oct;
	int             i;

	if (oa == 0 || oa->n == 0)
		return 0;

	oct = oa->arr[0];

	oa->n--;
	for (i = 0; i < oa->n; i++) {
		oa->arr[i] = oa->arr[i + 1];
	}

	return oct;
}

/*
 * make no attempt at weeding out duplicates 
 */
/*
 * not the fastest way you could do this 
 */
/*!
 * \brief Adds all the octets stored in one octarr struct to another, removing them
 *   at the same time from the first. If the target is non existent the source is
 *   returned. If both target and source is not NULL, then the source octarr will
 *   be removed.
 * \param target The octarr into which the octet structs should be moved
 * \param source The octarr struct from which the octet structs are picked
 * \return A octarr struct with all the octet structs in it.
 */
octarr_t       *
octarr_join(octarr_t * target, octarr_t * source)
{
	octet_t        *o;

	if (target == 0)
		return source;
	if (source == 0)
		return target;

	while ((o = octarr_pop(source)) != 0)
		octarr_add(target, o);

	octarr_free(source);

	return target;
}

/*
char           *
safe_strcat(char *dest, char *src, int *size)
{
	char           *tmp;
	int             dl, sl;

	if (src == 0 || *size <= 0)
		return 0;

	dl = strlen(dest);
	sl = strlen(src);

	if (sl + dl > *size) {
		*size = sl + dl + 32;
		tmp = Realloc(dest, *size);
		dest = tmp;
	}

	strcat(dest, src);

	return dest;
}
*/

/*!
 * \brief Removes a specific octet struct from a octarr struct, the octarr
 *   array will be collapsed after the octet is removed.
 * \param oa The octarr struct
 * \param n The array index of the octet struct to be removed
 * \return The removed octet struct or NULL if the array didn't contain
 *    a octet struct at the specific place. 
 */
octet_t        *
octarr_rm(octarr_t * oa, int n)
{
	octet_t        *o;
	int             i;

	if (oa == 0 || n > oa->n)
		return 0;

	o = oa->arr[n];
	oa->n--;

	for (i = n; i < oa->n; i++)
		oa->arr[i] = oa->arr[i + 1];

	return o;
}

/*
 * ====================================================================== 
 */

/*!
 * \brief splits a octetstrin into pieces at the places where a specific
 *   octet occurs.
 * \param o The octet to be split
 * \param c The octet at which the string should be split
 * \param ec An escape octet that if it occurs before a split octet, it 
 *   will prevent the string from being split at that position.
 * \param flag If 1 consecutive occurences of the split octet counts as one.
 * \param max The maximum size of the resulting array. 
 * \return A octarr struct with newly created octet string copies of the 
  *  substrings
 */
octarr_t       *
oct_split(octet_t * o, char c, char ec, int flag, int max)
{
	char           *cp, *sp;
	octet_t        *oct = 0;
	octarr_t       *oa;
	int             l, i, n = 0;

	if (o == 0)
		return 0;

	oct = octdup(o);
	oa = octarr_new(4);

	for (sp = oct->val, l = oct->len, i = 0; l && (max == 0 || i < max);
	     sp = cp) {
		for (cp = sp, n = 0; l; cp++, n++, l--) {
			if (*cp == ec)
				cp++;	/* skip escaped characters */
			if (*cp == c)
				break;
		}
		oct->len = n;

		octarr_add(oa, oct);

		if (flag)
			for (cp++; *cp == c; cp++, l--);
		else if (l)
			cp++, l--;
		else {
			oct = 0;
			break;
		}

		oct = oct_new(0, 0);
		oct->val = cp;
	}

	if (oct)
		octarr_add(oa, oct);

	return oa;
}

