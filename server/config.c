/***************************************************************************
                  config.c  - function to read the config file 
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include "locl.h"
RCSID("$Id$");

char *keyword[] = {
  "__querty__", "rulefile", "port", "unixdomainsocket", "certificate",
  "privatekey", "calist", "dhfile", "entropyfile",
  "passwd", "threads", "timeout", "logfile", "sslverifydepth", 
  "pidfile", "maxconn", NULL } ;


#define DEFAULT_PORT    9953 /* we should register one with IANA */
#define DEFAULT_TIMEOUT   30
#define DEFAULT_NTHREADS   5
#define DEFAULT_SSL_DEPTH  1

#define LINEBUF         1024

#define SYSTEM 32

/* roughly */
#define YEAR  31536000

extern char *pidfile ;

/*------------------------------------------------------------------ */

void conf_clear( srv_t *srv )
{
  ruleset_free( srv->root ) ;
  
  /* no more new connections until I'm done */
  pthread_mutex_lock(&srv->mlock); 

  
  pthread_mutex_unlock(&srv->mlock); 
}

/*------------------------------------------------------------------ */

spocp_result_t conf_get( void *vp, infotype_t arg, char *pl, char *key, void **res )
{
  srv_t     *srv = (srv_t *) vp ;
  octarr_t  *on ;

  switch( arg ) {
    case PLUGIN :
      if( srv->root == 0 || srv->root->db == 0 || srv->root->db->plugins == 0 ) *res = 0 ;
      else {
        if( key ) {
          on = pconf_get_keyval_by_plugin( srv->root->db->plugins, pl, key ) ; 
          if( on ) *res = ( void * ) on ;
          else *res = 0 ;
        }
        else 
          *res = ( void * ) pconf_get_keys_by_plugin( srv->root->db->plugins, pl ) ;
      }

      break ;

    case RULEFILE :
      *res = ( void * ) srv->rulefile ; 
      break ;

    case PORT : 
      *res = ( void * ) &srv->port ; 
      break ;

    case UNIXDOMAINSOCKET :
      *res = ( void * ) srv->uds ;
      break ;

    case CERTIFICATE :
      *res = ( void * ) srv->certificateFile ;
      break ;

    case PRIVATEKEY :
      *res = ( void * ) srv->privateKey ;
      break ;

    case CALIST :
      *res = ( void * ) srv->caList ;
      break ;

    case DHFILE :
      *res = ( void * ) srv->dhFile ;
      break ;

    case ENTROPYFILE :
      *res = ( void * ) srv->SslEntropyFile ;
      break ;

    case PASSWD :
      *res = ( void * ) srv->passwd ;
      break ;

    case NTHREADS :
      *res = ( void * ) &srv->threads ;
      break ;

    case TIMEOUT :
      *res = &srv->timeout ;
      break ;

    case LOGFILE :
      *res = ( void * ) srv->logfile ;
      break ;

    case SSLVERIFYDEPTH :
      *res = ( void * ) srv->sslverifydepth ;
      break ;

    case PIDFILE :
      *res = ( void * ) srv->pidfile ;
      break ;

    case MAXCONN :
      *res = ( void * ) &srv->nconn ;
      break ;

  }

  return SPOCP_SUCCESS ;
}

/*------------------------------------------------------------------ */

/* In some cases will gladly overwrite what has been previously specified,
   if the same keyword appears twice */

int read_config( char *file, srv_t *srv )
{
  FILE            *fp ;
  char            s[LINEBUF], *cp, *sp, plugin[256] ;
  char            section = 0 ;
  unsigned int    n = 0 ;
  long            lval ;
  int             i ;
  plugin_t        *plugins, *pl ;

  /* should never be necessary */
  if( !srv->root ) srv->root = ruleset_new( 0 ) ;
  
  if( !srv->root->db )
    srv->root->db = db_new() ;

  plugins = srv->root->db->plugins ;

  if(( fp = fopen( file, "r" )) == NULL ) {
    fprintf( stderr, "Could not find or open the configuration file \"%s\"\n", file ) ;
    return 0 ;
  }

  while( fgets( s, LINEBUF, fp )) {
    n++ ;
    rmcrlf( s ) ;

    if( *s == 0 || *s == '#' ) continue ;

    /* New section */
    if( *s == '[' ) {

      cp = find_balancing( s+1, '[',']' ) ;
      if( cp == 0 ) {
        fprintf( stderr, "Syntax error in configuration file on line %u\n", n ) ;
        return 0 ;
      }

      *cp = 0 ;
      sp = s+1 ;

      if( strcasecmp( sp, "system" ) == 0 ) section = SYSTEM ;
      else {
        section = PLUGIN ;
        strcpy( plugin, sp ) ;
      }

      continue ;
    }

    /* Within a section */

    rm_lt_sp( s, 1 ) ;  /* remove leading and trailing blanks */

    if( *s == 0 || *s == '#' ) continue ;

    cp = strchr( s, '=' ) ;
    if( cp == 0 ) {
      fprintf(stderr, "Error in line %d\n", n ) ;
      continue ;
    }

    sp = cp ;
    for( *cp++ = '\0' ; *cp && ( *cp == ' ' || *cp == '\t' ) ; cp++ ) *cp = '\0' ;
    for( sp-- ; sp != s && ( *sp == ' ' || *sp == '\t' ) ; sp-- ) *sp = '\0' ;
  
    if( *s == '\0' ) continue ;

    switch( section ) {
      case SYSTEM:
        for( i = 1 ; keyword[i] ; i++ )
          if( strcasecmp( keyword[i] , s ) == 0 ) break ;

        if( keyword[i] == 0 ) {
           fprintf(stderr, "Unknown keyword \"%s\"\n", s ) ;
           continue ;
        } 

        switch( i ) {
          case RULEFILE :
            fprintf( stderr, "rulefile: \"%s\"\n", cp ) ;
            if ( srv->rulefile ) free( srv->rulefile ) ;
            srv->rulefile = Strdup( cp ) ;
            break ;
    
          case CERTIFICATE :
            if ( srv->certificateFile ) free( srv->certificateFile ) ;
            srv->certificateFile = Strdup( cp ) ;
            break ;
    
          case PRIVATEKEY :
            if ( srv->privateKey ) free( srv->privateKey ) ;
            srv->privateKey = Strdup( cp ) ;
            break ;
    
          case CALIST :
            if ( srv->caList ) free( srv->caList ) ;
            srv->caList = Strdup( cp ) ;
            break ;
    
          case DHFILE :
            if ( srv->dhFile ) free( srv->dhFile ) ;
            srv->dhFile = Strdup( cp ) ;
            break ;
    
          case ENTROPYFILE :
            if ( srv->SslEntropyFile ) free( srv->SslEntropyFile ) ;
            srv->SslEntropyFile = Strdup( cp ) ;
            break ;
    
          case PASSWD :
            if ( srv->passwd ) free( srv->passwd ) ;
            srv->passwd = Strdup( cp ) ;
            break ;
    
          case LOGFILE :
            if ( srv->logfile ) free( srv->logfile ) ;
            srv->logfile = Strdup( cp ) ;
            break ;
    
          case TIMEOUT  :
            if( numstr( cp, &lval ) == SPOCP_SUCCESS )
            {
              if( lval >= 0 && lval <= YEAR )
                srv->timeout = ( unsigned int ) lval ;
              else {
                fprintf(stderr,"[timeout] value, out of range, setting default (%d)\n",
                        DEFAULT_TIMEOUT ) ;
                srv->timeout = DEFAULT_TIMEOUT ;
              }
            }
            else
            {
              fprintf(stderr,"[timeout] Non numeric value given, setting default (%d)\n",
                      DEFAULT_TIMEOUT ) ;
              srv->timeout = DEFAULT_TIMEOUT ;
            }
    
            break ;
    
          case UNIXDOMAINSOCKET:
            if( srv->uds ) free( srv->uds ) ;
            srv->uds = Strdup( cp ) ;
            break ;
    
          case PORT     :
            if( numstr( cp, &lval ) == SPOCP_SUCCESS ) {
              if( lval > 0L && lval < 65536 ) {
                srv->port = ( unsigned int ) lval ;
              }
              else {
                fprintf(stderr,"[port] number out of range\n") ;
                srv->port = DEFAULT_PORT ;
              }
            }
            else {
              fprintf(stderr,"[port] Non numeric value given for port\n" ) ;
            }
            break ;
    
          case NTHREADS:
            if( numstr( cp, &lval ) == SPOCP_SUCCESS ) {
              if( lval <= 0 ) {
                fprintf(stderr,"[threads] Error in specification, has to be > 0 \n" );
                return 0 ;
              }
              else {
                int level = ( int ) lval ;
    
                srv->threads = level ;
              }
            }
            else {
              fprintf( stderr, "[threads] Non numeric specification, not accepted\n" ) ;
              return 0 ;
            }
            break ;
    
          case SSLVERIFYDEPTH     :
            if( numstr( cp, &lval ) == SPOCP_SUCCESS ) {
              if( lval > 0L ) {
                srv->sslverifydepth = ( unsigned int ) lval ;
              }
              else {
                fprintf(stderr,"[sslverifydepth] number out of range\n") ;
                srv->sslverifydepth = 0 ;
              }
            }
            else {
              fprintf(stderr,"[sslverifydepth] Non numeric value given for port\n" ) ;
            }
            break ;
    
          case PIDFILE :
            if ( srv->pidfile ) free( srv->pidfile ) ;
            srv->pidfile = Strdup( cp ) ;
            break ;
    
          case MAXCONN :
            if( numstr( cp, &lval ) == SPOCP_SUCCESS ) {
              if( lval > 0L ) {
                srv->nconn = ( unsigned int ) lval ;
              }
              else {
                fprintf(stderr,"[nconn] number out of range\n") ;
                srv->sslverifydepth = 0 ;
              }
            }
            else {
              fprintf(stderr,"[nconn] Non numeric value given for port\n" ) ;
            }
            break ;
        }
        break ;
 
      case PLUGIN:
        pl = plugin_add_conf( plugins, plugin, s, cp ) ;
        if( !pl ) fprintf( stderr, "Error i rulefile, line %d\n", n ) ;
        else {
          if( !plugins ) plugins = pl ;
        }
        break ;

    }
  }

  if( srv->pidfile == 0 ) srv->pidfile = Strdup("spocp.pid") ;
  if( srv->timeout == 0 ) srv->timeout = DEFAULT_TIMEOUT ;
  if( srv->threads == 0 ) srv->threads = DEFAULT_NTHREADS ;
  if( srv->sslverifydepth == 0 ) srv->sslverifydepth = DEFAULT_SSL_DEPTH ;

  srv->root->db->plugins = plugins ;

  return 1 ;
}

/*
int set_backend_cachetime( srv_t *srv )
{
  int       i = 0 ;
  strarr_t *sa ;
  void     *vp = 0 ;

  traceLog("Setting backend cachetimes" ) ;

  conf_get( srv, CACHETIME, 0, &vp ) ;
  
  if( vp ) {
    sa = ( strarr_t * ) vp ;

    for( i = 0 ; i < sa->argc ; i++ ) {
      cachetime_new( sa->argv[i] ) ;
    }
  }

  return i ;
}
*/
