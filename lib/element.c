#include <stdlib.h>
#include <string.h>

#include <struct.h>
#include <spocp.h>
#include <func.h>
#include <rvapi.h>

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
	char	len[16];
	octet_t	*av = &ep->e.atom->val;
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
	int		i;
	varr_t		*va = ep->e.set;
	element_t	*vep;

	if (!ep) return 1;

	if (octcat( oct, "(1:*3:set", 9 ) != SPOCP_SUCCESS)
		return 0;

	for( i = 0 ; i < (int)va->n ; i++ ) {
		vep = ( element_t * ) va->arr[i];
		if (element_print(oct, vep) == 0)
			return 0;	
	}

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
	default:
		r = 0;
		break;
	}

	return r;
}

/*
 * ----------------------------------------------------------------------
 */

static void
element_set_reduce( element_t *ep )
{
	varr_t		*va;
	size_t		i;
	element_t	*te;
#ifdef XYDEBUG
	octet_t 	*op;
	char		*tmp;

	op = oct_new( 512, NULL);
	element_print( op, ep );
	tmp = oct2strdup( op, 0 );
	traceLog(LOG_DEBUG,"Reducing: [%s]", tmp );
	free( tmp );
#endif

	varr_rm_dup( ep->e.set, P_element_cmp, P_element_free ); 

#ifdef XYDEBUG
	op->len = 0;
	element_print( op, ep );
	tmp = oct2strdup( op, 0 );
	traceLog(LOG_DEBUG, "1:st Reduction to [%s]", tmp);
	free(tmp);
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
	free(tmp);
	oct_free( op );
#endif

}

element_t *
element_reduce( element_t *ep )
{
	element_t	*nep, *te, *prev, *next;

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

