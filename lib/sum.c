#include <struct.h> 
#include <func.h>
#include <wrappers.h>


element_t *
get_indexes( junc_t *jp )
{
	element_t	*ea = 0, *res = 0, *ep = 0, *te;
	index_t		*id;
	octet_t		oct;
	int		i = 0, j = 0;
	varr_t		*pa = 0, *earr = 0;

	if (jp->item[SPOC_ENDOFRULE]) {
		id = jp->item[SPOC_ENDOFRULE]->val.id ;
		if (id->n) {
			oct.len = SHA1HASHLEN;
			oct.val = id->arr[0]->uid;
			oct.size = 0;
			ea = element_new_atom( &oct );
		}
		else if (id->n > 1 ) {
			earr = 0;
			for( i = 0; i < id->n ; i++ ) {
				oct.len = SHA1HASHLEN;
				oct.val = id->arr[0]->uid;
				oct.size = 0;
				ep = element_new_atom( &oct );
				earr = varr_add( earr, ep );
			}
			ea = element_new_set( earr );
			earr = 0;
		}
	}

	/*
	 * --------------------------------------------------------------- 
	 */

	if (jp->item[SPOC_LIST]) {
		te = get_indexes(jp->item[SPOC_LIST]->val.list);
		if (te) 
			earr = varr_add( earr, te);
	}

	if (jp->item[SPOC_ENDOFLIST]) {
		te = get_indexes(jp->item[SPOC_ENDOFLIST]->val.list);
		if (te) 
			earr = varr_add( earr, te);
	}

	/*
	 * --------------------------------------------------------------- 
	 */

	if (jp->item[SPOC_ATOM])
		pa = get_all_atom_followers(jp->item[SPOC_ATOM], pa);

	if (jp->item[SPOC_PREFIX])
		pa = get_all_ssn_followers(jp->item[SPOC_PREFIX], SPOC_PREFIX, pa);

	if (jp->item[SPOC_SUFFIX])
		pa = get_all_ssn_followers(jp->item[SPOC_SUFFIX], SPOC_SUFFIX, pa);

	if (jp->item[SPOC_RANGE])
		pa = get_all_range_followers(jp->item[SPOC_RANGE], pa);

	/* Do next level */
	if( pa ) {
		for( j = 0 ; j < (int)pa->n ; j++ ) {
			te = get_indexes( varr_nth( pa, j ));
			if (te != NULL)
				earr = varr_add( earr, (void *) te );
		}
	}

	/*
	 * --------------------------------------------------------------- 
	 */

	/* gather all that is to be returned */
	if( earr ) {
		if (earr->n > 1 ) {
			ep = element_new_set( NULL );
			ep->e.set = earr;
		}
		else {
			ep = (element_t *) varr_pop( earr);
			varr_free( earr );
		}
	}

	/*
	 * --------------------------------------------------------------- 
	 */

	if( ea && ep ) {
		res = element_new_list( ep );
		element_list_add( res, ea );
	}
	else if (ea)
		res = ea;
	else if (ep)
		res = ep;

	return res;
}

