if BUILD_LDAPPROXY
lib_LTLIBRARIES = libldapproxyplugin.la

libldapproxyplugin_la_SOURCES = ldapproxy.c
libldapproxyplugin_la_LDFLAGS = -module
libldapproxyplugin_la_LIBADD = -lldap -llber ../../lib/libspocp.la

INCLUDES = -I$(srcdir)/../../include
endif BUILD_LDAPPROXY

