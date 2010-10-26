#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <locl.h>

#define RC_SPY 0

/*--------------------------------------------------------------------------------*/

void print_chunks(spocp_chunk_t *pp, char *label)
{
    char            *pps;

    pps = chunks2str(pp);
    printf("CHUNKS [%s]: %s\n", label, pps );
    Free(pps); 
}
/*--------------------------------------------------------------------------------*/

spocp_chunkwrap_t        *
get_object(spocp_charbuf_t * ib, spocp_chunk_t * head)
{
    spocp_chunk_t       *np = 0, *pp;
    char                c, *pps;
    spocp_chunkwrap_t   *result;

    result = (spocp_chunkwrap_t *) Calloc(1, sizeof(spocp_chunkwrap_t));
    
    if (head == 0)
        head = pp = get_chunk(ib);
    else
        for (pp = head; pp->next; pp = pp->next);

    if (pp == 0)
        return result;

    if(RC_SPY)
        print_chunks(pp, "");
    
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
            traceLog(LOG_ERR, "Error in rulefile - faulty s-expression" ) ;
            chunk_free(head);
            result->status = -1;
            return result;
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
                    traceLog(LOG_ERR, "Error in rulefile on ref def" ) ;
                    chunk_free(head);
                    result->status = -1;
                    return result;
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
    } else {    /* should be a boundary condition definition,
             * has three or more chunks
             */
        pp = chunk_add( pp, get_chunk( ib ));
        if( pp && oct2strcmp(pp->val, ":=") == 0 ) {
            pp = chunk_add( pp, get_chunk( ib ));
            
            if(RC_SPY)
                print_chunks(pp, "bc");
            
            if( pp && oct2strcmp( pp->val, "(" ) == 0 ) {
                pp = get_sexp( ib, pp);
            }
            else if( pp == 0) {
                traceLog(LOG_ERR,"Parse error at \"%s\"", ib->start);
                chunk_free(head);
                result->status = -1;
                return result;
            }
        }
        else {
            traceLog(LOG_ERR,"Error in rulefile on boundary condition") ;
            chunk_free(head);
            result->status = -1;
            return result;
        }
    }

    if (pp) {
        if(RC_SPY)
            print_chunks(head, "result");
        
        result->status = 1;
        result->chunk = head;
        return result;
    }
    else {
        chunk_free( head );
        return result;
    }
}

/*--------------------------------------------------------------------------------*/

void ruledef_return( spocp_ruledef_t *rd, spocp_chunk_t *c )
{
    spocp_chunk_t *ck = c;
    int p = 0;

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
