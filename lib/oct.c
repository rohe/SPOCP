/*
 * ! \file lib/oct.c \author Roland Hedberg <roland@catalogix.se> 
 */

/***************************************************************************
                          oct.c  -  description
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
#include <proto.h>

/*
 * ====================================================================== 
 */

/*
 * ! \brief Looking for a specific bytevalue, under the assumption that it
 * appears in pairs and that if a 'left' byte has been found a balancing
 * 'right' byte has to be found before the search for another 'right' byte can 
 * be done. \param o The 'string' to be searched \param left The byte
 * representing the leftmost byte of the pair \param right The byte
 * representing the rightmost byte of the pair. \return The length in number
 * of bytes from the start the first 'right' byte that was not part of a pair
 * was found. 
 */

int
oct_find_balancing(octet_t * o, char left, char right)
{
	int             seen = 0, n = o->len;
	char           *q = o->val;

	for (; n; q++, n--) {
		if (*q == left)
			seen++;
		else if (*q == right) {
			if (seen == 0)
				return (o->len - n);
			else
				seen--;
		}
	}

	return -1;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Increases the size of the menory that is connected to this struct. 
 * This routine will always increase, if needed, by doubling the size. \param
 * o The octet string representation \param new The minimum size required
 * \return The result code 
 */

spocp_result_t
oct_resize(octet_t * o, size_t new)
{
	char           *tmp;

	if (o->size == 0)
		return SPOCP_DENIED;	/* not dynamically allocated */

	if (new < o->size)
		return SPOCP_SUCCESS;	/* I don't shrink yet */

	while (o->size < new)
		o->size *= 2;

	tmp = Realloc(o->val, o->size * sizeof(char));

	o->val = tmp;

	return SPOCP_SUCCESS;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Assigns a string to a octet string structure \param oct The octet
 * string struct that should hold the string \param str The string 
 */

void
oct_assign(octet_t * oct, char *str)
{
	oct->val = str;
	oct->len = strlen(str);
	oct->size = 0;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Let one octet string struct point to the same information as
 * another one \param a The target \param b The source 
 */

void
octln(octet_t * a, octet_t * b)
{
	a->len = b->len;
	a->val = b->val;
	a->size = 0;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Duplicates a octet string struct \param oct The octet string that
 * should be copied \return The copy 
 */

octet_t        *
octdup(octet_t * oct)
{
	octet_t        *dup;

	dup = (octet_t *) Malloc(sizeof(octet_t));

	dup->size = dup->len = oct->len;
	if (oct->len != 0) {
		dup->val = (char *) Malloc(oct->len + 1);
		dup->val = memcpy(dup->val, oct->val, oct->len);
		dup->val[dup->len] = 0;
	} else {
		dup->len = 0;
		dup->val = 0;
	}

	return dup;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Copies the information from one octet string struct to another
 * \param cpy The destination \param oct The source \return The result code 
 */

spocp_result_t
octcpy(octet_t * cpy, octet_t * oct)
{
	if (cpy->size == 0) {
		cpy->val = (char *) Calloc(oct->len, sizeof(char));
	} else if (cpy->size < oct->len) {
		if (oct_resize(cpy, oct->len) != SPOCP_SUCCESS)
			return SPOCP_DENIED;
	}

	memcpy(cpy->val, oct->val, oct->len);
	cpy->size = cpy->len = oct->len;

	return 1;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Resets a octet string struct, that is the memory that is used to
 * hold the value bytes are returned and size and len is set to 0. \param oct
 * The octet string struct to be cleared 
 */

void
octclr(octet_t * oct)
{
	if (oct->size) {
		free(oct->val);
	}

	oct->size = oct->len = 0;
	oct->val = 0;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Concatenates a string of bytes of a specific length to the end of
 * the string held by the octet string struct \param oct The octet string
 * struct to which the string should be added \param s The beginning of the
 * string \param l The length of the string \return A Spocp result code 
 */

spocp_result_t
octcat(octet_t * oct, char *s, size_t l)
{
	size_t          n;

	n = oct->len + l;	/* new length */

	if (n > oct->size) {
		if (oct_resize(oct, n) != SPOCP_SUCCESS)
			return SPOCP_DENIED;
		oct->size = n;
	}

	memcpy(oct->val + oct->len, s, l);
	oct->len = n;

	return SPOCP_SUCCESS;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Compares bitwise a string to the string held by a octet string
 * struct \param o The octet string struct \param s The char string which is
 * supposed to be NULL terminated \return 0 is they are equal, otherwise not 0 
 */

int
oct2strcmp(octet_t * o, char *s)
{
	size_t          l, n;
	int             r;

	l = strlen(s);

	if (l == o->len) {
		return memcmp(s, o->val, o->len);
	} else {
		if (l < o->len)
			n = l;
		else
			n = o->len;

		r = memcmp(s, o->val, n);

		if (r == 0) {
			if (n == l)
				return -1;
			else
				return 1;
		} else
			return r;
	}
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Compares bitwise a specific number of bytes bwteen a string and
 * the string held by a octet string struct \param o The octet string struct
 * \param s The char string which has to be nullterminated \param l The number 
 * of bytes that should be comapred \return 0 is they are equal, otherwise not 
 * 0 
 */

int
oct2strncmp(octet_t * o, char *s, size_t l)
{
	size_t          n, ls;

	ls = strlen(s);

	n = MIN(ls, o->len);
	n = MIN(n, l);

	return memcmp(s, o->val, n);
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Compares two octet string structs \param a, b The two octet
 * strings \return 0 if they are equal, not 0 if they are unequal 
 */

int
octcmp(octet_t * a, octet_t * b)
{
	size_t          n;
	int             r;

	n = MIN(a->len, b->len);

	r = memcmp(a->val, b->val, n);

	if (r == 0)
		return (a->len - b->len);
	else
		return r;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Compares at the most n bytes of two octet strings to find out if
 * they are equal or not. \param a, b The two octet strings \param cn The
 * number of bytes that should be check \return 0 if equal, non 0 otherwise 
 */

int
octncmp(octet_t * a, octet_t * b, size_t cn)
{
	size_t          n;

	n = MIN(a->len, b->len);
	n = MIN(n, cn);

	return (memcmp(a->val, b->val, n));
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! Moves the octet string from one octet string struct to another. The
 * source octet string struct is cleared as a side effect. \param a,b
 * destination resp. source octet string struct 
 */
void
octmove(octet_t * a, octet_t * b)
{
	if (a->size)
		free(a->val);

	a->size = b->size;
	a->val = b->val;
	a->len = b->len;

	memset(b, 0, sizeof(octet_t));
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief finds a string of bytes in a octet string. The strin has to be
 * NULL terminated \param o The octet string \param needle the pattern that is 
 * searched for \return The index of the first byte in a sequence of bytes in
 * the octetstring that matched the pattern 
 */
int
octstr(octet_t * o, char *needle)
{
	int             n, m, nl;
	char           *op, *np, *sp;

	nl = strlen(needle);
	if (nl > (int) o->len)
		return -1;

	m = (int) o->len - nl + 1;

	for (n = 0, sp = o->val; n < m; n++, sp++) {
		if (*sp == *needle) {
			for (op = sp + 1, np = needle + 1; *np; op++, np++)
				if (*op != *np)
					break;

			if (*np == '\0')
				return n;
		}
	}

	if (n == m)
		return -1;
	else
		return n;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief The function returns the index of the first occurrence of the
 * character ch in the octet string op. \param op The octet string which is to 
 * be search for the byte. \param ch The byte value. \return The index of the
 * first occurence of a byte in the octet string that was similar to the byte 
 * looked for. 
 */
int
octchr(octet_t * op, char ch)
{
	int             len, i;
	char           *cp;

	len = (int) op->len;

	for (i = 0, cp = op->val; i != len; i++, cp++)
		if (*cp == ch)
			return i;

	return -1;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief The octpbrk() function locates the first occurrence in the octet
 * string s of any of the characters in the octet string acc. \param op The
 * octet string \param acc The set of bytes that is looked for. \return The
 * index of the first coccurence of a byte from the octet string acc 
 */
int
octpbrk(octet_t * op, octet_t * acc)
{
	size_t          i, j;

	for (i = 0; i < op->len; i++) {
		for (j = 0; j < acc->len; j++) {
			if (op->val[(int) i] == acc->val[(int) j])
				return i;
		}
	}

	return -1;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Create a new octet string struct and initialize it with the given
 * values \param size The size of the memory are that are set aside for
 * storing the string \param val A bytestring to bestored. \return The created 
 * and initialized octet string struct 
 */
octet_t        *
oct_new(size_t size, char *val)
{
	octet_t        *new;

	new = (octet_t *) Malloc(sizeof(octet_t));

	if (size) {
		new->val = (char *) Calloc(size, sizeof(char));
		new->size = size;
		new->len = size;
	} else {
		new->size = 0;
		new->val = 0;
	}

	if (val) {
		if (!size) {
			new->len = strlen(val);
			new->val = (char *) Calloc(new->len, sizeof(char));
			new->size = new->len;
		}

		memcpy(new->val, val, new->len);
	} else
		new->len = 0;

	return new;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Frees the memory space held by the octet string struct \param o
 * The octet string struct 
 */
void
oct_free(octet_t * o)
{
	if (o) {
		if (o->val && o->size && o->len)
			free(o->val);
		free(o);
	}
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Frees an array of octet string structs, the array has to be NULL
 * terminated \param oa The array of octet string structs 
 */
void
oct_freearr(octet_t ** oa)
{
	if (oa) {
		for (; *oa != 0; oa++) {
			if ((*oa)->size)
				free((*oa)->val);
			free(*oa);
		}
	}
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief copies the content of a octet string struct into a new character
 * string. If asked for this function escapes non printing characters (
 * characters outside the range 0x20-0x7E and the value 0x25 ) on the fly .
 * \param op The octet string struct \param ec The escape character. If this
 * is not NULL then non-printing characters will be represented by their value 
 * in two digit hexcode preceeded by the escape character. If the escape
 * character is '%' the a byte with the value 0x0d will be written as "%0d" 
 */
char           *
oct2strdup(octet_t * op, char ec)
{
	char            c, *str, *cp, *sp;
	unsigned char   uc;
	size_t          n, i;

	if (op->len == 0)
		return 0;

	if (ec == 0) {
		str = (char *) Malloc((op->len + 1) * sizeof(char));
		strncpy(str, op->val, op->len);
		str[op->len] = '\0';
	}

	for (n = 1, i = 0, cp = op->val; i < op->len; i++, cp++) {
		if (*cp >= 0x20 && *cp < 0x7E && *cp != 0x25)
			n++;
		else
			n += 4;
	}

	str = (char *) Malloc(n * sizeof(char));

	for (sp = str, i = 0, cp = op->val; i < op->len; i++) {
		if (!ec)
			*sp++ = *cp++;
		else if (*cp >= 0x20 && *cp < 0x7E && *cp != 0x25)
			*sp++ = *cp++;
		else {
			*sp++ = ec;
			uc = (unsigned char) *cp++;
			c = (uc & (char) 0xF0) >> 4;
			if (c > 9)
				*sp++ = c + (char) 0x57;
			else
				*sp++ = c + (char) 0x30;

			c = (uc & (char) 0x0F);
			if (c > 9)
				*sp++ = c + (char) 0x57;
			else
				*sp++ = c + (char) 0x30;
		}
	}

	*sp = '\0';

	return str;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! Copies no more than len characters from the octet string op into a string 
 * str. If an escape character and the escape flag do_escape is not NULL then
 * escaping will be done on the fly. \param op The source octet string \param
 * str The start of the memory area to which the string is copied \param len
 * The size of the memory are to which the string is to be copied \param ec
 * The escape character, is this is not 0 then escaping is done \return The
 * number of characters wtitten to str or -1 if the memory area was to small
 * to hold the possibly escaped copy. 
 */
int
oct2strcpy(octet_t * op, char *str, size_t len, char ec)
{
	char           *end = str + len;

	if (op->len == (size_t) 0 || (op->len + 1 > len))
		return -1;

	if (ec == 0) {
		if (op->len > len - 1)
			return -1;
		memcpy(str, op->val, op->len);
		str[op->len] = 0;
	} else {
		char           *sp, *cp;
		unsigned char   uc, c;
		size_t          i;

		for (sp = str, i = 0, cp = op->val; i < op->len && sp < end;
		     i++) {
			if (!ec)
				*sp++ = *cp++;
			else if (*cp >= 0x20 && *cp < 0x7E && *cp != 0x25)
				*sp++ = *cp++;
			else {
				if (sp + 3 >= end)
					return -1;
				*sp++ = ec;
				uc = (unsigned char) *cp++;
				c = (uc & (char) 0xF0) >> 4;
				if (c > 9)
					*sp++ = c + (char) 0x57;
				else
					*sp++ = c + (char) 0x30;

				c = (uc & (char) 0x0F);
				if (c > 9)
					*sp++ = c + (char) 0x57;
				else
					*sp++ = c + (char) 0x30;
			}
		}

		if (sp >= end)
			return -1;
		*sp = '\0';
	}
	return op->len;
}

/*
 * ---------------------------------------------------------------------- 
 */

/*
 * ! \brief Replaces part of a octet with another octet string \param oct The 
 * original octet string \param insert The octetstring to insert \param where
 * Where the insert should be placed \param replen The number of bytes in the
 * original that should be replaced \param noresize A flag that is set
 * prohibits the original from being resized \return A spocp result code 
 */
static          spocp_result_t
oct_replace(octet_t * oct, octet_t * insert, int where, int replen,
	    int noresize)
{
	int             n, r;
	char           *sp;

	if (where > (int) oct->len) {	/* would mean undefined bytes in the
					 * middle, no good */
		return -1;
	}

	/*
	 * final length 
	 */
	n = oct->len + insert->len - replen;

	if (n - (int) oct->size > 0) {
		if (noresize)
			return SPOCP_DENIED;
		if (oct_resize(oct, n) != SPOCP_SUCCESS)
			return SPOCP_DENIED;
		oct->size = (size_t) n;
	}

	/*
	 * where the insert should be placed 
	 */
	sp = oct->val + where;

	/*
	 * amount of bytes to move 
	 */
	r = oct->len - where - replen;

	/*
	 * move bytes, to create space for the insert 
	 */
	memmove(sp + insert->len, sp + replen, r);
	memcpy(sp, insert->val, insert->len);

	oct->len = n;

	return SPOCP_SUCCESS;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Replaces occurences of the form "$$(key)" with the value connected 
 * to the key 'key' in a keyval struct. \param src The original octet string
 * \param kvp A linked list of keyval_t structs \param noresize A flag that
 * denotes whether this function can resize the original octet string if
 * needed. \return A Spocp result code 
 */
spocp_result_t
oct_expand(octet_t * src, keyval_t * kvp, int noresize)
{
	int             n;
	keyval_t       *vp;
	octet_t         oct;

	if (kvp == 0)
		return SPOCP_SUCCESS;

	oct.size = 0;

	/*
	 * starts all over from the beginning of the string which means that
	 * it can handle recursively expansions 
	 */
	while ((n = octstr(src, "$$(")) >= 0) {

		oct.val = src->val + n + 3;
		oct.len = src->len - (n + 3);

		oct.len = octchr(&oct, ')');
		if (oct.len < 0)
			return 1;	/* is this an error or what ?? */

		for (vp = kvp; vp; vp = vp->next) {
			if (octcmp(vp->key, &oct) == 0) {
				oct.val -= 3;
				oct.len += 4;
				if (oct_replace
				    (src, vp->val, n, oct.len,
				     noresize) != SPOCP_SUCCESS)
					return SPOCP_DENIED;
				break;
			}
		}

		if (vp == 0) {
			LOG(SPOCP_WARNING) traceLog("Unkown global: \"%s\"",
						    oct.val);
			return SPOCP_DENIED;
		}
	}

	return SPOCP_SUCCESS;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Deescapes a octetstring. The escape is expected to be done as '\'
 * hex hex \param op The original octetstring. \return The number of bytes in
 * the octetstring after deescaping is done. 
 */

int
oct_de_escape(octet_t * op)
{
	register char  *tp, *fp, *ep;
	register char   c = 0;
	int             len;

	if (op == 0)
		return 0;

	len = op->len;
	ep = op->val + len;

	for (fp = tp = op->val; fp != ep; tp++, fp++) {
		if (*fp == '\\') {
			fp++;
			if (fp == ep)
				return -1;

			if (*fp == '\\') {
				*tp = *fp;
				len--;
			} else {
				if (*fp >= 'a' && *fp <= 'f')
					c = (*fp - 0x57) << 4;
				else if (*fp >= 'A' && *fp <= 'F')
					c = (*fp - 0x37) << 4;
				else if (*fp >= '0' && *fp <= '9')
					c = (*fp - 0x30) << 4;

				fp++;
				if (fp == ep)
					return -1;

				if (*fp >= 'a' && *fp <= 'f')
					c += (*fp - 0x57);
				else if (*fp >= 'A' && *fp <= 'F')
					c += (*fp - 0x37);
				else if (*fp >= '0' && *fp <= '9')
					c += (*fp - 0x30);

				*tp = c;

				len -= 2;
			}
		} else
			*tp = *fp;
	}

	op->len = len;

	return len;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * ! \brief Convert a octetstring representation of a number to a integer.
 * INT_MAX 2147483647 \param o The original octet string \return A integer
 * bigger or equal to 0 if the conversion was OK, otherwise -1 
 */
int
octtoi(octet_t * o)
{
	int             r = 0, d = 1;
	char           *cp;

	if (o->len > 10)
		return -1;
	if (o->len == 10 && oct2strcmp(o, "2147483647") > 0)
		return -1;

	for (cp = o->val + o->len - 1; cp >= o->val; cp--) {
		if (*cp < '0' || *cp > '9')
			return -1;
		r += (*cp - '0') * d;
		d *= 10;
	}

	return r;
}
