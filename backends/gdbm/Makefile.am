if BUILD_GDBM
lib_LTLIBRARIES = libgdbmplugin.la

libgdbmplugin_la_SOURCES = gdbm.c
libgdbmplugin_la_LDFLAGS = -module
libgdbmplugin_la_LIBADD = -lgdbm ../../lib/libspocp.la

INCLUDES = -I$(srcdir)/../../include
endif BUILD_GDBM
