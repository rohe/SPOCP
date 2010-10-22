bin_PROGRAMS = spocpput spocpget spocptest spocppatch spocpperf

#

spocpput_SOURCES = spocpput.c ../../server/readcnf.c
spocpput_LDADD = ../src/libspocpclient.la

#

spocpget_SOURCES = spocpget.c 
spocpget_LDADD = ../src/libspocpclient.la

#

spocptest_SOURCES = spocptest.c 
spocptest_LDADD = ../src/libspocpclient.la

#

spocppatch_SOURCES = spocppatch.c 
spocppatch_LDADD = ../src/libspocpclient.la

#
spocpperf_SOURCES = spocpperf.c 
spocpperf_LDADD = ../src/libspocpclient.la

#

INCLUDES =  -I. -I${top_srcdir}/include -I${top_srcdir}/server @SASL_CPPFLAGS@
