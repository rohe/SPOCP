
/***************************************************************************
				db0.h	-  description
					-------------------
	begin		 : Mon Jul 15 2002
	copyright		 : (C) 2002 by Umeå University, Sweden
	email		 : roland@catalogix.se

	COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
	of this package for details.

 ***************************************************************************/

#ifndef __DB0_H__
#define __DB0_H__

#include <config.h>

#include <struct.h>
#include <plugin.h>
#include <basefn.h>
#include <rbtree.h>
#include <dback.h>

#define SHA1HASHLEN 40

struct _branch;
struct _ruleinstance;
struct _varr;

/*
 * ------------------------------------------ 
 */

#define FORWARD	1
#define BACKWARD	2

/*
 * ------------------------------------------ 
 */

typedef struct _subelem {
	int		direction;
	int		list;	/* yes = 1, no = 0 */
	element_t	*ep;
	struct _subelem *next;
} subelem_t;

typedef struct _parr {
	int		n;
	int		size;
	int		*refc;
	void		**vect;
	cmpfn		*cf;	/* compare function */
	ffunc		*ff;	/* free function */
	dfunc		*df;	/* duplicate function */
	pfunc		*pf;	/* print function */
} parr_t;

typedef struct _node {
	void		*payload;
	struct _node	*next;
	struct _node	*prev;
} node_t;

typedef struct _ll {
	node_t	 *head;
	node_t	 *tail;
	cmpfn		*cf;
	ffunc		*ff;
	dfunc		*df;
	pfunc		*pf;
	int		n;
} ll_t;

typedef struct _strarr {
	char		**argv;
	int		argc;
	int		size;
} strarr_t;

/*********** where all the barnches starts *************/

typedef struct _junc {
	struct _branch *item[NTYPES];
} junc_t;

#define ARRFIND(a,c) ( (a) ? (a)->item[(c)] : NULL )

/***********************************************/

typedef struct _index {
	int		size;
	int		n;
	struct _ruleinstance **arr;
} spocp_index_t;

/*********** for prefixes ****************/

typedef struct _ssn {
	unsigned char	ch;
	struct _ssn	*left;
	struct _ssn	*down;
	struct _ssn	*right;
	junc_t	 *next;
	unsigned int	refc;	/* reference counter */
} ssn_t;

/****** for ranges *********************/

typedef struct _slnode {
	boundary_t	*item;
	unsigned int	refc;
	struct _slnode **next;
	int		sz;
	struct _varr	*junc;
} slnode_t;

typedef struct _slist {
	slnode_t	*head;
	slnode_t	*tail;
	int		n;
	int		lgn;
	int		lgnmax;
	int		type;
} slist_t;

/****** for atoms **********************/

typedef struct _bucket {
	octet_t	 val;
	unsigned long int hash;
	unsigned int	refc;
	junc_t	 *next;
} buck_t;

typedef struct _phash {
	unsigned char	pnr;
	unsigned char	density;
	unsigned int	size;
	unsigned int	n;
	unsigned int	limit;
	buck_t	**arr;
} phash_t;

/******************************************/

typedef struct _dset {
	struct _varr	*va;
	struct _dset	*next;
} dset_t;

/****** branches **********************/

typedef struct _branch {
	int		type;
	int		count;
	junc_t	 *parent;

	union {
		dset_t	 *set;
		phash_t	*atom;
		junc_t	 *list;
		junc_t	 *next;
		ssn_t		*prefix;
		ssn_t		*suffix;
		slist_t	*range[DATATYPES];
		spocp_index_t	*id;
	} val;

} branch_t;

/*
 * -------------------------------------------------- 
 */

typedef struct _com_param {
	element_t	*head;
	octarr_t	**blob;
	spocp_result_t	rc;
} comparam_t;

/*
 * -------------------------------------------------- 
 */

#define AND	1
#define OR	2
#define NOT	3
#define REF	4
#define SPEC 5

struct _bconddef;

typedef struct _bcondspec {
	char		*name;
	plugin_t	*plugin;
	octarr_t	*args;
	octet_t		*spec;
} bcspec_t;

struct _bconddef;

typedef struct _bcondexpr {
	int			type;
	struct _bconddef	*parent;

	union {
					/* AND or OR array of
					   bcondexp_t structs */
		struct _varr		*arr;	
		struct _bcondexpr	*single;	/* NOT */
		bcspec_t		*spec;		/* SPEC */
		struct _bconddef	*ref;		/* REF */
	} val;

} bcexp_t;

typedef struct _bconddef {
	char			*name;
	bcexp_t			*exp;
	struct _varr		*users;
	struct _varr		*rules;
	struct _bconddef	*next;
	struct _bconddef	*prev;
} bcdef_t;

/*********** rule info ****************/

typedef struct _ruleinstance {
	char		uid[SHA1HASHLEN + 1];	/* place for sha1 hash */
	octet_t		*rule;
	octet_t		*blob;
	octet_t		*bcexp;
	element_t	*ep;	/* only used for bcond checks */
	ll_t		*alias;
	bcdef_t		*bcond;
} ruleinst_t;

typedef struct _ruleinfo {
	rbt_t	*rules;	/* red/black tree */
} ruleinfo_t;

/*********** The database *************/

typedef struct _db {
	junc_t		*jp;
	ruleinfo_t	*ri;
	/* junc_t		*acit; */
	plugin_t	*plugins;
	bcdef_t		*bcdef;
	dback_t		*dback;
} db_t;

#endif
