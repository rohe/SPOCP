
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

/* ---------------------------------------------------------------------- */

spocp_result_t
motd_test(cmd_param_t * cpp, octet_t * blob)
{
	FILE           *fp;
	char            buf[256], *lp, *txt = 0;
	int             len, lt, tot;

	fp = fopen(motd, "r");
	if (fp == 0)
		return SPOCP_UNAVAILABLE;

	lp = fgets(buf, 256, fp);

	if (lp == 0) {
		fclose(fp);
		return SPOCP_UNAVAILABLE;
	}

	lp = &buf[strlen(buf) - 1];

	while (lp != buf && (*lp == '\r' || *lp == '\n'))
		*lp-- = '\0';

	lt = strlen(type);

	if (cpp->conf){
		char *cp = (char *) cpp->conf;

		len = strlen(buf);
		txt = (char *) malloc( lt + len + strlen(cp) + 9);

		sprintf( txt, "%s says \"%s\"", cp, buf);
	} 
	else
		txt = buf;

	len = strlen(txt);

	if (len == 0)
		return SPOCP_UNAVAILABLE;
	tot = len + DIGITS(len) + 2 + lt + DIGITS(lt);

	if (blob) {
		blob->len = blob->size = tot;
		blob->val = (char *) malloc(tot * sizeof(char));
		sprintf(blob->val, "%d:%s%d:%s", lt, type, len, txt);
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
	conffunc
};
