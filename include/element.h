/*
 *  element.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __ELEMENT_H
#define __ELEMENT_H

#include <config.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <regex.h>

#include <spocp.h>
#include <atom.h>
#include <boundary.h>
#include <varr.h>

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

#define RTYPE		0x0F

/*! number of data types for s-expression ranges */
#define DATATYPES      6

/*! Less then */
#define LT	0x10
/*! Equal, never used alone always together with either LT or GT  */
#define EQ	0x20
/*! Greater then */
#define GT	0x40

#define BTYPE 	0xF0

/*! The cache result has expired */
#define EXPIRED     0x10000000
/*! The result is cached */
#define CACHED      0x20000000

/*
 * ---------------------------------------------------------------------- 
 */

struct _list;
struct _range;

/*! \brief Where one element of a S-expression is stored */
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

/*
 * ------------------------------------------ 
 */

/*
 * === global things === 
 */

extern int      spocp_loglevel;

/*! Some buffert size */
#define SPOCP_MAXLINE       2048

element_t       *find_list(element_t * ep, char *tag);
varr_t          *get_simple_lists(element_t * ep, varr_t * ap);
char            *element2str( element_t *ep);
element_t       *element_reduce( element_t *ep ) ;
/*
 */

int             element_print( octet_t *oct, element_t *ep);
void            element_struct( octet_t *oct, element_t *ep);
int             element_size(element_t * e);
int             element_type(element_t * e);
element_t      *element_find_list(element_t * e, octet_t * tag);
element_t      *element_first(element_t * e);
element_t      *element_last(element_t * e);
element_t      *element_nth(element_t * e, int n);
element_t      *element_next(element_t * e, element_t * ep);
void            element_reverse(element_t * e);
element_t      *element_copy(element_t * e);
element_t      *element_parent(element_t * e);
void           *element_data(element_t * e);
element_t      *element_eval(octet_t * spec, element_t * e, 
                             spocp_result_t *rc);
int             element_cmp(element_t * a, element_t * b, spocp_result_t *rc);
void            element_free(element_t * e);

#define element_next( elem )  ((elem) ? (((element_t *)(elem))->next) : NULL ) ;

element_t       *element_list_add(element_t * e, element_t * a);
octet_t         *atoms_join(element_t * e, char *sep);
octet_t         *element_atom_sub(octet_t * val, element_t * ep);

spocp_result_t  get_element(octet_t * oct, element_t ** epp);
element_t	    *element_new(void);

void            list_free(list_t * lp);
list_t          *list_new(void);

spocp_result_t  do_prefix(octet_t *oct, element_t *ep);
spocp_result_t  do_suffix(octet_t *oct, element_t *ep);
spocp_result_t  do_set(octet_t *oct, element_t *ep);
spocp_result_t  do_range(octet_t *oct, element_t *ep);
spocp_result_t  do_list(octet_t * to, octet_t * lo, element_t * ep, 
                        char *base);

void            set_memberof(varr_t * va, element_t * parent);

element_t       *element_new_list( element_t *head);
element_t       *element_new_set( varr_t *varr );
element_t       *element_set_add(element_t * le, element_t * e);

#endif
