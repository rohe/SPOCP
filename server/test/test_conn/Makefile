CC=gcc
TOP=../../../
INCLUDE_OBJ = -I${TOP}libtest/ando/include -I${TOP}include -I. -I../.. -I${TOP}
ANDO=${TOP}libtest/ando/
SLIB=${TOP}lib/
SSRC=../../
SPOCPLIB=-L${SLIB}.libs -lspocp
OBJS = \
	${ANDO}testRunner.o \
	${SSRC}conn.o \
	${SSRC}iobuf.o \
	${SSRC}reply.o \
	test_conn.o
	
CFLAG_LM  = ${CFLAG} -ggdb -Wall
CFLAG_OBJ = ${CFLAG} -ggdb -Wall

#
LIB_X = ${SPOCPLIB}

LM = ./test

#
all: ${LM}
#
${LM}: ${OBJS}
	${CC} ${CFLAG_LM} ${CFLAG_OBJ} ${INCLUDE_OBJ} -o ${LM} ${OBJS} ${LIB_X}
#


${ANDO}testRunner.o: ${ANDO}src/testRunner.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${ANDO}src/testRunner.c -o ${ANDO}testRunner.o
${SSRC}conn.o: ${SSRC}conn.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SSRC}conn.c -o ${SSRC}conn.o
${SSRC}iobuf.o: ${SSRC}iobuf.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SSRC}iobuf.c -o ${SSRC}iobuf.o
${SSRC}reply.o: ${SSRC}reply.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SSRC}reply.c -o ${SSRC}reply.o
test_conn.o: ./test_conn.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ./test_conn.c
#
#
clean:
	rm -rf ${LM} ${OBJS}
#
run: all
	${LM}
#