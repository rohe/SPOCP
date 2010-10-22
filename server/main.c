
/***************************************************************************
                           main.c  -  description
                             -------------------
    begin                : Sat Oct 12 2010
    copyright            : (C) 2010 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include "locl.h"

#define NTHREADS        5

#define DEF_CNFG	"config"

typedef void    Sigfunc(int);
void            sig_pipe(int signo);
void            sig_chld(int signo);
int 			received_sigterm = 0;
int				reread_rulefile = 1;

static ruleset_t    *_init_ruleset(srv_t *srvp, char *path);
static void         _hostname(char *localhost, char *path);
void                _local_context(srv_t *srvp, char *localhost);

srv_t               *srv = NULL;

/*
 * used by libwrap if it's there 
 */
int             allow_severity;
int             deny_severity;

#ifdef HAVE_SASL
static const struct sasl_callback sasl_cb[] = {
    { SASL_CB_GETOPT, &parse_sasl_conf, NULL },
	{ SASL_CB_LIST_END, NULL, NULL}
};
#endif

static Sigfunc        *
xsignal(int signo, Sigfunc * func)
{
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if (signo == SIGALRM) {
#ifdef SA_INTERUPT
		act.sa_flags |= SA_INTERUPT;
#endif
	} else {
#ifdef  SA_RESTART
		act.sa_flags |= SA_RESTART;
#endif
	}
	if (sigaction(signo, &act, &oact) < 0)
		return (SIG_ERR);

	return (oact.sa_handler);
}

void
sig_pipe(int signo)
{
	/*
	 * fprintf(stderr, "SIGPIPE received\n") ; 
	 */

	return;
}

void
sig_chld(int signo)
{
	pid_t           pid;
	int             cstat;

	while ((pid = waitpid(-1, &cstat, WNOHANG)) > 0);
	return;
}

static void
sig_term( int signo )
{
	received_sigterm = signo;
	traceLog(LOG_NOTICE, "Received SIGTERM");
	wake_listener(1);
}

static void
sig_int( int signo )
{
	srv_free( srv );
	/*
	if (cnfg != DEF_CNFG)
		Free( cnfg );
		*/

	exit(0);
}

static void
sig_usr1( int signo )
{
	struct stat	statbuf;
	work_info_t	wi;
	conn_t		*con;

	traceLog(LOG_NOTICE, "Received SIGUSR1");
	stat( srv->rulefile, &statbuf );
	if (statbuf.st_mtime != srv->mtime ) {
		srv->mtime = statbuf.st_mtime;
		con = conn_new();
		con->srv = srv;
		con->rs = srv->root;
		memset( &wi, 0, sizeof( work_info_t ));
		wi.conn = con;
		wi.routine = &do_reread_rulefile;

		reread_rulefile = 1;
		tpool_add_work( srv->work, &wi);
	}
}

spocp_result_t 
get_rules( srv_t *srvp )
{
	spocp_result_t  rc = SPOCP_SUCCESS;
	int             nr = 0, r;
	dbackdef_t      dbc;
	struct timeval  start, end;

	memset( &dbc, 0, sizeof( dbackdef_t ));
	dbc.dback = srvp->dback;

	if (srvp->dback) {
		if ((nr = dback_read_rules(&dbc, srvp, &rc)) < 0)
			return rc;
	}

	if (nr == 0) {
		if (srvp->rulefile) {
			LOG(SPOCP_INFO)
				traceLog(LOG_INFO, "Opening rules file \"%s\"",
				    srvp->rulefile);

			if (0)
				gettimeofday(&start, NULL);

			/*
			dbc.dback = srvp->dback;
			dbc.handle = 0;
			*/

			r = read_rules(srvp, srvp->rulefile, &dbc) ;
			if( r == -1 ) {
	 			LOG(SPOCP_ERR)
					traceLog(LOG_ERR,"Error while reading rules");
				rc = SPOCP_OPERATIONSERROR;
			}
			if (0) {
				gettimeofday(&end, NULL);
				print_elapsed("readrule time:", start, end);
			}
		}
		else
			LOG(SPOCP_INFO)
				traceLog(LOG_INFO, "No rule file to start with");
	} else {
		LOG(SPOCP_INFO)
		    traceLog(LOG_INFO,
			"Got the rules from the persistent store, will not read the rulefile");
	}

	return rc;
}

void _hostname(char *localhost, char *path)
{
	gethostname(localhost, MAXNAMLEN);
#ifdef HAVE_GETDOMAINNAME
	getdomainname(path, MAXNAMLEN);
#else
	{
		char *pos;
		if(pos = strstr(localhost, ".")) 
            strncpy(path, pos+1, MAXNAMLEN);
		else 
            strcpy(path, "");
	}
#endif
    
    if (0)
		printf("Domain: %s\n", path);    
    
}

void _local_context(srv_t *srvp, char *localhost)
{
    int l;
    
    if (srvp->name){
        l = strlen(srvp->name) + 3;
		srvp->localcontext = (char *) Calloc(l, sizeof(char));        
		/* Flawfinder: ignore */
		sprintf(srvp->localcontext, "//%s", srvp->name);		
	}
	else {
        l = strlen(localhost) + 3;
		srvp->localcontext = (char *) Calloc(l, sizeof(char));        
		/* Flawfinder: ignore */
		sprintf(srvp->localcontext, "//%s", localhost);
	}    
}

ruleset_t *
_init_ruleset(srv_t *srvp, char *path)
{
    ruleset_t *rs;
    
    /* srvp->root = ruleset_new(0); */

	/*
	 * where I put the access rules for access to this server and its
	 * rules 
	 */
	snprintf(path, MAXNAMLEN, "%s/server", srvp->localcontext);
    rs = ruleset_add(&srvp->root, path, NULL);
    if (rs == NULL)
        return rs;
    
	/*
	 * access rules for operations 
	 */
	snprintf(path, MAXNAMLEN, "%s/operation", srvp->localcontext);
    rs = ruleset_add(&srvp->root, path, NULL);
    
    return rs;
}

int
main(int argc, char **argv)
{
	int                 debug = 0, conftest = 0, nodaemon = 0;
	int                 i = 0, dyncnf=0;
	struct timeval      start, end;
	char                *cnfg = DEF_CNFG;
	char                localhost[MAXNAMLEN + 1], path[MAXNAMLEN + 1];
	FILE                *pidfp;
	ruleset_t           *rs;

	/*
	 * Who am I running as ? 
     uname(&myname);
	 */

	srv = (srv_t *) Calloc(1, sizeof(srv_t));
#ifdef HAVE_SASL
    srv->use_sasl = 1;
#endif
#ifdef HAVE_SSL
    srv->use_ssl = 1;
#endif
    
	while ((i = getopt(argc, argv, "ASDhrtf:d:")) != EOF) {
		switch (i) {

            case 'A':
                srv->use_sasl = 0;
                break;
                
            case 'S':
                srv->use_ssl = 0;
                break;
                
            case 'D':
                nodaemon = 1;
                break;

            case 'f':
                cnfg = Strdup(optarg);
                dyncnf = 1;
                break;

            case 'd':
                debug = atoi(optarg);
                if (debug < 0)
                    debug = 0;
                break;

            case 't':
                conftest = 1;
                break;

            case 'r':
                srv->readonly = 1;

            case 'h':
            default:
                fprintf(stderr, "Usage: %s [-t] ", argv[0]);
                fprintf(stderr, "[-f configfile] ");
                fprintf(stderr, "[-D] [-d debuglevel] [-r] [-S] [-A]ß\n");
                exit(0);
		}
	}

    _hostname(localhost, path);
	srv->hostname = Strdup(localhost);    

    _local_context(srv, localhost);
    if ((rs = _init_ruleset(srv, path)) == NULL)
        exit(1);
    
	if (srv_init(srv, cnfg) < 0)
		exit(1);
    
	if (srv->port && srv->uds) {
		fprintf(stderr,
			"Sorry are not allowed to listen on both a unix domain socket and a port\n");
		exit(1);
	}

	if (srv->logfile) {
		spocp_open_log(srv->logfile, debug);
    }
	else if (debug)
		spocp_open_log(0, debug);

    /* -- */

	LOG(SPOCP_INFO) {
		traceLog(LOG_INFO, "Local context: \"%s\"", srv->localcontext);
		traceLog(LOG_INFO, "initializing backends");
		if (srv->root->db)
			plugin_display(srv->plugin);
	}

	if (srv->plugin) {
		run_plugin_init(srv);
	}

	if ( get_rules( srv ) != SPOCP_SUCCESS ) 
		exit(1);

	/* If only testing configuration and rulefile this is as far as I go */
	if (conftest) {
		traceLog(LOG_INFO,"Configuration was OK");
		exit(0);
	}

	gettimeofday(&start, NULL);
    /* printf("-starting- %d/%s\n", srv->port,srv->uds ); */

	if (srv->port || srv->uds) {

		/*
		 * stdin and stdout will not be used from here on, close to
		 * save file descriptors 
		 */

		fclose(stdin);
		fclose(stdout);

#ifdef HAVE_SSL
		/*
		 * build our SSL context, disregarding whether it will ever 
         * be used or not 
		 */

        if (use_ssl) {
            /*
             * mutex'es for openSSL to use 
             */
            THREAD_setup();
            
            if (srv->certificateFile && srv->privateKey && srv->caList) {
                traceLog(LOG_INFO,"Initializing the TLS/SSL environment");
                if (!(srv->ctx = tls_init(&srv))) {
                    return FALSE;
                }
            }
        }
#endif

#ifdef HAVE_SASL
		if (srv->use_sasl) {
			int r = sasl_server_init(sasl_cb, "spocp");
            
			if (r != SASL_OK) {
				traceLog( LOG_ERR,
				    "Unable to initialized SASL library: %s",
				     sasl_errstring(r, NULL, NULL));
				return FALSE;
			}
		}
#endif

        /* initiate the server access configuration */
		saci_init(srv->use_ssl, srv->use_sasl);
        
		if( nodaemon == 0 ) { 
#ifdef HAVE_DAEMON
			if (daemon(1, 1) < 0) {
				fprintf(stderr, "couldn't go daemon\n");
				exit(1);
			}
#else
			daemon_init();
#endif
		}

		if (srv->pidfile) {
			/*
			 * Write the PID file. 
			 */
			pidfp = fopen(srv->pidfile, "w");

			if (pidfp == (FILE *) 0) {
				fprintf(stderr, "Couldn't open pidfile \"%s\"\n",
                        srv->pidfile);
				exit(1);
			}
			fprintf(pidfp, "%d\n", (int) getpid());
			fclose(pidfp);
		}

		if (srv->port) {
			LOG(SPOCP_INFO) 
                traceLog( LOG_INFO, "Asked to listen on port %d", srv->port);

			if ((srv->listen_fd = spocp_stream_socket(srv->port)) < 0)
				exit(1);

			sprintf(srv->id, "spocp-%d", srv->port);

			srv->type = AF_INET;
		} else {
			LOG(SPOCP_INFO)
			    traceLog(LOG_INFO,"Asked to listen on unix domain socket");
			if ((srv->listen_fd = spocp_unix_domain_socket(srv->uds)) < 0)
				exit(1);

			/* Flawfinder: ignore */
			sprintf(srv->id, "spocp-%s", srv->uds);

			srv->type = AF_UNIX;
		}

		xsignal(SIGCHLD, sig_chld);
		xsignal(SIGPIPE, sig_pipe);
		xsignal(SIGINT, sig_int);
		xsignal(SIGTERM, sig_term);
		xsignal(SIGUSR1, sig_usr1);

		DEBUG(SPOCP_DSRV) 
            traceLog(LOG_DEBUG,"Creating threads");
        
		/*
		 * returns the pool the threads are picking work from 
		 */
		srv->work = tpool_init(srv->threads, 64, 1);
        if (srv->work == NULL)
            exit(1);
        DEBUG(SPOCP_DSRV)
            traceLog(LOG_DEBUG, "srv->work:%p", srv->work);
        
        /* sleep a while, allowing the threads to all be in order */
        sleep(1);
		spocp_srv_run(srv);

	} else {
		conn_t         *conn;

		saci_init(0, 0);
		DEBUG(SPOCP_DSRV) traceLog(LOG_DEBUG,"---->");

		LOG(SPOCP_INFO) traceLog(LOG_INFO,"Reading STDIN");

		/*
		 * If I want to use this I have to do init_server() first
		 * conn = spocp_open_connection( STDIN_FILENO, &srv ) ; 
		 */
		/*
		 * this is much simpler 
		 */
		conn = conn_new();
		conn_setup(conn, srv, STDIN_FILENO, "localhost", "127.0.0.1");

		LOG(SPOCP_INFO) traceLog(LOG_INFO,"Running server");

		spocp_server((void *) conn);

		gettimeofday(&end, NULL);

		print_elapsed("query time:", start, end);

		conn_free( conn );
	}

	srv_free( srv );
	if (dyncnf)
		Free( cnfg );

    /* printf("-exiting-"); */
	exit(0);
}


