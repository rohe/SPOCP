#include "locl.h"
RCSID("$Id$");

char           *
rm_lt_sp(char *s, int shift)
{
	char           *sp, *cp;
	int             f;

	if (s == 0)
		return 0;

	for (sp = s; *sp == ' ' || *sp == '\t'; sp++);
	for (cp = &sp[strlen(sp) - 1]; *cp == ' ' || *cp == '\t'; cp--)
		*cp = '\0';

	if (shift && sp != s) {
		f = strlen(sp);
		memmove(s, sp, f);
		s[f] = '\0';

		return s;
	} else
		return sp;
}
