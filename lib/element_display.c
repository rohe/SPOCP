/*
 *  element_display.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/5/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */
#include <string.h>

#include <octet.h>
#include <element.h>
#include <range.h>
#include <free.h>
#include <wrappers.h>

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
element_range_print( octet_t *oct, element_t *ep)
{
    range_t     *r;
    
    if (!ep) return 1;
    
    if (octcat( oct, "(1:*5:range", 11 ) != SPOCP_SUCCESS)
        return 0;
    
    r = ep->e.range;
    
    if( r->lower.type & SPOC_NUMERIC)
        if( !range_print( oct, r))
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
