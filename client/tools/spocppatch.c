/***************************************************************************
                         spocpdiff.c  -  updates rules in a Spocp server
                             -------------------

    begin                : Wed Oct 7 2003
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

#include "spocpcli.h"

/*================================================================================*/
/*
 Expected usage:

 spocpdiff spocpserver [ diff_file ] 

 If diff_file is missing it will take its input from STDIN 

 diff_file should be produced with "diff -Naur"
 */

static void
print_usage(char *prog)
{
	printf("Usage: %s spocpserver [ diff_file ]\n", prog);
	exit(1);
}

int
main(int argc, char **argv)
{
	char buf[BUFSIZ], *cp, *dfile = 0, *subject = 0;
	int tls = 0, i, transaction = 0, rid = 0;
	FILE *fp;
	int res;
	char *certificate = 0;
	char *passwd = 0;
	char *privatekey = 0;
	char *calist = 0;
	char *server = 0;
	char *path, *ruleid, *s;
	SPOCP *spocp;
	queres_t qres;

	spocpc_debug = 0;

	while ((i = getopt(argc, argv, "dhtTua:c:s:f:p:l:")) != EOF) {
		switch (i) {
		case 'a':
			subject = strdup(optarg);
			break;

		case 'c':
			certificate = strdup(optarg);
			break;

		case 'd':
			spocpc_debug = 1;
			break;

		case 'f':
			dfile = strdup(optarg);
			break;

		case 'l':
			calist = strdup(optarg);
			break;

		case 'p':
			privatekey = strdup(optarg);
			break;

		case 's':
			server = strdup(optarg);
			break;

		case 't':
			tls = 1;
			break;

		case 'T':
			transaction = 1;
			break;

		case 'u':
			rid = 1;
			break;

		case 'w':
			passwd = strdup(optarg);
			break;

		case 'h':
		default:
			fprintf(stderr,
			    "Usage: %s -s server -f rulefile [-d] [-T] [-a subject] or\n",
			    argv[0]);
			fprintf(stderr, "%s -t -s server -f rulefile [-d]\n",
			    argv[0]);
			fprintf(stderr,
			    "\t -c certificate -l calist -p privatekey\n");
			fprintf(stderr,
			    "\t -w passwd\n");
			exit(0);
		}
	}

	if (server == 0)
		print_usage(argv[0]);

	spocp = spocpc_init(server);

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

	if (spocpc_debug)
		traceLog("Spocpserver [%s]", server);

	if ((spocp = spocpc_open(spocp, 0, 5)) == 0) {
		fprintf(stderr, "Could not open connection to \"%s\"\n",
		    server);
		exit(1);
	}

	if (dfile) {
		if ((fp = fopen(dfile, "r")) == 0) {
			fprintf(stderr, "Could not open file \"%s\"\n", dfile);
			exit(1);
		}
	} else {
		if (spocpc_debug)
			traceLog("reading stdin");
		fp = stdin;
	}

	memset(&qres, 0, sizeof(queres_t));

#ifdef HAVE_SSL
	if (tls) {
		if (spocpc_attempt_tls(spocp, &qres) == SPOCPC_OK &&
		    qres.rescode == SPOCP_SSL_START) {
			if (spocpc_start_tls(spocp) == 0)
				exit(1);
		} else
			exit(1);
	}
	memset(&qres, 0, sizeof(queres_t));
#endif

	if (transaction) {
		if (spocpc_open_transaction(spocp, &qres) != SPOCPC_OK ||
		    qres.rescode != SPOCP_SUCCESS)
			exit(1);

	}

	while (fgets(buf, BUFSIZ, fp)) {
		cp = &buf[strlen(buf) - 1];
		while (*cp == '\n' || *cp == '\r' || *cp == ' ')
			*cp-- = '\0';

		if (spocpc_debug)
			traceLog("Got [%s]", buf);

		if (strncmp(buf, "---", 3) == 0 ||
		    strncmp(buf, "+++", 3) == 0 ||
		    strncmp(buf, "@@", 2) == 0 || *buf == ' ')
			continue;

		s = cp = buf;
		if (*s == ' ')
			continue;

		/* should have path SP [ruleid SP] rule */
		/* should do some error checking here ! */
		path = ++cp;
		cp = index(path, ' ');
		*cp++ = '\0';

		if (rid) {	/* get ruleid */
			ruleid = cp;
			cp = index(ruleid, ' ');
			*cp++ = '\0';
		} else
			ruleid = 0;

		if (spocpc_debug)
			traceLog("path [%s]", path);

		memset(&qres, 0, sizeof(queres_t));

		if (*s == '+') {
			if (spocpc_debug)
				traceLog("rule [%s]", cp);
			res = spocpc_send_add(spocp, path, cp, 0, 0, &qres);
		} else if (rid) {	/* can't do delete without a ruleid */
			if (spocpc_debug)
				traceLog("ruleid [%s]", ruleid);
			res = spocpc_send_delete(spocp, path, ruleid, &qres);
		} else
			res = SPOCPC_PARAM_ERROR;

		if (res == SPOCPC_OK) {
			if (qres.rescode == SPOCP_SUCCESS)
				printf("OK\n");
			else if (qres.rescode == SPOCP_DENIED)
				printf("DENIED\n");
			else
				printf("ERROR (%d): %s", qres.rescode,
				    qres.str);
		} else
			printf("PROCESS ERROR: %d", res);
	}

	if (transaction) {
		memset(&qres, 0, sizeof(queres_t));
		if (spocpc_commit(spocp, &qres) != SPOCPC_OK ||
		    qres.rescode != SPOCP_SUCCESS)
			printf("ERROR commiting");
	}

	memset(&qres, 0, sizeof(queres_t));
	spocpc_send_logout(spocp);
	spocpc_close(spocp);
	free_spocp(spocp);

	exit(0);
}
