
/***************************************************************************
			ldapset.c  -  description
			   -------------------
 ***************************************************************************/

#include <config.h>

/*
 * Format of the whole string ldapset = ldap_server *10( ";" DN) ";" dnvset
 * 
 * 
 * Format of the query part
 * 
 * dnvset	=	base |
 *			'(' dnvset ')' |
 *			'{' dnvset ext attribute '}' |
 *			dnvset SP conj SP dnvset |
 *			dnvset ext dnattribute |
 *			'<' dn '>'
 *
 * valvset	=	'[' string ']' |
 *			'(' valvset ')' |
 *			dnvset ext attribute |
 *			valvset SP conj SP valvset
 * 
 * base		=	'\' d
 * 
 * conj		=	"&" | "|"
 * ext		=	 "/" | "%" | "$"	 
 *	; base, onelevel resp. subtree search
 * 
 * a		=	%x41-5A / %x61-7A ; lower and upper case ASCII
 * d		=	%x30-39
 * k		=	a / d / "-" / ";"
 * anhstring	=	1*k
 * SP		=	0x20
 * 
 * 
 * attribute-list = attribute *[ "," attribute ] attribute = a [ anhstring]
 *	; as defined by [RFC2252]
 * 
 * dnattribute-list = dnattribute *[ "," dnattribute ]
 * 	; dnattribute = any attribute name which have attributetype
 *	distinguishedName (1.3.6.1.4.1.1466.115.121.1.12) like member,
 *	owner, roleOccupant, seeAlso, modifiersName, creatorsName,...
 * 
 * Ex, <cn=Group>/member & user {user$mail &
 * [sven.svensson@minorg.se]}/title,personalTitle & [mib] <cn=Group>/member &
 * {user$mail & [tw@minorg.se]} 
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lber.h>
#include <ldap.h>

#include <spocp.h>
#include <be.h>
#include <wrappers.h>
#include <plugin.h>
#include <rvapi.h>
#include <func.h>


typedef struct _scnode {
	char	 *str;
	int	   size;
	struct _scnode *left;
	struct _scnode *right;
	struct _scnode *up;
} scnode_t;


#define SC_SP(c)   ((c) == 0x20 )
#define SC_CONJ(c) ((c) == '&' || (c) == '|')
#define SC_EXT(c)  ((c) == '/' || (c) == '%' || (c) == '$')
#define SC_PAR(c)  ((c) == '(' || (c) == ')' || (c) == '{' || (c) == '}')
#define SC_SPEC(c) (SC_EXT(c) || SC_CONJ(c) || SC_SP(c) || SC_PAR(c))

#define SC_KEYWORD 0
#define SC_VALUE   1
#define SC_ATTR	2
#define SC_AND	 3
#define SC_OR	4
#define SC_VAL	 5
#define SC_DN	6
#define SC_BASE	7
#define SC_ONELEV  8
#define SC_SUBTREE 9
#define SC_UNDEF   10

#define SC_SYNTAXERROR 32

#define SC_TYPE_DN  16
#define SC_TYPE_VAL 17

typedef struct _strlistnode {
	char	 *str;
	struct _strlistnode *next;
	struct _strlistnode *prev;
} lsln_t;

typedef struct _vset {
	int	   scope;	/* SC_BASE | SC_ONELEV | SC_SUBTREE */
	int	   type;	/* SC_AND | SC_OR */
	int	   restype;	/* SC_VAL | SC_DN */
	int	   recurs;
	char	 *attr;
	lsln_t	   *attset;
	lsln_t	   *dn;
	lsln_t	   *val;
	struct _vset   *left;	/* which vset will I get the lefthand Set from 
				 */
	struct _vset   *right;	/* which vset will I get the righthand Set
				 * from */
	struct _vset   *up;	/* whom I'm delivering to */
} vset_t;

/*
 * ========================================================== 
 */

static char	*ls_strdup( char *s );

void	  scnode_free(scnode_t * scp);
scnode_t	 *scnode_new(int size);
scnode_t	 *scnode_get(octet_t * op, spocp_result_t * rc);
scnode_t	 *tree_top(scnode_t * scp);

vset_t	   *vset_new(void);
void	  vset_free(vset_t * sp);
vset_t	   *vset_get(scnode_t * scp, int type, octarr_t * oa,
			 spocp_result_t * rc);
void	  vset_print(vset_t * sp, int ns);

vset_t	   *vset_compact(vset_t * sp, LDAP * ld, spocp_result_t * rc);

lsln_t	   *get_results(LDAP *, LDAPMessage *, lsln_t *, lsln_t *, int,
				int);
void	  rm_children(vset_t * sp);

lsln_t	   *do_ldap_query(vset_t * vsetp, LDAP * ld, spocp_result_t * ret);
LDAP	 *open_conn(char *server, spocp_result_t * ret);

befunc	ldapset_test;

/*
 * ========================================================== 
 */

static lsln_t  *
lsln_new(char *s)
{
	lsln_t	   *new;

	new = (lsln_t *) calloc(1, sizeof(lsln_t));

	new->str = s;

	return new;
}

static void
lsln_free(lsln_t * slp)
{
	if (slp) {
		if (slp->str)
			free(slp->str);
		if (slp->next)
			free(slp->next);
		free(slp);
	}
}

static lsln_t  *
lsln_add(lsln_t * slp, char *s)
{
	lsln_t	   *loc;

	if (slp == 0)
		return lsln_new(s);

	for (loc = slp; loc; loc = loc->next) {
		if (strcmp(loc->str, s) == 0)
			return slp;
	}

	for (loc = slp; loc->next; loc = loc->next);

	loc->next = lsln_new(s);

	return slp;
}

static lsln_t  *
lsln_dup(lsln_t * old)
{
	lsln_t	   *new = 0;

	for (; old; old = old->next)
		new = lsln_add(new, old->str);

	return new;
}

static lsln_t  *
lsln_or(lsln_t * a, lsln_t * b)
{
	lsln_t	   *ls, *res = 0;

	LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "lsnl_or" ) ; 
	for (ls = a; ls; ls = ls->next) {
		res = lsln_add(res, ls->str);
		ls->str = 0;
	}

	for (ls = b; ls; ls = ls->next) {
		res = lsln_add(res, ls->str);
		ls->str = 0;
	}

/*
	for (; b; b = b->next) {
		for (ls = a; ls; ls = ls->next) {
			if (ls->str && strcmp(ls->str, b->str) == 0) {
				res = lsln_add(res, ls->str);
				ls->str = 0;
				break;
			}
		}
	}
*/

	return res;
}

static lsln_t  *
lsln_join(lsln_t * a, lsln_t * b)
{
	for (; b; b = b->next) {
		a = lsln_add(a, b->str);
		b->str = 0;
	}

	return a;
}

static lsln_t  *
lsln_and(lsln_t * a, lsln_t * b)
{
	lsln_t	   *ls, *res = 0;

	LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "lsnl_and" ) ; 
	for (; b; b = b->next) {
		for (ls = a; ls; ls = ls->next) {
			if (ls->str && strcmp(ls->str, b->str) == 0) {
				traceLog(LOG_DEBUG, "Add %s", ls->str);
				res = lsln_add(res, ls->str);
				ls->str = 0;
				break;
			}
		}
	}

	return res;
}

/*
 * this ought really to be UTF-8 aware 
 */
static int
lsln_find(lsln_t * sl, char *s)
{
	for (; sl; sl = sl->next)
		if (strcasecmp(sl->str, s) == 0)
			return 1;

	return 0;
}

/*
 * static lsln_t *lsln_first( lsln_t *sl ) { return sl ; }
 * 
 * static lsln_t *lsln_next( lsln_t *sl ) { return sl->next ; } 
 */

static char	*
lsln_get_val(lsln_t * sl)
{
	return sl->str;
}

static int
lsln_values(lsln_t * sl)
{
	int	   i = 0;

	for (; sl; sl = sl->next)
		i++;

	return i;
}

static char   **
lsln_to_arr(lsln_t * sl)
{
	char	**arr;
	int	   i;

	arr = (char **) calloc(lsln_values(sl) + 1, sizeof(char *));

	for (i = 0; sl; sl = sl->next, i++)
		arr[i] = sl->str;

	return arr;
}

static void
lsln_print( lsln_t *ll )
{
	int	i;
	char	*tmp;

	for( i=0 ;ll ; i++, ll = ll->next) {
		tmp = str_esc(ll->str, strlen(ll->str));
		traceLog( LOG_INFO, "(%d) \"%s\"", i, tmp);
		free(tmp);
	}
}

static char *
ls_strdup( char *s )
{
	char *new, *sp, *cp;

	if( strpbrk( s, "*()\\") == NULL ) {
		return strdup( s );
	}

	/* Probably much to big but better safe than sorry */
	new = ( char * ) malloc((strlen(s)*3+1) * sizeof( char ));

	for( sp = s, cp = new; *sp; sp++ ) {
		switch( *sp ) {
		case '*':
		  *cp++ = '\\';
		  *cp++ = '2';
		  *cp++ = 'a';
		  break;
		case '(':
		  *cp++ = '\\';
		  *cp++ = '2';
		  *cp++ = '8';
		  break;
		case ')':
		  *cp++ = '\\';
		  *cp++ = '2';
		  *cp++ = '9';
		  break;
		case '\\':
		  *cp++ = '\\';
		  *cp++ = '5';
		  *cp++ = 'c';
		  break;
		default:
		  *cp++ = *sp;
		}
	}
	*cp = '\0';

	return new;
}

/*
 * ========================================================== 
 */

static char	 *
safe_strcat(char *dest, char *src, int *size)
{
	char	 *tmp;
	int	   dl, sl;

	if (src == 0 || *size <= 0)
		return 0;

	dl = strlen(dest);
	sl = strlen(src);

	if (sl + dl > *size) {
		*size = sl + dl + 128;
		tmp = realloc(dest, *size);
		dest = tmp;
	}

	strcat(dest, src);

	return dest;
}

/*
 * ========================================================== 
 */

void
scnode_free(scnode_t * scp)
{
	if (scp) {
		if (scp->str)
			free(scp->str);
		if (scp->left)
			scnode_free(scp->left);
		if (scp->right)
			scnode_free(scp->right);

		free(scp);
	}
}

scnode_t	 *
scnode_new(int size)
{
	scnode_t	 *scp;

	scp = (scnode_t *) calloc(1, sizeof(scnode_t));

	if (size > 0) {
		scp->size = size;
		scp->str = (char *) calloc(scp->size, sizeof(char));
	} else {
		scp->size = 0;
		scp->str = 0;
	}

	return scp;
}

vset_t	   *
vset_new(void)
{
	vset_t	   *vs;

	vs = (vset_t *) calloc(1, sizeof(vset_t));

	/*
	 * defaults 
	 */
	/*
	 * vs->type = SC_OR ; 
	 */
	vs->restype = SC_VAL;

	return vs;
}

/*
 * Recursive freeing 
 */

void
vset_free(vset_t * sp)
{
	if (sp) {
		/*
		 * if( sp->attr ) free( sp->attr ) ; 
		 */
		if (sp->attset)
			lsln_free(sp->attset);
		if (sp->dn)
			lsln_free(sp->dn);
		if (sp->val)
			lsln_free(sp->val);
		if (sp->left)
			vset_free(sp->left);
		if (sp->right)
			vset_free(sp->right);

		free(sp);
	}
}

void
vset_print(vset_t * sp, int ns)
{
	char	  space[16];
	int	   i;
	lsln_t	   *ts;

	memset(space, 0, sizeof(space));
	memset(space, ' ', ns);

	printf("<-\n");
	if (sp->left)
		vset_print(sp->left, ns + 1);

	printf("--\n");
	if (sp->attset) {
		for (ts = sp->attset, i = 0; ts; ts = ts->next, i++)
			printf("%sattr[%d]: %s\n", space, i, lsln_get_val(ts));
	}

	if (sp->dn) {
		for (ts = sp->dn, i = 0; ts; ts = ts->next, i++)
			printf("%sdn[%d]: %s\n", space, i, lsln_get_val(ts));
	}

	if (sp->val) {
		for (ts = sp->val, i = 0; ts; ts = ts->next, i++)
			printf("%sval[%d]: %s\n", space, i, lsln_get_val(ts));
	}

	printf("->\n");
	if (sp->right)
		vset_print(sp->right, ns + 1);
}

scnode_t	 *
tree_top(scnode_t * scp)
{
	if (scp == 0)
		return 0;

	while (scp->up)
		scp = scp->up;

	return scp;
}

/*
 * routine that parses the valueset part of a ldapset boundary condition
 * returns a three structure with the individual elements 
 */
scnode_t	 *
scnode_get(octet_t * op, spocp_result_t * rc)
{
	char	 *cp, *sp, c;
	scnode_t	 *psc = 0, *nsc;
	int	   j = 0, l, d;

	*rc = 0;

	if (op == 0 || op->len == 0)
		return 0;

	for (l = op->len, sp = op->val; l;) {
		switch (*sp) {
		case ' ':
			if (psc == 0) {	/* can't be */
				*rc = SC_SYNTAXERROR;
				return 0;
			}

			sp++;
			l--;

			if ((*sp != '&' && *sp != '|') || *(sp + 1) != ' ') {
				*rc = SC_SYNTAXERROR;
				scnode_free(tree_top(psc));
				return 0;
			}

			nsc = scnode_new(2);
			*(nsc->str) = *sp;
			psc = tree_top(psc);
			nsc->left = psc;
			psc->up = nsc;
			psc = nsc;
			sp += 2;
			l -= 2;
			break;

		case '/':
		case '%':
		case '$':
			if (psc == 0) {	/* can't be */
				*rc = SC_SYNTAXERROR;
				return 0;
			}

			nsc = scnode_new(2);
			*(nsc->str) = *sp;
			psc = tree_top(psc);
			nsc->left = psc;
			psc->up = nsc;
			psc = nsc;

			sp++;
			l--;
			break;

		case '(':
		case '{':
			nsc = scnode_new(3);
			nsc->str[0] = *sp;
			if (*sp == '(')
				nsc->str[1] = ')';
			else
				nsc->str[1] = '}';

			op->val = ++sp;
			op->len = --l;
			nsc->left = scnode_get(op, rc);
			if (*rc) {
				scnode_free(nsc);
				scnode_free(tree_top(psc));
				return 0;
			}

			sp = op->val;
			l = op->len;
			if (psc) {
				psc->right = nsc;
				nsc->up = psc;
			} else
				psc = nsc;

			break;

		case ')':
		case '}':
			op->val = ++sp;
			op->len = --l;
			return tree_top(psc);
			break;

		case '"':
		case '<':
		default:
			c = *sp;
			d = 0;

			/*
			 * NULL Strings, not allowed 
			 */
			if ((c == '"' && (*(sp + 1) == '"')) ||
				(c == '<' && (*(sp + 1) == '>')) || (l - j == 1)) {
				scnode_free(tree_top(psc));
				*rc = SC_SYNTAXERROR;
				return 0;
			}

			if (*sp == '"') {	/* go to end marker */
				for (cp = ++sp, j = --l; j && *cp != '"';
					 cp++, j--);

			} else if (*sp == '<') {	/* go to end marker */
				for (cp = ++sp, j = --l; j && *cp != '>';
					 cp++, j--);

			} else
				for (cp = sp, j = l; j && !SC_SPEC(*cp);
					 cp++, j--);

			if (c == '"' || c == '<') {
				if (j == 0) {
					scnode_free(tree_top(psc));
					*rc = SC_SYNTAXERROR;
					return 0;
				}
				cp++;
				j--;
				d = 1;
			}

			nsc = scnode_new(cp - sp + 1);
			strncpy(nsc->str, sp, cp - sp - d);

			if (psc) {
				psc->right = nsc;
				nsc->up = psc;
			} else
				psc = nsc;

			sp = cp;
			l = j;

			break;
		}
	}

	op->val = sp;
	op->len = l;

	return tree_top(psc);
}

/*
 * Gets the valueset definition broken down into a tree structure and from
 * there assigns values to parts 
 */
vset_t	   *
vset_get(scnode_t * scp, int type, octarr_t * oa, spocp_result_t * rc)
{
	vset_t	   *sp = 0;
	char	  c, *cp, *ap;
	int	   n, rt = SC_UNDEF;
	char	 *dnp;


	c = *(scp->str);
	switch (c) {
	case '&':
	case '|':
		sp = vset_new();
		if (sp == 0)
			return 0;

		if (c == '&')
			sp->type = SC_AND;
		else
			sp->type = SC_OR;

		if (scp->left) {
			sp->left = vset_get(scp->left, SC_UNDEF, oa, rc);

			if (sp->left == 0) {
				vset_free(sp);
				if (!*rc)
					*rc = SPOCP_SYNTAXERROR;
				return 0;
			}

			sp->left->up = sp;
		}

		if (scp->right) {
			sp->right = vset_get(scp->right, SC_UNDEF, oa, rc);

			if (sp->right == 0) {
				vset_free(sp);
				if (!*rc)
					*rc = SPOCP_SYNTAXERROR;
				return 0;
			}

			sp->right->up = sp;
		}

		/*
		 * if( sp->right == 0 && sp->left == 0 ) { vset_free( sp ) ;
		 * return 0 ; } 
		 */

		if (sp->left) {
			if (sp->right) {
				if (sp->left->restype == SC_UNDEF) {
					rt = sp->right->restype;
				} else if (sp->right->restype == SC_UNDEF) {
					rt = sp->left->restype;
				} else if (sp->right->restype !=
					   sp->left->restype) {
					/*
					 * traceLog(LOG_DEBUG,"Trying to add DNs and
					 * attribute values") ; 
					 */
					rt = SC_DN;
				} else
					rt = sp->left->restype;
			} else
				rt = sp->left->restype;
		} else
			rt = sp->right->restype;

		if (type == SC_UNDEF)
			sp->restype = rt;
		else
			sp->restype = type;

		break;

	case '/':		/* base */
	case '%':		/* one */
	case '$':		/* sub */
		sp = vset_new();
		sp->restype = type;

		if (c == '/')
			sp->scope = SC_BASE;
		else if (c == '%')
			sp->scope = SC_ONELEV;
		else
			sp->scope = SC_SUBTREE;

		if (scp->left) {
			sp->left = vset_get(scp->left, SC_DN, oa, rc);

			if (sp->left == 0) {
				vset_free(sp);
				if (!*rc)
					*rc = SPOCP_SYNTAXERROR;
				return 0;
			}

			sp->left->up = sp;
		}
		if (scp->right) {
			sp->right = vset_get(scp->right, SC_ATTR, oa, rc);

			if (sp->right == 0) {
				vset_free(sp);
				if (!*rc)
					*rc = SPOCP_SYNTAXERROR;
				return 0;
			}

			sp->right->up = sp;
		}

		break;

	case '<':
		sp = vset_new();
		sp->restype = type;

		if (type == SC_VAL) {
			vset_free(sp);
			*rc = SPOCP_SYNTAXERROR;
			return 0;
		}

		sp->dn =
			lsln_add(sp->dn, strndup(scp->str + 1, scp->size - 3));
		break;

	case '[':
		sp = vset_new();
		sp->restype = type;

		if (type == SC_DN) {
			vset_free(sp);
			*rc = SPOCP_SYNTAXERROR;
			return 0;
		}

		sp->val =
			lsln_add(sp->val, strndup(scp->str + 1, scp->size - 3));
		break;

	case '{':		/* str == "{}" */
		sp = vset_get(scp->left, SC_UNDEF, oa, rc);

		if (sp == 0) {
			if (!*rc)
				*rc = SPOCP_SYNTAXERROR;
			return 0;
		}

		sp->restype = SC_DN;
		break;

	case '(':
		sp = vset_get(scp->left, SC_UNDEF, oa, rc);

		if (sp == 0) {
			if (!*rc)
				*rc = SPOCP_SYNTAXERROR;
			return 0;
		}

		sp->restype = SC_VAL;
		break;

	default:		/* just a string */
		sp = vset_new();
		sp->restype = type;

		if (type == SC_ATTR) {	/* this might be a comma separated
					 * list */
			ap = scp->str;
			do {
				if ((cp = index(ap, ',')) != 0) {
					*cp++ = 0;	/* don't have to reset 
							 * since it's not used 
							 * again */
					while (*cp == ',')
						cp++;
				}
				sp->attset = lsln_add(sp->attset, strdup(ap));
				/*
				 * sp->attr = strdup( scp->str ) ; 
				 */
				ap = cp;
			} while (cp != 0);
		} else if (type == SC_DN) {
			dnp = scp->str + 1;

			/*
			 * traceLog(LOG_DEBUG, "SC_DN:[%c][%c]", *scp->str, *dnp ) ; 
			 */

			if (*scp->str != '\\'
				|| (*dnp < '0' || *dnp >= ('0' + oa->n))) {
				vset_free(sp);
				*rc = SPOCP_SYNTAXERROR;
				return 0;
			} else {
				n = *dnp - '0' + 1;
				if (n >= oa->n) {
					*rc = SPOCP_SYNTAXERROR;
					return 0;
				}
				sp->dn =
					lsln_add(sp->dn,
						 oct2strdup(oa->arr[n], '\\'));
			}
		} else {
			dnp = scp->str + 1;

			/*
			 * traceLog(LOG_DEBUG, "[%c][%c]", *scp->str, *dnp ) ; 
			 */

			if (*scp->str != '\\') {
				sp->val = lsln_add(sp->val, strdup(scp->str));
				scp->str = 0;
				scp->size = 0;
			} else if (*dnp < '0' || *dnp >= ('0' + oa->n)) {
				vset_free(sp);
				*rc = SPOCP_SYNTAXERROR;
				return 0;
			} else {
				n = *dnp - '0' + 1;
				if (n >= oa->n) {
					*rc = SPOCP_SYNTAXERROR;
					return 0;
				}
				sp->dn =
					lsln_add(sp->dn,
						 oct2strdup(oa->arr[n], '\\'));
			}
		}
	}

	return sp;
}

lsln_t	   *
get_results(LDAP * ld,
		LDAPMessage * res, lsln_t * attr, lsln_t * val, int type, int ao)
{
	int	   i, nr, wildcard = 0;
	lsln_t	   *set = 0;
	LDAPMessage	*e;
	BerElement	 *be = NULL;
	char	**vect = 0, *ap, *dn;

	nr = ldap_count_entries(ld, res);

	/*
	 * could I have to many ?? 
	 */
	/*
	 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "Found %d matching entries", nr ) ; 
	 */
	if ( val && index(val->str,'*'))
		wildcard = 1;

	if (ao == SC_OR) {
		/*
		 * traceLog(LOG_DEBUG,"SC_OR") ; 
		 */
		if (type == SC_VAL)
			set = lsln_dup(val);

		for (e = ldap_first_entry(ld, res); e != NULL;
			 e = ldap_next_entry(ld, e)) {

			/*
			 * I'm might not be really interested in the attribute 
			 * values, only interested in the DNs and those are
			 * not listed among the val's 
			 */
			if (type == SC_DN) {
				/*
				 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "Collecting
				 * DNs" ) ; 
				 */
				dn = ldap_get_dn(ld, e);
				set = lsln_add(set, ls_strdup(dn));
				ldap_memfree(dn);
			} else {
				/*
				 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "Collecting
				 * attribute values" ) ; 
				 */
				for (ap = ldap_first_attribute(ld, e, &be);
					 ap != NULL;
					 ap = ldap_next_attribute(ld, e, be)) {
					/*
					 * unsigned comparision between
					 * attribute names 
					 */
					/*
					 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "Got
					 * attribute: %s", ap ) ; 
					 */

					if (lsln_find(attr, ap) == 0)
						continue;

					vect = ldap_get_values(ld, e, ap);

					for (i = 0; vect[i]; i++) {
						/*
						 * LOG( SPOCP_DEBUG )
						 * traceLog(LOG_DEBUG, "Among the ones I 
						 * look for" ) ; 
						 */
						/*
						 * disregard if not a value
						 * I'm looking for 
						 */
						if ( !wildcard &&
							lsln_find(val, vect[i]) == 0)
							continue;

						set =
							lsln_add(set,
								 ls_strdup(vect[i]));
					}

					ldap_value_free(vect);
				}
			}
		}
	} else {
		/*
		 * traceLog(LOG_DEBUG,"SC_AND") ; 
		 */
		for (e = ldap_first_entry(ld, res); e != NULL;
			 e = ldap_next_entry(ld, e)) {

			if (type == SC_DN) {
				/*
				 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "Collecting
				 * DNs" ) ; 
				 */
				dn = ldap_get_dn(ld, e);
				set = lsln_add(set, ls_strdup(dn));
				ldap_memfree(dn);
			} else {
				/*
				 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "Collecting
				 * attribute values" ) ; 
				 */

				for (ap = ldap_first_attribute(ld, e, &be);
					 ap != NULL;
					 ap = ldap_next_attribute(ld, e, be)) {

					/*
					 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "Got
					 * attribute: %s", ap ) ; 
					 */

					/*
					 * uninteresting attribute 
					 */
					if (lsln_find(attr, ap) == 0)
						continue;

					vect = ldap_get_values(ld, e, ap);

					for (i = 0; vect[i]; i++) {
						if (!wildcard &&
							lsln_find(val, vect[i]) == 0)
							continue;

						set =
							lsln_add(set,
								 ls_strdup(vect[i]));
					}

					ldap_value_free(vect);
				}
			}
		}
	}

	/*
	 * if( set ) { lsln_t *tmp ;
	 * 
	 * for( tmp = lsln_first( set ), i = 0 ; tmp ; tmp = lsln_next( tmp ), 
	 * i++ ) LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "set[%d]: %s", i, (char *)
	 * lsln_get_val( tmp )) ; } 
	 */

	return set;
}

/*
 * server is [ user [ : password ] @ ] host 
 */

LDAP	 *
open_conn(char *server, spocp_result_t * ret)
{
	LDAP	 *ld = 0;
	int	   rc, vers = 3;
	char	 *user = 0, *passwd = 0;

	if ((passwd = index(server, '@'))) {
		user = server;
		*passwd = 0;
		server = passwd + 1;
		if ((passwd = index(user, ':'))) {
			*passwd++ = 0;
		}
	}

	LOG( SPOCP_DEBUG) traceLog(LOG_DEBUG,"Opening LDAP con to %s", server ) ; 

	if ((ld = ldap_init(server, 0)) == 0) {
		LOG( SPOCP_WARNING )
			traceLog(LOG_DEBUG, "Error: Couldn't initialize the LDAP server") ; 
		*ret = SPOCP_UNAVAILABLE;
		return 0;
	}

	/*
	 * set version to 3, surprisingly enough openldap 2.1.4 seems to use 2 
	 * as default 
	 */

	if (ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &vers) !=
		LDAP_SUCCESS) {
		LOG( SPOCP_WARNING )
			traceLog(LOG_WARNING, "Error: Couldn't set the version") ; 
		*ret = SPOCP_UNAVAILABLE;
		ldap_unbind(ld);
		return 0;
	}

	/*
	 * automatic follow referrals 
	 */
	if (ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_ON) !=
		LDAP_SUCCESS) {
		LOG( SPOCP_WARNING )
			traceLog(LOG_WARNING, "Error: Couldn't set follow referrals") ; 
		*ret = SPOCP_UNAVAILABLE;
		ldap_unbind(ld);
		return 0;
	}


	if ((rc = ldap_simple_bind_s(ld, user, passwd)) != LDAP_SUCCESS) {
		LOG( SPOCP_WARNING )
			traceLog(LOG_WARNING, "LDAP bind failed to %s", server ) ; 
		*ret = SPOCP_UNAVAILABLE;
		ldap_unbind(ld);
		return 0;
	}

	/*
	 * if( use_tls && ldap_start_tls_s( ld, NULL, NULL ) != LDAP_SUCCESS ) 
	 * { if( use_tls > 1 ) { LOG( SPOCP_WARNING ) traceLog(LOG_WARNING,
	 * "LDAP_START_TLS failed" ) ; *ret = -1 ; ldap_unbind( ld ) ; return
	 * 0 ; } } 
	 */

	/*
	 * LOG( SPOCP_DEBUG) traceLog(LOG_DEBUG,"Bound OK" ) ; 
	 */

	return ld;
}

lsln_t	   *
do_ldap_query(vset_t * vsetp, LDAP * ld, spocp_result_t * ret)
{
	int	   rc, scope, size = 256, na;
	lsln_t	   *arr = 0, *sa = 0;
	lsln_t	   *attrset = vsetp->attset, *dn = vsetp->dn, *val =
		vsetp->val;
	char	 *base, *va, **attr;
	char	 *filter;
	LDAPMessage	*res = 0;

	/*
	 * openended queries are not allowed 
	 */
	if (vsetp->attset == 0)
		return 0;

	/*
	 * No base for the queries, nothing to do then 
	 */
	if (dn == 0)
		return 0;

	/*
	 * AND anything to the empty set and you will get the empty set 
	 */
	if ((vsetp->type == SC_AND) && (val == 0))
		return 0;

	  *ret = SPOCP_SUCCESS;
	filter = (char *) malloc(size * sizeof(char));

	na = lsln_values(vsetp->attset);
	attr = lsln_to_arr(vsetp->attset);

	if (vsetp->scope == SC_BASE)
		scope = LDAP_SCOPE_BASE;
	else if (vsetp->scope == SC_ONELEV)
		scope = LDAP_SCOPE_ONELEVEL;
	else
		scope = LDAP_SCOPE_SUBTREE;

	memset(filter, 0, size);

	/*
	 * same filter for all queries, so just do it once 
	 */

	if (na > 1)
		filter = safe_strcat(filter, "(|", &size);

	for (; attrset; attrset = attrset->next) {
		if (val) {
			if (lsln_values(val) == 1) {
				va = lsln_get_val(val);
				filter = safe_strcat(filter, "(", &size);
				filter =
					safe_strcat(filter, lsln_get_val(attrset),
						&size);
				filter = safe_strcat(filter, "=", &size);
				filter = safe_strcat(filter, va, &size);
				filter = safe_strcat(filter, ")", &size);
			} else {
				filter = safe_strcat(filter, "(|", &size);
				for (val = vsetp->val; val; val = val->next) {
					va = lsln_get_val(val);
					filter =
						safe_strcat(filter, "(", &size);
					filter =
						safe_strcat(filter,
							lsln_get_val(attrset),
							&size);
					filter =
						safe_strcat(filter, "=", &size);
					filter =
						safe_strcat(filter, va, &size);
					filter =
						safe_strcat(filter, ")", &size);
				}
				filter = safe_strcat(filter, ")", &size);
			}
		} else {
			filter = safe_strcat(filter, "(", &size);
			filter =
				safe_strcat(filter, lsln_get_val(attrset), &size);
			filter = safe_strcat(filter, "=*)", &size);
		}
	}

	if (na > 1)
		filter = safe_strcat(filter, ")", &size);

	/*
	 * val might be 0, that is I'm not really interested in the set of
	 * values 
	 */

	for (; dn; dn = dn->next) {

		base = lsln_get_val(dn);
		res = 0;

		LOG( SPOCP_DEBUG ) {
			char *ftmp, *btmp;
			ftmp = str_esc(filter, strlen(filter));
			btmp = str_esc(base, strlen(base));
			traceLog(LOG_DEBUG,"Using filter: %s, base: %s, scope: %d",
				ftmp, btmp, scope ) ; 
			free(ftmp);
			free(btmp);
		}

		if ((rc =
			 ldap_search_s(ld, base, scope, filter, attr, 0, &res))) {
			ldap_perror(ld, "During search");
			lsln_free(arr);
			arr = 0;
			*ret = SPOCP_OTHER;
		} else {
			if ((sa =
				 get_results(ld, res, vsetp->attset, val,
					 vsetp->restype, vsetp->type))) {
				arr = lsln_join(arr, sa);
				lsln_free(sa);
			}
		}

		if (res)
			ldap_msgfree(res);
	}

	free(filter);
	free(attr);
	/*
	 */

	LOG( SPOCP_DEBUG ) {
		if (arr == 0)
			traceLog(LOG_DEBUG, "LDAP return NULL (%d)", *ret );
		else {
			traceLog(LOG_DEBUG, "LDAP returned a set (%d)", *ret);
			lsln_print( arr );
		}
	} 

	return arr;
}

void
rm_children(vset_t * sp)
{
	vset_free(sp->left);
	vset_free(sp->right);

	sp->left = sp->right = 0;
}

vset_t	   *
vset_compact(vset_t * sp, LDAP * ld, spocp_result_t * rc)
{
	/*
	 * vset_t *tmp ; 
	 */
	lsln_t	   *arr;

	if (sp->left) {
		if ((sp->left = vset_compact(sp->left, ld, rc)) == 0) {
			vset_free(sp);
			return 0;
		}
	}
	if (sp->right) {
		if ((sp->right = vset_compact(sp->right, ld, rc)) == 0) {
			vset_free(sp);
			return 0;
		}
	}

	/*
	 * look for patterns 
	 */
	if (sp->scope) {	/* gather search info from below */
		if (sp->left && sp->left->restype == SC_DN && sp->left->dn) {
			sp->dn = sp->left->dn;
			sp->left->dn = 0;
			vset_free(sp->left);
			sp->left = 0;
		} else
			sp->dn = 0;

		if (sp->right && sp->right->restype == SC_ATTR) {	/* attribute 
									 * specification 
									 */
			sp->attset = sp->right->attset;
			sp->right->attset = 0;
			vset_free(sp->right);
			sp->right = 0;
		} else
			sp->attset = 0;
	} else if (sp->type == SC_AND) {
		/*
		 * am I heading for a query 
		 */
		if (sp->left->scope >= SC_BASE
			&& sp->left->scope <= SC_SUBTREE) {

			sp->dn = sp->left->dn;
			sp->left->dn = 0;
			sp->attset = sp->left->attset;
			sp->left->attset = 0;
			sp->scope = sp->left->scope;
			vset_free(sp->left);
			sp->left = 0;

			/*
			 * determine the type of values 
			 */
			if ((sp->right->restype == SC_VAL
				 || sp->right->restype == SC_UNDEF)
				&& sp->right->val) {
				sp->val = sp->right->val;
				sp->right->val = 0;
			} else {
				sp->val = sp->right->dn;
				sp->right->dn = 0;
			}

			vset_free(sp->right);
			sp->right = 0;

			/*
			 * doing the query 
			 */
			arr = do_ldap_query(sp, ld, rc);

			/*
			 * error in query 
			 */
			if (*rc != SPOCP_SUCCESS) {
				vset_free(sp);
				return 0;	/* error condition */
			}

			/*
			 * ok query 
			 */
			if (arr == 0) {	/* empty LDAP resultset, clean up */
				vset_free(sp);
				return 0;
			}

			/*
			 * what type of values did I get back ? 
			 */
			if (sp->restype == SC_DN) {
				if (sp->dn)
					lsln_free(sp->dn);
				sp->dn = arr;
				if (sp->val)
					lsln_free(sp->val);
				sp->val = 0;
			} else {
				if (sp->val)
					lsln_free(sp->val);
				sp->val = arr;
				if (sp->dn)
					lsln_free(sp->dn);
				sp->dn = 0;
			}

			/*
			 * don't need the attribute sets anymore 
			 */
			lsln_free(sp->attset);
			sp->attset = 0;
			/*
			 * Just so this can't be used as a query once more 
			 */
			sp->scope = 0;
		}
		/*
		 * No query, just add the two result sets 
		 */
		else {
			/*
			 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "ANDing two sets --" ) 
			 * ; 
			 */

			if (sp->left->dn && sp->right->dn) {
				if (sp->dn)
					lsln_free(sp->dn);
				sp->dn = lsln_and(sp->left->dn, sp->right->dn);

				rm_children(sp);
				if (sp->dn == 0) {
					/*
					 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "The
					 * resulting set was empty" ) ; 
					 */
					vset_free(sp);
					return 0;
				}
			} else {
				if (sp->left->val && sp->right->val) {
					if (sp->val)
						lsln_free(sp->val);

					sp->val =
						lsln_and(sp->left->val, sp->right->val);

				} else if (sp->left->val && sp->right->dn) {
					if( sp->restype == SC_DN ) {
						if (sp->dn)
							lsln_free(sp->dn);
					} else if (sp->val)
						lsln_free(sp->val);

					arr =
						lsln_and(sp->left->val, sp->right->dn);

					if( sp->restype == SC_DN ) 
						sp->dn = arr;
					else
						sp->val = arr;

				} else if (sp->left->dn && sp->right->val) {
					if( sp->restype == SC_DN ) {
						if (sp->dn)
							lsln_free(sp->dn);
					} else if (sp->val)
						lsln_free(sp->val);

					arr =
						lsln_and(sp->left->dn, sp->right->val);

					if( sp->restype == SC_DN ) 
						sp->dn = arr;
					else
						sp->val = arr;
				}
				else
					return 0;

				rm_children(sp);
				if (( sp->restype == SC_DN && sp->dn == 0) ||
					( sp->restype != SC_DN && sp->val == 0)) {
					/*
					 * LOG( SPOCP_DEBUG ) traceLog(LOG_DEBUG, "The
					 * resulting set was empty" ) ; 
					 */
					vset_free(sp);
					return 0;
				}
			}
		}
	} else if (sp->type == SC_OR) {
		if (sp->left == 0 && sp->right == 0);
		else if (sp->left == 0) {
			if (sp->right->dn) {
				if (sp->dn)
					lsln_free(sp->dn);
				sp->dn = sp->right->dn;
				sp->right->dn = 0;
			} else if (sp->right->val) {
				if (sp->val)
					lsln_free(sp->val);
				sp->val = sp->right->val;
				sp->right->val = 0;
			}
			rm_children(sp);
		} else if (sp->right == 0) {
			if (sp->left->dn) {
				if (sp->dn)
					lsln_free(sp->dn);
				sp->dn = sp->left->dn;
				sp->left->dn = 0;
			} else if (sp->left->val) {
				if (sp->val)
					lsln_free(sp->val);
				sp->val = sp->left->val;
				sp->left->val = 0;
			}
			rm_children(sp);
		} else if (sp->left->dn || sp->right->dn) {
			if (sp->right->dn && sp->left->dn) {
				if (sp->dn)
					lsln_free(sp->dn);
				sp->dn = lsln_or(sp->left->dn, sp->right->dn);
			} else if (sp->left->dn) {
				if (sp->dn)
					lsln_free(sp->dn);
				sp->dn = sp->left->dn;
				sp->left->dn = 0;
			} else {
				if (sp->dn)
					lsln_free(sp->dn);
				sp->dn = sp->right->dn;
				sp->right->dn = 0;
			}

			rm_children(sp);
		} else if (sp->left->val || sp->right->val) {
			if (sp->right->val && sp->left->val) {
				if (sp->val)
					lsln_free(sp->val);
				sp->val =
					lsln_or(sp->left->val, sp->right->val);
			} else if (sp->left->val) {
				if (sp->val)
					lsln_free(sp->val);
				sp->val = sp->left->val;
				sp->left->val = 0;
			} else {
				if (sp->val)
					lsln_free(sp->val);
				sp->val = sp->right->val;
				sp->right->val = 0;
			}

			rm_children(sp);
		} else {	/* error */
			vset_free(sp);
			return 0;
		}
	}

	return sp;
}

static int
P_ldapclose(void *con)
{
	return ldap_unbind_s((LDAP *) con);
}

/*
 */
spocp_result_t
ldapset_test(cmd_param_t * cpp, octet_t * blob)
{
	spocp_result_t  r = SPOCP_DENIED, rc = SPOCP_SUCCESS ;
	scnode_t	 *scp;
	vset_t	   *vset, *res;
	char	 *ldaphost, *tmp;
	octet_t	  *oct, *o, cb;
	LDAP	 *ld = 0;
	becon_t	  *bc = 0;
	octarr_t	 *argv;
	int	   cv = 0;
	pdyn_t	   *dyn = cpp->pd;

	if (cpp->arg == 0 || cpp->arg->len == 0)
		return SPOCP_MISSING_ARG;

	LOG( SPOCP_DEBUG ) {
		tmp = oct2strdup( cpp->arg, '%' );
		LOG( SPOCP_DEBUG) traceLog( LOG_DEBUG , "Ldapset test on: \"%s\"", tmp );
		free(tmp);
	}

	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	LOG( SPOCP_DEBUG ) {
		tmp = oct2strdup( oct, '%' );
		LOG( SPOCP_DEBUG) traceLog( LOG_DEBUG , "Expanded: \"%s\"", tmp);
		free(tmp);
	}

	cb.len = 0;
	if (dyn)
		cv = cached(dyn->cv, oct, &cb);

	if (cv) {
		LOG( SPOCP_DEBUG) traceLog(LOG_DEBUG,"ldapset: cache hit");

		if (cv == EXPIRED) {
			cached_rm(dyn->cv, oct);
			cv = 0;
		}
	}

	if (cv == 0) {

		argv = oct_split(oct, ';', '\\', 0, 0);

		/*
		 * LOG( SPOCP_DEBUG) traceLog(LOG_DEBUG, "filter: \"%s\"", arg->val ) ; 
		 */

		o = argv->arr[argv->n - 1];

		if ((scp = scnode_get(o, &rc)) != 0) {
			if ((vset = vset_get(scp, SC_UNDEF, argv, &rc)) != 0) {
				while (vset->up)
					vset = vset->up;

				if (0)
					vset_print(vset, 0);

				o = argv->arr[0];

				if (dyn == 0 || (bc = becon_get(o, dyn->bcp)) == 0) {

					ldaphost = oct2strdup(o, 0);
					ld = open_conn(ldaphost, &rc);
					free(ldaphost);

					if (ld == 0)
						r = SPOCP_UNAVAILABLE;
					else if (dyn && dyn->size) {
						if (!dyn->bcp)
							dyn->bcp =
								becpool_new(dyn->
									size);
						bc = becon_push(o,
								&P_ldapclose,
								(void *) ld,
								dyn->bcp);
					}
				} else
					ld = (LDAP *) bc->con;

				if (ld != 0) {
					res = vset_compact(vset, ld, &rc);

					if (res != 0) {
						if (res->restype == SC_DN
							&& res->dn)
							r = SPOCP_SUCCESS;
						else if ((res->restype ==
							  SC_VAL
							  || res->restype ==
							  SC_UNDEF)
							 && res->val)
							r = SPOCP_SUCCESS;
						else
							r = SPOCP_DENIED;

						vset_free(res);
					} else {
/*
						if (rc)
							r = rc;
						else
*/
						r = SPOCP_DENIED;
					}

					if (bc)
						becon_return(bc);
					else
						ldap_unbind_s(ld);
				} else
					vset_free(vset);

			} else
				r = rc;

			scnode_free(scp);
		} else
			r = rc;

		octarr_free(argv);
	}

	if (cv == (CACHED | SPOCP_SUCCESS)) {
		if (cb.len)
			octln(blob, &cb);
		r = SPOCP_SUCCESS;
	} else if (cv == (CACHED | SPOCP_DENIED)) {
		r = SPOCP_DENIED;
	} else {
		if (dyn && dyn->ct && (r == SPOCP_SUCCESS || r == SPOCP_DENIED)) {
			time_t	t;
			t = cachetime_set(oct, dyn->ct);
			dyn->cv =
				cache_value(dyn->cv, oct, t, (r | CACHED), 0);
		}
	}

	if (oct != cpp->arg)
		oct_free(oct);

	LOG( SPOCP_DEBUG) traceLog(LOG_DEBUG,"ldapset => %d", r);

	return r;
}

plugin_t	  ldapset_module = {
	SPOCP20_PLUGIN_STUFF,
	ldapset_test,
	NULL,
	NULL,
	NULL
};
