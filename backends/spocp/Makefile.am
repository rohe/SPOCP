if BUILD_SPOCP
lib_LTLIBRARIES = libspocpplugin.la

libspocpplugin_la_SOURCES = spocp.c
libspocpplugin_la_LDFLAGS = -module
libspocpplugin_la_LIBADD = ../../lib/libspocp.la

INCLUDES = -I$(srcdir)/../../include

EXTRA_DIST = spocpcli.h

endif BUILD_SPOCP
