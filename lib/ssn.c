
/***************************************************************************
                          ssn.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002, 2009 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <string.h>

#include <branch.h>
#include <element.h>
#include <varr.h>
#include <ssn.h>

#include <wrappers.h>

/*
 * This module builds and maintains a parse tree of characters, where
 * junc's are placed at end of words.
 * 
 */

varr_t         *get_rec_all_ssn_followers(ssn_t * ssn, varr_t * ja);

/* ------------------------------------------------------------------ */

ssn_t   *
ssn_new(char ch)
{
	ssn_t          *ssn;

	ssn = (ssn_t *) Calloc(1, sizeof(ssn_t));

	ssn->ch = ch;
	ssn->refc = 1;

	return ssn;
}

static junc_t  *
ssn_insert_forward(ssn_t ** top, char *str)
{
	ssn_t          *pssn = 0, *previous=0;
	unsigned char  *s;

	s = (unsigned char *) str;

	if (*top == 0) {
		pssn = *top = ssn_new(*s);

		for (s++; *s; s++) {
			pssn->down = ssn_new(*s);
			pssn = pssn->down;
		}

		pssn->next = junc_new();
	} else {
		pssn = *top;
		/*
		 * follow the branches as far as possible 
		 */
		while (*s) {
			if (*s == pssn->ch) {

				/*
				 * counter to keep track of if this node is
				 * used anymore 
				 */
				pssn->refc++;

				if (pssn->down) {
                    previous = pssn;
					pssn = pssn->down;
					s++;
				} else if (*++s) {
                    previous = 0;
					pssn->down = ssn_new(*s);
					pssn = pssn->down;
					break;
				} else
					break;
			} else if (*s < pssn->ch) {
				if (pssn->left) {
                    previous = pssn;
					pssn = pssn->left;
                }
				else {
                    previous = 0;
					pssn->left = ssn_new(*s);
					pssn = pssn->left;
					break;
				}
			} else if (*s > pssn->ch) {
				if (pssn->right) {
                    previous = pssn;
					pssn = pssn->right;
                }
				else {
                    previous = 0;
					pssn->right = ssn_new(*s);
					pssn = pssn->right;
					break;
				}
			}
		}

		if (*s == 0) {
            if (previous) {
                previous->next = junc_new();
            }
			else if (pssn->next == 0)
				pssn->next = junc_new();
		} else {
			for (s++; *s; s++) {
				pssn->down = ssn_new(*s);
				pssn = pssn->down;
			}

			pssn->next = junc_new();
		}
	}

	return pssn->next;
}

static junc_t  *
ssn_insert_backward(ssn_t ** top, char *str)
{
	ssn_t          *pssn, *previous=0;
	unsigned char  *s;

	s = (unsigned char *) &str[strlen(str) - 1];

	if (*top == 0) {
		pssn = *top = ssn_new(*s);

		for (s--; s >= (unsigned char *) str; s--) {
			pssn->down = ssn_new(*s);
			pssn = pssn->down;
		}

		pssn->next = junc_new();
	} else {
		pssn = *top;
		/*
		 * follow the branches as far as possible 
		 */
		while (*s) {
			if (*s == pssn->ch) {

				pssn->refc++;

				if (pssn->down) {
                    previous = pssn;
					pssn = pssn->down;
					s--;
				} else if (*--s) {
                    previous = pssn;
					pssn->down = ssn_new(*s);
					pssn = pssn->down;
					break;
				} else
					break;
			} else if (*s < pssn->ch) {
				if (pssn->left) {
                    previous = pssn;
					pssn = pssn->left;
                }
				else {
                    previous = 0;
					pssn->left = ssn_new(*s);
					pssn = pssn->left;
					break;
				}
			} else if (*s > pssn->ch) {
				if (pssn->right) {
                    previous = pssn;
					pssn = pssn->right;
                }
				else {
                    previous = 0;
					pssn->right = ssn_new(*s);
					pssn = pssn->right;
					break;
				}
			}
		}

		if (*s == 0) {
            /* I may want the previous one */
            if (previous) 
                previous->next = junc_new();
			else {
                if (pssn->next == 0)
                    pssn->next = junc_new();
            }
		} else {
			for (s--; s >= (unsigned char *) str; s--) {
				pssn->down = ssn_new(*s);
				pssn = pssn->down;
			}

			pssn->next = junc_new();
		}
	}

	return pssn->next;
}

/*
 * Will create a branch in necessary and returns the junc at the
 * end of the branch
 */
junc_t         *
ssn_insert(ssn_t ** top, char *str, int direction)
{
	if (direction == FORWARD)
		return ssn_insert_forward(top, str);
	else
		return ssn_insert_backward(top, str);
}

varr_t         *
ssn_match(ssn_t * pssn, char *sp, int direction)
{
	varr_t         *res = 0;
	unsigned char  *ucp;

	if (pssn == 0)
		return 0;

	if (direction == FORWARD)
		ucp = (unsigned char *) sp;
	else
		ucp = (unsigned char *) &sp[strlen(sp) - 1];

	while ((direction == FORWARD && *ucp) ||
	       (direction == BACKWARD && ucp >= (unsigned char *) sp)) {
		if (*ucp == pssn->ch) {
            if (pssn->refc == 0)
                return 0;
			/*
			 * have to collect the 'next hops' along the way 
			 * Could do this while storing the rule 
			 */
			if (pssn->next)
				res = varr_junc_add(res, pssn->next);

			if (pssn->down) {
				pssn = pssn->down;
				if (direction == FORWARD)
					ucp++;
				else
					ucp--;
			} else
				break;
		} else if (*ucp < pssn->ch) {
			if (pssn->left)
				pssn = pssn->left;
			else
				break;
		} else if (*ucp > pssn->ch) {
			if (pssn->right)
				pssn = pssn->right;
			else
				break;
		}
	}

	return res;
}

varr_t         *
ssn_lte_match(ssn_t * pssn, char *sp, int direction, varr_t * res)
{
	unsigned char  *ucp;
	ssn_t          *ps = 0;
	int             f = 0;

	if (direction == FORWARD)
		ucp = (unsigned char *) sp;
	else
		ucp = (unsigned char *) &sp[strlen(sp) - 1];

	while ((direction == FORWARD && *ucp) ||
	       (direction == BACKWARD && ucp >= (unsigned char *) sp)) {

		if (*ucp == pssn->ch) {
			ps = pssn;	/* to keep track of where I've been */

			if (pssn->down) {
				pssn = pssn->down;
				if (direction == FORWARD)
					ucp++;

				else
					ucp--;
			} else {
				f++;
				break;
			}
		} else if (*ucp < pssn->ch) {
			if (pssn->left)
				pssn = pssn->left;
			else {
				f++;
				break;
			}
		} else if (*ucp > pssn->ch) {
			if (pssn->right)
				pssn = pssn->right;
			else {
				f++;
				break;
			}
		}
	}

	/*
	 * have to get a grip on why I left the while-loop 
	 */
	if (f == 0) {		/* because I reached the 'end' of the string */
		/*
		 * get everything below and including this node 
		 */
		res = get_rec_all_ssn_followers(ps, res);
	}

	return res;
}

varr_t         *
get_rec_all_ssn_followers(ssn_t * ssn, varr_t * ja)
{
	if (ssn == 0)
		return ja;

	if (ssn->next)
		ja = varr_junc_add(ja, ssn->next);
	if (ssn->down)
		ja = get_rec_all_ssn_followers(ssn->down, ja);
	if (ssn->left)
		ja = get_rec_all_ssn_followers(ssn->left, ja);
	if (ssn->right)
		ja = get_rec_all_ssn_followers(ssn->right, ja);

	return ja;
}

varr_t         *
get_all_ssn_followers(branch_t * bp, int type, varr_t * ja)
{
	ssn_t          *ssn;

	if (type == SPOC_PREFIX)
		ssn = bp->val.prefix;
	else
		ssn = bp->val.suffix;

	return get_rec_all_ssn_followers(ssn, ja);
}

