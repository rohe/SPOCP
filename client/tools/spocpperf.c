/***************************************************************************
                         spocptest.c  -  sends queries to a Spocp server
                             -------------------

    begin                : Wed Oct 5 2003
    copyright            : (C) 2003 by Stockholm University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 **************************************************************************/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "spocpcli.h"

/*================================================================================*/
/*
 Expected usage:

 spocptest spocpserver *( spocpquery ) 

 If spocpquery is missing it will take queries from STDIN 
 */

static void
print_usage(char *prog)
{
	printf
	    ("Usage: %s -s <spocpserver> -f <queryfile> -n <number of connections>\n",
	    prog);
	printf("\t-q <number of queries> -d -r <repetitions>\n");
	printf("\t-t <tls> -p <privatekey> -q <passwd> -c <key> -l <calist> -\n");
}

int
main(int argc, char **argv)
{
	char		buf[BUFSIZ], *cp, *tmp;
	char		*filename = 0, *server = 0;
	char		*certificate = 0, *privatekey = 0, *calist = 0, *passwd = 0;
	int		tls = 0;
	int		i, j, k, nq = 512, nc = 1, repeat = 0, n = 0;
	SPOCP		*spocp;
	struct timeval	start, end;
	FILE 		*fp;
	pid_t		pid;
	queres_t	qres;
	octet_t		**o, path, *query;
	octarr_t	*arr = 0;

	spocpc_debug = 0;

	while ((i = getopt(argc, argv, "c:df:hl:n:p:q:r:s:t:w:")) != EOF) {
		switch (i) {

		case 'c':
			certificate = strdup(optarg);
			break;

		case 'd':
			spocpc_debug = 1;
			break;

		case 'f':
			filename = strdup(optarg);
			break;

		case 'l':
			calist = strdup(optarg);
			break;

		case 'n':
			nc = atoi(optarg);
			break;

		case 'p':
			privatekey = strdup(optarg);
			break;

		case 'q':
			nq = atoi(optarg);
			break;

		case 'r':
			repeat = atoi(optarg);
			break;

		case 's':
			server = strdup(optarg);
			break;

		case 't':
			tls = atoi(optarg);
			break;

		case 'w':
			passwd = strdup(optarg);
			break;

		case 'h':
		default:
			print_usage(argv[0]);
			exit(0);
		}
	}

	if (server == 0) {
		print_usage(argv[0]);
		exit(0);
	}

	if (repeat) {
		arr = octarr_new(nq);
		i = 0;
	}

	spocp = spocpc_init(server, 0, 0);

	/* parent also sends queries */
	for (j = 1; j < nc; j++) {
		if (fork() == 0)
			break;
	}

#ifdef HAVE_SSL
	if (tls) {
		spocpc_use_tls(spocp);
		spocpc_tls_use_cert(spocp, certificate);
		spocpc_tls_use_calist(spocp, calist);
		spocpc_tls_use_key(spocp, privatekey);
		spocpc_tls_use_passwd(spocp, passwd);

		if (tls & DEMAND)
			spocpc_tls_set_demand_server_cert(spocp);
		if (tls & VERIFY)
			spocpc_tls_set_verify_server_cert(spocp);
	}
#else
	{
		fprintf(stderr, "Can't use TLS/SSL, ignoring\n");
		tls = 0;
	}
#endif

	pid = getpid();

	if ((spocp = spocpc_open( spocp, 0, 5)) == 0) {
		fprintf(stderr, "Could not open connection to \"%s\"\n",
		    server);
		exit(1);
	}

#ifdef HAVE_SSL
	if (tls) {
		if ((spocpc_attempt_tls(spocp, &qres) == SPOCPC_OK) &&
		    qres.rescode == SPOCP_SSL_START) {
			if (spocpc_start_tls(spocp) == 0)
				exit(1);
		} else
			exit(1);
	}
	memset(&qres, 0, sizeof(queres_t));
#endif

	if (filename) {
		fp = fopen(filename, "r");
		if (fp == 0) {
			fprintf(stderr, "couldn't open \"%s\"\n", filename);
		}
	} else
		fp = stdin;

	gettimeofday(&start, NULL);

	memset( &path,0, sizeof( octet_t ));

	while (fgets(buf, 512, fp)) {
		cp = &buf[strlen(buf) - 1];
		while (*cp == '\n' || *cp == ' ')
			*cp-- = '\0';

		query = sexp_normalize( buf );
		if( query == 0 ) {
			fprintf(stderr,"[%s] not a s-expression\n", buf);
			continue ;
		}

		if (repeat)
			octarr_add(arr, query);

		memset(&qres, 0, sizeof(queres_t));
		if (spocpc_send_query(spocp, &path, query, &qres) == SPOCPC_OK &&
		    qres.rescode == SPOCP_SUCCESS) {
			if (qres.blob) {
				int l;
				for (o = qres.blob->arr, l=0;
				   l<qres.blob->n; l++,o++) {
					tmp = oct2strdup(*o, '\\');
					printf("[%s] ", tmp);
					free(tmp);
				}
				octarr_free(qres.blob);
			}
		}

		n++;
	}

	for (j = 0; j < repeat; j++) {
		for (k = 0; k < arr->n; k++) {
			memset(&qres, 0, sizeof(queres_t));
			if (spocpc_send_query(spocp, &path, arr->arr[k],
				&qres) == SPOCP_SUCCESS) {
				if (qres.blob) {
					int l;
					for (o = qres.blob->arr, l=0;
					   l < qres.blob->n; o++, l++) {
						tmp = oct2strdup(*o, '\\');
						printf("[%s] ", tmp);
						free(tmp);
					}
					octarr_free(qres.blob);
				}
			}
			n++;
		}
	}

	gettimeofday(&end, NULL);

	printf("[%d] %d queries in ", (int) pid, n);
	print_elapsed("", start, end);

	memset(&qres, 0, sizeof(queres_t));
	spocpc_send_logout(spocp);
	spocpc_close(spocp);
	free_spocp(spocp);

	exit(0);
}
