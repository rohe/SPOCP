if BUILD_MAILMATCH
lib_LTLIBRARIES = libmailmatchplugin.la

libmailmatchplugin_la_SOURCES = mailmatch.c
libmailmatchplugin_la_LDFLAGS = -module
libmailmatchplugin_la_LIBADD = ../../lib/libspocp.la

INCLUDES = -I$(srcdir)/../../include

endif BUILD_MAILMATCH
