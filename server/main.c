/***************************************************************************
                           main.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <config.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>  
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/utsname.h>

#include <pthread.h>

#ifdef HAVE_LIBSSL
#include <openssl/ssl.h>
#endif

#include <srv.h>
#include <spocp.h>
#include <func.h>
#include <macros.h>
#include <wrappers.h>

/* development thread :-) */
#define NTHREADS        5

/* srv_t    srv ; */

void thread_make(srv_t *srv, int i) ;

/*
int  timeout = 0 ;
int  threads = 0 ;
char *certificateFile = 0 ;
char *privateKey = 0 ;
char *caList = 0 ;
char *dhFile = 0 ;
char *SslEntropyFile = 0 ;
char *passwd = 0 ;

char *logfile = 0 ;
char *pidfile = 0 ;
int  port = 0 ;
char *uds = 0 ;
char *rulefile = 0 ;
int  sslverifydepth = 0 ;
char *server_id = 0 ;
int  srvtype = NATIVE ;
*/

typedef void Sigfunc(int) ;
void sig_pipe( int signo ) ;
void sig_chld(int signo) ;
 
/* used by libwrap if it's there */
int allow_severity ;
int deny_severity ;

Sigfunc * signal( int signo, Sigfunc *func )
{
  struct sigaction act,oact ;

  act.sa_handler = func ;
  sigemptyset(&act.sa_mask) ;
  act.sa_flags = 0 ;

  if( signo == SIGALRM) {
#ifdef SA_INTERUPT
  act.sa_flags |= SA_INTERUPT ;
#endif
  } else {
#ifdef  SA_RESTART
  act.sa_flags |= SA_RESTART ;
#endif
  }
  if( sigaction(signo,&act,&oact) < 0 )
    return(SIG_ERR) ;

  return(oact.sa_handler) ;
}

void sig_pipe( int signo )
{
  /*
  fprintf(stderr, "SIGPIPE received\n") ;
  */

  return ;
}

void sig_chld(int signo)
{
  pid_t pid ;
  int   stat ;

  while((pid = waitpid(-1,&stat,WNOHANG)) > 0 )  ;
  return ;
}

int main( int  argc, char **argv )
{
  int                 debug = 0 ;
  int                 i = 0, n ;
  unsigned int        clilen ;
  struct sockaddr_in  cliaddr ;
  struct timeval      start, end ;
  char                *cnfg = "config", localhost[NAME_MAX+1], path[NAME_MAX+1] ;
  srv_t               srv ;
  keyval_t            *globals = 0 ;
  FILE                *pidfp ;
  octet_t             oct ;

  /* Who am I running as ? */

  uname( &myname ) ;

  /*spocp_err = 0 ;*/

  memset( &srv, 0, sizeof( srv_t )) ;
  srv.protocol = NATIVE ;

  pthread_mutex_init( &(srv.mutex), NULL ) ;
  pthread_mutex_init( &(srv.mlock), NULL ) ;

  gethostname( localhost, NAME_MAX ) ;

  localcontext = ( char *) Calloc( strlen(localhost)+strlen( "//" ) + 1,
                                   sizeof( char) ) ;

  sprintf( localcontext, "//%s", localhost ) ;

  /*
   * truncating input strings to reasonable length
   */
  for( i = 0 ; i < argc ; i++ )
    if( strlen( argv[i] ) > 512 ) argv[i][512] = '\0' ;

  while(( i = getopt(argc,argv,"hf:d:")) != EOF ) {
    switch(i) {

      case 'f':
        cnfg = Strdup(optarg) ;
        break ;

      case 'd' :
	debug = atoi(optarg) ;
	if( debug < 0 ) debug = 0 ;
        break ;

/*
      case 's':
        srv.protocol = SOAP ;
        break ;
*/

      case 'h' :
      default:
        fprintf(stderr,"Usage: %s [-f configfile] [-d debuglevel]\n", argv[0]) ;
        exit(0) ;
    }
  }

  if( srv.root == 0 ) srv.root = ruleset_new( 0 ) ;
 
  /* where I put the access rules for access to this server and its rules */
  sprintf( path, "%s/server", localcontext) ;
  oct.val = path ;
  oct.len = strlen(path) ;
  ruleset_create( &oct, &srv.root ) ;

  sprintf( path, "%s/rules", localcontext ) ;
  oct.val = path ;
  oct.len = strlen(path) ;
  ruleset_create( &oct, &srv.root ) ;

  if( read_config( cnfg, &srv ) == 0 ) exit( 1 ) ;

  if( srv.port && srv.uds ) {
    fprintf(stderr,
           "Sorry are not allowed to listen on both a unix domain socket and a port\n") ;
    exit( 1 ) ;
  }

  if( srv.logfile ) spocp_open_log( srv.logfile, debug ) ;
  else if( debug ) spocp_open_log( 0, debug ) ;


  traceLog( "Local context: \"%s\"", localcontext ) ;
  traceLog( "initializing backends" ) ; 
  if( srv.root->db ) init_plugin( srv.root->db->plugins ) ;
  LOG( SPOCP_INFO ) if( srv.root->db ) plugin_display( srv.root->db->plugins ) ;
  if( srv.root->db->plugins ) run_plugin_init( &srv ) ;

  if( srv.rulefile == 0 ) {
    LOG( SPOCP_INFO ) traceLog( "No rule file to start with" ) ;
  }
  else {
    LOG( SPOCP_INFO ) traceLog( "Opening rules file \"%s\"", srv.rulefile ) ;
  }

  if( 0 ) gettimeofday(&start,NULL) ;

  if( srv.rulefile &&
      ( srv.root = read_rules( srv.root, srv.rulefile, &n, &globals )) == 0 ) {
    LOG( SPOCP_ERR ) traceLog( "No valid rules found") ;
    exit(1) ;
  }

  if( 0 ) {
    gettimeofday(&end,NULL) ;
    print_elapsed( "readrule time:", start, end ) ;
  }

  gettimeofday(&start,NULL) ;

  if( srv.port || srv.uds ) {

    /* stdin and stdout will not be used from here on, close to save
       file descriptors */

    fclose( stdin ) ;
    fclose( stdout ) ;

#ifdef HAVE_LIBSSL
    /* ---------------------------------------------------------- */
    /* build our SSL context, whether it will ever be used or not */

    /* mutex'es for openSSL to use */
    THREAD_setup() ;

    if( srv.certificateFile && srv.privateKey && srv.caList ) {
      if(!( srv.ctx = tls_init( &srv ))) {
        return FALSE ;
      }
    }

    /* ---------------------------------------------------------- */
#endif

    saci_init() ;
    daemon_init( "spocp", 0) ;

    if ( srv.pidfile ) {
      /* Write the PID file. */
      pidfp = fopen( srv.pidfile, "w" );

      if ( pidfp == (FILE*) 0 ) {
        fprintf( stderr, "Couldn't open pidfile \"%s\"\n", srv.pidfile );
        exit( 1 );
      }
      fprintf( pidfp, "%d\n", (int) getpid() );
      fclose( pidfp );
    }

    if( srv.port ) {
      LOG( SPOCP_INFO ) traceLog( "Asked to listen on port %d", srv.port ) ;

      if(( srv.listen_fd = spocp_stream_socket( srv.port )) < 0 ) 
        exit( 1 ) ;

      srv.id = ( char *) Malloc( 16 ) ;
      sprintf( srv.id, "spocp-%d", srv.port ) ;

      srv.type = AF_INET ;
    }
    else {
      LOG( SPOCP_INFO ) traceLog( "Asked to listen on unix domain socket" ) ;
      if(( srv.listen_fd = spocp_unix_domain_socket( srv.uds )) < 0 ) 
        exit(1) ;

      srv.id = ( char *) Malloc( 7+strlen( srv.uds ) ) ;
      sprintf( srv.id, "spocp-%s", srv.uds ) ;

      srv.type = AF_LOCAL ;
    }

    signal(SIGCHLD, sig_chld) ;
    signal(SIGPIPE, sig_pipe) ;

    clilen = sizeof(cliaddr) ;

    DEBUG( SPOCP_DSRV ) traceLog("Creating threads") ;

    srv.worker = (thread_t *) Calloc( srv.threads, sizeof( thread_t )) ;

    /* let 'em lose */
    for( i = 0 ; i < srv.threads ; i++ ) {
      thread_make( &srv, i ) ;
    }

   /*signal( SIGINT, sig_int ) ;*/

    LOG( SPOCP_INFO ) traceLog( "server started") ;

    for ( ; ; ) pause() ; /* get out of the way */
    
  }
  else {
    conn_t *conn ;

    DEBUG( SPOCP_DSRV ) traceLog("---->") ;

    LOG( SPOCP_INFO ) traceLog( "Reading STDIN" ) ;
    conn = ( conn_t *) Calloc( 1, sizeof( conn_t )) ;
    conn->fd = STDIN_FILENO ;
    conn->srv = &srv ;
    init_connection( conn ) ;

    LOG( SPOCP_INFO ) traceLog( "Running server" ) ;
    spocp_server( (void *) conn ) ;
 
    gettimeofday(&end,NULL) ;
    print_elapsed( "query time:", start, end ) ;
  }

  exit(0) ;
}

