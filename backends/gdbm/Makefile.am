if BUILD_GDBM
lib_LTLIBRARIES = libgdbmplugin.la

libgdbmplugin_la_SOURCES = gdbm.c
libgdbmplugin_la_LDFLAGS = -module
libgdbmplugin_la_LIBADD = -lspocp -L../../lib

INCLUDES = -I../../server/ 
endif BUILD_GDBM
