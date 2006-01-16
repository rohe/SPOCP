AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libspocpclient.la

libspocpclient_la_SOURCES = clilib.c

libspocpclient_la_LDFLAGS = -version-info 2:1:0 -L../../lib -lspocpbasic @SASL_LDFLAGS@ @SASL_LIBS@

INCLUDES = -I../include @SASL_CPPFLAGS@
