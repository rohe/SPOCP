if BUILD_RBL
lib_LTLIBRARIES = librblplugin.la

librblplugin_la_SOURCES = rbl.c
librblplugin_la_LDFLAGS = -module

librblplugin_la_LIBADD = -lspocp -L../../lib

INCLUDES = -I../../server/ 

endif BUILD_RBL
