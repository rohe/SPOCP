if BUILD_MAILMATCH
lib_LTLIBRARIES = libmailmatchplugin.la

libmailmatchplugin_la_SOURCES = mailmatch.c
libmailmatchplugin_la_LDFLAGS = -module
libmailmatchplugin_la_LIBADD = -lspocp -L../../lib

INCLUDES = -I../../server/ 

endif BUILD_MAILMATCH
