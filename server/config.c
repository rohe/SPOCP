
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

#define RULEFILE		1
#define PORT			2
#define UNIXDOMAINSOCKET	3
#define CERTIFICATE		4
#define PRIVATEKEY		5
#define CALIST			6
#define DHFILE			7
#define ENTROPYFILE		8
#define PASSWD			9
#define NTHREADS		10
#define TIMEOUT			11
#define LOGFILE			12
#define SSLVERIFYDEPTH		13
#define PIDFILE			14
#define MAXCONN			15
#define CLIENTCERT              16

char           *keyword[] = {
	"__querty__", "rulefile", "port", "unixdomainsocket", "certificate",
	"privatekey", "calist", "dhfile", "entropyfile",
	"passwd", "threads", "timeout", "logfile", "sslverifydepth",
	"pidfile", "maxconn", "clientcert", NULL
};


#define DEFAULT_PORT    9953	/* we should register one with IANA */
#define DEFAULT_TIMEOUT   30
#define DEFAULT_NTHREADS   5
#define DEFAULT_SSL_DEPTH  1

#define LINEBUF         1024

#define SYSTEM 20
#define PLUGIN 21
#define DBACK  22

/*
 * roughly 
 */
#define YEAR  31536000

extern char    *pidfile;

char           *err_msg = "Error in configuration file, line %d: %s";

/*------------------------------------------------------------------ */

spocp_result_t
conf_get(void *vp, int arg, void **res)
{
	srv_t          *srv = (srv_t *) vp;

	switch (arg) {
	case RULEFILE:
		*res = (void *) srv->rulefile;
		break;

	case PORT:
		*res = (void *) &srv->port;
		break;

	case UNIXDOMAINSOCKET:
		*res = (void *) srv->uds;
		break;

	case CERTIFICATE:
		*res = (void *) srv->certificateFile;
		break;

	case PRIVATEKEY:
		*res = (void *) srv->privateKey;
		break;

	case CALIST:
		*res = (void *) srv->caList;
		break;

	case DHFILE:
		*res = (void *) srv->dhFile;
		break;

	case ENTROPYFILE:
		*res = (void *) srv->SslEntropyFile;
		break;

	case PASSWD:
		*res = (void *) srv->passwd;
		break;

	case NTHREADS:
		*res = (void *) &srv->threads;
		break;

	case TIMEOUT:
		*res = &srv->timeout;
		break;

	case LOGFILE:
		*res = (void *) srv->logfile;
		break;

	case SSLVERIFYDEPTH:
		*res = (void *) srv->sslverifydepth;
		break;

	case PIDFILE:
		*res = (void *) srv->pidfile;
		break;

	case MAXCONN:
		*res = (void *) &srv->nconn;
		break;

	}

	return SPOCP_SUCCESS;
}

/*------------------------------------------------------------------ */

/*
 * In some cases will gladly overwrite what has been previously specified, if
 * the same keyword appears twice 
 */

int
read_config(char *file, srv_t * srv)
{
	FILE           *fp;
	char            s[LINEBUF], *cp, *sp, pluginname[256];
	char            section = 0, *dbname = 0, *dbload = 0;
	unsigned int    n = 0;
	long            lval;
	int             i;
	plugin_t       *plugins, *pl = 0;
	dback_t        *dbp = 0;
	const conf_com_t *ccp;
	spocp_result_t  r;

	/*
	 * should never be necessary 
	 */
	if (!srv->root)
		srv->root = ruleset_new(0);

	if (!srv->root->db)
		srv->root->db = db_new();

	plugins = srv->plugin;

	if ((fp = fopen(file, "r")) == NULL) {
		traceLog
		    ("Could not find or open the configuration file \"%s\"",
		     file);
		return 0;
	}

	while (fgets(s, LINEBUF, fp)) {
		n++;
		rmcrlf(s);

		if (*s == 0 || *s == '#')
			continue;

		/*
		 * New section 
		 */
		if (*s == '[') {

			cp = find_balancing(s + 1, '[', ']');
			if (cp == 0) {
				traceLog(err_msg, n, "Section specification");
				return 0;
			}

			*cp = 0;
			sp = s + 1;

			if (strcasecmp(sp, "server") == 0)
				section = SYSTEM;
			else if (strcasecmp(sp, "dback") == 0)
				section = DBACK;
			else {
				section = PLUGIN;
				strcpy(pluginname, sp);
				pl = 0;
			}

			continue;
		}

		/*
		 * Within a section The directives are of the form: key *SP
		 * "=" *SP val *SP val val = 1*nonspacechar / '"' char '"' 
		 */

		rm_lt_sp(s, 1);	/* remove leading and trailing blanks */

		/*
		 * empty line or comment 
		 */
		if (*s == 0 || *s == '#')
			continue;

		cp = strchr(s, '=');
		if (cp == 0) {
			traceLog(err_msg, n, "syntax error");
			continue;
		}

		sp = cp;
		for (*cp++ = '\0'; *cp && (*cp == ' ' || *cp == '\t'); cp++)
			*cp = '\0';
		for (sp--; sp >= s && (*sp == ' ' || *sp == '\t'); sp--)
			*sp = '\0';

		/*
		 * no key, not good 
		 */
		if (*s == '\0')
			continue;

		switch (section) {
		case SYSTEM:
			for (i = 1; keyword[i]; i++)
				if (strcasecmp(keyword[i], s) == 0)
					break;

			if (keyword[i] == 0) {
				traceLog(err_msg, n, "Unknown keyword");
				continue;
			}

			switch (i) {
			case RULEFILE:
				if (srv->rulefile)
					free(srv->rulefile);
				srv->rulefile = Strdup(cp);
				break;

			case CERTIFICATE:
				if (srv->certificateFile)
					free(srv->certificateFile);
				srv->certificateFile = Strdup(cp);
				break;

			case PRIVATEKEY:
				if (srv->privateKey)
					free(srv->privateKey);
				srv->privateKey = Strdup(cp);
				break;

			case CALIST:
				if (srv->caList)
					free(srv->caList);
				srv->caList = Strdup(cp);
				break;

			case DHFILE:
				if (srv->dhFile)
					free(srv->dhFile);
				srv->dhFile = Strdup(cp);
				break;

			case ENTROPYFILE:
				if (srv->SslEntropyFile)
					free(srv->SslEntropyFile);
				srv->SslEntropyFile = Strdup(cp);
				break;

			case PASSWD:
				if (srv->passwd)
					free(srv->passwd);
				srv->passwd = Strdup(cp);
				break;

			case LOGFILE:
				if (srv->logfile)
					free(srv->logfile);
				srv->logfile = Strdup(cp);
				break;

			case TIMEOUT:
				if (numstr(cp, &lval) == SPOCP_SUCCESS) {
					if (lval >= 0 && lval <= YEAR)
						srv->timeout =
						    (unsigned int) lval;
					else {
						traceLog(err_msg, n,
							 "Value out of range");
						srv->timeout = DEFAULT_TIMEOUT;
					}
				} else {
					traceLog(err_msg, n,
						 "Non numeric value");
					srv->timeout = DEFAULT_TIMEOUT;
				}

				break;

			case UNIXDOMAINSOCKET:
				if (srv->uds)
					free(srv->uds);
				srv->uds = Strdup(cp);
				break;

			case PORT:
				if (numstr(cp, &lval) == SPOCP_SUCCESS) {
					if (lval > 0L && lval < 65536) {
						srv->port =
						    (unsigned int) lval;
					} else {
						traceLog(err_msg, n,
							 "Number out of range");
						srv->port = DEFAULT_PORT;
					}
				} else {
					traceLog(err_msg, n,
						 "Non numeric value");
				}
				break;

			case NTHREADS:
				if (numstr(cp, &lval) == SPOCP_SUCCESS) {
					if (lval <= 0) {
						traceLog(err_msg, n,
							 "Value out of range");
						return 0;
					} else {
						int             level =
						    (int) lval;

						srv->threads = level;
					}
				} else {
					traceLog(err_msg, n,
						 "Non numeric specification");
					return 0;
				}
				break;

			case SSLVERIFYDEPTH:
				if (numstr(cp, &lval) == SPOCP_SUCCESS) {
					if (lval > 0L) {
						srv->sslverifydepth =
						    (unsigned int) lval;
					} else {
						traceLog(err_msg, n,
							 "number out of range");
						srv->sslverifydepth = 0;
					}
				} else {
					traceLog(err_msg, n,
						 "Non numeric value");
				}
				break;

			case PIDFILE:
				if (srv->pidfile)
					free(srv->pidfile);
				srv->pidfile = Strdup(cp);
				break;

			case MAXCONN:
				if (numstr(cp, &lval) == SPOCP_SUCCESS) {
					if (lval > 0L) {
						srv->nconn =
						    (unsigned int) lval;
					} else {
						traceLog(err_msg, n,
							 "Number out of range");
						srv->sslverifydepth = 0;
					}
				} else {
					traceLog(err_msg, n,
						 "Non numeric value");
				}
				break;

#ifdef HAVE_SSL
			case CLIENTCERT:
				if (strcasecmp(cp, "none") == 0)
					srv->clientcert = NONE;
				else if (strcasecmp(cp, "demand") == 0)
					srv->clientcert = DEMAND;
				else if (strcasecmp(cp, "hard") == 0)
					srv->clientcert = HARD;

				break;
#endif
			}
			break;

		case PLUGIN:
			if (pl == 0) {
				if (strcmp(s, "load") != 0) {
					traceLog(err_msg, n,
						 "First directive in plugin sector has to be \"load\"");
					section = 0;
				}

				if ((pl =
				     plugin_load(plugins, pluginname,
						 cp)) == 0)
					section = 0;
				else {
					/*
					 * The last one is placed last 
					 */
					for (; pl->next; pl = pl->next);
				}

				if (plugins == 0)
					plugins = pl;
			} else {
				if (strcmp(s, "poolsize") == 0) {
					if (numstr(cp, &lval) == SPOCP_SUCCESS) {
						if (lval <= 0) {
							traceLog(err_msg, n,
								 "Value out of range");
						} else {
							int             level =
							    (int) lval;

							if (pl->dyn == 0)
								pl->dyn =
								    pdyn_new
								    (level);
							if (pl->dyn->size == 0)
								pl->dyn->size =
								    level;
						}
					} else {
						traceLog(err_msg, n,
							 "Non numeric specification");
					}
				} else if (strcmp(s, "cachetime") == 0) {
					if (plugin_add_cachedef(pl, cp) < 0)
						traceLog(err_msg, n,
							 "Cachetime def");
				} else if (pl->ccmds == 0) {	/* No
								 * directives
								 * allowed */
					traceLog(err_msg, n,
						 "Directive where there should not be one");
				} else {
					for (ccp = pl->ccmds; ccp; ccp++) {
						if (strcmp(ccp->name, s) == 0) {
							r = ccp->func(&pl->
								      conf,
								      ccp->
								      cmd_data,
								      1, &cp);
							if (r != SPOCP_SUCCESS) {
								traceLog
								    (err_msg,
								     n,
								     ccp->
								     errmsg);
							}
							break;
						}
					}
					if (ccp == 0) {
						traceLog(err_msg, n,
							 "Unknown directive");
					}
				}
			}
			break;

		case DBACK:
			if (dbp == 0) {
				if (strcmp(s, "name") == 0) {
					dbname = Strdup(cp);
					if (dbname && dbload) {
						dbp =
						    dback_load(dbname, dbload);
						free(dbname);
						free(dbload);
					}
				} else if (strcmp(s, "load") == 0) {
					dbload = Strdup(cp);
					if (dbname && dbload) {
						dbp =
						    dback_load(dbname, dbload);
						free(dbname);
						free(dbload);
					}
				} else
					traceLog(err_msg, n,
						 "Unknown directive");
			} else {
				for (ccp = dbp->ccmds; ccp && *ccp->name;
				     ccp++) {
					if (strcmp(ccp->name, s) == 0) {
						r = ccp->func(&dbp->conf,
							      ccp->cmd_data, 1,
							      &cp);
						if (r != SPOCP_SUCCESS) {
							traceLog(err_msg, n,
								 ccp->errmsg);
						}
						break;
					}
				}
				if (ccp == 0) {
					traceLog(err_msg, n,
						 "Unknown directive");
				}
			}
			break;
		}
	}

	if (srv->pidfile == 0)
		srv->pidfile = Strdup("spocp.pid");
	if (srv->timeout == 0)
		srv->timeout = DEFAULT_TIMEOUT;
	if (srv->threads == 0)
		srv->threads = DEFAULT_NTHREADS;
	if (srv->sslverifydepth == 0)
		srv->sslverifydepth = DEFAULT_SSL_DEPTH;

	srv->plugin = plugins;
	srv->dback = dbp;

	return 1;
}

