if BUILD_SPOCP
lib_LTLIBRARIES = libspocpplugin.la

libspocpplugin_la_SOURCES = spocp.c
libspocpplugin_la_LDFLAGS = -module
libspocpplugin_la_LIBADD = -lspocp -L../../lib

INCLUDES = -I../../server/ -I/usr/local/spocp/include
endif BUILD_SPOCP
