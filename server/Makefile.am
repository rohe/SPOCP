bin_PROGRAMS = spocd dbg_spocd

common_SOURCES = con.c socket.c rdwr.c \
	aci.c ruleset.c util.c read.c ss.c 

spocd_SOURCES = main.c srv.c thread.c tls.c \
	config.c $(common_SOURCES)

spocd_LDADD = $(WRAP_LIBS) $(MODULE_LIBS) $(top_builddir)/lib/libspocp.la 

#debug version

dbg_spocd_SOURCES = $(spocd_SOURCES)

dbg_spocd_LDADD = $(WRAP_LIBS) $(MODULE_LIBS) $(top_builddir)/lib/libspocp.la 

dbg_spocd_LDFLAGS = -static

#

INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include -I/usr/local/ssl/include 

EXTRA_DIST = srv.h srvconf.h

include_HEADERS = srvconf.h

