#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ldapset.h"


/*
 * ========================================================== 
 */

static lsln_t  *
lsln_new(char *s)
{
	lsln_t	*new;

	new = (lsln_t *) calloc(1, sizeof(lsln_t));

	new->str = s;

	return new;
}

void
lsln_free(lsln_t * slp)
{
	if (slp) {
		if (slp->str)
			free(slp->str);
		if (slp->next)
			free(slp->next);
		free(slp);
	}
}

lsln_t  *
lsln_add(lsln_t * slp, char *s)
{
	lsln_t	*loc;

	if (slp == 0)
		return lsln_new(s);

	for (loc = slp; loc; loc = loc->next) {
		if (strcmp(loc->str, s) == 0)
			return slp;
	}

	for (loc = slp; loc->next; loc = loc->next);

	loc->next = lsln_new(s);

	return slp;
}

lsln_t  *
lsln_dup(lsln_t * old)
{
	lsln_t	 *new = 0;

	for (; old; old = old->next)
		new = lsln_add(new, old->str);

	return new;
}

lsln_t  *
lsln_or(lsln_t * a, lsln_t * b)
{
	lsln_t	 *ls, *res = 0;

	for (ls = a; ls; ls = ls->next) {
		res = lsln_add(res, ls->str);
		ls->str = 0;
	}

	for (ls = b; ls; ls = ls->next) {
		res = lsln_add(res, ls->str);
		ls->str = 0;
	}

/*
	for (; b; b = b->next) {
		for (ls = a; ls; ls = ls->next) {
			if (ls->str && strcmp(ls->str, b->str) == 0) {
				res = lsln_add(res, ls->str);
				ls->str = 0;
				break;
			}
		}
	}
*/

	return res;
}

lsln_t  *
lsln_join(lsln_t * a, lsln_t * b)
{
	for (; b; b = b->next) {
		a = lsln_add(a, b->str);
		b->str = 0;
	}

	return a;
}

lsln_t  *
lsln_and(lsln_t * a, lsln_t * b)
{
	lsln_t	 *ls, *res = 0;

	for (; b; b = b->next) {
		for (ls = a; ls; ls = ls->next) {
			if (ls->str && strcmp(ls->str, b->str) == 0) {
				res = lsln_add(res, ls->str);
				ls->str = 0;
				break;
			}
		}
	}

	return res;
}

/*
 * this ought really to be UTF-8 aware 
 */
int
lsln_find(lsln_t * sl, char *s)
{
	for (; sl; sl = sl->next)
		if (strcasecmp(sl->str, s) == 0)
			return 1;

	return 0;
}

/*
 * static lsln_t *lsln_first( lsln_t *sl ) { return sl ; }
 * 
 * static lsln_t *lsln_next( lsln_t *sl ) { return sl->next ; } 
 */

char	*
lsln_get_val(lsln_t * sl)
{
	return sl->str;
}

int
lsln_values(lsln_t * sl)
{
	int	 i = 0;

	for (; sl; sl = sl->next)
		i++;

	return i;
}

char   **
lsln_to_arr(lsln_t * sl)
{
	char	**arr;
	int	 i;

	arr = (char **) calloc(lsln_values(sl) + 1, sizeof(char *));

	for (i = 0; sl; sl = sl->next, i++)
		arr[i] = sl->str;

	return arr;
}

void
lsln_print( lsln_t *ll )
{
	int	i;
	char	*tmp;

	for( i=0 ;ll ; i++, ll = ll->next) {
		tmp = str_esc(ll->str, strlen(ll->str));
		free(tmp);
	}
}

