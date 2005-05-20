#include <stdlib.h>
#include <string.h>

#include <struct.h>
#include <spocp.h>
#include <func.h>
#include <rvapi.h>
#include <wrappers.h>

/*
 * ----------------------------------------------------------------------
 */

/* void element_reduce( element_t *ep ) ; */

/*
 * ----------------------------------------------------------------------
 */

static int
P_element_cmp( void *a, void *b )
{
    return element_cmp( (element_t *) a, (element_t *) b );
}

static void
P_element_free( void *a )
{
    element_free((element_t *) a );
}

/*
 * ----------------------------------------------------------------------
 */

static int
element_atom_print( octet_t *oct, element_t *ep)
{
    char    len[16];
    octet_t *av = &ep->e.atom->val;
    /* should check what the atom contains */

    if (!ep) return 1;

    sprintf( len, "%u:", (unsigned int) av->len );
    
    if (octcat( oct, len, strlen(len)) != SPOCP_SUCCESS)
        return 0;;
    if (octcat( oct, av->val, av->len) != SPOCP_SUCCESS)
        return 0;;

    return 1;
}

static int
element_list_print( octet_t *oct, element_t *ep)
{
    if (!ep) return 1;

    if (octcat( oct, "(", 1 ) != SPOCP_SUCCESS)
        return 0;

    for( ep = ep->e.list->head; ep ; ep = ep->next )
        if (element_print(oct, ep) == 0)
            return 0;   

    if (octcat( oct, ")", 1 ) != SPOCP_SUCCESS)
        return 0;

    return 1;
}

static int
element_set_print( octet_t *oct, element_t *ep)
{
    int     i;
    varr_t      *va = ep->e.set;
    element_t   *vep;

    if (!ep) return 1;

    if (octcat( oct, "(1:*3:set", 9 ) != SPOCP_SUCCESS)
        return 0;

    if (va) {
        for( i = 0 ; i < (int)va->n ; i++ ) {
            vep = ( element_t * ) va->arr[i];
            if (element_print(oct, vep) == 0)
                return 0;   
        }
    }

    if (octcat( oct, ")", 1 ) != SPOCP_SUCCESS)
        return 0;

    return 1;
}

static int 
numeric_boundary_print( octet_t *oct, boundary_t *b)
{
    char    num[64], spec[64];
    int     type;

    type = b->type - SPOC_NUMERIC;

    if (!type) 
            return 1;

    sprintf( num,"%ld", b->v.num );

    switch (type){
    case (GT|EQ):
        sprintf( spec, "2:ge%u:%s", (unsigned int) strlen(num), num);
        break;
    case GT:
        sprintf( spec, "2:gt%u:%s", (unsigned int) strlen(num), num);
        break;
    case LT|EQ:
        sprintf( spec, "2:le%u:%s", (unsigned int) strlen(num), num);
        break;
    case LT:
        sprintf( spec, "2:lt%u:%s", (unsigned int) strlen(num), num);
        break;
    }

    if( octcat( oct, spec, strlen(spec)) != SPOCP_SUCCESS)
        return 0; 

    return 1;
}

static int
range_numeric_print( octet_t *oct, range_t *r)
{
    if( numeric_boundary_print( oct, &r->lower) == 0)
            return 0;
    if( numeric_boundary_print( oct, &r->upper) == 0)
            return 0;

    return 1;
}

static int
element_range_print( octet_t *oct, element_t *ep)
{
    range_t     *r;

    if (!ep) return 1;

    if (octcat( oct, "(1:*5:range", 11 ) != SPOCP_SUCCESS)
        return 0;

    r = ep->e.range;

    if( r->lower.type & SPOC_NUMERIC)
            if( !range_numeric_print( oct, r))
                    return 0;

    if (octcat( oct, ")", 1 ) != SPOCP_SUCCESS)
        return 0;

    return 1;
}

int
element_print( octet_t *oct, element_t *ep)
{
    int r = 1;

    if (!ep) return 1;

    switch( ep->type ) {
    case SPOC_ATOM:
        r = element_atom_print( oct, ep);
        break;
    case SPOC_LIST:
        r = element_list_print( oct, ep);
        break;
    case SPOC_SET:
        r = element_set_print( oct, ep);
        break;
    case SPOC_RANGE:
        r = element_range_print( oct, ep);
        break;
    default:
        r = 0;
        break;
    }

    return r;
}

static void
element_list_struct( octet_t *oct, element_t *ep)
{
    char        line[128];
    if (!ep) return ;

    sprintf(line,"[%p,parent=%p,next=%p](", (void*)ep, 
        (void*)ep->memberof,(void *)ep->next);
    octcat( oct, line, strlen(line));

    for( ep = ep->e.list->head; ep ; ep = ep->next )
        element_struct(oct, ep);

    octcat( oct, ")", 1 );
}

static void
element_set_struct( octet_t *oct, element_t *ep)
{
    int     i;
    varr_t      *va = ep->e.set;
    element_t   *vep;
    char        line[128];

    if (!ep) return ;

    sprintf(line,"[%p,parent=%p,next=%p](set", (void *)ep,
            (void *)ep->memberof,(void *)ep->next);

    octcat( oct, line, strlen(line));

    if (va) {
        for( i = 0 ; i < (int)va->n ; i++ ) {
            vep = ( element_t * ) va->arr[i];
            element_struct(oct, vep);
        }
    }

    octcat( oct, ")", 1 );
}

void
element_struct( octet_t *oct, element_t *ep)
{
    char        line[256], *tmp;

    if (!ep) return;

    switch( ep->type ) {
    case SPOC_ATOM:
        tmp = oct2strdup(&ep->e.atom->val, '\\');
        sprintf(line,"[%p,parent=%p,next=%p,val=%s]",
            (void *)ep,(void *)ep->memberof,(void *)ep->next, tmp);
        octcat(oct,line,strlen(line));
        Free(tmp);
        break;
    case SPOC_LIST:
        element_list_struct( oct, ep);
        break;
    case SPOC_SET:
        element_set_struct( oct, ep);
        break;
    default:
        octcat(oct,"B",1);
        break;
    }
}


char *
element2str( element_t *e )
{
    octet_t *o;
    char *tmp;

    o = oct_new( 512, NULL);
    element_print( o, e );
    tmp = oct2strdup(o, 0);
    oct_free(o);

    return tmp;
}

/*
 * ----------------------------------------------------------------------
 */

static void
element_set_reduce( element_t *ep )
{
    varr_t      *va;
    size_t      i;
    element_t   *te;
#ifdef XYDEBUG
    octet_t     *op;
    char        *tmp;

    op = oct_new( 512, NULL);
    element_print( op, ep );
    tmp = oct2strdup( op, 0 );
    traceLog(LOG_DEBUG,"Reducing: [%s]", tmp );
    Free( tmp );
#endif

    varr_rm_dup( ep->e.set, P_element_cmp, P_element_free ); 

#ifdef XYDEBUG
    op->len = 0;
    element_print( op, ep );
    tmp = oct2strdup( op, 0 );
    traceLog(LOG_DEBUG, "1:st Reduction to [%s]", tmp);
    Free(tmp);
#endif

    va = ep->e.set;
    if( va->n > 1 ) {
        for( i = 0; i < va->n; i++) {
            if( va->arr[i] == 0 ) continue;

            te = (element_t *) va->arr[i];
            va->arr[i] = element_reduce(te);
        }
    }

#ifdef XYDEBUG
    op->len = 0;
    element_print( op, ep );
    tmp = oct2strdup( op, 0 );
    traceLog(LOG_DEBUG, "2:nd Reduction to [%s]", tmp);
    Free(tmp);
    oct_free( op );
#endif

}

element_t *
element_reduce( element_t *ep )
{
    element_t   *nep, *te, *prev, *next;

    if (ep == 0)
        return NULL;

    switch( ep->type ){
    case SPOC_LIST:
        prev = 0;
        for( ep = ep->e.list->head; ep ; ep = ep->next ) {
            next = ep->next;
            nep = element_reduce( ep );
            if (ep != nep) {
                if( prev )
                    prev->next = nep;
                nep->next = next;
                ep = nep;
            }
            prev = ep;
        }
        break;

    case SPOC_SET:
        element_set_reduce( ep );
        if( ep->e.set->n == 1 ) {
            te = (element_t *) varr_pop( ep->e.set );
            element_free( ep );
            ep = te;
        }
        break;
    }

    return ep;
}

