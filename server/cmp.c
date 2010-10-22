#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <spocp.h>
#include <element.h>
#include <octet.h>
#include <varr.h>
#include <verify.h>
#include <wrappers.h>
#include <range.h>
#include <log.h>

/*
 *
 */

#ifdef USE_IPV6
int     ipv6cmp(struct in6_addr *ia1, struct in6_addr *ia2);
#endif
int     ipv4cmp(struct in_addr *ia1, struct in_addr *ia2);

int     elementcmp( element_t *a, element_t *b, octet_t *str );

extern int debug;

/*
 * ----------------------------------------------------------------------
 */

static int
atomcmp( atom_t *a, atom_t *b, octet_t *str )
{
	char *c,*s;
	int r = octcmp( &a->val, &b->val );
	
	if (debug) {
		c = oct2strdup( &a->val, '%' );
		s = oct2strdup( &b->val, '%' );
	
		printf("atomcmp: [%s][%s] => %d\n", c,s,r); 

		Free(c);
		Free(s);
	}
	if( str && r == 0 ) {
		octcat( str, a->val.val, a->val.len );
		octcat( str, " ", 1 );
	}

	return r;
}

static int
atom2boundarycmp( atom_t *a, boundary_t *b, int *dir)
{
	int		res = 0, v = 0;
	struct in_addr	ia4;
#ifdef USE_IPV6
	struct in6_addr	ia6;
#endif
	long		l;
	
	switch (b->type & 0x07) {
	case SPOC_ALPHA:
		v = octcmp(&a->val, &b->v.val);
		break;

	case SPOC_DATE:
		if (is_date( &a->val) == SPOCP_SUCCESS) {
			v = octcmp(&a->val, &b->v.val);
			res = 0;
		}
		break;

	case SPOC_TIME:
		if (is_time( &a->val) == SPOCP_SUCCESS) {
			if (is_numeric( &a->val, &l) == SPOCP_SUCCESS)
				v = l - b->v.num;

			res = 0;
		}
		break;

	case SPOC_NUMERIC:
		if (is_numeric( &a->val, &l) == SPOCP_SUCCESS) {
			v = l - b->v.num;
			res = 0;
		}
		break;

	case SPOC_IPV4:
		if (is_ipv4( &a->val, &ia4) == SPOCP_SUCCESS ) {
			v = ipv4cmp(&ia4, &b->v.v4);
			res = 0;
		}
		break;

#ifdef USE_IPV6
	case SPOC_IPV6:	/* Can be viewed as a array of unsigned ints */
		if (is_ipv6( &a->val, &ia6) == SPOCP_SUCCESS) {
			v = ipv6cmp(&ia6, &b->v.v6);
			res = 0;
		}
		break;
#endif

	default:
		res = 1;
		break;
	}

	if( res == 0 )
		*dir = v;

	return res;
}

static int
atom2rangecmp( atom_t *a, range_t *r )
{
	int d;

	if ( r->lower.type & BTYPE ) {
		if ( atom2boundarycmp( a, &r->lower, &d ) == 1 ) 
			return 1;
		else if ((r->lower.type & BTYPE) == (EQ|GT) && d >= 0 ) ;
		else if ((r->lower.type & BTYPE) == GT && d > 0 ) ;
		else 
			return 1;	
	}

	if( r->upper.type & BTYPE ) {
		if ( atom2boundarycmp( a, &r->upper, &d ) == 1 ) 
			return 1;
		else if ((r->upper.type & BTYPE) == (EQ|LT) && d <= 0);
		else if ((r->upper.type & BTYPE) == LT && d < 0 );
		else 
			return 1;	
	}

	return 0;
}

/* anything but 0 is failure */
static int
atom2prefixcmp( atom_t *a, atom_t *s)
{
	if( a->val.len >= s->val.len )
		return octncmp( &a->val, &s->val, s->val.len);
	else
		return 1;
}

static int
atom2suffixcmp( atom_t *a, atom_t *s)
{
	octet_t tmp;
	size_t n;

	if( a->val.len >= s->val.len ) {
		n = s->val.len - a->val.len;
		tmp.val = s->val.val + n;
		tmp.len = s->val.len - n;
		return octcmp( &tmp, &s->val);	
	}
	else
		return 1;
}

static int
suffixcmp( atom_t *a, atom_t *b)
{
	return octcmp( &a->val, &b->val );
}

static int
prefixcmp( atom_t *a, atom_t *b)
{
	return octcmp( &a->val, &b->val );
}

static int
rangecmp( range_t *a, range_t *b )
{
	int             r;
    spocp_result_t  rc;

    r = boundarycmp( &a->lower, &b->lower, &rc );
	r = boundarycmp( &a->lower, &b->upper, &rc ) ;

	return 0;
}

static int
element2setcmp( element_t *a, varr_t *s, octet_t *str)
{
	int i;

	for( i = 0 ; i < (int) s->n ; i++ )
		if (elementcmp( a, s->arr[i], str) == 0 ) return 0;

	return 1;	
}

static int
listcmp( list_t *la, list_t *lb, octet_t *str )
{
	element_t *a, *b;

	if (str) 
		octcat( str, "(", 1 );

	for( a = la->head, b = lb->head; a && b ; a = a->next, b = b->next) 
		if (elementcmp( a, b, str ) != 0) return 1;

	/* The b list is longer ? */
	if (b != NULL) return 1; 

	if (str) 
		octcat( str, ")", 1 );

	return 0;
}

int
elementcmp( element_t *a, element_t *b, octet_t *str )
{
	switch (a->type) {
	case SPOC_ATOM:
		switch ( b->type ) {
		case SPOC_ATOM: 
			return atomcmp( a->e.atom, b->e.atom, str);	

		case SPOC_PREFIX:
			return atom2prefixcmp( a->e.atom, b->e.atom);
			
		case SPOC_SUFFIX:
			return atom2suffixcmp( a->e.atom, b->e.atom);

		case SPOC_RANGE:
			return atom2rangecmp( a->e.atom, b->e.range);

		case SPOC_SET:
			return element2setcmp( a, b->e.set, str );

		default:
			return 1;

		}
		break;

	case SPOC_PREFIX:
		if (b->type == SPOC_PREFIX)
			return prefixcmp( a->e.atom, b->e.atom);
		break;

	case SPOC_SUFFIX:
		if (b->type == SPOC_SUFFIX)
			return suffixcmp( a->e.atom, b->e.atom);
		break;

	case SPOC_RANGE:
		if (b->type == SPOC_RANGE)
			return rangecmp( a->e.range, b->e.range);
		else if( b->type == SPOC_SET )
			return element2setcmp( a, b->e.set, str );
		break;

	case SPOC_LIST:
		if( b->type == SPOC_LIST)
			return listcmp( a->e.list, b->e.list, str);
		break;
	} 

	return 1;
}

/*
static int
list2setcmp( list_t *l, varr_t *s )
{
	return 0;
}
*/
