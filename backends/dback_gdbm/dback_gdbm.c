#include <stdlib.h>
#include <gdbm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <dback.h>
#include "../../server/srvconf.h"
#include <log.h>

extern gdbm_error gdbm_errno;
extern char    *gdbm_version;

/* #define AVLUS 1 */

/*
 * function prototypes 
 */

conf_args       db_gdbm_set_file;

dbackfn         db_gdbm_get;
dbackfn         db_gdbm_put;
dbackfn         db_gdbm_replace;
dbackfn         db_gdbm_delete;
dbackfn         db_gdbm_allkeys;
dbackfn         db_gdbm_begin;

/*
 */

static octet_t *
octnew(void)
{
	return (octet_t *) calloc(1, sizeof(octet_t));
}

static octet_t *
datum2octet(octet_t * o, datum * d)
{
	if (d->dptr == 0) {
		if (o)
			o->len = 0;
		return o;
	}

	if (o == 0)
		o = octnew();

	o->val = d->dptr;

	if (d->dptr)
		o->len = o->size = d->dsize;
	else
		o->len = o->size = 0;

	return o;
}

static void
octet2datum(datum * d, octet_t * o)
{
	d->dptr = o->val;
	d->dsize = o->len;
}

static void
char2datum(datum * d, char *str)
{
	d->dptr = str;
	if (str)
		d->dsize = strlen(str);
	else
		d->dsize = 0;
}

/*
 * ---------------------------------------------------------------------- 
 */

spocp_result_t
db_gdbm_set_file(void **conf, void *cd, int argc, char **argv)
{
	*conf = (void *) strdup(argv[0]);

	if (*conf == 0)
		return SPOCP_NO_MEMORY;
	else
		return SPOCP_SUCCESS;
}

/*
 * ---------------------------------------------------------------------- 
 */

void           *
db_gdbm_put(dbackdef_t * dbc, void *vkey, void *vdat, spocp_result_t * rc)
{
	datum           dkey, dcontent;
	char           *file = (char *) dbc->dback->conf;
	GDBM_FILE       dbf;
	char           *key = (char *) vkey;
	octet_t        *dat = (octet_t *) vdat;
	int				c;

	dbf = gdbm_open(file, 0, GDBM_WRCREAT | GDBM_SYNC, S_IRUSR | S_IWUSR, 0);

	if (dbf == 0) {
		char *gret;

		traceLog(LOG_DEBUG,"DBACK: Failed to open persistent store; gdbm file %s", file);
		gret = gdbm_strerror( gdbm_errno );
		traceLog( LOG_DEBUG,"DBACK: Gdbm reported: %s", gret);

		*rc = SPOCP_OPERATIONSERROR;
		return 0;
	} else
		*rc = SPOCP_SUCCESS;

#ifdef AVLUS
	traceLog( LOG_DEBUG, "DBACK: Stored under key \"%s\"", key);
	oct_print(LOG_DEBUG, "DBACK: Datum", dat);
#endif
	char2datum(&dkey, key);
	octet2datum(&dcontent, dat);

	c = gdbm_store(dbf, dkey, dcontent, GDBM_INSERT) ;
	if (c == 1)
		*rc = SPOCP_EXISTS;
	else if( c== -1) {
		traceLog(LOG_DEBUG,"DBACK: Other gdbm error");
		*rc = SPOCP_OPERATIONSERROR;
	}

	gdbm_close(dbf);

	return 0;
}

/*
 * ---------------------------------------------------------------------- 
 */

void           *
db_gdbm_replace(dbackdef_t * dbc, void *vkey, void *vdat, spocp_result_t * rc)
{
	datum           dkey, dcontent;
	char           *file = (char *) dbc->dback->conf;
	GDBM_FILE       dbf;
	char           *key = (char *) vkey;
	octet_t        *dat = (octet_t *) vdat;

	dbf = gdbm_open(file, 0, GDBM_WRCREAT | GDBM_SYNC, S_IRUSR | S_IWUSR, 0);

	if (dbf == 0) {
		*rc = SPOCP_OPERATIONSERROR;
		return 0;
	} else
		*rc = SPOCP_SUCCESS;

	char2datum(&dkey, key);
	octet2datum(&dcontent, dat);

	if (gdbm_store(dbf, dkey, dcontent, GDBM_REPLACE) == 1)
		*rc = SPOCP_EXISTS;

	if (!dbc->handle)
		gdbm_close(dbf);

	return 0;
}

/*
 * ---------------------------------------------------------------------- 
 */

void           *
db_gdbm_get(dbackdef_t * dbc, void *vkey, void *null, spocp_result_t * rc)
{
	datum           dkey, dcontent;
	char           *file = (char *) dbc->dback->conf;
	GDBM_FILE       dbf;
	char           *key = (char *) vkey;
	octet_t        *content = 0;

	dbf = gdbm_open(file, 0, GDBM_READER, 0, 0);

	if (dbf == 0) {
		*rc = SPOCP_OPERATIONSERROR;
		return 0;
	} else
		*rc = SPOCP_SUCCESS;

	char2datum(&dkey, key);

	dcontent = gdbm_fetch(dbf, dkey);

	gdbm_close(dbf);

	content = datum2octet(content, &dcontent);

	return (void *) content;
}

/*
 * ---------------------------------------------------------------------- 
 */

void           *
db_gdbm_delete(dbackdef_t * dbc, void *v0, void *v1, spocp_result_t * rc)
{
	datum           dkey;
	int             res;
	char           *gdbmfile = (char *) dbc->dback->conf;
	GDBM_FILE       dbf;
	char           *key = (char *) v0;

	dbf = gdbm_open(gdbmfile, 0, GDBM_WRITER | GDBM_SYNC, 0, 0);

	if (dbf == 0) {
		traceLog( LOG_WARNING, "DBACK: Couldn't open GDBM file for deleting" );
		*rc = SPOCP_OPERATIONSERROR;
		return 0;
	} else
		*rc = SPOCP_SUCCESS;

	char2datum(&dkey, key);

	res = gdbm_delete(dbf, dkey);

	traceLog( LOG_DEBUG, "DBACK: Removing datum connected with key %s, res=%d", key, res );

	if (res == -1)
		*rc = SPOCP_UNAVAILABLE;

	gdbm_close(dbf);

	return 0;
}

/*
 * ---------------------------------------------------------------------- *
 * 
 * void *db_gdbm_open( dbackdef_t *dbc, void *v1, void *v2, spocp_result_t *rc )
 * { char *gdbmfile = (char *) dbc->conf ; GDBM_FILE dbf ;
 * 
 * *rc = SPOCP_SUCCESS ;
 * 
 * if( dbc->handle == 0 ) {
 * 
 * dbf = gdbm_open( gdbmfile, 0, GDBM_READER, 0, 0 ) ;
 * 
 * dbc->handle = ( void * ) dbf ;
 * 
 * if( dbf == 0 ) *rc = SPOCP_OPERATIONSERROR ; }
 * 
 * return dbc->oonhande ; }
 * 
 * * ---------------------------------------------------------------------- *
 * 
 * void *db_gdbm_close( dbackdef_t *dbc, void *v1, void *v2, spocp_result_t *rc )
 * { GDBM_FILE dbf ;
 * 
 * *rc = SPOCP_SUCCESS ;
 * 
 * if( dbc->handle ) { dbf = ( GDBM_FILE ) dbc->handle ;
 * 
 * gdbm_close( dbf ) ; }
 * 
 * return 0 ; }
 * 
 * * ---------------------------------------------------------------------- *
 * 
 * void *db_gdbm_firstkey( dbackdef_t *dbc, void *v1, void *v2, spocp_result_t *rc 
 * ) { GDBM_FILE dbf = ( GDBM_FILE ) dbc->handle ; datum key ; octet_t *res = 0 
 * ;
 * 
 * if( dbf == 0 ) { *rc = SPOCP_OPERATIONSERROR ; } else { key =
 * gdbm_firstkey( dbf ) ; res = datum2octet( res, &key ) ; *rc = SPOCP_SUCCESS
 * ; }
 * 
 * return ( void * ) res ; }
 * 
 * * ---------------------------------------------------------------------- *
 * 
 * void *db_gdbm_nextkey( dbackdef_t *dbc, void *v1, void *v2, spocp_result_t *rc
 * ) { GDBM_FILE dbf = ( GDBM_FILE ) dbc->handle ; datum key, nextkey ; octet_t 
 * *res = 0 ;
 * 
 * if( def == 0 ) { *rc = SPOCP_OPERATIONSERROR ; } else { octet2datum( &key,
 * (octet_t *) v1 ) ; nextkey = gdbm_nextkey( dbf, key ) ; res = datum2octet(
 * res, &nextkey ) ; *rc = SPOCP_SUCCESS ; }
 * 
 * return ( void *) res ; }
 * 
 * * ---------------------------------------------------------------------- 
 */

void           *
db_gdbm_allkeys(dbackdef_t * dbc, void *v1, void *v2, spocp_result_t * rc)
{
	GDBM_FILE       dbf;
	datum           key, nextkey;
	octet_t        *oct;
	octarr_t       *oa = 0;
	char           *file = (char *) dbc->dback->conf;

	if (!dbc )
		return 0;

	dbf = gdbm_open(file, 0, GDBM_READER, 0, 0);

	if (dbf == 0) {
		char *ret;

		ret = gdbm_strerror( gdbm_errno );
		/* will return dbf==0 and gdbm_errno==3 if file doesn't exist */
		traceLog(LOG_WARNING,"DBACK: GDBM dback return %s when trying to open %s",ret, file);

		*rc = SPOCP_OPERATIONSERROR;
		return 0;
	} 

	key = gdbm_firstkey(dbf);
	while (key.dptr) {
		oct = datum2octet(0, &key);
		oa = octarr_add(oa, oct);
		nextkey = gdbm_nextkey(dbf, key);
		key = nextkey;
	}

	gdbm_close( dbf );

	return (void *) oa;
}

/*
 * ---------------------------------------------------------------------- 
 */

void           *
db_gdbm_begin(dbackdef_t * dbc, void *v1, void *v2, spocp_result_t * rc)
{
	*rc = SPOCP_NOT_SUPPORTED;

	return 0;
}

/*
 * ---------------------------------------------------------------------- 
 */

static conf_com_t gdbm_confcmd[] = {
	{"gdbmfile", db_gdbm_set_file, NULL, "The GDBM data store"},
	{NULL}
};

/*
 * ---------------------------------------------------------------------- 
 */

struct dback_struct gdbm_dback = {
	SPOCP20_DBACK_STUFF,
	db_gdbm_get,
	db_gdbm_put,
	db_gdbm_replace,
	db_gdbm_delete,
	db_gdbm_allkeys,

	db_gdbm_begin,
	NULL,
	NULL,
	NULL,

	NULL,			/* No init function */

	gdbm_confcmd
};
