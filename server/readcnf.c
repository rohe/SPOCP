#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <locl.h>

/*--------------------------------------------------------------------------------*/

spocp_chunk_t        *
get_object(spocp_charbuf_t * ib, spocp_chunk_t * head)
{
	spocp_chunk_t        *np = 0, *pp;
	char		c;

	if (head == 0)
		head = pp = get_chunk(ib);
	else
		for (pp = head; pp->next; pp = pp->next);

	if (pp == 0)
		return 0;

	/* It's a S-expression */
	if (*pp->val->val == '/' || oct2strcmp(pp->val, "(") == 0) {

		/* starts with a path specification */
		if( *pp->val->val == '/' )
			pp = chunk_add( pp, get_chunk( ib )) ;

		np = get_sexp(ib, pp);

		/*
		 * So do I have the whole S-expression ? 
		 */
		if (np == 0) {
			traceLog( "Error in rulefile" ) ;
			chunk_free(head);
			return 0;
		}

		for (; pp->next; pp = pp->next);

		c = charbuf_peek(ib);

		if( c == '=' ) {
			np = get_chunk(ib);
			if (oct2strcmp(np->val, "=>") == 0) { /* boundary condition */
				pp = chunk_add(pp, np);
	
				np = get_chunk(ib);

				if (oct2strcmp(np->val, "(") == 0) 
					pp = get_sexp( ib, chunk_add(pp, np));

				if (pp == 0) {
					traceLog( "Error in rulefile" ) ;
					chunk_free(head);
					return 0;
				}

				for (; pp->next; pp = pp->next);

				if( charbuf_peek(ib) == '=' ) 
					np = get_chunk(ib);
			}

			if (oct2strcmp(np->val, "==") == 0) { /* blob */
				pp = chunk_add(pp, np);
				pp = chunk_add( pp, get_chunk( ib ));
			}
		}
	} else if ( oct2strcmp(pp->val, ";include" ) == 0 ) {
		pp = chunk_add( pp, get_chunk( ib )) ;
	} else {	/* should be a boundary condition definition,
			 * has threeor more chunks
			 */
		pp = chunk_add( pp, get_chunk( ib ));
		if( pp && oct2strcmp(pp->val, ":=") == 0 ) {
			pp = chunk_add( pp, get_chunk( ib ));
			if( oct2strcmp( pp->val, "(" ) == 0 ) {
				pp = get_sexp( ib, pp);
			}
		}
		else {
			traceLog("Error in rulefile") ;
			chunk_free(head);
			return 0;
		}
	}

	if (pp)
		return head;
	else {
		chunk_free( head );
		return 0;
	}
}

/*--------------------------------------------------------------------------------*/

void ruledef_return( spocp_ruledef_t *rd, spocp_chunk_t *c )
{
	spocp_chunk_t *ck = c;
	int	p = 0;

	rd->rule = c ;

	do {
		if( oct2strcmp( ck->val, "(" ) == 0 )
			p++;
		else if( oct2strcmp( ck->val, ")" ) == 0 )
			p--;

		ck = ck->next;
	} while( p ) ;
 
        if( ck && oct2strcmp( ck->val, "=>") == 0 ) {
		ck = ck->next;
		rd->bcond = ck;
		do {
			if( oct2strcmp( ck->val, "(" ) == 0 )
				p++;
			else if( oct2strcmp( ck->val, ")" ) == 0 )
				p--;

			ck = ck->next;
		} while( p );
	}
	else
		rd->bcond = 0;

        if( ck && oct2strcmp( ck->val, "==") == 0 ) {
		rd->blob = ck->next;
	}
	else
		rd->blob = 0;
}


/*================================================================================*/
