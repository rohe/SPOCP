if BUILD_CERT
lib_LTLIBRARIES = libcertplugin.la

libcertplugin_la_SOURCES = cert.c
libcertplugin_la_LDFLAGS = -module
libcertplugin_la_LIBADD = ../../lib/libspocp.la

INCLUDES = -I$(srcdir)/../../include
endif BUILD_CERT
