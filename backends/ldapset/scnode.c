#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <spocp.h>
#include <wrappers.h>
#include <func.h>
#include <proto.h>
#include <ldapset.h>


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

static scnode_t	*
scnode_new(int size)
{
	scnode_t	*scp;

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

void
scnode_print( scnode_t *sc, char *in )
{
	int n;

	n = strlen(in);
	in[n] = ' ';

	printf("%s%d:%s\n",in,sc->size, sc->str);

	if( sc->left ) {
		printf("%s->\n", in);
		scnode_print( sc->left, in);
	}
	if( sc->right ) {
		printf("%s<-\n", in);
		scnode_print( sc->right, in);
	}
	in[n] = '\0';
}

static scnode_t	*
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
scnode_t	*
scnode_get(octet_t * op, spocp_result_t * rc)
{
	char		*cp, *sp, c, in[32];
	scnode_t	*psc = 0, *nsc;
	int	 	d, striprdn=0;

	*rc = 0;
	memset( in, 0, 32 );

	if (op == 0 || op->len == 0)
		return 0;

	for( sp = op->val ; *sp == ' '; sp++) ;

	for ( ; *sp && (*sp != '}' && *sp != ')') ;) {
		switch (*sp) {
		case '&':
		case '|':
			nsc = scnode_new(2);
			*(nsc->str) = *sp;
			psc = tree_top(psc);
			nsc->right = psc;
			psc->up = nsc;
			psc = nsc;
			sp++;

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
			++sp;
			psc = tree_top(psc);

			nsc->right = psc;
			psc->up = nsc;
			psc = nsc;

			break;

		case '(':
		case '{':
			nsc = scnode_new(3);
			nsc->str[0] = *sp;
			if (*sp == '(')
				nsc->str[1] = ')';
			else {
				nsc->str[1] = '}';

				cp = find_balancing( sp+1, '{', '}' );
#ifdef AVLUS
				traceLog(LOG_DEBUG,"Find balancin in: %s",sp+1);
				traceLog(LOG_DEBUG,"Find balancin out: %s",cp);
#endif
				if (cp == NULL) {
					*rc = SC_SYNTAXERROR;
					return 0;
				}
				striprdn = 0;
				cp++;
			
				while( *cp == '.' && *(cp+1) == '.' ) {
					striprdn++;
					cp += 2;
					if (*cp == '/' || *cp == '$' \
						|| *cp == '%') {
						cp++;
					}	
				}
#ifdef AVLUS
				traceLog(LOG_DEBUG,"striprdn: %d",striprdn);
#endif
				nsc->striprdn = striprdn;
			}

			sp++;
			while(*sp == ' ')
				sp++;

			op->len -= (sp - op->val);
			op->val = sp;
			nsc->right = scnode_get(op, rc);
			if (*rc) {
				scnode_free(nsc);
				scnode_free(tree_top(psc));
				return 0;
			}

			sp = op->val;
#ifdef AVLUS
			traceLog(LOG_DEBUG,"\"%s\"", sp);
#endif
			if ( *sp == '.' && *(sp+1) == '.' ) {
				sp += 2;
			
				while ( *sp == '/' && *(sp+1) == '.' \
					&& *(sp+2) == '.') {
					sp += 3;
				}
			}
#ifdef AVLUS
			traceLog(LOG_DEBUG,"skipped to \"%s\"", sp );
#endif

			if (psc) {
				psc->left = nsc;
				nsc->up = psc;
			} else
				psc = nsc;

			break;

		case '\\':
			nsc = scnode_new(3);
			nsc->str[0] = *sp;
			nsc->str[1] = *++sp;

			if (psc) {
				psc->left = nsc;
				nsc->up = psc;
			} else
				psc = nsc;

			++sp;
			break;
			
		case '"':
		case '<':
		default:
			c = *sp;
			d = 0;

			/*
			* NULL Strings, not allowed 
			*/
			if ( *(sp + 1) == c ) {
				scnode_free(tree_top(psc));
				*rc = SC_SYNTAXERROR;
				return 0;
			}

			/* go to end marker */
			if(c == '"'||c =='<'){
				for (cp = ++sp ; *cp && (*cp != c) ; cp++);
				if (*cp == '\0') {
					scnode_free(tree_top(psc));
					*rc = SC_SYNTAXERROR;
					return 0;
				}
				cp++;
				d = 1;
			} else {
#ifdef AVLUS
				traceLog(LOG_DEBUG, "Just a string: %s", sp);
#endif
				for (cp = sp; *cp && !SC_SPEC(*cp); cp++);
#ifdef AVLUS
				traceLog(LOG_DEBUG,"-[%s]-", cp );
#endif
			}

			nsc = scnode_new(cp - sp + 1);
			strncpy(nsc->str, sp, cp - sp - d);

			if (psc) {
				psc->left = nsc;
				nsc->up = psc;
			} else
				psc = nsc;

			sp = cp;
			break;
		}
		while (*sp == ' ')
			sp++;
	}

	if( *sp == '}' || *sp == ')' ) 
		sp++;

	op->len -= (sp - op->val);
	op->val = sp;

	return tree_top(psc);
}

