/*! 
 * \file plugin.h
 * \author Roland Hedberg <roland@catalogix.se>
 * \brief Struct definitions and function prototypes for backends 
 */
#ifndef __plugin_h
#define __plugin_h

#include <config.h>

#include <time.h>

#include <struct.h>
#include <be.h>

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#include <spocp.h>
#include <rdwr.h>

struct _pdyn;
struct _varr;


struct _cmd_param;


/*
 * ---------------------------------------------------------------------- 
 */

/****** caching **********************/

/*! \brief Struct for keeping cached values */ 
typedef struct {
	/*! A hash of the query that produced this cachevalue */
	unsigned int    hash;
	/*! A 'dynamic' blob to be returned */
	octet_t         blob;
	/*! When this cache becomes invalid */
	unsigned int    timeout;
	/*! The cached result */
	int             res;
} cacheval_t;

/*! \brief Information about how long things should be cached */
typedef struct cachetime_t {
	/*! The cache time */
	time_t          limit;
	/*! A pattern that can be applied to a query to see if this information should
	 *  be used when caching */
	octet_t         pattern;
	/*! The next piece of caching information */
	struct cachetime_t *next;
} cachetime_t;

/*
 * ------------------------------------ 
 */

/*! \brief A set of cached results */
typedef struct {
	/*! an array in which to store the cached results */
	struct _varr   *va;
#ifdef HAVE_LIBPTHREAD
	/*! A read/write lock on the cache */
	pthread_rdwr_t  rw_lock;
#endif
} cache_t;

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

/*
 * ---------------------------------------------------------------------- 
 */
/*
 * plugin info 
 */

/*! function to call for a raw-args */
typedef         spocp_result_t(conf_args) (void **conf, void *cmd_data,
					   int argc, char **argv);

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
/*! function prototype for the backend test function */
typedef         spocp_result_t(befunc) (cmd_param_t * cdp, octet_t * blob);
/*! function prototype for the backend initialization function */
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
	unsigned long   magic;
	/*! The runtime stuff */
	pdyn_t         *dyn;
	/*! the dynamic library handle */ 
	void           *handle;	
	/*! where the plugin can keep its data */
	void           *conf;	
	/*! The name by which the backend should be known to the server*/
	char           *name;
	/*! A pointer to the backend test function */
	befunc         *test;
	/*! A pointer to the backend initialization function */
	beinitfn       *init;
	/*! A pointer to a set of commands that should be used when specified
	 * directives appear in the configuration file */
	conf_com_t     *ccmds;
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
                                NULL

/*! default struct settings for a persistent store backend */
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

/*
 * ---------------------------------------------------------------------- 
 */

pdyn_t         *pdyn_new(int size);
void            pdyn_free(pdyn_t * pdp);

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
plugin_t       *plugin_load(plugin_t * top, char *name, char *load);
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
