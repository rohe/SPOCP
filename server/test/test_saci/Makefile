CC=gcc
TOP=../../../
INCLUDE_OBJ = -I${TOP}libtest/ando/include -I${TOP}include -I. -I../.. -I${TOP}
ANDO=${TOP}libtest/ando/
SLIB=${TOP}lib/
SSRC=../../
SPOCPLIB=-L${SLIB}.libs -lspocp
OBJS = \
	${ANDO}testRunner.o \
	${SSRC}ruleset.o \
	${SSRC}saci.o \
	test_saci.o
	
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
${SSRC}ruleset.o: ${SSRC}ruleset.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SSRC}ruleset.c -o ${SSRC}ruleset.o
${SSRC}saci.o: ${SSRC}saci.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SSRC}saci.c -o ${SSRC}saci.o
test_saci.o: ./test_saci.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ./test_saci.c
#
#
clean:
	rm -rf ${LM} ${OBJS}
#
run: all
	${LM}
#