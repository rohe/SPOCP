#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <spocp.h>
#include <func.h>
#include <struct.h>
#include <db0.h>
#include <wrappers.h>

#include <srv.h>

/* 
   The following two routines are based on 
   ideas borrowed from tcpwrapper by
   Wietse Venema, Eindhoven University of Technology, The Netherlands.
*/

/* string_match - match string against pattern */

static int string_match(tok, string)
char   *tok;
char   *string;
{
    int     n, ls;

    if (tok[0] == '.') {                        /* suffix */
       ls = strlen( string ) ;

       n = ls - strlen(tok);
       /* have to be more bytes in string than in tok */
       return ( n > 0 && strcasecmp(tok, string + n) == 0 );
    } else if (strcasecmp(tok, "ALL") == 0 ) {            /* all: match any */
        return 1 ;
    } else if (tok[(n = strlen(tok)) - 1] == '.') {     /* prefix */
        return (strncasecmp(tok, string, n) == 0);
    } else {                                    /* exact match */
        return (strcasecmp(tok, string) == 0);
    }
}

/* masked_match - match address against netnumber/netmask */

static int masked_match( char *net_tok, char *mask_tok, char *string)
{
  struct in_addr net;
  struct in_addr mask;
  struct in_addr addr;

  if ( is_ipv4_s(string, &net) != SPOCP_SUCCESS ) return 0 ;

  if( mask_tok ) {
   if ( is_ipv4_s(net_tok, &net) != SPOCP_SUCCESS 
        || ( mask_tok != 0 && is_ipv4_s(mask_tok, &mask)) != SPOCP_SUCCESS ) {
        return 0;
    }
    return ((addr.s_addr & mask.s_addr) == net.s_addr) ;
  }
  else {
    if ( is_ipv4_s(net_tok, &net) != SPOCP_SUCCESS ) return 0 ;

    return (addr.s_addr == net.s_addr);
  }
}

regexp_t *new_pattern( char *s )
{
  regexp_t *rp ;

  if( s == 0 || *s == 0 ) return 0 ;

  rp = (regexp_t *) Malloc ( sizeof( regexp_t )) ;

  rp->regex = Strdup( s ) ;

  if( regcomp( &rp->preg, s, REG_EXTENDED | REG_NOSUB ) != 0 ) {
    traceLog("Error compiling regex \"%s\"", s ) ;
    return 0 ;
  }

  return rp ;
}

void rm_pattern( regexp_t *rp )
{
  if( rp ) {
    regfree( &rp->preg ) ;
    free( rp->regex ) ;
    free( rp ) ;
  }
}

int match_pattern( regexp_t *rp, char *s )
{
  if( regexec( &rp->preg, s, 0, 0, 0 ) >= 0 ) return TRUE ;
  else return FALSE ;
}

int set_access( char *al )
{
  int r = 0 ;

  for( ; *al ; al++ ) {
    switch( *al ) {
      case 'q': 
       if( (r & QUERY) == 0 )  r |= QUERY;
       break ;

      case 'l': 
       if( (r & LIST) == 0 )  r |= LIST ;
       break ;

      case 'a': 
       if( (r & ADD) == 0 )  r |= ADD ;
       break ;

      case 'r': 
       if( (r & REMOVE) == 0 )  r |= REMOVE ;
       break ;

      case 'b': 
       if( (r & BEGIN ) == 0 )  r |= BEGIN ;
       break ;

      case 'i': 
       if( (r & ALIAS) == 0 )  r |= ALIAS ;
       break ;

      case 'c': 
       if( (r & COPY) == 0 )  r |= COPY ;
       break ;

    }
  }

  return r ; 
}

aci_t *new_aci( )
{
  aci_t *aci ;

  aci = ( aci_t * ) Calloc ( 1, sizeof( aci_t )) ;

  return aci ;
}

void free_aci( aci_t *a )
{
  if( a ) {
    if( a->string ) free( a->string ) ;
    if( a->net ) free( a->net ) ;
    if( a->mask ) free( a->mask ) ;
    rm_pattern( a->cert ) ;
    free( a ) ;
  }
}

void free_aci_chain( aci_t *a )
{
  if( a ) {
    if( a->next ) free_aci_chain( a->next ) ;
    free_aci( a ) ;
  }
}

/*

 client_aci = rulesetname ":" qlarb [ ":" host [ ":" cert ]] 

 host = ipnum [ "/" mask ]

 */
int add_client_aci( char *str, ruleset_t **rs )
{
  char           **arr, *sp, *cp ;
  int            n ;
  ruleset_t      *rp ;
  aci_t          *aci ;
  struct in_addr addr ;
  octet_t        oct ;

  arr = line_split( str, ':', 0, 0, 0, &n ) ;

  if( n < 2 ) return 0 ; /* too few elements */

  /* ruleset name, is of the same form as a file name on unix  */
  sp = rm_lt_sp( arr[0], 0 ) ;

  oct.val = sp ;
  if( sp && *sp ) oct.len = strlen( sp ) ;
  else oct.len = 0 ;

  if(( rp = find_ruleset( &oct, *rs )) == 0 )
    rp = create_ruleset( &oct, rs ) ;

  /* next the access rights */

  sp = rm_lt_sp( arr[1], 0 ) ;

  if( rp->aci == 0 ) aci = rp->aci = new_aci() ;
  else {
    for( aci = rp->aci ; aci->next ; aci = aci->next ) ;
    aci->next = new_aci() ;
    aci = aci->next ;
  }

  if( sp == 0 || *sp == '\0' ) aci->access = ALL ;
  else aci->access = set_access( sp ) ;

  /* next the host specification */

  sp = rm_lt_sp( arr[2], 0 ) ;
 
  if( sp && (cp = strchr( sp, '/' ))) {  /* net/mask specification */
    *cp++ = '\0' ;
    if( is_ipv4_s( sp, &addr ) != SPOCP_SUCCESS  ||
        is_ipv4_s( cp, &addr ) != SPOCP_SUCCESS ) return 0 ;

    aci->net = Strdup(sp) ;
    aci->mask = Strdup(cp) ;
  }
  else {
    if( sp == 0 || *sp == '\0' ) aci->string = Strdup( "ALL" ) ;
    else aci->string = Strdup( sp ) ;
  }

  /* And lastly the client cert, if it's there */

  if( n > 2 ) {
    sp = rm_lt_sp( arr[3], 0 ) ;

    if( sp == 0 || *sp == 0 ) ; /* Not a valid pattern */
    else aci->cert = new_pattern( sp ) ;
  }

  return 1 ;
}

int rm_aci( char *str, ruleset_t *rs )
{
  char           **arr ;
  int            n ;

  arr = line_split( str, ':', 0, 0, 0, &n ) ;

  if( n < 2 ) return 0 ; /* too few elements */

  return 1 ;
}

int write_access( char *sp, char a )
{
  if( a & QUERY ) *sp = 'q' ;
  sp++ ;
  if( a & LIST ) *sp = 'l' ;
  sp++ ;
  if( a & ADD ) *sp = 'a' ;
  sp++ ;
  if( a & REMOVE ) *sp = 'r' ;
  sp++ ;
  if( a & BEGIN ) *sp = 'b' ;
  sp++ ;
  if( a & ALIAS ) *sp = 'i' ;
  sp++ ;
  if( a & COPY ) *sp = 'c' ;
  sp++ ;

  *sp = '\0' ;

  return 0 ;
}

/*
int print_aci( conn_t *conn, ruleset_t *rs )
{
  aci_t         *aci ;
  char          access[16] ;
  spocp_iobuf_t *out = conn->out ;

  for( aci = rs->aci ; aci ; aci = aci->next ) {
    add_to_iobuf( out, rs->name ) ;
    add_to_iobuf( out, " : " ) ;
    write_access( access, aci->access ) ;
    add_to_iobuf( out, access ) ;
    add_to_iobuf( out, " : " ) ;

    if( aci->string ) 
      add_to_iobuf( out, aci->string ) ;
    else {
      add_to_iobuf( out, aci->net ) ;
    add_to_iobuf( out, "/" ) ;
      add_to_iobuf( out, aci->mask) ;
    }
    if( aci->cert ) {
      add_to_iobuf( out, " : " ) ;
      add_to_iobuf( out, aci->cert->regex ) ;
    }
    send_results( conn ) ;
  }

  return 1 ;
}
*/

/* If no access control information is specied everything is allowed
   as soon as any access control information is specified everything 
   that is explicitly allowed is so, everything else is disallowed.
   This is per resultset chain */

spocp_result_t rs_access_allowed( ruleset_t *rs, spocp_req_info_t *sri, char type ) 
{
  aci_t         *aci ;
  int            ai = 0, c = 0, nc ;
  spocp_result_t r = SPOCP_DENIED ; 

  if( sri == 0 ) return SPOCP_SUCCESS ; /* root */

  if( rs == 0 ) return SPOCP_UNAVAILABLE ; /* access to empty database not very usefull */

  /* work our way up the chain if access is given somewhere above it's valid here */
  for( nc = 0 ; rs ; rs = rs->up ) {
    if( rs->aci == 0 ) continue ;

    ai++ ;

    for( aci = rs->aci ; !c && aci ; aci = aci->next ) {
      if( type && aci->access ) {
        if( aci->string ) c = string_match( aci->string, sri->hostname ) ;
        else c = masked_match( aci->net, aci->mask, sri->hostaddr ) ;
      }
    }
  }

  /* If no aci's then default is allow */
  if( ai == 0 ) r = SPOCP_SUCCESS ;

  return r ;
}
