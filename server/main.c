
/***************************************************************************
                           main.c  -  description
                             -------------------
    begin                : Sat Oct 12 2002
    copyright            : (C) 2002 by Umeå University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include "locl.h"
RCSID("$Id$");

/*
 * development thread :-) 
 */
#define NTHREADS        5

/*
 * srv_t srv ; 
 */

typedef void    Sigfunc(int);
void            sig_pipe(int signo);
void            sig_chld(int signo);

/*
 * used by libwrap if it's there 
 */
int             allow_severity;
int             deny_severity;

Sigfunc        *
signal(int signo, Sigfunc * func)
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
	int             stat;

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
	return;
}

int
main(int argc, char **argv)
{
	int             debug = 0;
	int             i = 0, nrules = 0;
	unsigned int    clilen;
	struct sockaddr_in cliaddr;
	struct timeval  start, end;
	char           *cnfg =
	    "config", localhost[MAXNAMLEN + 1], path[MAXNAMLEN + 1];
	srv_t           srv;
	FILE           *pidfp;
	octet_t         oct;
	ruleset_t      *rs;

	/*
	 * Who am I running as ? 
	 */

	uname(&myname);

	/*
	 * spocp_err = 0 ;
	 */

	memset(&srv, 0, sizeof(srv_t));

	pthread_mutex_init(&(srv.mutex), NULL);
	pthread_mutex_init(&(srv.mlock), NULL);

	gethostname(localhost, MAXNAMLEN);
	getdomainname(path, MAXNAMLEN);

	if (0)
		printf("Domain: %s\n", path);

	srv.hostname = strdup(localhost);

	localcontext = (char *) Calloc(strlen(localhost) + strlen("//") + 1,
				       sizeof(char));

	sprintf(localcontext, "//%s", localhost);

	/*
	 * truncating input strings to reasonable length
	 */
	for (i = 0; i < argc; i++)
		if (strlen(argv[i]) > 512)
			argv[i][512] = '\0';

	while ((i = getopt(argc, argv, "hf:d:")) != EOF) {
		switch (i) {

		case 'f':
			cnfg = Strdup(optarg);
			break;

		case 'd':
			debug = atoi(optarg);
			if (debug < 0)
				debug = 0;
			break;

		case 'h':
		default:
			fprintf(stderr,
				"Usage: %s [-f configfile] [-d debuglevel]\n",
				argv[0]);
			exit(0);
		}
	}

	if (srv.root == 0)
		srv.root = ruleset_new(0);

	/*
	 * where I put the access rules for access to this server and its
	 * rules 
	 */
	sprintf(path, "%s/server", localcontext);
	oct.val = path;
	oct.len = strlen(path);
	if ((rs = ruleset_create(&oct, &srv.root)) == 0)
		exit(1);
	rs->db = db_new();

	/*
	 * access rules for operations 
	 */
	sprintf(path, "%s/operation", localcontext);
	oct.val = path;
	oct.len = strlen(path);
	if ((rs = ruleset_create(&oct, &srv.root)) == 0)
		exit(1);
	rs->db = db_new();

	if (srv_init(&srv, cnfg) < 0)
		exit(1);

	if (srv.port && srv.uds) {
		fprintf(stderr,
			"Sorry are not allowed to listen on both a unix domain socket and a port\n");
		exit(1);
	}

	if (srv.logfile)
		spocp_open_log(srv.logfile, debug);
	else if (debug)
		spocp_open_log(0, debug);


	traceLog("Local context: \"%s\"", localcontext);
	traceLog("initializing backends");
	LOG(SPOCP_INFO) if (srv.root->db)
		plugin_display(srv.plugin);
	if (srv.plugin) {
		run_plugin_init(&srv);
	}
	if (srv.dback) {
		dbcmd_t         dbc;
		spocp_result_t  rc;

		dbc.handle = 0;
		dbc.dback = srv.dback;
		/*
		 * does the persistent database store need any initialization
		 * ?? * if( srv.dback->init ) srv.dback->init( dbcmd_t *dbc )
		 * ; 
		 */

		if ((nrules = dback_read_rules(&dbc, &srv, &rc)) < 0)
			exit(1);
	}

	if (nrules == 0) {
		dbcmd_t         dbc;

		if (srv.rulefile == 0) {
			LOG(SPOCP_INFO) traceLog("No rule file to start with");
		} else {
			LOG(SPOCP_INFO) traceLog("Opening rules file \"%s\"",
						 srv.rulefile);
		}

		if (0)
			gettimeofday(&start, NULL);

		dbc.dback = srv.dback;
		dbc.handle = 0;

		if (srv.rulefile
		    && read_rules(&srv, srv.rulefile, &dbc) != 0) {
			LOG(SPOCP_ERR) traceLog("Error while reading rules");
			exit(1);
		}

		if (0) {
			gettimeofday(&end, NULL);
			print_elapsed("readrule time:", start, end);
		}
	} else {
		LOG(SPOCP_INFO)
		    traceLog
		    ("Got the rules from the persistent store, will not read the rulefile");
	}

	gettimeofday(&start, NULL);

	if (srv.port || srv.uds) {

		/*
		 * stdin and stdout will not be used from here on, close to
		 * save file descriptors 
		 */

		fclose(stdin);
		fclose(stdout);

#ifdef HAVE_SSL
		/*
		 * ---------------------------------------------------------- 
		 */
		/*
		 * build our SSL context, whether it will ever be used or not 
		 */

		/*
		 * mutex'es for openSSL to use 
		 */
		THREAD_setup();

		if (srv.certificateFile && srv.privateKey && srv.caList) {
			traceLog("Initializing the TLS/SSL environment");
			if (!(srv.ctx = tls_init(&srv))) {
				return FALSE;
			}
		}

		/*
		 * ---------------------------------------------------------- 
		 */
#endif

#ifdef HAVE_SASL
		{
			int             r = sasl_server_init(NULL, "spocp");
			if (r != SASL_OK) {
				traceLog
				    ("Unable to initialized SASL library: %s",
				     sasl_errstring(r, NULL, NULL));
				return FALSE;
			}
		}
#endif

		saci_init();
#ifdef HAVE_DAEMON
		if (daemon(1, 1) < 0) {
			fprintf(stderr, "couldn't go daemon\n");
			exit(1);
		}
#else
		daemon_init("spocp", 0);
#endif

		if (srv.pidfile) {
			/*
			 * Write the PID file. 
			 */
			pidfp = fopen(srv.pidfile, "w");

			if (pidfp == (FILE *) 0) {
				fprintf(stderr,
					"Couldn't open pidfile \"%s\"\n",
					srv.pidfile);
				exit(1);
			}
			fprintf(pidfp, "%d\n", (int) getpid());
			fclose(pidfp);
		}

		if (srv.port) {
			LOG(SPOCP_INFO) traceLog("Asked to listen on port %d",
						 srv.port);

			if ((srv.listen_fd =
			     spocp_stream_socket(srv.port)) < 0)
				exit(1);

			srv.id = (char *) Malloc(16);
			sprintf(srv.id, "spocp-%d", srv.port);

			srv.type = AF_INET;
		} else {
			LOG(SPOCP_INFO)
			    traceLog("Asked to listen on unix domain socket");
			if ((srv.listen_fd =
			     spocp_unix_domain_socket(srv.uds)) < 0)
				exit(1);

			srv.id = (char *) Malloc(7 + strlen(srv.uds));
			sprintf(srv.id, "spocp-%s", srv.uds);

			srv.type = AF_LOCAL;
		}

		signal(SIGCHLD, sig_chld);
		signal(SIGPIPE, sig_pipe);

		clilen = sizeof(cliaddr);

		DEBUG(SPOCP_DSRV) traceLog("Creating threads");
		/*
		 * returns the pool the threads are picking work from 
		 */
		srv.work = tpool_init(srv.threads, 64, 1);

		spocp_srv_run(&srv);

	} else {
		conn_t         *conn;

		saci_init();
		DEBUG(SPOCP_DSRV) traceLog("---->");

		LOG(SPOCP_INFO) traceLog("Reading STDIN");

		/*
		 * If I want to use this I have to do init_server() first
		 * conn = spocp_open_connection( STDIN_FILENO, &srv ) ; 
		 */
		/*
		 * this is much simpler 
		 */
		conn = conn_new();
		conn_setup(conn, &srv, STDIN_FILENO, "localhost", "127.0.0.1");

		LOG(SPOCP_INFO) traceLog("Running server");

		spocp_server((void *) conn);

		gettimeofday(&end, NULL);

		print_elapsed("query time:", start, end);
	}

	exit(0);
}
