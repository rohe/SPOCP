AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include
lib_LTLIBRARIES = libspocp.la libspocpbasic.la

libspocpbasic_la_SOURCES = strfunc.c oct.c octarr.c sexp.c wrappers.c log.c \
	sexptool.c parse.c

libspocp_la_SOURCES =  \
    aci.c dback.c match.c sexptool.c \
    atom.c dbapi.c oct.c sha1.c \
    atommatch.c display.c octarr.c skiplist.c \
    basic.c dset.c parr.c \
    bcexp.c element.c parse.c spocp.c \
    bcond.c element_I.c plugin.c spocp_result.c \
    be_cpool.c element_display.c range.c ssn.c \
    boundary.c eval.c rangestore.c ssn_b.c \
    boundarysl.c rdb.c strfunc.c \
    branch.c hash.c rdwr.c subelem.c \
    bsdrb.c hashfunc.c resset.c varr.c \
    cache.c index.c rm.c verify.c \
    cachetime.c junc.c ruleinfo.c wrappers.c \
    check.c list.c ruleinst.c \
    ll.c rvapi.c \
    db0.c log.c sexp.c

libspocp_la_LDFLAGS = -version-info 2:2:0
libspocpbasic_la_LDFLAGS = -version-info 2:1:0

