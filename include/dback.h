#ifndef __DBACK_H
#define __DBACK_H

#include <plugin.h>

struct __dback;
struct _dback_cmd_param;

/*! function prototype for the dback functions */
typedef void   *(dbackfn) (struct _dback_cmd_param *, void *, void *,
			   spocp_result_t *);

typedef struct dback_struct dback_t;

/*! Struct for the backend handling a persistent store to publish its
  functions */
struct dback_struct {
	/*! Major version of the Spocp library this pluggin was built agains */
	int             version;
	/*! Minor version of the Spocp library this pluggin was built agains */
	int             minor_version;
	/*! Name of the backend sourcefile */
	const char     *filename;
	/*! A magic number, this is here so the server can test whether the library
	 * to be loaded really is a spocp backend */
	unsigned long   magic;	

	/*! the dynamic library handle */ 
	void           *handle;	
	/*! where the plugin can keep its data */
	void           *conf;	
	/*! The name by which the backend should be known to the server*/
	char           *name;

	/*
	 * These are absolute necessities 
	 */
	/*! The function that gets a dataitem from a data store */
	dbackfn        *get;
	/*! The function that puts a dataitem from a data store */
	dbackfn        *put;
	/*! The function that replaces one dataitem in a data store with another */
	dbackfn        *replace;
	/*! The function that deletes a dataitem from a data store */
	dbackfn        *delete;
	/*! A function that retrieves keys which can be used to retrieve all 
	    dataitems from a store */
	dbackfn        *allkeys;

	/*
	 * These are optional 
	 */
	/*! start a transaction against a datastore */
	dbackfn        *begin;
	/*! ends a transaction against a datastore, without commiting any 
        changes  */
	dbackfn        *end;
	/*! commit a transaction against a datastore */
	dbackfn        *commit;
	/*! reset the datastore to the state that was when the transaction
	    was started */
	dbackfn        *rollback;

	/*! Initialize the data store */
	dbackfn        *init;

	/*! functions that should be used to handle configuration directives for
	    this backend */
	const conf_com_t *ccmds;
};

typedef struct _dback_cmd_param {
	dback_t        *dback;
	void           *handle;
} dbackdef_t;

/*
 * ---------------------------------------------------------------------- 
 */

octet_t         *datum_make(octet_t * rule, octet_t * blob, char *bcname);
spocp_result_t  datum_parse(octet_t * arg, octet_t * rule, octet_t * blob, 
                            char **bcname);

dback_t         *dback_load(char *name, char *load);

/*
 * dback_t *init_dback( plugin_t *pl ) ; 
 */
spocp_result_t  dback_init(dbackdef_t * dbc);

spocp_result_t  dback_save(dbackdef_t * d, char *, octet_t *, octet_t *, char *);
spocp_result_t  dback_replace(dbackdef_t * d, char *, octet_t *, octet_t *,
			      char *);
spocp_result_t  dback_read(dbackdef_t * d, char *, octet_t *, octet_t *, char **);
spocp_result_t  dback_delete(dbackdef_t * d, char *key);
octarr_t       *dback_all_keys(dbackdef_t * dbc, spocp_result_t * r);

/*
 * void *dback_open( dback_t *dback, spocp_result_t *r ) ; spocp_result_t
 * dback_close( dback_t *dback, void *vp ) ; octet_t *dback_first_key( dback_t 
 * *, void *h, spocp_result_t * ) ; octet_t *dback_next_key( dback_t *, void
 * *h, octet_t *, spocp_result_t * ) ; 
 */

spocp_result_t  dback_begin(dbackdef_t * d);
spocp_result_t  dback_end(dbackdef_t * d);
spocp_result_t  dback_commit(dbackdef_t *);
spocp_result_t  dback_rollback(dbackdef_t *);
#endif
