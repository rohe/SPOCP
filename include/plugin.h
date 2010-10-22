/*! 
 * \file plugin.h
 * \author Roland Hedberg <roland@catalogix.se>
 * \brief Struct definitions and function prototypes for backends 
 */
#ifndef __PLUGIN_H
#define __PLUGIN_H

/*
#include <config.h>
*/

#include <element.h>
#include <varr.h>
#include <cache.h>

#if defined HAVE_LIBPTHREAD || defined HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include <spocp.h>
#include <rdwr.h>
#include <be.h>

/*
 * ------------------------------------ 
 */

/*! \brief Storage for dynamic stuff for backends */
typedef struct {
	/*! The result cache */
	cache_t        *cv;
	/*! Information about caching */
	cachetime_t    *ct;
	/*! The connection pool */
	becpool_t      *bcp;
	/*! The max size of the connection pool */
	int             size;
} pdyn_t;

/*! \brief Storage for statistics */
typedef struct {
	int operations;
	struct timeval op_start;
	struct timeval op_end;
} stat_t ;
/*
 * ---------------------------------------------------------------------- 
 */
/*
 * plugin info 
 */

/*! \brief Prototype for functions that handle configuration directives */
typedef	spocp_result_t(conf_args) (void **conf, void *cmd_data,
					   int argc, char **argv);

typedef	spocp_result_t(conf_free) (void **conf);

/*! \brief The struct that plugins MUST use to document their capabilities */
typedef struct {

    /*! Name of this command */
	const char     *name;

    /*! The function to be called when this directive is to be parsed */
	conf_args      *func;

    /*! Extra data, for functions which implement multiple commands... */
	void           *cmd_data;

    /*! 'usage' message, in case of syntax errors */
	const char     *errmsg;
} conf_com_t;

/*! \brief The command parameters to the backend test function */
typedef struct {
	/*! The query S-expression */
	element_t      *q;	
	/*! The rule S-expression */
	element_t      *r;	
	/*! the result of the 'XPath' parsing of the query S-expression */
	element_t      *x;
	/*! The plugin specific part of a boundary condition */ 
	octet_t        *arg;	
	/*! runtime stuff */
	pdyn_t         *pd;
	/*! where the plugin can keep its data */
	void           *conf;	
} cmd_param_t;

/*
 * backend functions 
 */
/*! \brief prototype for the backend test function */
typedef         spocp_result_t(befunc) (cmd_param_t * cdp, octet_t * blob);
/*! \brief prototype for the backend initialization function */
typedef         spocp_result_t(beinitfn) (void **conf, pdyn_t ** d);

/*! \brief Information about a dynamically plugged in backend */
typedef struct plugin_t {
	/*! Major version of the Spocp library this pluggin was built agains */
	int             version;
	/*! Minor version of the Spocp library this pluggin was built agains */
	int             minor_version;
	/*! Name of the backend sourcefile */
	const char     *filename;
	/*! Next backend */
	struct plugin_t *next;
	/*! A magic number, this is here so the server can test whether the library
	 * to be loaded really is a spocp backend */
	unsigned long	magic;
	/*! The runtime stuff */
	pdyn_t          *dyn;
	/*! Runtime statistics */
	stat_t          *stat;
	/*! the dynamic library handle */ 
	void            *handle;	
	/*! where the plugin can keep its data */
	void            *conf;	
	/*! The name by which the backend should be known to the server*/
	char            *name;
	/*! A pointer to the backend test function */
	befunc          *test;
	/*! A pointer to the backend initialization function */
	beinitfn        *init;
	/*! A pointer to a set of commands that should be used when specified
	 * directives appear in the configuration file */
	conf_com_t      *ccmds;
	/*! For the backend to release all it's private configuration */
	conf_free       *free;
} plugin_t;

/*! The Spocp library major version number */
#define MODULE_MAGIC_NUMBER_MAJOR 20040321
/*! The Spocp library minor version number */
#define MODULE_MAGIC_NUMBER_MINOR 0	/* 0...n */
/*! The magic number, has to fit in a unsigned long */
#define MODULE_MAGIC_COOKIE 0x53503230UL	/* "SPO20" */

/*! default struct settings for a backend */
#define SPOCP20_PLUGIN_STUFF	MODULE_MAGIC_NUMBER_MAJOR, \
				MODULE_MAGIC_NUMBER_MINOR, \
				__FILE__, \
				NULL, \
				MODULE_MAGIC_COOKIE, \
                                NULL, \
                                NULL, \
                                NULL, \
                                NULL, \
                                NULL

/*! default struct settings for a persistent backend store */
#define SPOCP20_DBACK_STUFF	MODULE_MAGIC_NUMBER_MAJOR, \
				MODULE_MAGIC_NUMBER_MINOR, \
				__FILE__, \
				MODULE_MAGIC_COOKIE, \
                                NULL, \
                                NULL, \
                                NULL

/*
 * ---------------------------------------------------------------------- 
 */

time_t          cachetime_set(octet_t * str, cachetime_t * ct);
cachetime_t    *cachetime_new(octet_t * s);
void            cachetime_free(cachetime_t * s);
void            cache_free( cache_t *cp );

/*
 * ---------------------------------------------------------------------- 
 */

pdyn_t         *pdyn_new(int size);
void            pdyn_free(pdyn_t * pdp);

void            plugin_unload( plugin_t * );
void            plugin_unload_all( plugin_t * );

/*
 * ---------------------------------------------------------------------- 
 */

/*
octarr_t       *pconf_get_keyval(plugin_t * pl, char *key);
octarr_t       *pconf_get_keyval_by_plugin(plugin_t * top, char *pname,
					   char *key);
octarr_t       *pconf_get_keys_by_plugin(plugin_t * top, char *pname);
plugin_t       *plugin_add_conf(plugin_t * top, char *pname, char *key,
				char *val);
plugin_t       *init_plugin(plugin_t * top);
*/

plugin_t       *plugin_match(plugin_t * top, octet_t * oct);
/*! Defined in lib/plugin.c */
plugin_t       *plugin_get(plugin_t * top, char *name);
plugin_t       *plugin_load(plugin_t * top, char *name, char *load, 
                            plugin_t * stat);
void            plugin_display(plugin_t * pl);
int             plugin_add_cachedef(plugin_t * top, char *s);

/*
 * --- cache.c ---- 
 */

int             cached(cache_t * v, octet_t * arg, octet_t * blob);
void            cached_rm(cache_t * v, octet_t * arg);
cache_t        *cache_value(cache_t *, octet_t *, unsigned int, int,
			    octet_t *);

/*
 * void cacheval_free( cacheval_t * ) ; 
 */

/*
 * ---------------------------------------------------------------------- 
 */

#endif
