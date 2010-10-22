/*
 *  bcond.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/4/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __BCOND_H
#define __BCOND_H

#include <octet.h>
#include <varr.h>
#include <plugin.h>
/* #include <index.h> */

/*
 * -------------------------------------------------- 
 */

#define AND	1
#define OR	2
#define NOT	3
#define REF	4
#define SPEC 5

struct _bconddef;
struct _checked;

typedef struct _bcondspec {
	char		*name;
	plugin_t	*plugin;
	octarr_t	*args;
	octet_t		*spec;
} bcspec_t;

typedef struct _bcondexpr {
	int			type;
	struct _bconddef	*parent;
    
	union {
		/* AND or OR array of bcondexp_t structs */
		varr_t              *arr;	
		struct _bcondexpr	*single;	/* NOT */
		bcspec_t			*spec;		/* SPEC */
		struct _bconddef	*ref;		/* REF */
	} val;
    
} bcexp_t;

typedef struct _bconddef {
	char			*name;
	bcexp_t			*exp;
	varr_t          *users;
	varr_t          *rules;
	struct _bconddef	*next;
	struct _bconddef	*prev;
} bcdef_t;

/*! Local struct used to store parsed information about bcond s-expressions */
typedef struct _stree {
	/*! Is this a list/set or not */
	int             list;
	/*! The atom */
	octet_t         val;
	/*! Next element */
	struct _stree  *next;
	/*! Previous element */
	struct _stree  *part;
} bcstree_t;

bcexp_t     *transv_stree(plugin_t * plt, bcstree_t * st, bcdef_t * list, 
                          bcdef_t * parent);
void        bcexp_free(bcexp_t * bce);
bcexp_t     *varr_bcexp_pop(varr_t * va);
void        bcexp_delink(bcexp_t * bce);
bcstree_t   *parse_bcexp(octet_t * sexp);

bcspec_t    *bcspec_new(plugin_t * plt, octet_t * spec);
void        bcspec_free(bcspec_t * bcs);

bcdef_t     *bcdef_find(bcdef_t * bcd, octet_t * pattern);
bcdef_t *   bcdef_new(void);
void        bcdef_free(bcdef_t * bcd);

void        bcstree_free(bcstree_t * stp);
char        *make_hash(char *str, octet_t * input);


#endif
