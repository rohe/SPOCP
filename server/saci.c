#include "locl.h"

typedef char *(argfunc)( conn_t *r ) ;

typedef struct {
  char *var ;
  argfunc *af ;
  char type ;
  char dynamic ;
} arg_t  ;

static char * get_path( conn_t *r ) ;
static char * get_action( conn_t *r ) ;
static char * get_arguments( conn_t *r ) ;
static char * get_ip( conn_t *r ) ;
static char * get_host( conn_t *r ) ;
static char * get_inv_host( conn_t *r ) ;
static char * get_subject( conn_t *r ) ;
#ifdef HAVE_SSL
static char * get_ssl_vers( conn_t *c ) ;
static char * get_ssl_cipher( conn_t *c ) ;
static char * get_ssl_subject( conn_t *c ) ;
static char * get_ssl_issuer( conn_t *c ) ;
#endif
static char * get_transpsec( conn_t *r ) ;

#define TPSEC_X509	0
#define TPSEC_X509_WCC	1
#define TPSEC_KERB	2

char *tpsec[] = {
  "(12:TransportSec(4:vers%{ssl_vers})(12:chiphersuite%{ssl_cipher}))", 
  "(12:TransportSec(4:vers%{ssl_vers})(12:chiphersuite%{ssl_cipher})(7:autname4:X509(7:subject%{ssl_subject})(6:issuer%{ssl_issuer})))", 
  "(12:TransportSec(4:vers%{kerb_vers})(7:autname8:kerberos%{kerb_realm}%{kerb_localpart}))"
} ;

char *srvquery = "(6:server(2:ip%{ip})(4:host%{host})%{transportsec})" ;
char *operquery = "(9:operation%{operation}(4:path%{path})(6:server(2:ip%{ip})(4:host%{host})%{transportsec}))" ;

arg_t **tpsec_X509 ;
arg_t **tpsec_X509_wcc ;
arg_t **srvq ;
arg_t **operq ;

arg_t transf[] = {
  { "ip", get_ip, 'a', FALSE },
  { "host", get_host, 'a', FALSE },
  { "invhost", get_inv_host, 'l', FALSE },
  { "subject", get_subject, 'l', FALSE },
  { "operation", get_action, 'l', TRUE },
  { "arguments", get_arguments, 'l', TRUE },
  { "path", get_path, 'l', TRUE },
#ifdef HAVE_SSL
  { "ssl_vers", get_ssl_vers, 'a', FALSE },
  { "ssl_cipher", get_ssl_cipher, 'a', FALSE },
  { "ssl_subject", get_ssl_subject, 'a', FALSE },
  { "ssl_issuer", get_ssl_issuer, 'a', FALSE },
#endif
  { "transportsec", get_transpsec, 'l', FALSE },
  { NULL, NULL, 0, FALSE }
} ;

int ntransf = (sizeof( transf ) / sizeof( transf[0] )) ;

/*
arg_t spocp_default[] = {
  { "(4:http(8:resource", 0, 'l' },
  { NULL, path_to_sexp, 'l' },
  { "1:-)(6:action", NULL, 'l' },
  { NULL, get_method, 'a' },
  { NULL, authz_type, 'a' },
  { ")(7:subject(2:ip", NULL, 'l' },
  { NULL, get_ip, 'a' },
  { ")(4:host", NULL, 'l' },
  { NULL, get_host, 'a' },
  { ")))", NULL, 'l' },
  { NULL, NULL, 0 }
} ;
*/

/* ---------------------------------------------------------------------- */

/*
static void argt_free( arg_t **arg )
{
  int i ;

  if( arg ) {
    for( i = 0 ; arg[i] || arg[i]->type != 0 ; i++ ) {
      if( arg[i]->var ) free( arg[i]->var ) ;
      free( arg[i] ) ;
    }
    if( arg[i] ) free( arg[i] ) ;

    free( arg ) ;
  }
}
*/

/* ---------------------------------------------------------------------- */

static char *get_ip( conn_t *conn )
{
  return( conn->sri.hostaddr ) ;
}

static char *get_host( conn_t *conn )
{
  return( conn->sri.hostname ) ;
}

static char *get_subject( conn_t *conn )
{
  char sexp[512], *sp ;
  unsigned int size = 512 ;

  if( conn->sri.sexp_subject == 0 ) {
    sp = sexp_printv( sexp, &size, "o", conn->sri.subject ) ;
    if( sp ) conn->sri.sexp_subject = strdup( sexp ) ;
  }

  return( conn->sri.sexp_subject ) ;
}

static char *get_inv_host( conn_t *conn )
{
  char **arr, format[16], list[512], *sp ;
  int   n, i, j ;
  unsigned int size = 512 ;

  if( conn->sri.invhost ) return conn->sri.invhost ;

  arr = line_split( conn->sri.hostname, '.', 0, 0, 0, &n ) ; 
  
  for( i = 0, j = n ; i < (n+1)/2 ; i++, j-- ) {
    if( i == j ) break ;
    sp = arr[i] ;
    arr[i] = arr[j] ;
    arr[j] = sp ;
  }

  for( i = 0 ; i <= n ; i ++ ) format[i] = 'a' ;
  format[i] = '\0' ;

  sexp_printa( list, &size, format, (void **) arr ) ;

  conn->sri.invhost = strdup( list ) ;

  return ( conn->sri.invhost ) ;
}

static char *get_path( conn_t *conn )
{
  char         *res, *rp ;
  unsigned int size ;

  if( conn->oppath == 0 ) return 0 ;

  size = conn->oppath->len + 9 ;

  res = ( char * ) Malloc ( size * sizeof( char )) ;
  
  rp = sexp_printv( res, &size, "o", conn->oppath ) ;

  if( rp == 0 ) free( res ) ;

  return rp ;
}

static char *get_action( conn_t *conn )
{
  char         *res, *rp ;
  unsigned int size = conn->oper.len + 9 ;

  /* traceLog("Get_action") ; */

  res = ( char * ) Malloc ( size * sizeof( char )) ;
  
  rp = sexp_printv( res, &size, "o", &conn->oper ) ;

  if( rp == 0 ) free( res ) ;

  return rp ;
}

static char *get_arguments( conn_t *conn )
{
  char        *res, *rp, format[32] ;
  unsigned int size ;
  int          i ;

  if( conn->oparg == 0 ) return 0 ;

  for( i = 0, size = 0 ; i < conn->oparg->n ; i++ ) {
    size += conn->oparg->arr[i]->len + 5 ;
    format[i] = 'o' ;
  }
  format[i] = '\0' ;

  res = ( char * ) Malloc ( size * sizeof( char )) ;
  
  rp = sexp_printa( res, &size, format, (void **) conn->oparg->arr ) ;

  if( rp == 0 ) free( res ) ;

  return rp ;
}

#ifdef HAVE_SSL
static char * get_ssl_vers( conn_t *c ) 
{
  return( c->ssl_vers ) ;
}

static char * get_ssl_cipher( conn_t *c ) 
{
  return( c->cipher ) ;
}

static char * get_ssl_subject( conn_t *c ) 
{
  return( c->subjectDN ) ;
}

static char * get_ssl_issuer( conn_t *c ) 
{
  return( c->issuerDN ) ;
}
#endif

/* ---------------------------------------------------------------------- */

static char *sexp_constr( conn_t *conn, arg_t **ap )
{
  int          i ;
  unsigned int size = 2048 ;
  char        *argv[32], format[32] ;
  char         sexp[2048], *res ;

  if( ap == 0 ) return 0 ;

  for( i = 0 ; ap[i] ; i++ ) {
    format[i] = ap[i]->type ;

    if( ap[i]->var ) argv[i] = ap[i]->var ;
    else argv[i] = ap[i]->af( conn ) ;

    if(0) LOG(SPOCP_DEBUG) traceLog( "SEXP argv[%d]: [%s]", i, argv[i]) ;
  }
  format[i] = '\0' ;
  argv[i] = 0 ;

  if(0) LOG(SPOCP_DEBUG) traceLog( "SEXP Format: [%s]", format ) ;

  if( sexp_printa( sexp, &size, format, (void **) argv ) == 0 ) res = 0 ; 
  else res = Strdup( sexp ) ;

  /* only free them temporary stuff */
  for( i = 0 ; ap[i] ; i++ ) {
    if( ap[i]->af && ap[i]->dynamic == TRUE ) free( argv[i] ) ;
  }

  return res ;
}

/* ---------------------------------------------------------------------- */

static char *get_transpsec( conn_t *conn )
{
/* XXX fixa SPOCP_LAYER-jox här */
#ifdef HAVE_SSL
  if( conn->ssl != NULL  ) {
    if( conn->transpsec == 0 ) {
      if( conn->subjectDN )
        conn->transpsec = sexp_constr( conn, tpsec_X509_wcc ) ;
      else
        conn->transpsec = sexp_constr( conn, tpsec_X509 ) ;
    }
    return conn->transpsec ;
  }
  else 
#endif
    return "" ;

}

/* ---------------------------------------------------------------------- */

/* format is typically (3:foo%{var})
   which is then split into three parts
   "(3:foo", %{var} and ")"
   var is looked up in the table of known variables and
   if it exists the function connected to that variable is
   included in the list
 */

static arg_t **parse_format( const char *format )
{
  arg_t **arg ;
  char  *sp, *vp, *tp ;
  int    i = 0, j = 0, n ;
  char  *copy ; 
  
  copy = Strdup( (char *) format) ;

  /* This is the max size I know that for sure */
  n = ( ntransf * 2 ) + 2 ;

  arg = ( arg_t ** ) Calloc ( n, sizeof( arg_t * )) ; 

  for( sp = copy ; *sp ; ) {
    if(( vp = strstr( sp , "%{" ))) {
      vp += 2 ;
      tp = find_balancing( vp, '{', '}' ) ; 
      if( tp == 0 ) { /* format error or not, your guess is as good as mine */
        break ; /* I guess it's a string */
      }
      else {
        if( vp - 2 != sp ) {
          *(vp-2) = '\0' ;
          arg[j] = (arg_t * ) malloc( sizeof( arg_t )) ;
          arg[j]->var = strdup( sp ) ;
          arg[j]->af = 0 ;
          arg[j]->type = 'l' ; 
          arg[j]->dynamic = TRUE ;
          j++ ;
          *(vp-2) = '$' ;
        }

        *tp = '\0' ;
        for( i = 0 ; transf[i].type ; i++ ) {
          /* traceLog( "(%d)[%s] == [%s] ?", i, transf[i].var, vp ) ; */
          if( strcmp( transf[i].var, vp ) == 0 ) {
            arg[j] = (arg_t * ) malloc( sizeof( arg_t )) ;
            arg[j]->var = 0 ;
            arg[j]->af = transf[i].af ;
            arg[j]->type = transf[i].type ;
            arg[j]->dynamic = transf[i].dynamic ;
            j++ ;
            break ;
          }
        }

        /* unknown variable, clear out and close down */
        if( transf[i].type == 0 ) {
          traceLog( "parse_format: variable [%s] unknown", vp ) ;
          for( i = 0 ; arg[i] && i < n ; i++ ) free( arg[i] ) ;
          free( arg ) ;
          return 0 ;
        }
      }

      sp = tp+1 ;
    }
    else break ;
  }

  if( *sp ) { /* trailing string, which is quite ok */
    arg[j] = (arg_t * ) malloc( sizeof( arg_t )) ;
    arg[j]->var = strdup( sp ) ;
    arg[j]->af = 0 ;
    arg[j]->type = 'l' ; 
  }

  return arg ;
}

/* ---------------------------------------------------------------------- */

void saci_init( void )
{
/*
  arg_t **tp_sec_kerb ;
*/

  tpsec_X509 = parse_format( tpsec[TPSEC_X509] ) ;
  if( tpsec_X509 == 0 ) traceLog( "Could not parse TPSEC_X509" ) ;
  else LOG(SPOCP_DEBUG) traceLog( "Parsed TPSEC_X509 OK" ) ;

  tpsec_X509_wcc = parse_format( tpsec[TPSEC_X509_WCC] ) ;
  if( tpsec_X509_wcc == 0 ) traceLog( "Could not parse TPSEC_X509_WCC" ) ;
  else LOG(SPOCP_DEBUG) traceLog( "Parsed TPSEC_X509_WCC OK" ) ;

/*
  tpsec_kerb = parse_format( tpsec[TPSEC_KERB] ) ;
*/
  srvq = parse_format( srvquery ) ;
  if( srvq == 0 ) traceLog( "*ERR* Could not parse srvquery" ) ;
  else LOG(SPOCP_DEBUG) traceLog( "Parsed srvquery OK" ) ;

  operq = parse_format( operquery ) ;
  if( operq == 0 ) traceLog( "*ERR* Could not parse operquery" ) ;
  else LOG(SPOCP_DEBUG) traceLog( "Parsed operquery OK" ) ;

}

/* ---------------------------------------------------------------------- */

static spocp_result_t spocp_access( conn_t *con, arg_t **arg, char *path )
{
  spocp_result_t res = SPOCP_DENIED ; /* the default */
  ruleset_t  *rs ;
  octet_t     oct ;
  char       *sexp ;
  element_t  *ep = 0 ;
  comparam_t  comp ;
 
  /* no ruleset or rules means everything is allowed */
  if( con->rs == 0 || rules( con->rs->db) == 0 ) {
    if( 0 ) traceLog("No rules to tell me what to do" );
    return SPOCP_SUCCESS ;
  }

  oct_assign( &oct, path ) ;

  if( 0 ) traceLog( "Looking for ruleset [%s](%p)", path, con->rs ) ;

  rs = con->rs ;

  /* No ruleset means everything is allowed !!! */
  if( ruleset_find( &oct, &rs ) == 0  || rs->db == 0 ) return SPOCP_SUCCESS ;
  /* The same if there is no rules in the ruleset */
  if( rs->db->ri->rules == 0 ) return SPOCP_SUCCESS  ;

  if( 0 ) traceLog( "Making the internal access query" ) ;
  sexp = sexp_constr( con, arg ) ;
  
  LOG( SPOCP_DEBUG ) traceLog("Internal access Query: \"%s\" in \"%s\"", sexp, rs->name ) ;

  /* is it a valid assumption to assume only 'printable' characters in sexp ? */
  oct_assign( &oct, sexp) ;

  if(( res = element_get( &oct, &ep )) != SPOCP_SUCCESS ) {
    traceLog("The S-expression \"%s\" didn't parse OK", sexp ) ;
    
    free( sexp ) ;
    return res ;
  }

  if( oct.len ) { /* shouldn't be */
    free( sexp ) ;
    element_free( ep ) ;
    return SPOCP_DENIED ;
  }

  comp.rc = SPOCP_SUCCESS ;
  comp.head = ep ;
  comp.blob = 0 ;

  res = allowed( rs->db->jp, &comp ) ;

  free( sexp ) ;
  element_free( ep ) ;

  return res ;
}

spocp_result_t server_access( conn_t *con )
{
  char      path[NAME_MAX+1] ;

  sprintf( path, "%s/server", localcontext) ;

  return spocp_access( con, srvq, path ) ;
}

spocp_result_t operation_access( conn_t *con )
{
  char           path[NAME_MAX+1] ;
  spocp_result_t r ;

  sprintf( path, "%s/operation", localcontext) ;

  r = spocp_access( con, operq, path ) ;

  if( r != SPOCP_SUCCESS ) traceLog( "Operation disallowed" ) ;

  return r;
}
