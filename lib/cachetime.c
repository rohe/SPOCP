/*!
 * \file cachetime.c
 * \author Roland Hedberg <roland@catalogix.se>
 * \brief Functions for setting and storing cachetime definitions 
 */
#include <string.h>

#include <plugin.h>
#include <spocp.h>
#include <wrappers.h>

/*!
 * \brief checks if a byte array is a representation of a integer, and if so
 *  does the conversion.
 * \param op THe byte array
 * \param l A Pointer to a long int where the integer value can be stored
 * \return A Spocp return code
 */
spocp_result_t  is_numeric(octet_t * op, long *l);

/*!
 * \brief Given the present time and how long this result should be cached, 
 *  calculated the time when it will expire
 * \param str The key under which the result is to be stored
 * \param ct A linked list of cache time definitions
 * \return The expire time or 0 if there was no rule for how long it should 
 *   be stored.
 */
time_t
cachetime_set(octet_t * str, cachetime_t * ct)
{
	cachetime_t    *xct = 0;
	int             l, max = -1;

	for (; ct; ct = ct->next) {
		if (ct->pattern.size == 0 && max == -1) {
			max = 0;
			xct = ct;
		} else {
			l = ct->pattern.len;
			if (l > max && octncmp(str, &ct->pattern, l) == 0) {
				xct = ct;
				max = l;
			}
		}
	}

	if (xct)
		return xct->limit;
	else
		return 0;
}

/*!
 * \brief Store a cache time definition.
 * expects the format time [ 1*SP pattern ] 
 * \param s The definition 
 * \result A cachetime_t struct with all the fields filled in.
 */
cachetime_t    *
cachetime_new(octet_t * s)
{
	cachetime_t    *new;
	long int        li;
	octarr_t       *arg;

	arg = oct_split(s, ' ', '\0', 0, 0);

	if (is_numeric(arg->arr[0], &li) != SPOCP_SUCCESS) {
		octarr_free(arg);
		return NULL;
	}

	new = (cachetime_t *) Malloc(sizeof(cachetime_t));
	new->limit = (time_t) li;

	if (arg->n > 1)
		octcpy(&new->pattern, arg->arr[1]);
	else
		memset(&new->pattern, 0, sizeof(octet_t));

	octarr_free(arg);

	return new;
}

/*!
 * \brief Remove a linked list of cachetime definitions
 * \param ct The head of the list
 */
void
cachetime_free(cachetime_t * ct)
{
	if (ct) {
		if (ct->pattern.size)
			free(ct->pattern.val);
		if (ct->next)
			cachetime_free(ct->next);
		free(ct);
	}
}
