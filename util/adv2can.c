#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASCII_LOWER(c) ( (c) >= 'a' && (c) <= 'z' )
#define ASCII_UPPER(c) ( (c) >= 'A' && (c) <= 'Z' )
#define DIGIT(c)       ( (c) >= '0' && (c) <= '9' )
#define ASCII(c)       ( ASCII_LOWER(c) || ASCII_UPPER(c) )
#define HEX_LOWER(c)   ( (c) >= 'a' && (c) <= 'f' )
#define HEX_UPPER(c)   ( (c) >= 'A' && (c) <= 'F' )
#define HEX(c)         ( DIGIT(c) || HEX_LOWER(c) || HEX_UPPER(c) )
#define ALPHA(c)       ( ASCII(c) || DIGIT(c) )
#define WHITESPACE(c)  ( (c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n' )
#define S0(c)          ( (c) == '+' || (c) == '/' || (c) == '=' )
#define BASE64(c)      ( ALPHA(c) || S0(c) )
#define S1(c)          ( (c) == '-' || (c) == '.' || (c) == '_' || (c) == ':' || (c) == '*' )
#define SIMPLE(c)      ( S0(c) || S1(c) )
#define TOKEN(c)       ( ALPHA(c) || SIMPLE(c) )

#define DIGITS(n) ( (n) >= 100000 ? 6 : (n) >= 10000 ? 5 : (n) >= 1000 ? 4 : (n) >= 100 ? 3 : ( (n) >= 10 ? 2 : 1 ) )

static char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*--------------------------------------------------------------------------------*/

typedef struct _octet {
  char *val ;   /* pointer to where the char array starts */
  int   size ;  /* The size of the array, 0 if static array */
  int   len ;   /* length of the used array */
} octet_t ;

typedef struct _ptree {
  int            list ;
  octet_t        val ;
  struct _ptree *next ;
  struct _ptree *part ;
} ptree_t ;

typedef struct _io_buffert {
  char *str ;
  char *start ;
  int  size ;
  FILE *fp ;
} iobuf_t ;

/*--------------------------------------------------------------------------------*/

ptree_t *get_sexpr( iobuf_t *io ) ;

/*--------------------------------------------------------------------------------*/

static int pos( char c )
{
  char *p;

  for( p = base64 ; *p ; p++ )
    if( *p == c ) return p - base64;

  return -1 ;
}

static int base64_decode( char *str )
{
  char *p, *q;
  int   i, x ;
  unsigned int c;
  int   done = 0;

  for( p = str, q = str ; *p && !done ; p += 4 ){

    for( c = 0, done = 0, i = 0 ; i < 4 ; i++ ) {
      if( i > 1 && p[i] == '=' ) done++ ;
      else {
        if( done ) return -1 ;
        x = pos( p[i] );

        if( x >= 0 ) c += x;
        else         return -1 ;

        if( i < 3 )  c <<= 6;
      }
    } 

    if(done < 3) *q++ = (c & 0x00ff0000) >> 16 ;
    if(done < 2) *q++ = (c & 0x0000ff00) >> 8 ;
    if(done < 1) *q++ = (c & 0x000000ff) >> 0 ;
  }

  return q - str ;
}

/*--------------------------------------------------------------------------------*/

static char *get_more( iobuf_t *io ) 
{
  char *r ;
  
  do {
    r = fgets( io->str, io->size, io->fp ) ;
  } while( r && *r == '#' ) ;

  io->start = io->str ; 
 
  return r ;
}

static char *mycpy( char *res, char *start, int len, int sofar )
{
  char *tmp ;

  if( res == 0 ) {
    res = (char *) malloc((len + 1 ) * sizeof(char)) ;
    strncpy( res, start, len ) ;
    res[len] = 0 ;
  }
  else {
    tmp = (char *) realloc( res, ( sofar + len + 1) * sizeof(char)) ;
    res = tmp ;
    strncpy( &res[sofar], start, len ) ;
    sofar += len ;
    res[sofar] = 0 ;
  }

  return res ;
} 

static int rm_sp( char *str, int len )
{
  char *rp, *cp ;
  int i ;

  for( rp = cp = str, i = 0 ; i < len ; i++, rp++ ) {
    if( WHITESPACE( *rp )) continue ;
    else *cp++ = *rp ;
  } 

  return cp - str ;
}

static int de_escape( char *str, int len )
{
  char *fp, *tp ;
  char  c = 0 ;
  int   i ;

  for( fp = tp = str, i = 0 ; i < len  ; tp++, fp++, i++ ) {
    if( *fp == '\\' ) {
      fp++ ;
      i++ ;

      if( *fp == '\\' ) {
        *tp = *fp ;
      }
      else {
        if( *fp >= 'a' && *fp <= 'f' ) c = (*fp - 0x57) << 4 ;
        else if( *fp >= 'A' && *fp <= 'F' ) c = (*fp - 0x37) << 4 ;
        else if( *fp >= '0' && *fp <= '9' ) c = (*fp - 0x30) << 4 ;
        else return 0 ;

        fp++ ;
        i++ ;

        if( *fp >= 'a' && *fp <= 'f' ) c += (*fp - 0x57) ;
        else if( *fp >= 'A' && *fp <= 'F' ) c += (*fp - 0x37) ;
        else if( *fp >= '0' && *fp <= '9' ) c += (*fp - 0x30) ;
        else return 0 ;

        *tp = c ;
      }
    }
    else *tp = *fp ;
  }

  return tp - str ;
}

/*--------------------------------------------------------------------------------*/

static octet_t *oct_cpy( octet_t *oct, char *val, int size, int len )
{
  oct->val = val ;
  oct->len = len ;
  oct->size = size ;

  return oct ;
}

/*--------------------------------------------------------------------------------*/

static char *oct2strdup( octet_t *op, char ec )
{
  char          c, *str, *cp, *sp ;
  unsigned char uc ;
  int           n, i ;
  
  if( op->len == 0 ) return 0 ;

  /* calculate the necessary length of the result string */
  for( n = 1, i = 0, cp = op->val ; i < op->len ; i++, cp++ ) {
    if( *cp >= 0x20 && *cp < 0x7E && *cp != 0x25 ) n++ ;
    else n += 3 ;
  }

  str = ( char * ) malloc( n  * sizeof( char )) ;

  for( sp = str, i = 0, cp = op->val ; i < op->len ; i++ ) {
    if( !ec ) *sp++ = *cp++ ;
    else if( *cp >= 0x20 && *cp < 0x7E && *cp != 0x25 ) *sp++ = *cp++ ;
    else {
      *sp++ = ec ;
      uc = ( unsigned char ) *cp++ ;
      c = ( uc & (char) 0xF0 ) >> 4 ;
      if( c > 9 ) *sp++ = c + (char) 0x57 ;
      else *sp++ = c + (char) 0x30 ;

      c = ( uc & (char) 0x0F ) ;
      if( c > 9 ) *sp++ = c + (char) 0x57 ;
      else *sp++ = c + (char) 0x30 ;
    }
  }

  *sp = '\0' ;

  return str ;
}

/*--------------------------------------------------------------------------------*/

static octet_t *get_token( octet_t *oct, iobuf_t *io )
{
  char *cp, *res = 0 ;
  int  len = 0 , sofar = 0 ;

  do {
    for( cp = io->start ; *cp && TOKEN(*cp) ; cp++ ) ;
  
    if( *cp == 0 ) {
      len = cp - io->start ;
      res = mycpy( res, io->start, len, sofar ) ;

      if( get_more( io ) == 0 ) return 0 ;
    }
    else break ;
  } while( 1 ) ;

  if( cp != io->start ) { /* never got off the ground */
    len = cp - io->start ;
    res = mycpy( res, io->start, len, sofar ) ;
    sofar += len ;
    io->start = cp ;
  }

  oct_cpy( oct, res, sofar, len ) ;

  return oct ;
}

static octet_t *get_base64( octet_t *oct, iobuf_t *io )
{
  char *cp, *res = 0 ;
  int  len, sofar = 0, done = 0 ;

  do {
    for( cp = io->start ; *cp && (TOKEN(*cp) || WHITESPACE(*cp)); cp++ ) ;
    if( *cp && *cp == '|' ) done = 1 ;
 
    if( *cp == 0 ) {
      len = cp - io->start ;
      res = mycpy( res, io->start, len, sofar ) ;
      sofar += len ; 
      if( get_more( io ) == 0 ) return 0 ;
    }
    else break ;
  } while( 1 ) ;

  if( cp != io->start ) { /* never got off the ground */
    len = cp - io->start ;
    res = mycpy( res, io->start, len, sofar ) ;
    sofar += len ;
    io->start = ++cp ;
  }

  len = sofar ;

  if( res ) {
    /* if there are any whitespace chars remove them */
    sofar = rm_sp( res, sofar ) ;
    /* de base64 */

    sofar = base64_decode( res ) ;
    res[sofar] = 0 ;
  }

  oct_cpy( oct, res, len, sofar ) ;

  return oct ;
}

static octet_t *get_hex( octet_t *oct, iobuf_t *io )
{
  char         *cp, *res = 0, *rp ;
  unsigned char c ;
  int           len, sofar = 0, i ;

  do {
    for( cp = io->start ; *cp && (HEX(*cp) || WHITESPACE(*cp)) ; cp++ ) ;
    if( *cp && *cp == '%' ) break ;
  
    if( *cp == 0 ) {
      len = cp - io->start ;
      res = mycpy( res, io->start, len, sofar ) ;
      sofar += len ;
      if( get_more( io ) == 0 ) return 0 ;
    }
    else break ;
  } while( 1 ) ;

  if( cp != io->start ) { 
    len = cp - io->start ;
    res = mycpy( res, io->start, len, sofar ) ;
    sofar += len ;
    io->start = ++cp ;
  }

  len = sofar ;

  if( res ) {
    /* if there are any whitespace chars remove them */
    sofar = rm_sp( res, sofar ) ;

    /* convert hex hex to byte */
    for( rp = cp = res, i = 0 ; i < sofar ; rp++, cp++  ) {
      if( *rp >= 'a' && *rp <= 'f' ) c = (*rp - 0x57) << 4 ;
      else if( *rp >= 'A' && *rp <= 'F' ) c = (*rp - 0x37) << 4 ;
      else if( *rp >= '0' && *rp <= '9' ) c = (*rp - 0x30) << 4 ;
      else return 0 ;

      rp++ ;

      if( *rp >= 'a' && *rp <= 'f' ) c += (*rp - 0x57) ;
      else if( *rp >= 'A' && *rp <= 'F' ) c += (*rp - 0x37) ;
      else if( *rp >= '0' && *rp <= '9' ) c += (*rp - 0x30) ;
      else return 0 ;

      *cp = c ;
      i += 2 ;
    }

    sofar = cp - res ;
  }

  if( res == 0 ) return 0 ;

  oct_cpy( oct, res, len, sofar ) ;

  return oct ;
}

#define HEXCHAR 1
#define SLASH   2
#define HEXDUO  4

static octet_t *get_quote( octet_t *oct, iobuf_t *io )
{
  char    *cp, *res = 0 ;
  int      len, sofar = 0 ;
  int      expect = 0, done = 0 ;

  do {
    for( cp = io->start ; *cp ; cp++ ) {
      if( expect ) {
        if( expect == HEXCHAR || expect == HEXDUO ) {
          if( HEX(*cp) ) {
            if( expect == HEXCHAR ) expect = 0 ;
            else                    expect = HEXCHAR ;
          }
          else return 0 ;
        }
        else {
          if( *cp == '\\' ) expect = 0 ;
          else if( HEX( *cp ) ) expect = HEXCHAR ;
          else return 0 ;
        }
        continue ;
      } 

      if( TOKEN(*cp) || WHITESPACE(*cp) ) continue ;

      if( *cp == '\\' ) expect = HEXDUO | SLASH ;
      else if( *cp == '"' ) break ;
      else return 0 ;
    }

    if( *cp == 0 ) {
      len = cp - io->start ;
      res = mycpy( res, io->start, len, sofar ) ;
      sofar += len ;
      if( get_more( io ) == 0 ) return 0 ;
    }
    else break ;
  } while( !done ) ;

  if( cp != io->start ) { 
    len = cp - io->start ;
    res = mycpy( res, io->start, len, sofar ) ;
    sofar += len ;
    io->start = ++cp ;
  }

  if( res ) {
    len = sofar ;
    sofar = rm_sp( res, sofar ) ;

    /* de escape */
    sofar = de_escape( res, sofar ) ;
  }

  if( res == 0 ) return 0 ; /* not allowed */

  oct_cpy( oct, res, len, sofar ) ;

  return oct ;
}

static int skip_whites( iobuf_t *io )
{
  char *bp ;

  bp = io->start ;
 
  do {
    while( WHITESPACE( *bp ) && *bp ) bp++ ;
 
    if( *bp == 0 ) {
      if( get_more( io ) == 0 ) return 0 ;
      bp = io->start ;
    }
    else break ;

  } while( 1 ) ;
  
  io->start = bp ;

  return 1 ;
}
  
static ptree_t *get_list( iobuf_t *io ) 
{
  ptree_t *ptp, *npt, *cpt = 0 ;

  ptp = ( ptree_t * ) calloc ( 1, sizeof( ptree_t )) ;
  ptp->list = 1 ;

  do {
    if( skip_whites( io ) == 0 ) break ;

    if( *io->start == ')' ) {
      io->start++ ;
      break ;
    }

    if(( npt = get_sexpr( io )) == 0 ) return 0 ;

    if( ptp->part == 0 ) ptp->part = npt ;
    else cpt->next = npt ;

    cpt = npt ;

  } while( 1 ) ;

  return ptp ;
}

ptree_t *get_sexpr( iobuf_t *io )
{
  octet_t elem, *o = 0 ;
  ptree_t *ptp = 0 ;

  memset( &elem, 0, sizeof( octet_t )) ;

  if( skip_whites( io ) == 0 ) return 0 ;

  /* what kind of string or list do I have here */

  switch( *io->start ) {
    case '(' :
      io->start++ ;
      ptp = get_list( io ) ;
      break ;

    case '|':
      io->start++ ;
      o = get_base64( &elem, io ) ;
      break ;

    case '%':
      io->start++ ;
      o = get_hex( &elem, io ) ;
      break ;

    case '"':
      io->start++ ;
      o = get_quote( &elem, io ) ;
      break ;

    default:
      if( TOKEN( *io->start ) ) o = get_token( &elem, io ) ;
      break ;
  }

  if( ptp == 0 && o != 0 ) {
    ptp = ( ptree_t * ) calloc (1, sizeof( ptree_t )) ;
    oct_cpy( &ptp->val, elem.val, elem.size, elem.len ) ;
  }

  return ptp ;
}

/*--------------------------------------------------------------------------------*/

static int length_sexp ( ptree_t *ptp )
{
  int len = 0 ;
  ptree_t *pt ;

  if( ptp->list ) {
    len = 2 ;
    for( pt = ptp->part ; pt ; pt = pt->next ) len += length_sexp( pt ) ;
  }
  else {
    len = DIGITS( ptp->val.len ) ;
    len++ ;
    len += ptp->val.len ;
  }

  return len ;
}

static void recreate_sexp ( octet_t *o, ptree_t *ptp ) 
{
  ptree_t *pt ;
  int     len = 0 ;

  if( ptp->list ) {
    *o->val++ = '(' ;
    o->len++ ;
    for( pt = ptp->part ; pt ; pt = pt->next ) recreate_sexp( o, pt ) ;
    *o->val++ = ')' ;
    o->len++ ;
  }
  else {
    sprintf( o->val, "%d:", ptp->val.len ) ;
    len = DIGITS( ptp->val.len ) ;
    len++ ;
    o->len += len ;
    o->val += len ;
    strncpy( o->val, ptp->val.val, ptp->val.len ) ;
    o->len += ptp->val.len; 
    o->val += ptp->val.len; 
  }
}

/*--------------------------------------------------------------------------------*/

int main( int argc, char **argv )
{
  iobuf_t  buf ;
  ptree_t *ptp ;
  int      len ;
  octet_t  oct ;
  char    *tmp, *val ;

  if( argc > 1 ) {
    if(( buf.fp = fopen( argv[1], "r" )) == 0 ) return 1 ;
  }
  else buf.fp = stdin ;

  buf.str = (char *) calloc ( BUFSIZ, sizeof( char )) ;
  buf.size = BUFSIZ ;
  buf.start = buf.str ;

  oct.size = 0 ;

  if( get_more( &buf ) == 0 ) return 0 ;

  while(( ptp = get_sexpr( &buf )) != 0 ) {
    len = length_sexp( ptp ) ;

    if( oct.size < len ) {
      if( oct.size ) free( oct.val ) ;
      oct.val = ( char * ) malloc ( len * sizeof( char )) ;
      oct.size = len ;
    }

    oct.len = 0 ;
    val = oct.val ;
    recreate_sexp( &oct, ptp ) ;
    oct.val = val ;
    tmp = oct2strdup( &oct, '\\' ) ;
    printf( "%s\n", tmp ) ;
    free( tmp ) ;
  }

  return 0 ;
}
