#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ldapset.h"

char           *strndup(const char *old, size_t sz);

static char *kw[] = { "", "VALUE", "ATTR", "AND","OR","VAL","DN", \
	"BASE","ONELEV","SUBTREE","UNDEF" };
/*
 * ========================================================== 
 */

static vset_t	 *
vset_new(void)
{
	vset_t	 *vs;

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

static void
rm_children(vset_t * sp)
{
	vset_free(sp->left);
	vset_free(sp->right);

	sp->left = sp->right = 0;
}


void
vset_print(vset_t * sp, int ns)
{
	char	space[16];
	int	 i;
	lsln_t	 *ts;

	memset(space, 0, sizeof(space));
	memset(space, ' ', ns);

	if (sp->left)
		vset_print(sp->left, ns + 2);

	printf("--%s:%s:%s:%s\n",kw[sp->scope],kw[sp->type], \
			kw[sp->restype],kw[sp->striprdn]);
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

	if (sp->right)
		vset_print(sp->right, ns + 1);
}

/*
 * Gets the valueset definition broken down into a tree structure and from
 * there assigns values to parts 
 */
vset_t	 *
vset_get(scnode_t * scp, int type, octarr_t * oa, spocp_result_t * rc)
{
	vset_t	*vs = 0;
	char	c, *cp, *ap;
	int	n, rt = SC_UNDEF;
	char	*dnp;


	c = *(scp->str);
	switch (c) {
	case '&':
	case '|':
		vs = vset_new();
		if (vs == 0)
			return 0;

		if (c == '&')
			vs->type = SC_AND;
		else
			vs->type = SC_OR;

		if (scp->right) {
			vs->left = vset_get(scp->right, SC_UNDEF, oa, rc);

			if (vs->left == 0) {
				vset_free(vs);
				if (!*rc)
					*rc = SPOCP_SYNTAXERROR;
				return 0;
			}

			vs->left->up = vs;
		}

		if (scp->left) {
			vs->right = vset_get(scp->left, SC_UNDEF, oa, rc);

			if (vs->right == 0) {
				vset_free(vs);
				if (!*rc)
					*rc = SPOCP_SYNTAXERROR;
				return 0;
			}

			vs->right->up = vs;
		}

		/*
		* if( vs->right == 0 && vs->left == 0 ) { vset_free( vs ) ;
		* return 0 ; } 
		*/

		if (vs->left) {
			if (vs->right) {
				if (vs->left->restype == SC_UNDEF) {
					rt = vs->right->restype;
				} else if (vs->right->restype == SC_UNDEF) {
					rt = vs->left->restype;
				} else if (vs->right->restype !=
					 vs->left->restype) {
					/*
					* traceLog(LOG_DEBUG,"Trying to add DNs and
					* attribute values") ; 
					*/
					rt = SC_DN;
				} else
					rt = vs->left->restype;
			} else
				rt = vs->left->restype;
		} else
			rt = vs->right->restype;

		if (type == SC_UNDEF)
			vs->restype = rt;
		else
			vs->restype = type;

		break;

	case '/':		/* base */
	case '%':		/* one */
	case '$':		/* sub */
		vs = vset_new();
		vs->restype = type;

		if (c == '/')
			vs->scope = SC_BASE;
		else if (c == '%')
			vs->scope = SC_ONELEV;
		else
			vs->scope = SC_SUBTREE;

		vs->striprdn = scp->striprdn;

		if (scp->right) {
			vs->left = vset_get(scp->right, SC_DN, oa, rc);

			if (vs->left == 0) {
				vset_free(vs);
				if (!*rc)
					*rc = SPOCP_SYNTAXERROR;
				return 0;
			}

			vs->left->up = vs;
		}
		if (scp->left) {
			vs->right = vset_get(scp->left, SC_ATTR, oa, rc);

			if (vs->right == 0) {
				vset_free(vs);
				if (!*rc)
					*rc = SPOCP_SYNTAXERROR;
				return 0;
			}

			vs->right->up = vs;
		}

		break;

	case '<':
		vs = vset_new();
		vs->restype = type;

		if (type == SC_VAL) {
			vset_free(vs);
			*rc = SPOCP_SYNTAXERROR;
			return 0;
		}

		vs->dn =
			lsln_add(vs->dn, strndup(scp->str + 1, scp->size - 3));
		break;

	case '{':		/* str == "{}" */
		vs = vset_get(scp->right, SC_UNDEF, oa, rc);

		if (vs == 0) {
			if (!*rc)
				*rc = SPOCP_SYNTAXERROR;
			return 0;
		}

		vs->restype = SC_DN;
		vs->striprdn = scp->striprdn;
		break;

	case '(':
		vs = vset_get(scp->right, SC_UNDEF, oa, rc);

		if (vs == 0) {
			if (!*rc)
				*rc = SPOCP_SYNTAXERROR;
			return 0;
		}

		vs->restype = SC_VAL;
		break;

	default:		/* just a string */
		vs = vset_new();
		vs->restype = type;

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
				vs->attset = lsln_add(vs->attset, strdup(ap));
				/*
				* vs->attr = strdup( scp->str ) ; 
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
				vset_free(vs);
				*rc = SPOCP_SYNTAXERROR;
				return 0;
			} else {
				n = *dnp - '0' + 1;
				if (n >= oa->n) {
					*rc = SPOCP_SYNTAXERROR;
					return 0;
				}
				vs->dn =
					lsln_add(vs->dn,
						oct2strdup(oa->arr[n], '\\'));
			}
		} else {
			dnp = scp->str + 1;

			/*
			* traceLog(LOG_DEBUG, "[%c][%c]", *scp->str, *dnp ) ; 
			*/

			if (*scp->str != '\\') {
				vs->val = lsln_add(vs->val, strdup(scp->str));
				scp->str = 0;
				scp->size = 0;
			} else if (*dnp < '0' || *dnp >= ('0' + oa->n)) {
				vset_free(vs);
				*rc = SPOCP_SYNTAXERROR;
				return 0;
			} else {
				n = *dnp - '0' + 1;
				if (n >= oa->n) {
					*rc = SPOCP_SYNTAXERROR;
					return 0;
				}
				vs->dn =
					lsln_add(vs->dn,
						oct2strdup(oa->arr[n], '\\'));
			}
		}
	}

	return vs;
}

vset_t	 *
vset_compact(vset_t * sp, LDAP *ld, spocp_result_t * rc)
{
	/*
	* vset_t *tmp ; 
	*/
	lsln_t	 *arr;

	if (sp->left) {
#ifdef AVLUS
		traceLog(LOG_DEBUG,"Do left");
#endif
		if ((sp->left = vset_compact(sp->left, ld, rc)) == 0) {
			vset_free(sp);
			return 0;
		}
	}
	if (sp->right) {
#ifdef AVLUS
		traceLog(LOG_DEBUG,"Do right");
#endif
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

		if (sp->right && sp->right->restype == SC_ATTR) {
			/* attribute specification */
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
