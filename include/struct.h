/*!
 * \file struct.h
 * \author Roland hedberg <soalnd@catalogix.se>
 * \brief Basic structs into which the result of the parsing of a S-expression in placed
 */
/***************************************************************************
                          struct.h  -  description
                             -------------------
    begin                : Mon Jun 3 2002
    copyright            : (C) 2002 by SPOCP
    email                : roland@catalogix.se
 ***************************************************************************/

#ifndef __STRUCT_H
#define __STRUCT_H

#include <config.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <regex.h>

#include <spocp.h>

#ifndef FALSE
/*! FALSE, what more can I say */
#define FALSE          0
#endif
#ifndef TRUE
/*! TRUE, what more can I say */
#define TRUE           1
#endif

/*! S-expression ATOM type */
#define SPOC_ATOM      0
/*! S-expression LIST type */
#define SPOC_LIST      1
/*! S-expression SET type */
#define SPOC_SET       2
/*! S-expression PREFIX type */
#define SPOC_PREFIX    3
/*! S-expression SUFFIX type */
#define SPOC_SUFFIX    4
/*! S-expression RANGE type */
#define SPOC_RANGE     5
/*! End of a list in a S-expression */
#define SPOC_ENDOFLIST 6
/*! The end of the whole S-expression */
#define SPOC_ENDOFRULE 7
/*! S-expression ANY type */
#define SPOC_ANY       8
/*
 * --- 
 */
/*! S-expression ARRAY type, only used in and around backends */
#define SPOC_ARRAY     9
/*! NULL element, necessary placeholder */
#define SPOC_NULL     10

/*
 * #define SPOC_OR 9 #define SPOC_REPEAT 10 #define SPOC_EXTREF 11 
 */

/*! Number of basic S-expression element types */
#define NTYPES         9

/*
 * ---------------------------------------------------------------------- 
 */

/*! S-expression range types */
/*! Date, following the format RFC3339 ? */
#define SPOC_DATE      0
/*! Time, format HH:MM:SS */
#define SPOC_TIME      1
/*! bytestring */
#define SPOC_ALPHA     2
/*! IPv4 number */
#define SPOC_IPV4      3
/*! number */
#define SPOC_NUMERIC   4
/*! IPv6 number */
#define SPOC_IPV6      5

/*! number of data types for s-expression ranges */
#define DATATYPES      6

/*! Less then */
#define LT          0x10
/*! Equal, never used alone always together with either LT or GT  */
#define GLE         0x20
/*! Greater then */
#define GT          0x40

/*! The cache result has expired */
#define EXPIRED     1
/*! The result is cached */
#define CACHED      2

/*
 * ---------------------------------------------------------------------- 
 */

/*! \brief Where atoms are places */
typedef struct _atom {
	/*! A hash sum calculated over the array of octets representing the atom */
	unsigned int    hash;
	/*! The octet array itself */
	octet_t         val;
} atom_t;

struct _list;
struct _range;

/*! \brief Where one element of a S-xpression is stored */
typedef struct _element {
	/*! The type of the element */
	int             type;
	/*! The next element in the chain */
	struct _element *next;
	/*! The element (list or set) this element is a subelement of */
	struct _element *memberof;

	/*! The element itself */
	union {
		/*! atom, suffix or prefix  */
		atom_t         *atom;	
		/*! A list of elements  */
		struct _list   *list;
		/*! A set of elements */
		struct _varr   *set;
		/*!  A range */
		struct _range  *range;
		/*/
		int             id;
		*/
	} e;

} element_t;

/****** and structure **********************/

/*! \brief A list of elements */
typedef struct _list {
	/*! The first element in the list */
	element_t      *head;
} list_t;

/*! \brief Representing one side of a range */
typedef struct _boundary {
	/*! The range type */
	int             type;

	/*! Where the boundary value is stored */
	union {
		/*! integer boundary value, number or time */
		long int        num;
		/*! IP version 4 representation */
		struct in_addr  v4;
		/*! IP version 6 representation */
		struct in6_addr v6;
		/*! byte string */
		octet_t         val;
	} v;

} boundary_t;

/*! \brief range definition */
typedef struct _range {
	/*! lower boundary */
	boundary_t      lower;
	/*! upper boundary */
	boundary_t      upper;
} range_t;

/*
 * ------------------------------------------ 
 */

/*
 * === global things === 
 */

extern int      spocp_loglevel;

/*! Some buffert size */
#define SPOCP_MAXLINE       2048

#endif
