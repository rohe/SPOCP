/*!
 * \file lib/sexp.c
 * \author Roland Hedberg <roland@catalogix.se>
 * \brief Basic S-expression functions
 */
/***************************************************************************
                          sexp.c  -  description
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

#include <spocp.h>
#include <proto.h>
#include <macros.h>
#include <wrappers.h>

/*!
 * \brief Grabs the length field from a S-expression
 * \param op A octet struct containing the remaining part of the S-expression
 * \param rc pointer to a result code enum.
 * \return The length of the following atom or -1 if something went wrong.
 */
int
get_len(octet_t * op, spocp_result_t *rc)
{
	char	*sp;
	int	n = 0;
	size_t  l;

	if( op->len == 0) {
		*rc = SPOCP_MISSING_CHAR;
		return -1;
	}

	for ( n = 0, l = 0, sp = op->val ; l < op->len && DIGIT(*sp) ; l++,sp++) {
		if (n)
			n *= 10;

		n += *sp -'0' ;
	}

	if (n == 0) {
		sp = op->val;
		traceLog("No digit found at \"%c%c%c%c\"", *sp, *(sp+1),
		    *(sp+2), *(sp+3) ) ;
		*rc = SPOCP_SYNTAXERROR;
		return -1;
        }

	if (l == op->len) {
		*rc = SPOCP_MISSING_CHAR;
		return -1;
	}

	/*
	 * ep points to the first non digit character 
	 * which has to be ':'
	 */
	if (*sp != ':') {
		*rc = SPOCP_SYNTAXERROR;
		return -1;
	}

	op->len -= l;
	op->val = sp;

	return n;
}

/*!
 * \brief Removes a atom from the S-expression that is parsed. The format
 *  of the arom is &lt;len&gt;":"&lt;atom&gt;
 * \param so A octet struct containing the S-expression from which
 *   the atom should be removed.
 * \param ro Pointer to a octet struct where the octet is placed
 * \return A spocp result code.
 */
spocp_result_t
get_str(octet_t * so, octet_t * ro)
{
	spocp_result_t rc;

	if (ro == 0)
		return SPOCP_OPERATIONSERROR;

	ro->size = 0;

	if (( ro->len = get_len(so, &rc)) < 0 ) 
		return rc;

	if ((so->len - 1) < ro->len) {
		LOG(SPOCP_ERR) {
			traceLog
			    ("expected length of string (%d) exceeds input length (%d)",
			     ro->len, so->len);
			traceLog("Offending part \"%c%c%c\"", so->val[0],
				 so->val[1], so->val[2]);
		}
		return SPOCP_MISSING_CHAR;
	}

	if (ro->len == 0) {
		LOG(SPOCP_ERR) traceLog("Zero length strings not allowed");
		return SPOCP_SYNTAXERROR;
	}

	so->val++;
	so->len--;

	ro->val = so->val;
	ro->size = 0;		/* signifying that it's not dynamically
				 * allocated */

	so->val += ro->len;
	so->len -= ro->len;

	DEBUG(SPOCP_DPARSE)
	    traceLog("Got 'string' \"%*.*s\"", ro->len, ro->len, ro->val);

	return SPOCP_SUCCESS;
}


/*!
 * \brief Finds the length of a S-expression
 * \param sexp A pointer to a octet struct containing the S-expression.
 * \return -1 if no proper s-exp 0 if missing chars otherwise the number of
 * characters 
 */

int
sexp_len(octet_t * sexp)
{
	spocp_result_t  r;
	int             depth = 0;
	octet_t         oct, str;

	oct.val = sexp->val;
	oct.len = sexp->len;

	/*
	 * first one should be a '(' 
	 */
	if (*oct.val != '(')
		return -1;

	oct.val++;
	oct.len--;

	while (oct.len) {
		if (*oct.val == '(') {	/* start of list */
			depth++;

			oct.len--;
			oct.val++;
		} else if (*oct.val == ')') {	/* end of list */
			depth--;

			if (depth < 0) {
				oct.len--;
				break;	/* done */
			}

			oct.len--;
			oct.val++;
		} else {
			r = get_str(&oct, &str);
			if (r == SPOCP_SYNTAXERROR)
				return -1;
			else if (r == SPOCP_MISSING_CHAR)
				return 0;
		}
	}

	if (depth >= 0)
		return 0;

	return (sexp->len - oct.len);
}


/*--------------------------------------------------------------------------------*/

/*!
 * \brief Constructs a octet string given a format specification and a
 *    array of void pointers to arguments.
 * \param sexp A memory area where the result will be placed
 * \param size A poiinter to a integer containing the size of the memory area.
 *   this integer will be updated to reflect the size of the memory area that
 *   was not used.
 * \param format The format specification, the allowed characters are '(',
 *   ')', 'o' (octet struct), 'O' (null terminated array of octet structs),
 *   'x' (octarr struct), 'a' (null terminated string), 'A' ( null terminated
 *   array of null terminated strings), 'l' (a null teminated string that
 *   is written verbatim), 'L' (a null terminated array of null terminated
 *   strings that are written verbatim), 'u' (a octet that is added
 *   verbatim to the target string) and 'U' (a octarr which octet values
 *   are added verbatim).
 *   When 'o','O','a' or 'A' strings are added they are preceeded by a
 *   length specification.
 *   Characters outside the range specified above are just ignored.
 * \param argv A array of void pointers that should be at least as big as
 *    the number of format characters in the format string.
 * \return A pointer to the resulting string, or NULL if the memory
 *    area is not big enough.
 */
char           *
sexp_printa(char *sexp, unsigned int *size, char *format, void **argv)
{
	char           *s, *sp, **arr;
	octet_t        *o, **oa;
	int             i, n, argc = 0;
	unsigned int    bsize = *size;
	octarr_t       *oarr;

	if (format == 0 || *format == '\0')
		return 0;

	memset(sexp, 0, bsize);
	sp = sexp;
	bsize -= 2;

	while (*format && bsize) {
		/*
		 * traceLog( "Sexp [%s][%c]", sexp, *format ) ; 
		 */
		switch (*format++) {
		case '(':	/* start of a list */
			*sp++ = '(';
			bsize--;
			break;

		case ')':	/* end of a list */
			*sp++ = ')';
			bsize--;
			break;

		case 'o':	/* octet */
			if ((o = (octet_t *) argv[argc++]) == 0);
			if (o && o->len) {
				n = snprintf(sp, bsize, "%d:", o->len);
				if (n < 0 || (unsigned int) n > bsize)
					return 0;
				bsize -= n;
				sp += n;

				if (o->len > bsize)
					return 0;
				memcpy(sp, o->val, o->len);
				bsize -= o->len;
				sp += o->len;
			}
			break;

		case 'O':	/* octet array */
			if ((oa = (octet_t **) argv[argc++]) == 0)
				return 0;

			for (i = 0; oa[i]; i++) {
				o = oa[i];
				if (o->len == 0)
					continue;
				n = snprintf(sp, bsize, "%d:", o->len);
				if (n < 0 || (unsigned int) n > bsize)
					return 0;
				bsize -= n;
				sp += n;

				if (o->len > bsize)
					return 0;
				memcpy(sp, o->val, o->len);
				bsize -= o->len;
				sp += o->len;
			}
			break;

		case 'X':	/* ocarr */
			if ((oarr = (octarr_t *) argv[argc++]) == 0)
				return 0;

			for (i = 0; i < oarr->n; i++) {
				o = oarr->arr[i];
				if (o->len == 0)
					continue;
				n = snprintf(sp, bsize, "%d:", o->len);
				if (n < 0 || (unsigned int) n > bsize)
					return 0;
				bsize -= n;
				sp += n;

				if (o->len > bsize)
					return 0;
				memcpy(sp, o->val, o->len);
				bsize -= o->len;
				sp += o->len;
			}
			break;

		case 'a':	/* atom */
			if ((s = (char *) argv[argc++]) == 0);
			if (s && *s) {
				n = snprintf(sp, bsize, "%d:%s", strlen(s), s);
				if (n < 0 || (unsigned int) n > bsize)
					return 0;
				bsize -= n;
				sp += n;
			}
			break;

		case 'A':	/* sequence of atoms */
			if ((arr = (char **) argc[argv++]) == 0)
				return 0;
			for (i = 0; arr[i]; i++) {
				n = snprintf(sp, bsize, "%d:%s",
					     strlen(arr[i]), arr[i]);
				if (n < 0 || (unsigned int) n > bsize)
					return 0;
				bsize -= n;
				sp += n;
			}
			break;

		case 'l':	/* list */
			if ((s = (char *) argv[argc++]) == 0);
			if (s && *s) {
				n = snprintf(sp, bsize, "%s", s);
				if (n < 0 || (unsigned int) n > bsize)
					return 0;
				bsize -= n;
				sp += n;
			}
			break;

		case 'L':	/* array of lists */
			if ((arr = (char **) argv[argc++]) == 0)
				return 0;

			for (i = 0; arr[i]; i++) {
				n = snprintf(sp, bsize, "%s", arr[i]);
				if (n < 0 || (unsigned int) n > bsize)
					return 0;
				bsize -= n;
				sp += n;
			}
			break;

		case 'u':	/* octet list */
			if ((o = (octet_t *) argv[argc++]) == 0)
				return 0;

			if (o->len > bsize)
				return 0;
			memcpy(sp, o->val, o->len);
			bsize -= o->len;
			sp += o->len;

			break;

		case 'U':	/* array of octet lists */
			if ((oa = (octet_t **) argv[argc++]) == 0)
				return 0;

			for (i = 0; oa[i]; i++) {
				o = oa[i];
				if (o == NULL || o->len == 0)
					continue;
				if (o->len > bsize)
					return 0;
				memcpy(sp, o->val, o->len);
				bsize -= o->len;
				sp += o->len;
			}
			break;

		}

	}

	*size = bsize;

	return sexp;
}

/*--------------------------------------------------------------------------------*/

/*!
 * \brief Constructs a octet string given a format specification and a
 *    varianble amount of arguments.
 * \param sexp A memory area where the result will be placed
 * \param size A poiinter to a integer containing the size of the memory area.
 *   this integer will be updated to reflect the size of the memory area that
 *   was not used.
 * \param fmt The format specification, the allowed characters are '(',
 *   ')', 'o' (octet struct), 'O' (null terminated array of octet structs),
 *   'x' (octarr struct), 'a' (null terminated string), 'A' ( null terminated
 *   array of null terminated strings), 'l' (a null teminated string that
 *   is written verbatim), 'L' (a null terminated array of null terminated
 *   strings that are written verbatim), 'u' (a octet that is added
 *   verbatim to the target string) and 'U' (a octarr which octet values
 *   are added verbatim).
 *   When 'o','O','a' or 'A' strings are added they are preceeded by a
 *   length specification.
 *   Characters outside the range specified above are just ignored.
 * \param ... A varying number of arguments of varying types.
 * \return A pointer to the resulting string, or NULL if the memory area is
 *   not big enough.
 */
char           *
sexp_printv(char *sexp, unsigned int *size, char *fmt, ...)
{
	va_list         ap;
	char           *s, *sp, **arr;
	octet_t        *o, **oa;
	int             n, i;
	unsigned int    bsize = *size;
	octarr_t       *oarr;

	if (fmt == 0)
		return 0;

	va_start(ap, fmt);

	memset(sexp, 0, bsize);
	sp = sexp;

	while (*fmt && bsize)
		switch (*fmt++) {
		case '(':	/* start of a list */
			*sp++ = '(';
			bsize--;
			break;

		case ')':	/* end of a list */
			*sp++ = ')';
			bsize--;
			break;

		case 'o':	/* octet */
			o = va_arg(ap, octet_t *);
			if (o && o->len) {
				n = snprintf(sp, bsize, "%d:", o->len);
				if (n < 0 || (unsigned int) n > bsize)
					return 0;
				bsize -= n;
				sp += n;

				if (o->len > bsize)
					return 0;
				memcpy(sp, o->val, o->len);
				bsize -= o->len;
				sp += o->len;
			}
			break;

		case 'O':	/* octet array */
			oa = va_arg(ap, octet_t **);
			if (oa) {
				for (i = 0; oa[i]; i++) {
					o = oa[i];
					if (o->len == 0)
						continue;
					n = snprintf(sp, bsize, "%d:", o->len);
					if (n < 0 || (unsigned int) n > bsize)
						return 0;
					bsize -= n;
					sp += n;

					if (o->len > bsize)
						return 0;
					memcpy(sp, o->val, o->len);
					bsize -= o->len;
					sp += o->len;
				}
			}
			break;

		case 'a':	/* atom */
			s = va_arg(ap, char *);
			if (s && *s) {
				n = snprintf(sp, bsize, "%d:%s", strlen(s), s);
				if (n < 0 || (unsigned int) n > bsize)
					return 0;
				bsize -= n;
				sp += n;
			}
			break;

		case 'A':	/* sequence of atoms */
			arr = va_arg(ap, char **);
			if (arr) {
				for (i = 0; arr[i]; i++) {
					n = snprintf(sp, bsize, "%d:%s",
						     strlen(arr[i]), arr[i]);
					if (n < 0 || (unsigned int) n > bsize)
						return 0;
					bsize -= n;
					sp += n;
				}
			}
			break;

		case 'l':	/* list */
			s = va_arg(ap, char *);
			if (s && *s) {
				n = snprintf(sp, bsize, "%s", s);
				if (n < 0 || (unsigned int) n > bsize)
					return 0;
				bsize -= n;
				sp += n;
			}
			break;

		case 'L':	/* array of lists */
			arr = va_arg(ap, char **);
			if (arr) {
				for (i = 0; arr[i]; i++) {
					n = snprintf(sp, bsize, "%s", arr[i]);
					if (n < 0 || (unsigned int) n > bsize)
						return 0;
					bsize -= n;
					sp += n;
				}
			}
			break;

		case 'u':	/* octet list */
			o = va_arg(ap, octet_t *);
			if (o) {
				if (o->len > bsize)
					return 0;
				memcpy(sp, o->val, o->len);
				bsize -= o->len;
				sp += o->len;
			}
			break;

		case 'U':	/* array of octet lists */
			oa = va_arg(ap, octet_t **);
			if (oa) {
				for (i = 0; oa[i]; i++) {
					o = oa[i];
					if (o->len == 0)
						continue;
					if (o->len > bsize)
						return 0;
					memcpy(sp, o->val, o->len);
					bsize -= o->len;
					sp += o->len;
				}
			}
			break;

		case 'X':	/* octarr */
			oarr = va_arg(ap, octarr_t *);
			if (oarr) {
				for (i = 0; i < oarr->n; i++) {
					o = oarr->arr[i];
					if ( o == NULL || o->len == 0)
						continue;
					n = snprintf(sp, bsize, "%d:", o->len);
					if (n < 0 || (unsigned int) n > bsize)
						return 0;
					bsize -= n;
					sp += n;

					if (o->len > bsize)
						return 0;
					memcpy(sp, o->val, o->len);
					bsize -= o->len;
					sp += o->len;
				}
			}
			break;

		}

	va_end(ap);

	*size = bsize;

	return sexp;
}
