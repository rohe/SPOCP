if BUILD_LDAPSET
lib_LTLIBRARIES = libldapsetplugin.la

libldapsetplugin_la_SOURCES = ldapset.c
libldapsetplugin_la_LDFLAGS = -module
libldapsetplugin_la_LIBADD = -lldap -llber ../../lib/libspocp.la

INCLUDES = -I$(srcdir)/../../include
endif BUILD_LDAPSET

