if BUILD_GDBM
lib_LTLIBRARIES = libdbackgdbm.la

libdbackgdbm_la_SOURCES = dback_gdbm.c
libdbackgdbm_la_LDFLAGS = -module
libdbackgdbm_la_LIBADD = -lgdbm ../../lib/libspocp.la

INCLUDES = -I$(srcdir)/../../include
endif BUILD_GDBM
