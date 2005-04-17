#ifndef __LDAPSET_H
#define __LDAPSET_H

#include <spocp.h>
#include <lber.h>
#include <ldap.h>

#define SC_SP(c)   ((c) == 0x20 )
#define SC_CONJ(c) ((c) == '&' || (c) == '|')
#define SC_EXT(c)  ((c) == '/' || (c) == '%' || (c) == '$')
#define SC_PAR(c)  ((c) == '(' || (c) == ')' || (c) == '{' || (c) == '}')
#define SC_SPEC(c) (SC_EXT(c) || SC_CONJ(c) || SC_SP(c) || SC_PAR(c))

#define SC_KEYWORD	0
#define SC_VALUE	1
#define SC_ATTR		2
#define SC_AND		3
#define SC_OR		4
#define SC_VAL		5
#define SC_DN		6
#define SC_BASE		7
#define SC_ONELEV	8
#define SC_SUBTREE 	9
#define SC_UNDEF	10

#define SC_SYNTAXERROR 32

#define SC_TYPE_DN  16
#define SC_TYPE_VAL 17

typedef struct _scnode {
	char	*str;
	int	 size;
	int	striprdn;
	struct _scnode *left;
	struct _scnode *right;
	struct _scnode *up;
} scnode_t;

void		scnode_print( scnode_t *sc, char *in );
scnode_t	*scnode_get(octet_t * op, spocp_result_t * rc);
void		scnode_free(scnode_t * scp);

/*----------------------------------------------------------------------*/

typedef struct _strlistnode {
	char			*str;
	struct _strlistnode	*next;
	struct _strlistnode	*prev;
} lsln_t;

lsln_t  *lsln_add(lsln_t *lp, char *s);
int	lsln_find(lsln_t *lp, char *s);
lsln_t  *lsln_dup(lsln_t *lp);
int  	lsln_values(lsln_t *lp);
char  	**lsln_to_arr(lsln_t *lp);
lsln_t  *lsln_or(lsln_t *a, lsln_t *b);
lsln_t  *lsln_and(lsln_t *a, lsln_t *b);
lsln_t  *lsln_join(lsln_t *a, lsln_t *b);
void	lsln_free(lsln_t * lp);
void	lsln_print(lsln_t * lp);
char	*lsln_get_val(lsln_t * sl);

/*----------------------------------------------------------------------*/

typedef struct _vset {
	int	scope;	/* SC_BASE | SC_ONELEV | SC_SUBTREE */
	int	type;	/* SC_AND | SC_OR */
	int	restype;	/* SC_VAL | SC_DN */
	int	striprdn;
	int	recurs;
	char	*attr;
	lsln_t	*attset;
	lsln_t	*dn;
	lsln_t	*val;
	struct _vset	*left;	/* which vset will I get the lefthand Set from 
				*/
	struct _vset	*right;	/* which vset will I get the righthand Set
   				* from */
	struct _vset	*up;	/* whom I'm delivering to */
} vset_t;

void	vset_free(vset_t * sp);
vset_t	*vset_get(scnode_t *, int, octarr_t *, spocp_result_t *);
void	vset_print(vset_t *, int);
vset_t	*vset_compact(vset_t *, LDAP *, spocp_result_t *);

/*----------------------------------------------------------------------*/

lsln_t	*do_ldap_query(vset_t * vsetp, LDAP * ld, spocp_result_t * ret);


#endif
