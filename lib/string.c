
/***************************************************************************
                          string.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in tdup level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <func.h>

#include <struct.h>
#include <wrappers.h>
#include <spocp.h>
#include <macros.h>

/*--------------------------------------------------------------------------------*/

strarr_t       *
strarr_new(int size)
{
	strarr_t       *new;

	new = (strarr_t *) Malloc(sizeof(strarr_t));

	new->size = size;
	new->argc = 0;
	if (size)
		new->argv = (char **) Calloc(size, sizeof(char *));
	else
		new->argv = 0;

	return new;
}

strarr_t       *
strarr_add(strarr_t * sa, char *arg)
{
	char          **arr;

	if (sa == 0)
		sa = strarr_new(4);

	if (sa->argc == sa->size) {
		if (sa->size)
			sa->size *= 2;
		else
			sa->size = 4;

		arr = (char **) Realloc(sa->argv, sa->size * sizeof(char *));
		sa->argv = arr;
	}

	sa->argv[sa->argc++] = strdup(arg);

	return sa;
}

void
strarr_free(strarr_t * sa)
{
	int             i;

	if (sa) {
		for (i = 0; i < sa->argc; i++)
			free(sa->argv[i]);
		free(sa->argv);
		free(sa);
	}
}

void
rmcrlf(char *s)
{
	char           *cp;
	int             flag = 0;

	for (cp = &s[strlen(s) - 1]; cp >= s && (*cp == '\r' || *cp == '\n');
	     flag++, cp--);

	if (flag) {
		*++cp = '\0';
	}
}

char           *
rmlt(char *s)
{
	char           *cp;
	int             f = 0;

	cp = &s[strlen(s) - 1];

	while (*cp == ' ' || *cp == '\t')
		cp--, f++;
	if (f)
		*(++cp) = '\0';

	for (cp = s; *cp == ' ' || *cp == '\t'; cp++);

	return cp;
}

char          **
line_split(char *s, char c, char ec, int flag, int max, int *parts)
{
	char          **res, *cp, *sp;
	register char   ch;
	char           *tmp;
	int             i, n = 0;

	if (flag) {
		while (*s == c)
			s++;
		for (i = 0, cp = &s[strlen(s) - 1]; *cp == c; cp--, i++);
		if (i)
			*(++cp) = '\0';
	}

	if (max == 0) {
		/*
		 * find out how many strings there are going to be 
		 */
		for (cp = s; *cp; cp++)
			if (*cp == c)
				n++;
		max = n + 1;
	} else
		n = max;

	res = (char **) Calloc(n + 3, sizeof(char *));
	tmp = Strdup(s);

	for (sp = tmp, i = 0, max--; sp && i < max; sp = cp, i++) {
		for (cp = sp; *cp; cp++) {
			ch = *cp;
			if (ch == ec)
				cp += 2;	/* skip escaped characters */
			if (ch == c || ch == '\0')
				break;
		}

		if (*cp == '\0')
			break;

		*cp = '\0';
		if (*sp)
			res[i] = Strdup(sp);

		if (flag)
			for (cp++; *cp == c; cp++);
		else
			cp++;
	}

	res[i] = Strdup(sp);

	*parts = i;

	free(tmp);

	return res;
}

/*
 * --------------------------------------------------------------------- 
 */

char           *
find_balancing(char *p, char left, char right)
{
	int             seen = 0;
	char           *q = p;

	for (; *q; q++) {
		/*
		 * if( *q == '\\' ) q += 2 ; skip escaped characters 
		 */

		if (*q == left)
			seen++;
		else if (*q == right) {
			if (seen == 0)
				return (q);
			else
				seen--;
		}
	}
	return (NULL);
}

void
charmatrix_free(char **arr)
{
	int             i;

	if (arr) {
		for (i = 0; arr[i] != 0; i++) {
			free(arr[i]);
		}
		free(arr);
	}
}

/*
 * ---------------- 
 */

char           *
lstrndup(char *s, int n)
{
	char           *tmp;

	tmp = (char *) Malloc((n + 1) * sizeof(char));
	strncpy(tmp, s, n);
	tmp[n] = '\0';

	return tmp;
}
