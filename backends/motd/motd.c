
/***************************************************************************
                           motd.c  -  description
                             -------------------
    begin                : Thu Oct 30 2003
    copyright            : (C) 2003 by Stockholm University, Sweden
    email                : roland@catalogix.se

   COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
   of this package for details.

 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>
#include <macros.h>

befunc	motd_test;
conf_args set_name;

char	*motd = "/etc/motd";
char	*type = "Text/plain";

spocp_result_t set_name( void **lh, void *cd, int argc, char **argv);
spocp_result_t release_name( void **lh, void *cd, int argc, char **argv);

/* ====================================================================== */

spocp_result_t
set_name( void **lh, void *cd, int argc, char **argv)
{
	if (argc >= 1) {
		*lh = strdup(argv[0]);
		return SPOCP_SUCCESS;
	}
	else
		return SPOCP_MISSING_ARG;
}

spocp_result_t
release_name( void **lh, void *cd, int argc, char **argv)
{
	if (*lh)
		free( *lh );

	return SPOCP_SUCCESS;
}

/* ---------------------------------------------------------------------- */

spocp_result_t
motd_test(cmd_param_t * cpp, octet_t * blob)
{
	FILE           *fp;
	char            buf[256], *str, *txt = 0, *arr[3];
	int             tot;
	unsigned int	lt;

	fp = fopen(motd, "r");
	if (fp == 0)
		return SPOCP_UNAVAILABLE;

	;

	if (fgets(buf, 256, fp) == 0) {
		fclose(fp);
		return SPOCP_UNAVAILABLE;
	}

	rmcrlf(buf);

	lt = strlen(type);

	if (cpp->conf){
		char	*cp = (char *) cpp->conf;
		int	l;

		l = lt + strlen(buf) + strlen(cp) + 16;
		txt = (char *) malloc(l);

		sprintf( txt, "%s says \"%s\"", cp, buf);
	} 
	else
		txt = buf;

	if (txt == 0 || *txt == 0 )
		return SPOCP_UNAVAILABLE;


	if (blob) {
		lt += strlen(txt) + 24 ;
		tot = lt;
		str = (char *) calloc(lt, sizeof(char));
		arr[0] = type;
		arr[1] = txt;
		arr[2] = 0;
		sexp_printa( str, &lt, "aa", (void **) arr);
	
		octset(blob, str, tot-lt); 
	}

	if (txt != buf) free(txt);

	return SPOCP_SUCCESS;
}

conf_com_t conffunc[] = {
	{ "who", set_name, NULL, "The message guy" },
	{NULL}
};

plugin_t        motd_module = {
	SPOCP20_PLUGIN_STUFF,
	motd_test,
	NULL,
	conffunc,
	release_name
};
