/*!
 * \file plugin.c
 * \author Roland Hedberg <roland@catalogix.se>
 * \brief Functions that handles backends
 */
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

/*!
 * \brief Create a new pdyn_t struct
 * \param size The maximum size of the connection pool
 * \return The pdyn_t struct
 */
pdyn_t         *
pdyn_new(int size)
{
	pdyn_t         *pp;

	pp = (pdyn_t *) Calloc(1, sizeof(pdyn_t));
	pp->size = size;

	return pp;
}

/*!
 * \brief Frees a pdyn_t struct
 * \param pdp pointer to the struct to be freed.
 */
void
pdyn_free(pdyn_t * pdp)
{
	if (pdp) {
		if (pdp->ct)
			cachetime_free(pdp->ct);
		if (pdp->bcp) 
			becpool_rm( pdp->bcp, 1 );
		if (pdp->cv)
			cache_free( pdp->cv);
		
		free(pdp);
	}
}

/*
 * ------------------------------------------------------------------------------------ 
 */

/*!
 * \brief Find a specific backend
 * \param top The head of a list of plugin structs
 * \param oct The name of the plugin that should be found, represented as a
 *   octet struct.
 * \return A pointer to the corresponding plugin struct or NULL if there
 *   was no loaded backend with that name.
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

/*!
 * \brief Find a specific backend
 * \param top The head of a list of plugin structs
 * \param name The name of the plugin that should be found, represented as a
 *   NULL terminated string.
 * \return A pointer to the corresponding plugin struct or NULL if there
 *   was no loaded backend with that name.
 */
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

/*!
 * \brief Add information about how results should be cached
 * \param pl A pointer to the plugin struct that hands the information
 *   for the backend in question
 * \param s A string representing the cache definition
 * \return TRUE/FALSE
 */
int
plugin_add_cachedef(plugin_t * pl, char *s)
{
	cachetime_t    *ct = 0;
	octet_t         oct;

	if (s == 0 || *s == '\0')
		return FALSE;

	oct_assign(&oct, s);

	ct = cachetime_new(&oct);

	if (ct == 0)
		return FALSE;

	if (pl->dyn == 0)
		pl->dyn = pdyn_new(0);

	ct->next = pl->dyn->ct;
	pl->dyn->ct = ct;

	return TRUE;
}

/*
 * ------------------------------------------------------------------------------------ 
 */
/*!
 * \brief Load a backend
 * \param top The top of the list of plugin structs representing the backends
 *   loaded so far.
 * \param name The name of the backend
 * \param load The filename of the backend library
 * \return The new top of the plugin list
 */
plugin_t       *
plugin_load(plugin_t * top, char *name, char *load)
{
	plugin_t       *pl, *new;
	char           *modulename;
	void           *handle;

	traceLog(LOG_INFO, "Loading plugin: \"%s\"", name ) ;

	for (pl = top; pl; pl = pl->next) {
		if (strcmp(pl->name, name) == 0) {
			traceLog(LOG_ERR,"%s: Already loaded that plugin", name);
			return top;
		}
	}

	handle = dlopen(load, RTLD_LAZY);
	if (!handle) {
		traceLog(LOG_ERR,"%s: Unable to open %s library: [%s]", name, load,
			 dlerror());
		return top;
	}

	modulename = (char *) Malloc(strlen(name) + strlen("_module") + 1);
	sprintf(modulename, "%s_module", name);

	new = (plugin_t *) dlsym(handle, modulename);

	free(modulename);

	if (new == 0 || new->magic != MODULE_MAGIC_COOKIE) {
		traceLog(LOG_ERR,"%s: Not a proper plugin_struct", name);
		dlclose(handle);
		return top;
	}

	new->handle = handle;
	new->name = strdup(name);
	new->next = NULL;

	if (top == 0)
		return new;
	else {
		for (pl = top; pl->next; pl = pl->next);
		pl->next = new;
	}

	return top;
}

void
plugin_unload( plugin_t *pl )
{
	if (pl) {
		if( pl->dyn )
			pdyn_free( pl->dyn );
		if (pl->conf);
			/* how do I remove this */
		if (pl->name) 
			free( pl->name );
		if (pl->handle)
			dlclose( pl->handle );

	}
}

void
plugin_unload_all( plugin_t *top )
{
	plugin_t *pl, *next ;

	for (pl = top; pl; pl = next) {
		next = pl->next;
		plugin_unload(pl);
	}
}

/*!
 * \brief List the name of all backends that has been successfully 
 *  loaded.
 * \param pl The top of the list of plugin structs
 */
void
plugin_display(plugin_t * pl)
{
	for (; pl; pl = pl->next)
		traceLog(LOG_INFO,"Active backend: %s", pl->name);
}
