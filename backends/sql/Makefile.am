if BUILD_SQL
lib_LTLIBRARIES = libsqlplugin.la

libsqlplugin_la_SOURCES = sql.c
libsqlplugin_la_LDFLAGS = -module
libsqlplugin_la_LIBADD = ../../lib/libspocp.la

INCLUDES = -I$(srcdir)/../../include

endif BUILD_SQL
