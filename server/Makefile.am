bin_PROGRAMS = spocd dbg_spocd

common_SOURCES = iobuf.c conn.c socket.c init.c \
	saci.c ruleset.c util.c read.c ss.c readcnf.c

spocd_SOURCES = main.c srv.c tls.c \
	config.c tpool.c cpool.c pool.c run.c $(common_SOURCES)

spocd_LDADD = $(WRAP_LIBS) $(MODULE_LIBS) $(top_builddir)/lib/libspocp.la #@SASL_LDFLAGS@ @SASL_LIBS@

#debug version

dbg_spocd_SOURCES = $(spocd_SOURCES)

dbg_spocd_LDADD = $(WRAP_LIBS) $(MODULE_LIBS) $(top_builddir)/lib/libspocp.la #@SASL_LDFLAGS@ @SASL_LIBS@

dbg_spocd_LDFLAGS = -static

#

INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include #@SASL_CPPFLAGS@

EXTRA_DIST = srv.h srvconf.h readcnf.h

include_HEADERS = srvconf.h readcnf.h

