AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include
lib_LTLIBRARIES = libspocp.la

libspocp_la_SOURCES =  \
	db0.c hash.c hashfunc.c log.c ssn.c verify.c \
	backend.c element.c match.c wrappers.c index.c \
	cache.c free.c input.c string.c rbtree.c octnode.c \
	parr.c io.c spocp.c rm.c print.c slist.c list.c \
	extref.c ll.c sha1.c aci.c subelem.c be_cpool.c \
	plugin.c cachetime.c

libspocp_la_LDFLAGS = -version-info 1:1:1

