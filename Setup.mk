all:
	aclocal -I cf
	autoheader
	autoconf -I cf
	libtoolize -c
	automake -ac

clean:
	rm -rf *~ config.* libtool stamp-h1 confcache autom4te.cache depcomp install-sh ltconfig ltmain.sh missing mkinstalldirs config.h.in configure stamp-h.in aclocal.m4 .deps
	find . -name Makefile.in -exec rm \{\} \;
	find . -name Makefile -exec rm \{\} \;
