/***************************************************************************
                          string.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in tdup level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <func.h>

#include <struct.h>
#include <wrappers.h>
#include <spocp.h>
#include <macros.h>

/* -1 if no proper s-exp 
   0 if missing chars
   otherwise the number of characters */

int sexp_len( octet_t *sexp )
{
  spocp_result_t r ;
  int            depth = 0  ;
  octet_t        oct, str ;

  oct.val = sexp->val ;
  oct.len = sexp->len ;

  /* first one should be a '(' */
  if( *oct.val != '(' ) return -1 ;

  oct.val++ ;
  oct.len-- ;

  while( oct.len ) {
    if( *oct.val == '(' ) {      /* start of list */
      depth++ ;

      oct.len-- ;
      oct.val++ ;
    }
    else if( *oct.val == ')' ) { /* end of list */
      depth-- ;

      if( depth < 0 ) {
        oct.len-- ;
        break ;                   /* done */
      }

      oct.len-- ;
      oct.val++ ;
    }
    else {
      r = get_str( &oct, &str ) ;
      if( r == SPOCP_SYNTAXERROR ) return -1 ;
      else if( r == SPOCP_MISSING_CHAR ) return 0 ;
    }
  }

  if( depth >= 0 ) return 0 ;

  return (sexp->len - oct.len) ;
}


/*--------------------------------------------------------------------------------*/

char *sexp_printa( char *sexp, unsigned int *size, char *format, void **argv )
{
  char        *s, *sp, **arr ;
  octet_t     *o, **oa ;
  int          i, n, argc = 0 ;
  unsigned int bsize = *size ;
  octarr_t    *oarr ;

  if( format == 0 || *format == '\0' ) return 0 ;

  memset( sexp, 0, bsize ) ;
  sp = sexp ;
  bsize -= 2 ; 

  while (*format && bsize ) {
    /* traceLog( "Sexp [%s][%c]", sexp, *format ) ; */
    switch(*format++) {
      case '(':           /* start of a list */
        *sp++ = '(' ;
        bsize-- ;
        break ;

      case ')':           /* end of a list */
        *sp++ = ')' ;
        bsize-- ;
        break ;

      case 'o':           /* octet */
        if(( o = ( octet_t *) argv[argc++]) == 0 )  ;
        if( o && o->len ) {
          n = snprintf( sp, bsize, "%d:", o->len);
          if( n < 0 || (unsigned int ) n > bsize ) return 0 ;
          bsize -= n ;
          sp += n ;

          if( o->len > bsize ) return 0 ;
          memcpy( sp, o->val, o->len ) ;
          bsize -= o->len ;
          sp += o->len ;
        }
        break;

      case 'O':           /* octet array */
        if(( oa = ( octet_t **) argv[argc++] ) == 0 ) return 0 ;

        for( i = 0 ; oa[i] ; i++ ) {
          o = oa[i] ;
          if( o->len == 0 ) continue ;
          n = snprintf( sp, bsize, "%d:", o->len);
          if( n < 0 || (unsigned int ) n > bsize ) return 0 ;
          bsize -= n ;
          sp += n ;

          if( o->len > bsize ) return 0 ;
          memcpy( sp, o->val, o->len ) ;
          bsize -= o->len ;
          sp += o->len ;
        }
        break;

      case 'X':           /* ocarr */
        if(( oarr = ( octarr_t *) argv[argc++] ) == 0 ) return 0 ;

        for( i = 0 ; i < oarr->n ; i++ ) {
          o = oarr->arr[i] ;
          if( o->len == 0 ) continue ;
          n = snprintf( sp, bsize, "%d:", o->len);
          if( n < 0 || (unsigned int ) n > bsize ) return 0 ;
          bsize -= n ;
          sp += n ;

          if( o->len > bsize ) return 0 ;
          memcpy( sp, o->val, o->len ) ;
          bsize -= o->len ;
          sp += o->len ;
        }
        break;

      case 'a':           /* atom */
        if(( s = (char *) argv[argc++] ) == 0 ) ;
        if( s && *s ) {
          n = snprintf( sp, bsize, "%d:%s", strlen(s), s);
          if( n < 0 ||  (unsigned int ) n > bsize ) return 0 ;
          bsize -= n ;
          sp += n ;
        }
        break;

      case 'A':           /* sequence of atoms */
        if(( arr = (char **) argc[argv++] ) == 0 ) return 0 ;
        for( i = 0 ; arr[i] ; i++ ) {
          n = snprintf( sp, bsize, "%d:%s", strlen(arr[i]), arr[i]);
          if( n < 0 || (unsigned int ) n > bsize ) return 0 ;
          bsize -= n ;
          sp += n ;
        }
        break;

      case 'l':           /* list */
        if(( s = (char *) argv[ argc++ ] ) == 0 ) ;
        if( s && *s ) {
          n = snprintf( sp, bsize, "%s", s);
          if( n < 0 ||  (unsigned int ) n > bsize ) return 0 ;
          bsize -= n ;
          sp += n ;
        }
        break;

      case 'L':           /* array of lists */
        if(( arr = (char **) argv[ argc++ ]) == 0 ) return 0 ;

        for( i = 0 ; arr[i] ; i++ ) {
          n = snprintf( sp, bsize, "%s", arr[i]);
          if( n < 0 ||  (unsigned int ) n > bsize ) return 0 ;
          bsize -= n ;
          sp += n ;
        }
        break;

      case 'u':           /* octet list */
        if(( o = (octet_t *) argv[ argc++ ]) == 0 ) return 0 ;

        if( o->len > bsize ) return 0 ;
        memcpy( sp, o->val, o->len ) ;
        bsize -= o->len ;
        sp += o->len ;

        break;

      case 'U':           /* array of octet lists */
        if(( oa = (octet_t **) argv[ argc++ ]) == 0 ) return 0 ;

        for( i = 0 ; oa[i] ; i++ ) {
          o = oa[i] ;
          if( o->len == 0 ) continue ;
          if( o->len > bsize ) return 0 ;
          memcpy( sp, o->val, o->len ) ;
          bsize -= o->len ;
          sp += o->len ;
        }
        break;

    }

  }

  *size = bsize ;

  return sexp ;
}

/*--------------------------------------------------------------------------------*/

char *sexp_printv( char *sexp, unsigned int *size, char *fmt, ... )
{
  va_list ap ;
  char    *s, *sp, **arr ;
  octet_t *o, **oa ;
  int     n, i ;
  unsigned int bsize = *size ;
  octarr_t *oarr ;
 
  if( fmt == 0 ) return 0 ;

  va_start( ap, fmt ) ;

  memset( sexp, 0, bsize ) ;
  sp = sexp ;

  while (*fmt && bsize )
    switch(*fmt++) {
      case '(':           /* start of a list */
        *sp++ = '(' ;
        bsize-- ;
        break ;

      case ')':           /* end of a list */
        *sp++ = ')' ;
        bsize-- ;
        break ;

      case 'o':           /* octet */
        o = va_arg(ap, octet_t *) ;
        if( o && o->len ) {
          n = snprintf( sp, bsize, "%d:", o->len);
          if( n < 0 || (unsigned int ) n > bsize ) return 0 ;
          bsize -= n ;
          sp += n ;

          if( o->len > bsize ) return 0 ;
          memcpy( sp, o->val, o->len ) ;
          bsize -= o->len ;
          sp += o->len ;
        }
        break;

      case 'O':           /* octet array */
        oa = va_arg(ap, octet_t **) ;
        if( oa ) {
          for( i = 0 ; oa[i] ; i++ ) {
            o = oa[i] ;
            if( o->len == 0 ) continue ;
            n = snprintf( sp, bsize, "%d:", o->len);
            if( n < 0 || (unsigned int ) n > bsize ) return 0 ;
            bsize -= n ;
            sp += n ;

            if( o->len > bsize ) return 0 ;
            memcpy( sp, o->val, o->len ) ;
            bsize -= o->len ;
            sp += o->len ;
          }
        }
        break;

      case 'a':           /* atom */
        s = va_arg(ap, char *) ;
        if( s && *s ) {
          n = snprintf( sp, bsize, "%d:%s", strlen(s), s);
          if( n < 0 || (unsigned int ) n > bsize ) return 0 ;
          bsize -= n ;
          sp += n ;
        }
        break;

      case 'A':           /* sequence of atoms */
        arr = va_arg(ap, char **) ;
        if( arr ) {
          for( i = 0 ; arr[i] ; i++ ) {
            n = snprintf( sp, bsize, "%d:%s", strlen(arr[i]), arr[i]);
            if( n < 0 || (unsigned int ) n > bsize ) return 0 ;
            bsize -= n ;
            sp += n ;
          }
        }
        break;

      case 'l':           /* list */
        s = va_arg(ap, char *) ;
        if( s && *s ) {
          n = snprintf( sp, bsize, "%s", s);
          if( n < 0 || (unsigned int ) n > bsize ) return 0 ;
          bsize -= n ;
          sp += n ;
        }
        break;

      case 'L':           /* array of lists */
        arr = va_arg(ap, char **) ;
        if( arr ) {
          for( i = 0 ; arr[i] ; i++ ) {
            n = snprintf( sp, bsize, "%s", arr[i]);
            if( n < 0 || (unsigned int ) n > bsize ) return 0 ;
            bsize -= n ;
            sp += n ;
          }
        }
        break;

      case 'u':           /* octet list */
        o = va_arg(ap, octet_t *) ;
        if( o ) {
          if( o->len > bsize ) return 0 ;
          memcpy( sp, o->val, o->len ) ;
          bsize -= o->len ;
          sp += o->len ;
        }
        break;

      case 'U':           /* array of octet lists */
        oa = va_arg(ap, octet_t **) ;
        if( oa ) {
          for( i = 0 ; oa[i] ; i++ ) {
            o = oa[i] ;
            if( o->len == 0 ) continue ;
            if( o->len > bsize ) return 0 ;
            memcpy( sp, o->val, o->len ) ;
            bsize -= o->len ;
            sp += o->len ;
          }
        }
        break;

      case 'X':           /* ocarr */
        oarr = va_arg( ap, octarr_t *) ;
        if( oarr ) {
          for( i = 0 ; i < oarr->n ; i++ ) {
            o = oarr->arr[i] ;
            if( o->len == 0 ) continue ;
            n = snprintf( sp, bsize, "%d:", o->len);
            if( n < 0 || (unsigned int ) n > bsize ) return 0 ;
            bsize -= n ;
            sp += n ;

            if( o->len > bsize ) return 0 ;
            memcpy( sp, o->val, o->len ) ;
            bsize -= o->len ;
            sp += o->len ;
          }
        }
        break;

    }

  va_end( ap ) ;

  *size = bsize ;

  return sexp ;
}

