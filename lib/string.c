/*! \file lib/string.c
 * \author Roland Hedberg <roland@catalogix.se>
 * \brief Basic functions working on null terminated strings
 */
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

#ifdef HAVE_LIBIDN
#include <locale.h>
#include <stringprep.h>
#endif

#include <wrappers.h>
#include <spocp.h>
#include <macros.h>

/*--------------------------------------------------------------------------------*/

/*!
 * \brief Removes CR (0x0D) and LR (0x0A) from the end of a string
 * \param s The string on wich the work should be done
 */
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

/*!
 * \brief Removes leading and trailing spaces (0x20 or 0x09) from the end of a string
 * \param s The string on wich the work should be done
 * \return A pointer to the resulting string
 */
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

/*!
 * \brief Splits a string into pieces, placing the pieces in a array of strings
 * \param s The string that is to be split
 * \param c THe character at which the split should occur
 * \param ec The escape character if any, a split character that is proceeded by the 
 *   escape character is not used as a pointer for splitting
 * \param flag If 1 then leading and trailing split characters are ignored
 * \param max The maximum size of the resulting array. While splitting is the 
 *   number of substring reaches this number the splitting is stopped.
 * \param parts A pointer to a integer which will contain the size of the resulting
 *   array.
 * \return A array of strings, each one a newly created copy of a substring
 */
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

/*!
 * \brief Find the matching right hand character.
 * \param p The string to search
 * \param left The left hand side character
 * \param right The right hand side character
 * \return A pointer to the matching right hand character if there is one or NULL
 *   if there isn't
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

/*
 * --------------------------------------------------------------------- 
 */

/*!
 * \brief frees an array of char arrays
 * \param m A pointer to the array of arrays 
 */

void charmatrix_free( char **m )
{
	int i ;

	if (m) {
		for (i = 0; m[i] != 0; i++ ) {
			free(m[i]);
		}
		free(m);
	}
}


/*
 * --------------------------------------------------------------------
 */
/*!
 * \brief if necessary libraries are present this function converts the input 
 * to utf8 and normalizes it.
 * \param s The input string
 * \return Returns a newly allocated zero-terminated string.
 */
char *normalize( char *s )
{
	char *buf;
#ifdef HAVE_LIBIDN
	int	bufsize, rc;
	char	*utf8;


	traceLog(LOG_INFO,"Doing stringprep on %s", s);
	/* stringprep_locale_charset(); */
	utf8 = stringprep_locale_to_utf8(s);

	bufsize = strlen(utf8) * 2;
	buf = (char *) malloc ( bufsize * sizeof( char ) );
	strcpy(buf,utf8);

	rc = stringprep (buf, bufsize, 0, stringprep_nameprep);
	if (rc != STRINGPREP_OK) {
		traceLog(LOG_INFO,"Stringprep failed with rc %d", rc);
		free(buf);
		buf = utf8;
	} else 
		free(utf8);
	
	traceLog(LOG_INFO,"result %s", buf);
#else
	buf = strdup(s);
#endif

	return buf;
}

