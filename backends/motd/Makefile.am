if BUILD_MOTD
lib_LTLIBRARIES = libmotdplugin.la

libmotdplugin_la_SOURCES = motd.c
libmotdplugin_la_LDFLAGS = -module
libmotdplugin_la_LIBADD = ../../lib/libspocp.la ../../lib/libspocpbasic.la

INCLUDES = -I$(srcdir)/../../include

endif BUILD_MOTD
