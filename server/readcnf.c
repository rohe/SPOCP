#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <locl.h>

/*
#define ASCII_LOWER(c) ( (c) >= 'a' && (c) <= 'z' )
#define ASCII_UPPER(c) ( (c) >= 'A' && (c) <= 'Z' )
#define DIGIT(c)       ( (c) >= '0' && (c) <= '9' )
#define ASCII(c)       ( ASCII_LOWER(c) || ASCII_UPPER(c) )
#define HEX_LOWER(c)   ( (c) >= 'a' && (c) <= 'f' )
#define HEX_UPPER(c)   ( (c) >= 'A' && (c) <= 'F' )
#define HEX(c)         ( DIGIT(c) || HEX_LOWER(c) || HEX_UPPER(c) )
#define ALPHA(c)       ( ASCII(c) || DIGIT(c) )
#define WHITESPACE(c)  ( (c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n' )
#define S1(c)          ( (c) == '-' || (c) == '.' || (c) == '_' || (c) == ':' || (c) == '*' )
#define SIMPLE(c)      ( S0(c) || S1(c) )
#define TOKEN(c)       ( ALPHA(c) || SIMPLE(c) )
*/

#define S0(c)          ( (c) == '+' || (c) == '/' || (c) == '=' )
#define BASE64(c)      ( ALPHA(c) || S0(c) )

#define PR0(c)		((c) == '!' || (c) == '#' || (c) == '$' )
#define PR1(c)		((c) == '&' || (c) == '\'')
#define PR2(c)		((c) >= '*' && (c) <= '.') 
#define PR5(c)		((c) >= '0' && (c) <= '[') 
#define PR3(c)		((c) >= ']' && (c) <= '{')
#define PR4(c)		((c) == '}' || (c) == '~')
#define VCHAR(c)	(PR1(c) || PR0(c) || PR2(c) || PR3(c) || PR4(c) || PR5(c)) 

#define DIGITS(n) ( (n) >= 100000 ? 6 : (n) >= 10000 ? 5 : (n) >= 1000 ? 4 : (n) >= 100 ? 3 : ( (n) >= 10 ? 2 : 1 ) )

static char     base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*--------------------------------------------------------------------------------*/

static spocp_chunk_t        *
chunk_new(octet_t * o)
{
	spocp_chunk_t        *cp;

	cp = (spocp_chunk_t *) Malloc(sizeof(spocp_chunk_t));
	cp->val = o;
	cp->prev = cp->next = 0;

	return cp;
}

void
chunk_free(spocp_chunk_t * c)
{
	if (c) {
		if (c->val)
			oct_free(c->val);
		if (c->next)
			chunk_free(c->next);
		free(c);
	}
}

/*--------------------------------------------------------------------------------*/

static int
pos(char c)
{
	char           *p;

	for (p = base64; *p; p++)
		if (*p == c)
			return p - base64;

	return -1;
}

static int
base64_decode(char *str)
{
	char           *p, *q;
	int             i, x;
	unsigned int    c;
	int             done = 0;

	for (p = str, q = str; *p && !done; p += 4) {

		for (c = 0, done = 0, i = 0; i < 4; i++) {
			if (i > 1 && p[i] == '=')
				done++;
			else {
				if (done)
					return -1;
				x = pos(p[i]);

				if (x >= 0)
					c += x;
				else
					return -1;

				if (i < 3)
					c <<= 6;
			}
		}

		if (done < 3)
			*q++ = (c & 0x00ff0000) >> 16;
		if (done < 2)
			*q++ = (c & 0x0000ff00) >> 8;
		if (done < 1)
			*q++ = (c & 0x000000ff) >> 0;
	}

	return q - str;
}

/*--------------------------------------------------------------------------------*/

char    *
get_more(spocp_charbuf_t * io)
{
	char           *r;

	do {
		r = fgets(io->str, io->size, io->fp);
	} while (r && *r == '#');

	if( r ) io->start = io->str;

	return r;
}

static char    *
mycpy(char *res, char *start, int len, int sofar)
{
	char           *tmp;

	if (res == 0) {
		res = (char *) malloc((len + 1) * sizeof(char));
		strncpy(res, start, len);
		res[len] = 0;
	} else {
		tmp = (char *) realloc(res, (sofar + len + 1) * sizeof(char));
		res = tmp;
		strncpy(&res[sofar], start, len);
		sofar += len;
		res[sofar] = 0;
	}

	return res;
}

static int
rm_sp(char *str, int len)
{
	char           *rp, *cp;
	int             i;

	for (rp = cp = str, i = 0; i < len; i++, rp++) {
		if (WHITESPACE(*rp))
			continue;
		else
			*cp++ = *rp;
	}

	return cp - str;
}

/*--------------------------------------------------------------------------------*/

static int de_escape( char *str, int len )
{
  octet_t tmp ;

  tmp.val = str ;
  tmp.len = len ;

  oct_de_escape( &tmp ) ;

  return tmp.len ;
}

/*--------------------------------------------------------------------------------*/

static octet_t *
get_token(spocp_charbuf_t * io)
{
	char           *cp, *res = 0;
	int             len = 0, sofar = 0;
	octet_t		*oct = 0 ;

	do {
		for (cp = io->start; *cp && VCHAR(*cp); cp++);

		if (*cp == 0) {
			len = cp - io->start;
			res = mycpy(res, io->start, len, sofar);

			if (get_more(io) == 0)
				return 0;
		} else
			break;
	} while (1);

	if (cp != io->start) {	/* never got off the ground */
		len = cp - io->start;
		res = mycpy(res, io->start, len, sofar);
		sofar += len;
		io->start = cp;
	}

	oct = oct_new( 0, 0 ) ;
	oct_assign( oct, res ) ;


	return oct;
}

/*--------------------------------------------------------------------------------*/

static octet_t *
get_path(spocp_charbuf_t * io)
{
	char           *cp, *res = 0;
	int             len = 0, sofar = 0;
	octet_t		*oct = 0 ;

	do {
		for (cp = io->start; *cp && (ALPHA(*cp) || *cp == '/'); cp++);

		if (*cp == 0) {
			len = cp - io->start;
			res = mycpy(res, io->start, len, sofar);

			if (get_more(io) == 0)
				return 0;
		} else
			break;
	} while (1);

	if (cp != io->start) {	/* never got off the ground */
		len = cp - io->start;
		res = mycpy(res, io->start, len, sofar);
		sofar += len;
		io->start = cp;
	}

	oct = oct_new( 0, 0 ) ;
	oct_assign( oct, res ) ;


	return oct;
}

/*--------------------------------------------------------------------------------*/

static octet_t *
get_base64( spocp_charbuf_t * io)
{
	char           *cp, *res = 0;
	int             len, sofar = 0, done = 0;
	octet_t		*oct = 0 ;

	do {
		for (cp = io->start; *cp && (BASE64(*cp) || WHITESPACE(*cp));
		     cp++);
		if (*cp && *cp == '|')
			done = 1;

		if (*cp == 0) {
			len = cp - io->start;
			res = mycpy(res, io->start, len, sofar);
			sofar += len;
			if (get_more(io) == 0)
				return 0;
		} else
			break;
	} while (1);

	if (cp != io->start) {	/* never got off the ground */
		len = cp - io->start;
		res = mycpy(res, io->start, len, sofar);
		sofar += len;
		io->start = ++cp;
	}

	len = sofar;

	if (res) {
		/*
		 * if there are any whitespace chars remove them 
		 */
		sofar = rm_sp(res, sofar);
		/*
		 * de base64 
		 */

		sofar = base64_decode(res);
		res[sofar] = 0;
	}


	oct = oct_new( 0, 0 ) ;
	oct_assign( oct, res ) ;

	return oct;
}

static octet_t *
get_hex( spocp_charbuf_t * io)
{
	char           *cp, *res = 0, *rp;
	unsigned char   c;
	int             len, sofar = 0, i;
	octet_t 	* oct;

	do {
		for (cp = io->start; *cp && (HEX(*cp) || WHITESPACE(*cp));
		     cp++);
		if (*cp && *cp == '%')
			break;

		if (*cp == 0) {
			len = cp - io->start;
			res = mycpy(res, io->start, len, sofar);
			sofar += len;
			if (get_more(io) == 0)
				return 0;
		} else
			break;
	} while (1);

	if (cp != io->start) {
		len = cp - io->start;
		res = mycpy(res, io->start, len, sofar);
		sofar += len;
		io->start = ++cp;
	}

	len = sofar;

	if (res) {
		/*
		 * if there are any whitespace chars remove them 
		 */
		sofar = rm_sp(res, sofar);

		/*
		 * convert hex hex to byte 
		 */
		for (rp = cp = res, i = 0; i < sofar; rp++, cp++) {
			if (*rp >= 'a' && *rp <= 'f')
				c = (*rp - 0x57) << 4;
			else if (*rp >= 'A' && *rp <= 'F')
				c = (*rp - 0x37) << 4;
			else if (*rp >= '0' && *rp <= '9')
				c = (*rp - 0x30) << 4;
			else
				return 0;

			rp++;

			if (*rp >= 'a' && *rp <= 'f')
				c += (*rp - 0x57);
			else if (*rp >= 'A' && *rp <= 'F')
				c += (*rp - 0x37);
			else if (*rp >= '0' && *rp <= '9')
				c += (*rp - 0x30);
			else
				return 0;

			*cp = c;
			i += 2;
		}

		sofar = cp - res;
	}

	if (res == 0)
		return 0;

	oct = oct_new( 0, 0 ) ;
	oct_assign( oct, res ) ;

	return oct;
}

#define HEXCHAR 1
#define SLASH   2
#define HEXDUO  4

static octet_t *
get_quote( spocp_charbuf_t * io)
{
	char           *cp, *res = 0;
	int             len, sofar = 0;
	int             expect = 0, done = 0;
	octet_t 	*oct;

	do {
		for (cp = io->start; *cp; cp++) {
			if (expect) {
				if (expect == HEXCHAR || expect == HEXDUO) {
					if (HEX(*cp)) {
						if (expect == HEXCHAR)
							expect = 0;
						else
							expect = HEXCHAR;
					} else
						return 0;
				} else {
					if (*cp == '\\')
						expect = 0;
					else if (HEX(*cp))
						expect = HEXCHAR;
					else
						return 0;
				}
				continue;
			}

			if (*cp == '\\')
				expect = HEXDUO | SLASH;
			else if (*cp == '"')
				break;
		}

		if (*cp == 0) {
			len = cp - io->start;
			res = mycpy(res, io->start, len, sofar);
			sofar += len;
			if (get_more(io) == 0)
				return 0;
		} else
			break;
	} while (!done);

	if (cp != io->start) {
		len = cp - io->start;
		res = mycpy(res, io->start, len, sofar);
		sofar += len;
		io->start = ++cp;
	}

	if (res) {
		/*
			len = sofar;
			sofar = rm_sp(res, sofar);
		*/

		/*
		 * de escape 
		 */
		sofar = de_escape(res, sofar);
	}

	if (res == 0)
		return 0;	/* not allowed */

	oct = oct_new( 0, 0 ) ;
	oct_assign( oct, res ) ;


	return oct;
}

static int
skip_whites(spocp_charbuf_t * io)
{
	char           *bp;

	bp = io->start;

	do {
		while (WHITESPACE(*bp) && *bp)
			bp++;

		if (*bp == 0) {
			if (get_more(io) == 0)
				return 0;
			bp = io->start;
		} else
			break;

	} while (1);

	io->start = bp;

	return 1;
}

/*--------------------------------------------------------------------------------*/

static char
charbuf_peek( spocp_charbuf_t *cb )
{
	if (skip_whites(cb) == 0)
		return 0;

	return( *cb->start ) ;
}

/*--------------------------------------------------------------------------------*/

static spocp_chunk_t        *
get_chunk(spocp_charbuf_t * io)
{
	octet_t        *o = 0;
	spocp_chunk_t        *cp = 0;

	if (skip_whites(io) == 0)
		return 0;

	/*
	 * what kind of string or list do I have here 
	 */

	switch (*io->start) {

	case '(':
	case ')':
		o = oct_new(1, io->start);
		io->start++;
		break;

	case '/':
		o = get_path(io);
		break;

	case '|':
		io->start++;
		o = get_base64(io);
		break;

	case '%':
		io->start++;
		o = get_hex(io);
		break;

	case '"':
		io->start++;
		o = get_quote(io);
		break;

	default:
		if (VCHAR(*io->start))
			o = get_token(io);
		else
			return NULL;
		break;
	}

	if (o)
		cp = chunk_new(o);

	return cp;
}

/*--------------------------------------------------------------------------------*/

static spocp_chunk_t		*
chunk_add( spocp_chunk_t *pp, spocp_chunk_t *np )
{
	pp->next = np;
	if (np)
		np->prev = pp;

	return pp->next ;
}

/*--------------------------------------------------------------------------------*/

static spocp_chunk_t		*
get_sexp( spocp_charbuf_t *ib, spocp_chunk_t *pp )
{
	int	par = 1;
	spocp_chunk_t *np ;

	while (par) {
		if ((np = get_chunk(ib)) == 0)
			break;

		pp = chunk_add(pp, np);

		if (oct2strcmp(np->val, "(") == 0)
			par++;
		else if (oct2strcmp(np->val, ")") == 0)
			par--;
	}

	return np ;
}

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
	} else {	/* should be a boundary condition definition, has three chunks */
		pp = chunk_add( pp, get_chunk( ib ));
		if( pp && oct2strcmp(pp->val, ":=") == 0 ) 
			pp = chunk_add( pp, get_chunk( ib ));
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


char *
chunk2sexp( spocp_chunk_t *c )
{
	int	p = 0, len = 0 ;
	spocp_chunk_t	*ck = c ;
	char	*res = 0, *sp, lf[16] ;

	do {
		if( oct2strcmp( ck->val, "(" ) == 0 )
			len++, p++;
		else if( oct2strcmp( ck->val, ")" ) == 0 )
			len++, p--;
		else {
			len += ck->val->len + DIGITS(ck->val->len) + 1 ;	
		}
		ck = ck->next ;
	} while( p ) ;
 
	sp = res = (char *) malloc (len+1) ;

	ck = c ;
	do {
		if( oct2strcmp( ck->val, "(" ) == 0 )
			p++, *sp++ = '(' ;
		else if( oct2strcmp( ck->val, ")" ) == 0 )
			p--, *sp++ = ')' ;
		else {
			sprintf( lf, "%d:", ck->val->len ) ;
			memcpy( sp, lf, strlen(lf)) ;
			sp += strlen( lf ) ;
			memcpy( sp, ck->val->val, ck->val->len ) ;
			sp += ck->val->len ;
		}
		ck = ck->next ;
	} while( p ) ;
 
	*sp = '\0' ;

	return res;
}

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
