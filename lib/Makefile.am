AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include
lib_LTLIBRARIES = libspocp.la

libspocp_la_SOURCES =  \
	db0.c hash.c hashfunc.c log.c ssn.c verify.c \
	match.c wrappers.c index.c rdwr.c \
	free.c input.c string.c rbtree.c octarr.c \
	parr.c io.c spocp.c rm.c slist.c list.c \
	ll.c sha1.c subelem.c be_cpool.c oct.c sexp.c \
	plugin.c cachetime.c rvapi.c aci.c varr.c bcond.c \
	dback.c dbapi.c cache.c

libspocp_la_LDFLAGS = -version-info 1:2:0

