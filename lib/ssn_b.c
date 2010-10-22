/*
 *  ssn_b.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 12/9/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

#include <ssn.h>
#include <branch.h>
#include <ruleinfo.h>

#include <wrappers.h>

void
ssn_free(ssn_t * pssn)
{
	ssn_t          *down;
    
	while (pssn) {
		if (pssn->down) {
			/*
			 * there should not be anything to the left or right.
			 * No next either 
			 */
			down = pssn->down;
			Free(pssn);
			pssn = down;
		} else {
			if (pssn->next)
				junc_free(pssn->next);
			Free(pssn);
			pssn = 0;
		}
	}
}

/*!
 * \brief Deletes the part of a ssn struct that matches a character string
 * \param top The starting point of the ssn tree
 * \param sp The string that specifies which branch that should be removed
 * \param direction The direction in which the string was stored
 * \return A pointer to the junc_t struct found at the end of the branch.
 */
junc_t         *
ssn_delete(ssn_t ** top, char *sp, int direction)
{
	unsigned char   *up;
	ssn_t           *pssn = *top, *ssn, *prev = 0;
	junc_t          *jp;
    unsigned int    **refc;
    int             n=0;
    
    refc = (unsigned int **) Calloc(strlen(sp), sizeof(unsigned int *));
    
	if (direction == FORWARD)
		up = (unsigned char *) sp;
	else
		up = (unsigned char *) &sp[strlen(sp) - 1];
    
	while ((direction == FORWARD && *up) ||
	       (direction == BACKWARD && up >= (unsigned char *) sp)) {
		if (*up == pssn->ch) {
            
			refc[n++] = &pssn->refc;
            
			/*
			 * Only one linked list onward from here 
			 */
			if (pssn->refc == 0) {
				if (*top == pssn) {
					*top = 0;
					jp = 0;
					ssn_free(pssn);
				} else {
					/*
					 * First make sure noone will follow
					 * this track anymore 
					 */
					if (prev->down == pssn)
						prev->down = 0;
					else if (prev->left == pssn)
						prev->left = 0;
					else
						prev->right = 0;
                    
					/*
					 * follow down until the 'next' link
					 * is found 
					 */
					for (ssn = pssn; ssn->down;
					     ssn = ssn->down);
					jp = ssn->next;
                    
					ssn_free(pssn);
					junc_free(jp);
				}
                
                for (n--; n >= 0; n--) {
                    *refc[n] -= 1 ;
                }
                Free(refc);
				return 0;
			}
            
			if (pssn->down) {
				prev = pssn;	/* keep track on where we
                                 * where */
				pssn = pssn->down;
				if (direction == FORWARD)
					up++;
				else
					up--;
			} else
				break;
		} else if (*up < pssn->ch) {
			if (pssn->left) {
				prev = pssn;
				pssn = pssn->left;
			} else {
                Free(refc);
				return 0;
            }
		} else if (*up > pssn->ch) {
			if (pssn->right) {
				prev = pssn;
				pssn = pssn->right;
			} else {
                Free(refc);
				return 0 ;
            }
		}
	}

    for (n--; n >= 0; n--) {
        *refc[n] -= 1 ;
    }
    Free(refc);

	return pssn->next;
}

/*!
 * \brief Duplicates a ssn struct 
 * \param old The ssn struct that will be duplicated
 * \param ri Ruleinfo, will just be linked to
 * \return The copy 
 */
ssn_t          *
ssn_dup(ssn_t * old, ruleinfo_t * ri)
{
	ssn_t          *new;
    
	if (old == 0)
		return 0;
    
	new = ssn_new(old->ch);
    
	new->refc = old->refc;
    
	new->left = ssn_dup(old->left, ri);
	new->right = ssn_dup(old->right, ri);
	new->down = ssn_dup(old->down, ri);
	new->next = junc_dup(old->next, ri);
    
	return new;
}
