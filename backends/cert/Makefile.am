if BUILD_CERT
lib_LTLIBRARIES = libcertplugin.la

libcertplugin_la_SOURCES = cert.c
libcertplugin_la_LDFLAGS = -module
libcertplugin_la_LIBADD = -lspocp -L../../lib

INCLUDES = -I../../server/ 
endif BUILD_CERT
