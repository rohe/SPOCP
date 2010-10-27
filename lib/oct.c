/*!
 * \file lib/oct.c
 * \author Roland Hedberg <roland@catalogix.se> 
 * \brief A set of functions working on octet_t structs
 */

/***************************************************************************
                          oct.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2005 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in tdup level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>

#include <octet.h>
#include <log.h>
#include <proto.h>
#include <wrappers.h>

/* #define AVLUS 1 */

/*
 * ====================================================================== 
 */

void print_oct_info(char *label, octet_t *op)
{
     printf("%s: len=%d, size=%d, val=%p, base=%p\n", label,
            (int) op->len, (int) op->size, op->val, op->base);
}

void position_val_pointer( octet_t *active, octet_t *base)
{
    int     offset;
    
    offset = base->val - base->base;
    active->val = active->base + offset;
    active->len = base->len;
}

char *optimized_allocation(octet_t *base, int *size)
{
    int     offset;
    
    if (base->size || base->len) {
        offset = base->val - base->base;
        *size = base->len + offset;
        return (char *) Calloc(*size, sizeof(char));
    }
    else {
        *size = 0;
        return 0;
    }
}        

/*! \brief Looking for a specific bytevalue, under the assumption that it
 * appears in pairs and that if a 'left' byte has been found a balancing
 * 'right' byte has to be found before the search for another 'right' byte can 
 * be done.
 * \param o The 'string' to be searched
 * \param left The byte representing the leftmost byte of the pair
 * \param right The byte representing the rightmost byte of the pair.
 * \return The length in number
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
/*! \brief Increases the size of the menory that is connected to this struct. 
 * This routine will always increase, if needed, by doubling the size.
 * \param o The octet string representation
 * \param new The minimum size required
 * \return The result code 
 */

spocp_result_t
oct_resize(octet_t * o, size_t new)
{
    char    *tmp;
    int     offset;
        
    if (o->size == 0)
        return SPOCP_DENIED;    /* not dynamically allocated */

    if (new < o->size)
        return SPOCP_SUCCESS;   /* I don't shrink yet */

    while (o->size < new)
        o->size *= 2;

    tmp = Realloc(o->base, o->size * sizeof(char));

    offset = o->val - o->base;    
    o->base = tmp;
    o->val = o->base + offset;

    return SPOCP_SUCCESS;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Assigns a string to a octet string structure
 * \param oct The octet string struct that should hold the string
 * \param str The string 
 */

void
oct_assign(octet_t * oct, char *str)
{
    oct->base = oct->val = str;
    oct->len = strlen(str);
    /* That size is set to zero will prevent oct_free from freeing the string */
    oct->size = 0;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Let one octet string struct point to the same information as
 * another one
 * \param a The target
 * \param b The source 
 */

int
octln(octet_t * a, octet_t * b)
{
    if( a == 0 || b == 0 )
         return -1;

    a->len = b->len;
    a->val = b->val;
    a->base = b->base;
    a->size = 0;
    return 1;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Create a new octet string struct and let it point to the same
 * information as the source 
 * \param src The source 
 * \return A pointer to a new octet struct, that points to the same data as
 * src does. Since the octet size is set to NULL, deleting this struct will
 * not influence the information stored in 'src'.
 */

octet_t *
octcln(octet_t *src)
{
    octet_t *res;

    if( src == 0 )
        return 0;

    res = ( octet_t *) Malloc( sizeof( octet_t ));

    octln(res, src);

    return res;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Duplicates a octet string struct
 * \param oct The octet string that should be copied 
 * \return The copy 
 */

octet_t        *
octdup(octet_t * oct)
{
    octet_t     *dup;
    int         size;

    if( oct == NULL ) 
        return NULL;

    dup = (octet_t *) Malloc(sizeof(octet_t));

    if (oct->len != 0 || oct->size != 0) {
        dup->base = optimized_allocation(oct, &size);
        dup->size = size;
        memcpy(dup->base, oct->base, size);
        position_val_pointer(dup, oct);
    } else {
        dup->size = dup->len = 0;
        dup->base = dup->val = 0;
    }

    return dup;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Copies the information from one octet string struct to another
 * \param cpy The destination
 * \param oct The source
 * \return The result code 
 */

spocp_result_t
octcpy(octet_t * cpy, octet_t * oct)
{
    int size;
    
    if (cpy == NULL || oct == NULL) 
        return SPOCP_DENIED;
    
    if (cpy->size == 0) {
        cpy->base = optimized_allocation(oct, &size);
        cpy->size = size;
        memcpy(cpy->base, oct->base, size);
        position_val_pointer(cpy, oct);
    } else {
        if (cpy->size < oct->size) {
            if (oct_resize(cpy, oct->size) != SPOCP_SUCCESS)
                return SPOCP_DENIED;
        }

        memcpy(cpy->base, oct->base, oct->size);
        position_val_pointer(cpy, oct);        
    }

    return SPOCP_SUCCESS;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Resets a octet string struct, that is the memory that is used to
 * hold the value bytes are returned and size and len is set to 0.
 * \param oct * The octet string struct to be cleared 
 */

void
octclr(octet_t * oct)
{
    if (oct == NULL)
        return ;
        
    if (oct->size) {
        Free(oct->base);
    }

    oct->size = oct->len = 0;
    oct->base = oct->val = 0;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Concatenates a string of bytes of a specific length to the end of
 * the string held by the octet string struct
 * \param oct The octet string struct to which the string should be added
 * \param s The beginning of the string
 * \param l The length of the string
 * \return A Spocp result code 
 */

spocp_result_t
octcat(octet_t * oct, char *s, size_t l)
{
    size_t  n;
    int     offset;

    /* empty instance */
    if (oct->len == 0 && oct->size == 0) {
        n = l;
        oct->base = oct->val = (char *) Calloc(n, sizeof(char));
        oct->size = n;
    } else {
        offset = oct->val - oct->base;
        n = oct->len + l + offset;  /* new length */

        if (n > oct->size) {
            if (oct_resize(oct, n) != SPOCP_SUCCESS)
                return SPOCP_DENIED;
            oct->size = n;
        }
    }
    /* Add to end of string */
    memcpy(oct->val + oct->len, s, l);
    oct->len = oct->len + l;

    return SPOCP_SUCCESS;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Compares bitwise a string to the string held by a octet string
 * struct
 * \param o The octet string struct
 * \param s The char string which is supposed to be NULL terminated
 * \return 0 is they are equal, otherwise not 0 
 */

int
oct2strcmp(octet_t * o, char *s)
{
    size_t          l, n;
    int             r;

    if( o == 0 && (s == 0 || *s == '\0')) return 0;
    if( o->len == 0 && (s == 0 || *s == '\0')) return 0;
    if( o == 0 || s == 0 || *s == '\0' ) return -1;
    
    l = strlen(s);

    if (l == o->len) {
        return memcmp(o->val, s, o->len);
    } else {
        if (l < o->len)
            n = l;
        else
            n = o->len;

        r = memcmp(o->val, s, n);

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
/*! \brief Compares bitwise a specific number of bytes bewteen a string and
 * the string held by a octet string struct. oct2strncmp as strncmp does not
 * compare more than l characters, possibly less.
 * \param o The octet string struct
 * \param s The char string which has to be nullterminated
 * \param l The number of bytes that should be compared
 * \return 0 is they are equal, otherwise not 
 * 0 
 */

int
oct2strncmp(octet_t * o, char *s, size_t l)
{
    size_t  n, ls;

    if (s == 0 || *s == '\0') {
        if (o == 0 || o->len == 0) 
            return 0;
        else 
            return 1;
    }
    if (o == 0 || o->len == 0) {
        return -1;
    }
    
    ls = strlen(s);

    n = MIN(ls, o->len);
    n = MIN(n, l);

    return memcmp(s, o->val, n);
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Compares two octet string structs
 * \param a, b The two octet strings
 * \return 0 if they are equal, not 0 if they are unequal 
 */

int
octcmp(octet_t * a, octet_t * b)
{
    size_t          n;
    int             r;

    if (a == 0 && b == 0 ) 
        return 0;
    if (a == 0) 
        return -1;
    if (b == 0 ) 
        return 1;

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
/*!
 * \brief Compares two octet string structs in reverse order
 * \param a, b The two octet strings
 * \return 0 if they are equal, not 0 if they are unequal 
 */

int
octrcmp(octet_t *a, octet_t *b)
{
    size_t  n;
    char    *s, *c;

    if (a == 0 && b == 0 ) 
        return 0;
    if (a == 0) 
        return -1;
    if (b == 0 ) 
        return 1;
    
    n = MIN(a->len, b->len);
    for (s = a->val, c = b->val ; n ; n--, s++, c++) {
        if (*s != *c)
            break;
    }

    
    if (n) {
        return (*s - *c);
    } else {
        return (a->len - b->len);
    }
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Compares at the most n bytes of two octet strings to find out if
 * they are equal or not.
 * \param a, b The two octet strings
 * \param cn The number of bytes that should be check
 * \return 0 if equal, non 0 otherwise 
 */

int
octncmp(octet_t * a, octet_t * b, size_t cn)
{
    size_t          n;

    if (a == 0 && b == 0 ) 
        return 0;
    if (a == 0) 
        return -1;
    if (b == 0 ) 
        return 1;
    
    n = MIN(a->len, b->len);
    n = MIN(n, cn);

    return (memcmp(a->val, b->val, n));
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Moves the octet string from one octet string struct to another. The
 * source octet string struct is cleared as a side effect.
 * \param a,b destination resp. source octet string struct 
 */
void
octmove(octet_t * a, octet_t * b)
{
    if (a == 0 || b == 0)
        return ;

    if (a->size)
        Free(a->base);

    a->size = b->size;
    a->base = b->base;
    a->val = b->val;
    a->len = b->len;

    memset(b, 0, sizeof(octet_t));
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief finds a string of bytes in a octet string. The string has to be
 * NULL terminated
 * \param o The octet string
 * \param needle the pattern that is searched for
 * \return The index of the first byte in a sequence of bytes in
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
/*! \brief The function returns the index of the first occurrence of the
 * character ch in the octet string op.
 * \param op The octet string which is to be search for the byte.
 * \param ch The byte value.
 * \return The index of the first occurence of a byte in the octet string
 * that was similar to the byte looked for or -1 if non existent
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
/*! \brief The octpbrk() function locates the first occurrence in the octet
 * string s of any of the characters in the octet string acc.
 * \param op The octet string
 * \param acc The set of bytes that is looked for.
 * \return The
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
/*! \brief Create a new octet string struct and initialize it with the given
 * values
 * \param size The size of the memory that are set aside for
 * storing the string
 * \param val A bytestring to be stored.
 * \return The created and initialized octet string struct 
 */
octet_t        *
oct_new(size_t size, char *val)
{
    octet_t *new;
    size_t  l;

    new = (octet_t *) Malloc(sizeof(octet_t));

    new->len = 0;

    if (size) {
        new->base = new->val = (char *) Calloc(size, sizeof(char));
        new->size = size;
    } else {
        new->size = 0;
        new->base = new->val = 0;
    }

    if (val) {
        l = strlen(val);
        if (size == 0) {
            new->val = new->base = (char *) Calloc(l, sizeof(char));
            new->size = new->len = l;
        }
        else if (l > new->size) {
            new->len = new->size;
        }
        else {
            new->len = l;
        }
        
        memcpy(new->base, val, new->len);
    } 

    return new;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Frees the memory space held by a octet string struct
 * \param o * The octet string struct 
 */
void
oct_free(octet_t * o)
{
    if (o) {
#ifdef AVLUS
        traceLog(LOG_INFO,"oct_free %p", o);
        oct_print(LOG_INFO,"oct_free", o);
        traceLog(LOG_INFO,"oct_free size:%d len:%d",o->size, o->len);
#endif
        if (o->base && o->size)
            Free(o->base);
        Free(o);
    }
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Frees an array of octet string structs, the array has to be NULL
 * terminated
 * \param oa The array of octet string structs 
 */
void
oct_freearr(octet_t ** oa)
{
    if (oa) {
        for (; *oa != 0; oa++) {
            if ((*oa)->size)
                Free((*oa)->val);
            Free(*oa);
        }
    }
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief copies the content of a octet string struct into a new character
 * string. If asked for this function escapes non printing characters (
 * characters outside the range 0x20-0x7E and the value 0x25 ) on the fly .
 * \param op The octet string struct
 * \param ec The escape character. If this
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

    if (op == 0 || op->len == 0)
        return 0;

    if (ec == 0) {
        str = (char *) Malloc((op->len + 1) * sizeof(char));
        strncpy(str, op->val, op->len);
        str[op->len] = '\0';
    }
    else {
        for (n = 1, i = 0, cp = op->val; i < op->len; i++, cp++) {
            if (*cp >= 0x20 && *cp < 0x7E && *cp != 0x25)
                n++;
            else
                n += 4;
        }

        str = (char *) Malloc(n * sizeof(char));
    }

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
/*! Copies no more than len characters from the octet string op into a string 
 * str. If an escape character and the escape flag do_escape is not NULL then
 * escaping will be done on the fly.
 * \param op The source octet string
 * \param str The start of the memory area to which the string is copied
 * \param len The size of the memory to which the string is to be copied 
 * \param ec The escape character, if this is not 0 then escaping is done
 * \return The
 * number of characters written to str or -1 if the memory area was to small
 * to hold the possibly escaped copy. 
 */
int
oct2strcpy(octet_t * op, char *str, size_t len, char ec)
{
    char    *end = str + len;
    int     l;

    if (op->len == (size_t) 0 || (op->len + 1 > len))
        return -1;

    if (ec == 0) {
        if (op->len > len - 1)
            return -1;
        memcpy(str, op->val, op->len);
        str[op->len] = 0;
        l = op->len;
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
        l = sp - str;
    }
    return l;
}

/*
 * ---------------------------------------------------------------------- 
 */
/*! \brief Deescapes a octetstring. The escape is expected to be done as '\'
 * hex hex
 * \param op The original octetstring.
 * \return The number of bytes in the octetstring after deescaping is done. 
 */

int
oct_de_escape(octet_t * op, char ec)
{
    register char  *tp, *fp, *ep;
    register char   c = 0;
    int             len;

    if (op == 0)
        return 0;

    len = op->len;
    ep = op->val + len;

    for (fp = tp = op->val; fp != ep; tp++, fp++) {
        if (*fp == ec) {
            fp++;
            if (fp == ep)
                return -1;

            if (*fp == ec) {
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
/*! \brief Convert a octetstring representation of a number into an 
 * unsigned integer. INT_MAX = 2147483647
 * \param o The original octet string
 * \return A integer
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

/*
 * ---------------------------------------------------------------------- 
 */
/*!
 * \brief Creates an octet struct around a given string
 * \param str The string
 * \param dynamic A flag that tells whether the string should be freed
 *   together with the rest of the struct
 * \return Newly allocated octet struct
 */
octet_t *str2oct( char *str, int dynamic )
{
    octet_t *op;

    op = (octet_t *) Calloc ( 1, sizeof( octet_t ));

    if (str == 0)
        return op;
        
    op->len = strlen( str );
    op->base = op->val = str;
    if( dynamic ) op->size = op->len;

    return op;
}


/*!
 * \brief Assigns a string to a octet struct
 * \param oct The octet struct
 * \param s The string
 * \param len the length of the string that matters in this context
 */
void 
octset( octet_t *oct, char *s, int len)
{
    /* Since size is set to the length of the string if this struct 
     is freed the string is also freed.
     If this is not what you wanted use oct_assign() instead */
    oct->base = oct->val = s;
    oct->len = oct->size = len;
}

char *str_esc( char *str, int len)
{
    octet_t oct;

    oct.base = oct.val = str;
    if (len == 0)
        oct.len = strlen(str);
    else
        oct.len = len;
    return oct2strdup( &oct, '\\');
}

void oct_print( int pri, char *tag, octet_t *o )
{
    char *tmp;

    tmp = oct2strdup( o, '\\');
    traceLog(pri,"%s: '%s'", tag, tmp);
    Free(tmp);
}

/*
 * ---------------------------------------------------------------------- 
 */

int oct_step(octet_t *oct, int len)
{
    if (oct->len < len) 
        return 0;
    
    oct->len -= len;
    oct->val += len;
    return 1;
}
