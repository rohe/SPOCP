#include <config.h>

#include <sys/types.h>
#include <string.h>
#include <dlfcn.h>

#include <spocp.h>
#include <plugin.h>
#include <wrappers.h>
#include <func.h>

/*
 * ------------------------------------------------------------------------------------ 
 */

plugin_t       *plugin_get(plugin_t * top, char *name);

/*
 * ------------------------------------------------------------------------------------ 
 */

pdyn_t         *
pdyn_new(int size)
{
	pdyn_t         *pp;

	pp = (pdyn_t *) Calloc(1, sizeof(pdyn_t));
	pp->size = size;

	return pp;
}

void
pdyn_free(pdyn_t * pdp)
{
	if (pdp) {
		if (pdp->ct)
			cachetime_free(pdp->ct);
		free(pdp);
	}
}

/*
 * ------------------------------------------------------------------------------------ 
 * *
 * 
 * static pconf_t *pconf_new( char *key, octet_t *val ) { pconf_t *new ;
 * 
 * new = ( pconf_t * ) Malloc ( sizeof( pconf_t )) ; new->key = Strdup( key )
 * ; new->val = octarr_add( 0, octdup(val)) ; new->next = 0 ;
 * 
 * return new ; }
 * 
 * static void pconf_free( pconf_t *pc ) { if( pc ) { if( pc->key ) free(
 * pc->key ) ; if( pc->val ) octarr_free( pc->val ) ; if( pc->next )
 * pconf_free( pc->next ) ; free( pc ) ; } }
 * 
 * static plugin_t *pconf_set_keyval( plugin_t *top, char *pname, char *key,
 * char *val ) { plugin_t *pl ; pconf_t *pc = 0 ; octet_t oct ;
 * 
 * pl = plugin_get( top, pname ) ;
 * 
 * if( top == 0 ) top = pl ;
 * 
 * if( key ) { for( pc = pl->conf ; pc ; pc = pc->next ) { if( strcmp( key,
 * pc->key ) == 0 ) break ; } }
 * 
 * oct.val = val ; oct.size = 0 ; oct.len = strlen( val ) ;
 * 
 * if( pc == 0 ) { if( !pl->conf ) pl->conf = pconf_new( key, &oct ) ; else {
 * for( pc = pl->conf ; pc->next ; pc = pc->next ) ; pc->next = pconf_new( key, 
 * &oct ) ; } } else { octarr_add( pc->val, octdup( &oct ) ) ; }
 * 
 * return top ; }
 * 
 * octarr_t *pconf_get_keys_by_plugin( plugin_t *top, char *pname ) { pconf_t
 * *pc ; octarr_t *res = 0 ; plugin_t *pl ; octet_t oct ;
 * 
 * if( pname == 0 || *pname == '\0' ) return 0 ;
 * 
 * oct.val = pname ; oct.len = strlen( pname ) ; oct.size = 0 ;
 * 
 * if(( pl = plugin_match( top, &oct )) == 0 ) return 0 ;
 * 
 * for( pc = pl->conf ; pc ; pc = pc->next ) { oct.val = pc->key ; oct.len =
 * strlen( pc->key ) ; res = octarr_add( res, octdup( &oct ) ) ; }
 * 
 * return res ; }
 * 
 * octarr_t *pconf_get_keyval( plugin_t *pl, char *key ) { pconf_t *pc ;
 * 
 * for( pc = pl->conf ; pc ; pc = pc->next ) { if( strcmp( pc->key, key ) == 0
 * ) return pc->val ; }
 * 
 * return 0 ; }
 * 
 * octarr_t *pconf_get_keyval_by_plugin( plugin_t *top, char *pname, char *key
 * ) { plugin_t *pl ; octet_t oct ;
 * 
 * if( pname == 0 || *pname == '\0' ) return 0 ;
 * 
 * oct.val = pname ; oct.len = strlen( pname ) ; oct.size = 0 ;
 * 
 * if(( pl = plugin_match( top, &oct )) == 0 ) return 0 ;
 * 
 * return pconf_get_keyval( pl, key ) ; }
 * 
 * 
 * *
 * ------------------------------------------------------------------------------------ 
 */

/*
 * static plugin_t *plugin_new( char *name ) { plugin_t *new ;
 * 
 * new = ( plugin_t * ) Calloc ( 1, sizeof( plugin_t )) ;
 * 
 * #ifdef HAVE_LIBPTHREAD pthread_mutex_init( &new->mutex, NULL ) ; #endif
 * 
 * new->name = Strdup( name ) ;
 * 
 * return new ; } 
 */

plugin_t       *
plugin_match(plugin_t * top, octet_t * oct)
{
	plugin_t       *pl;

	if (top == 0)
		return 0;

	for (pl = top; pl; pl = pl->next)
		if (oct2strcmp(oct, pl->name) == 0)
			return pl;

	return 0;
}

plugin_t       *
plugin_get(plugin_t * top, char *name)
{
	plugin_t       *pl;

	if (top == 0)
		return 0;

	for (pl = top; pl; pl = pl->next)
		if (strcmp(name, pl->name) == 0)
			return pl;

	/*
	 * no matching definition found 
	 */

	return 0;
}

int
plugin_add_cachedef(plugin_t * pl, char *s)
{
	cachetime_t    *ct = 0;
	octet_t         oct;

	if (s == 0 || *s == '\0')
		return -1;

	oct_assign(&oct, s);

	ct = cachetime_new(&oct);

	if (ct == 0)
		return -1;

	if (pl->dyn == 0)
		pl->dyn = pdyn_new(0);

	ct->next = pl->dyn->ct;
	pl->dyn->ct = ct;

	return 1;
}

/*
 * plugin_t *plugin_add_conf( plugin_t *top, char *pname, char *key, char *val 
 * ) { if( key == 0 || *key == '\0' ) return 0 ;
 * 
 * return pconf_set_keyval( top, pname, key, val ) ; } 
 */

/*
 * ------------------------------------------------------------------------------------ 
 */

/*
 * static void plugin_free( plugin_t *pl ) { if( pl ) { if( pl->handle )
 * dlclose( pl->handle ) ; if( pl->name ) free( pl->name ) ; if( pl->dyn )
 * pdyn_free( pl->dyn ) ; * don't know how to free this if( pl->conf )
 * xyz_free( pl->conf ) ; * free( pl ) ; } }
 * 
 * static plugin_t *plugin_rm( plugin_t **top, plugin_t *pl ) { plugin_t
 * *prev, *next ;
 * 
 * next = pl->next ;
 * 
 * if( pl == *top ) { *top = next ; } else { for( prev = *top ; prev->next !=
 * pl ; prev = prev->next ) ; prev->next = next ; }
 * 
 * plugin_free( pl ) ;
 * 
 * return next ; } 
 */

/*
 * ------------------------------------------------------------------------------------ 
 */

plugin_t       *
plugin_load(plugin_t * top, char *name, char *load)
{
	plugin_t       *pl, *new;
	char           *modulename;
	void           *handle;

	for (pl = top; pl; pl = pl->next) {
		if (strcmp(pl->name, name) == 0) {
			traceLog("%s: Already loaded that plugin", name);
			return top;
		}
	}

	handle = dlopen(load, RTLD_LAZY);
	if (!handle) {
		traceLog("%s: Unable to open %s library: [%s]", name, load,
			 dlerror());
		return top;
	}

	modulename = (char *) Malloc(strlen(name) + strlen("_module") + 1);
	sprintf(modulename, "%s_module", name);

	new = (plugin_t *) dlsym(handle, modulename);

	free(modulename);

	if (new == 0 || new->magic != MODULE_MAGIC_COOKIE) {
		traceLog("%s: Not a proper plugin_struct", name);
		dlclose(handle);
		return top;
	}

	new->handle = handle;
	new->name = strdup(name);

	if (top == 0)
		return new;
	else {
		for (pl = top; pl->next; pl = pl->next);
		pl->next = new;
	}

	return top;
}

void
plugin_display(plugin_t * pl)
{
	for (; pl; pl = pl->next)
		traceLog("Active backend: %s", pl->name);
}
