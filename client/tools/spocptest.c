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
	printf("Usage: %s spocpserver [ spocpquery ]\n", prog);
}

int
main(int argc, char **argv)
{
	char	buf[BUFSIZ], *cp, *rule, *path, *sp, *tmp;
	int	i, ok = 0, r, j;
	SPOCP	*spocp;
	queres_t	qres;
	octet_t		**o;

	spocpc_debug = 0;

	if (argc < 2) {
		print_usage(argv[0]);
		exit(1);
	}

	if (spocpc_debug)
		traceLog("[%d] arguments", argc);

	if ((spocp = spocpc_open(0, argv[1], 3)) == 0) {
		fprintf(stderr, "Could not open connection to \"%s\"\n",
		    argv[1]);
		exit(1);
	}
	if (spocpc_debug)
		traceLog("Spocpserver [%s]", argv[1]);

	if (argc > 2) {
		for (i = 2; i < argc; i++) {
			if (*argv[i] != '(') {
				path = argv[i];
				rule = index(path, ' ');
				if(rule)
					*rule++ = '\0';
			} else {
				rule = argv[i];
				path = 0;
			}

			memset(&qres, 0, sizeof(queres_t));

			r = spocpc_send_query(spocp, path, rule, &qres) ;

			if ( r == SPOCPC_OK && qres.rescode == SPOCP_SUCCESS) {
				if (qres.blob) {
					for (o = qres.blob->arr, j=0 ;
					    j < qres.blob->n ; o++, j++) {
						tmp = oct2strdup(*o, '\\');
						printf("[%s] ", tmp);
						free(tmp);
					}
					octarr_free(qres.blob);
				}
				printf("OK\n");
			} else
				printf("DENIED\n");

		}
	} else {
		if (spocpc_debug)
			traceLog("reading stdin");
		while (fgets(buf, BUFSIZ, stdin)) {
			cp = &buf[strlen(buf) - 1];
			while (*cp == '\n' || *cp == '\r' || *cp == ' ')
				*cp-- = '\0';

			if (spocpc_debug)
				traceLog("Got [%s]", buf);

			if (strncmp(buf, "OK ", 3) == 0) {
				ok = SPOCP_SUCCESS;
				sp = buf + 3;
			} else if (strncmp(buf, "DENIED ", 7) == 0) {
				ok = SPOCP_DENIED;
				sp = buf + 7;
			} else {
				ok = -1;
				sp = buf;
			}

			if (*sp != '(') {
				path = sp;
				rule = index(path, ' ');
				*rule++ = '\0';
			} else {
				rule = sp;
				path = 0;
			}

			memset(&qres, 0, sizeof(queres_t));

			r = spocpc_send_query(spocp, path, rule, &qres);

			if (r == SPOCPC_OK) {
				if (qres.rescode != ok) {
					printf("%d != %d on %s\n",
					    qres.rescode, ok,  rule);
				}
			}
		}
	}

	spocpc_send_logout(spocp, &qres);
	spocpc_close(spocp);
	free_spocp(spocp);

	exit(0);
}
