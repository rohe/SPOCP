if BUILD_MOTD
lib_LTLIBRARIES = libmotdplugin.la

libmotdplugin_la_SOURCES = motd.c
libmotdplugin_la_LDFLAGS = -module

libmotdplugin_la_LIBADD = -lspocp -L../../lib

INCLUDES = -I../../server/ 

endif BUILD_MOTD
