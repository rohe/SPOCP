#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <netdb.h>

#include <spocp.h>
#include <be.h>
#include <plugin.h>
#include <rvapi.h>

befunc          rbl_test;

/*
 * Typical rbl entry in DNS 1.5.5.192.blackholes.mail-abuse.org. IN A
 * 127.0.0.2 
 */
/*
 * argument to the call:
 * 
 * rbldomain:hostaddr
 * 
 * can for instance be blackholes.mail-abuse.org:192.5.5.1
 * 
 */

spocp_result_t
rbl_test(cmd_param_t * cpp, octet_t * blob)
{
	char           *rblhost;
	char           *hostaddr, *s, *r, *c, *d;
	int             n, l;
	octarr_t       *argv;
	octet_t        *oct, *domain, *host;

	if (cpp->arg == 0)
		return SPOCP_MISSING_ARG;

	if ((oct = element_atom_sub(cpp->arg, cpp->x)) == 0)
		return SPOCP_SYNTAXERROR;

	argv = oct_split(oct, ';', 0, 0, 0);

	if (oct != cpp->arg)
		oct_free(oct);

	if (argv->n != 2) {
		octarr_free(argv);
		return SPOCP_SYNTAXERROR;
	}

	domain = argv->arr[0];
	host = argv->arr[1];

	l = domain->len + host->len + 2;

	rblhost = (char *) malloc(l * sizeof(char));
	/*
	 * point to end of string 
	 */
	s = host->val + host->len - 1;

	r = rblhost;

	/*
	 * reverse the order 
	 */
	hostaddr = host->val;

	do {
		for (c = s; *c != '.' && c >= hostaddr; c--);
		if (*c == '.')
			d = c;
		else
			d = 0;

		for (c++; c <= s;)
			*r++ = *c++;

		if (d) {
			*r++ = '.';
			s = d - 1;
		}
	} while (d);

	*r++ = '.';

	c = domain->val;

	/*
	 * add after the inverted hostaddr 
	 */
	l = (int) domain->len;
	for (n = 0; n < l; n++)
		*r++ = *c++;

	if (*r != '.') {
		*++r = '.';
	}
	*++r = '\0';

	if (gethostbyname(rblhost) != 0)
		return SPOCP_SUCCESS;
	else
		return SPOCP_DENIED;
}

plugin_t        rbl_module = {
	SPOCP20_PLUGIN_STUFF,
	rbl_test,
	NULL,
	NULL
};
