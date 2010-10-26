
/***************************************************************************
                  config.c  - function to read the config file 
                             -------------------
    begin                : Sat Oct 12 2010
    copyright            : (C) 2010 by UmeŒ University, Sweden
    email                : roland.hedberg@adm.umu.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include "locl.h"

#define RULEFILE        1
#define PORT            2
#define UNIXDOMAINSOCKET    3
#define CERTIFICATE     4
#define PRIVATEKEY      5
#define CALIST          6
#define DHFILE          7
#define ENTROPYFILE     8
#define PASSWD          9
#define NTHREADS        10
#define TIMEOUT         11
#define LOGGING         12
#define SSLVERIFYDEPTH  13
#define PIDFILE         14
#define MAXCONN         15
#define CLIENTCERT      16
#define NAME            17

char    *keyword[] = {
    "__querty__", "rulefile", "port", "unixdomainsocket", "certificate",
    "privatekey", "calist", "dhfile", "entropyfile",
    "passwd", "threads", "timeout", "log", "sslverifydepth",
    "pidfile", "maxconn", "clientcert", "name", NULL
};

#ifdef HAVE_SASL
struct overflow_conf {
    const char              *key;
    const char              *value;
    struct overflow_conf    *next;
};

struct overflow_conf *overflow_first = NULL;
struct overflow_conf *overflow_last = NULL;
#endif

#define DEFAULT_PORT        4751    /* Registered with IANA */
#define DEFAULT_TIMEOUT     30
#define DEFAULT_NTHREADS    5
#define DEFAULT_SSL_DEPTH   1

#define LINEBUF             1024

#define SYSTEM 20
#define PLUGIN 21
#define DBACK  22

#define SKIP_WHITESPACE(x)  while(WHITESPACE(*x)) x++

#ifndef HAVE_STRNDUP
char *strndup(const char *s, size_t n);
#endif

/*
 * roughly 
 */
#define YEAR  31536000

extern char *pidfile;

static char *err_msg = "Error in configuration file, line %d: %s";

/* ------------------------------------------------------------------ */
/* Chops up a string into serveral pieces, the divider between pieces
 * are white spsce characters. If white space characters are positioned
 * somewhere between two '"' they are protected and there will not be
 * any splitting at that position. 
 */

static char **strchop( char *s, int *argc)
{
    char *fp, *cp;
    char **arr = 0;
    int  i,n;

    for( cp = s, i = 0, n = 0; *cp ; ){
        if (*cp == '"' && n == 0) {
            cp++;
            while( *cp && *cp != '"') 
                cp++, n++;
            if ( *cp == '\0' )
                return NULL;    
            cp++;
            if ( *cp && !(WHITESPACE(*cp)))
                return NULL;    
            n = 0;
            SKIP_WHITESPACE(cp); 
        }
        else if (WHITESPACE(*cp)) {
            i++;
            n = 0;
            SKIP_WHITESPACE(cp); 
        }
        else 
            cp++, n++;
    }

    arr = (char **) Calloc( i+1, sizeof( char *));

    for( fp = cp = s, i = 0, n = 0; *cp ; ){
        if (*cp == '"' && n == 0) {
            cp++;
            fp=cp;
            while( *cp && *cp != '"') 
                cp++, n++;
            if ( *cp == 0 ) {
                charmatrix_free(arr);
                return NULL;
            }

            cp++;

            if ( *cp && !(WHITESPACE(*cp))) {
                charmatrix_free(arr);
                return NULL;    
            }

            arr[i++] = strndup( fp, n );
            n = 0;
            SKIP_WHITESPACE(cp);
            fp = cp;
        }
        else if (WHITESPACE(*cp)) {
            arr[i++] = strndup( fp, n );
            n = 0;
            SKIP_WHITESPACE(cp);
            fp = cp;
        }
        else cp++, n++;
    }

    if (n) {
        arr[i++] = strndup( fp, n );
    }

    *argc = i;

    return arr;
}

#ifdef HAVE_SASL
static void
add_overflow_directive(const char *key, const char *value)
{
    struct overflow_conf *this = overflow_last;

    if(this != NULL)
        this = this->next;
    this = Calloc(sizeof(struct overflow_conf), 1);
    this->key = strdup(key);
    this->value = strdup(value);
    if(overflow_last != NULL)
        overflow_last->next = this;
    overflow_last = this;
    if(!overflow_first)
        overflow_first = this;
}

static const char
*get_overflown(const char *key)
{
    struct overflow_conf *this = overflow_first;

    for(; this != NULL; this = this->next)
        if((strcmp(this->key, key) == 0))
            break;
    if(this)
        return(this->value);
    return(NULL);
}
#endif

/*------------------------------------------------------------------ */
/*
 * In some cases will gladly overwrite what has been previously specified, if
 * the same keyword appears twice 
 */

int
read_config(char *file, srv_t *srvp)
{
    FILE                *fp;
    char                s[LINEBUF], *cp, *sp, pluginname[256];
    char                section = 0, *dbname = 0, *dbload = 0;
    unsigned int        n = 0;
    long                lval;
    int                 i;
    plugin_t            *plugins, *pl = 0;
    dback_t             *dbp = 0;
    const conf_com_t    *ccp;
    spocp_result_t      r;

    /*
     * should never be necessary 

    if (!srvp->root)
        srvp->root = ruleset_new(0);
     */

    if (!srvp->root->db)
        srvp->root->db = db_new();

    plugins = srvp->plugin;

    if ((fp = fopen(file, "r")) == NULL) {
        traceLog(LOG_ERR,
            "Could not find or open the configuration file \"%s\"", file);
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
                traceLog(LOG_ERR, err_msg, n, "Section specification");
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
                strlcpy(pluginname, sp, sizeof( pluginname));
                pl = 0;
            }

            continue;
        }

        /*
         * Within a section The directives are of the form: 
         * key *SP * "=" *SP val *(*SP val)
         * val = 1*nonspacechar / '"' char '"' 
         */

        rmlt(s, 1); /* remove leading and trailing blanks */

        /*
         * empty line or comment 
         */
        if (*s == 0 || *s == '#')
            continue;

        cp = strchr(s, '=');
        if (cp == 0) {
            traceLog(LOG_ERR, err_msg, n, "syntax error");
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
#ifdef HAVE_SASL
                if((strncmp("sasl_", s, 5) == 0))
                    add_overflow_directive(s, cp);
                else
#endif
                traceLog(LOG_ERR, err_msg, n, "Unknown keyword");
                continue;
            }
            /*
            else {
                traceLog(LOG_ERR, "valid configuration keyword: %s (%d)",s,i);
            }
            */
                
            switch (i) {
            case RULEFILE:
                if (srvp->rulefile)
                    free(srvp->rulefile);
                srvp->rulefile = Strdup(cp);
                break;

            case CERTIFICATE:
                if (srvp->certificateFile)
                    free(srvp->certificateFile);
                srvp->certificateFile = Strdup(cp);
                break;

            case PRIVATEKEY:
                if (srvp->privateKey)
                    free(srvp->privateKey);
                srvp->privateKey = Strdup(cp);
                break;

            case CALIST:
                if (srvp->caList)
                    free(srvp->caList);
                srvp->caList = Strdup(cp);
                break;

            case DHFILE:
                if (srvp->dhFile)
                    free(srvp->dhFile);
                srvp->dhFile = Strdup(cp);
                break;

            case ENTROPYFILE:
                if (srvp->SslEntropyFile)
                    free(srvp->SslEntropyFile);
                srvp->SslEntropyFile = Strdup(cp);
                break;

            case PASSWD:
                if (srvp->passwd)
                    free(srvp->passwd);
                srvp->passwd = Strdup(cp);
                break;

            case LOGGING:
                if (srvp->logfile)
                    free(srvp->logfile);
                srvp->logfile = Strdup(cp);
                break;

            case TIMEOUT:
                if (numstr(cp, &lval) == SPOCP_SUCCESS) {
                    if (lval >= 0 && lval <= YEAR)
                        srvp->timeout =
                            (unsigned int) lval;
                    else {
                        traceLog(LOG_ERR, err_msg, n,
                             "Value out of range");
                        srvp->timeout = DEFAULT_TIMEOUT;
                    }
                } else {
                    traceLog(LOG_ERR, err_msg, n,
                         "Non numeric value");
                    srvp->timeout = DEFAULT_TIMEOUT;
                }

                break;

            case UNIXDOMAINSOCKET:
                if (srvp->uds)
                    free(srvp->uds);
                srvp->uds = Strdup(cp);
                break;

            case PORT:
                if (numstr(cp, &lval) == SPOCP_SUCCESS) {
                    if (lval > 0L && lval < 65536) {
                        srvp->port =
                            (unsigned int) lval;
                    } else {
                        traceLog(LOG_ERR, err_msg, n,
                             "Number out of range");
                        srvp->port = DEFAULT_PORT;
                    }
                } else {
                    traceLog(LOG_ERR, err_msg, n,
                         "Non numeric value");
                }
                break;

            case NTHREADS:
                if (numstr(cp, &lval) == SPOCP_SUCCESS) {
                    if (lval <= 0) {
                        traceLog(LOG_ERR, err_msg, n,
                             "Value out of range");
                        return 0;
                    } else {
                        int             level =
                            (int) lval;

                        srvp->threads = level;
                    }
                } else {
                    traceLog(LOG_ERR, err_msg, n,
                         "Non numeric specification");
                    return 0;
                }
                break;

            case SSLVERIFYDEPTH:
                if (numstr(cp, &lval) == SPOCP_SUCCESS) {
                    if (lval > 0L) {
                        srvp->sslverifydepth =
                            (unsigned int) lval;
                    } else {
                        traceLog(LOG_ERR, err_msg, n,
                             "number out of range");
                        srvp->sslverifydepth = 0;
                    }
                } else {
                    traceLog(LOG_ERR, err_msg, n,
                         "Non numeric value");
                }
                break;

            case PIDFILE:
                if (srvp->pidfile)
                    Free(srvp->pidfile);
                srvp->pidfile = Strdup(cp);
                break;

            case MAXCONN:
                if (numstr(cp, &lval) == SPOCP_SUCCESS) {
                    if (lval > 0L) {
                        srvp->nconn =
                            (unsigned int) lval;
                    } else {
                        traceLog(LOG_ERR, err_msg, n,
                             "Number out of range");
                        srvp->sslverifydepth = 0;
                    }
                } else {
                    traceLog(LOG_ERR, err_msg, n,
                         "Non numeric value");
                }
                break;

#ifdef HAVE_SSL
            case CLIENTCERT:
                if (strcasecmp(cp, "none") == 0)
                    srvp->clientcert = NONE;
                else if (strcasecmp(cp, "demand") == 0)
                    srvp->clientcert = DEMAND;
                else if (strcasecmp(cp, "hard") == 0)
                    srvp->clientcert = HARD;

                break;
#endif
            case NAME:
                if (srvp->name)
                    Free(srvp->name);
                srvp->name = Strdup(cp);
                break;
            }
            break;

        case PLUGIN:
            if (pl == 0) {
                if (strcmp(s, "load") != 0) {
                    traceLog(LOG_ERR, err_msg, n,
                         "First directive in plugin sector has to be \"load\"");
                    section = 0;
                }

                if ((pl = plugin_load(plugins, pluginname, cp, 0)) == 0){
                    traceLog(LOG_ERR, err_msg, n, "Unable to load plugin");
                    section = 0;
                }
                else {
                    /*
                     * The last one is placed last 
                     */
                    traceLog(LOG_INFO, "Loaded plugin: '%s'", pluginname);
                    for (; pl->next; pl = pl->next);
                }

                if (plugins == 0)
                    plugins = pl;
            } else {
                if (strcmp(s, "poolsize") == 0) {
                    if (numstr(cp, &lval) == SPOCP_SUCCESS) {
                        if (lval <= 0) {
                            traceLog(LOG_ERR, err_msg, n, 
                                    "Value out of range");
                        } else {
                            int level = (int) lval;

                            if (pl->dyn == 0)
                                pl->dyn = pdyn_new(level);
                            if (pl->dyn->size == 0)
                                pl->dyn->size = level;
                        }
                    } else {
                        traceLog(LOG_ERR, err_msg, n,
                             "Non numeric specification");
                    }
                } else if (strcmp(s, "cachetime") == 0) {
                    if (plugin_add_cachedef(pl, cp) == FALSE )
                        traceLog(LOG_ERR, err_msg, n,
                             "Cachetime def");
                } else if (pl->ccmds == 0) {    /* No directives allowed */
                    traceLog(LOG_ERR, err_msg, n,
                         "Directive where there should not be one");
                } else {
                    for (ccp = pl->ccmds; ccp; ccp++) {
                        int     np=0, j;
                        char    **arr;

                        arr = strchop(cp,&np);

                        for (j=0; j<np; j++)
                            traceLog(LOG_ERR, "%s:%s", cp, arr[j]);

                        if (strcmp(ccp->name, s) == 0) {
                            r = ccp->func(&pl->conf, ccp->cmd_data, np, arr);
                            if (r != SPOCP_SUCCESS) {
                                traceLog(LOG_ERR, err_msg, n, ccp->errmsg);
                            }
                            charmatrix_free( arr );
                            break;
                        }
                    }
                    if (ccp == 0) {
                        traceLog(LOG_ERR,err_msg, n, "Unknown directive");
                    }
                }
            }
            break;

        case DBACK:
            if (dbp == 0) {
                if (strcmp(s, "name") == 0) {
                    dbname = Strdup(cp);
                    if (dbname && dbload) {
                        dbp = dback_load(dbname, dbload);
                        free(dbname);
                        free(dbload);
                    }
                } else if (strcmp(s, "load") == 0) {
                    dbload = Strdup(cp);
                    if (dbname && dbload) {
                        dbp = dback_load(dbname, dbload);
                        free(dbname);
                        free(dbload);
                    }
                } else
                    traceLog(LOG_ERR,err_msg, n,
                         "Unknown directive");
            } else {
                for (ccp = dbp->ccmds; ccp && *ccp->name;
                     ccp++) {
                    if (strcmp(ccp->name, s) == 0) {
                        r = ccp->func(&dbp->conf,
                                  ccp->cmd_data, 1,
                                  &cp);
                        if (r != SPOCP_SUCCESS) {
                            traceLog(LOG_ERR,err_msg, n,
                                 ccp->errmsg);
                        }
                        break;
                    }
                }
                if (ccp == 0) {
                    traceLog(LOG_ERR,err_msg, n,
                         "Unknown directive");
                }
            }
            break;
        }
    }

    fclose(fp);

    if (srvp->pidfile == 0)
        srvp->pidfile = Strdup("spocd.pid");
    if (srvp->timeout == 0)
        srvp->timeout = DEFAULT_TIMEOUT;
    if (srvp->threads == 0)
        srvp->threads = DEFAULT_NTHREADS;
    if (srvp->sslverifydepth == 0)
        srvp->sslverifydepth = DEFAULT_SSL_DEPTH;

    srvp->plugin = plugins;
    srvp->dback = dbp;

    return 1;
}

#ifdef HAVE_SASL
int
parse_sasl_conf(void *context, const char *plugin_name,
        const char *option, const char **result, unsigned *len)
{
    int opt_len = (5 + strlen(option));
    char *opt;

    if(plugin_name)
        opt_len += (strlen(plugin_name) + 1);

    opt = Calloc(1, opt_len);

    if(plugin_name)
    {
        snprintf(opt, opt_len, "sasl_%s_%s", plugin_name, option);
        *result = get_overflown(opt);
    }

    if(*result == NULL)
    {
        snprintf(opt, opt_len, "sasl_%s", option);
        *result = get_overflown(opt);
    }
    Free(opt);

    if(*result != NULL)
    {
        if(len)
            *len = strlen(*result);
        return(SASL_OK);
    }
    return(SASL_FAIL);
}
#endif
