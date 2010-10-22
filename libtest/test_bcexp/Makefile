# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#CC=lint
CC=gcc
INCLUDE_OBJ = -I../ando/include -I../../include -I. -I..
SLIB=../../lib/
OBJS = \
	${SLIB}basic.o \
	${SLIB}bcexp.o\
	${SLIB}be_cpool.o \
	${SLIB}cache.o \
	${SLIB}cachetime.o \
	${SLIB}hashfunc.o \
	${SLIB}log.o \
	${SLIB}oct.o \
	${SLIB}octarr.o \
	${SLIB}plugin.o\
	${SLIB}rdwr.o \
	${SLIB}sexp.o \
	${SLIB}varr.o \
	${SLIB}verify.o \
	${SLIB}wrappers.o \
	test_bcexp.o\
	../ando/testRunner.o 
	
CFLAG_LM  = ${CFLAG} -ggdb -Wall
CFLAG_OBJ = ${CFLAG} -ggdb -Wall

#
LIB_X   = ${LIB}

LM = ./test

#
all: ${LM}
#
${LM}: ${OBJS}
	${CC} ${CFLAG_LM} ${CFLAG_OBJ} ${INCLUDE_OBJ} -o ${LM} ${OBJS} ${LIB_X}
#
${SLIB}basic.o: ${SLIB}basic.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}basic.c -o ${SLIB}basic.o
${SLIB}bcexp.o: ${SLIB}bcexp.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}bcexp.c -o ${SLIB}bcexp.o
${SLIB}be_cpool.o: ${SLIB}be_cpool.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}be_cpool.c -o ${SLIB}be_cpool.o
${SLIB}cache.o: ${SLIB}cache.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}cache.c -o ${SLIB}cache.o
${SLIB}cachetime.o: ${SLIB}cachetime.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}cachetime.c -o ${SLIB}cachetime.o
${SLIB}hashfunc.o: ${SLIB}hashfunc.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}hashfunc.c -o ${SLIB}hashfunc.o
${SLIB}log.o: ${SLIB}log.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}log.c -o ${SLIB}log.o
${SLIB}oct.o: ${SLIB}oct.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}oct.c -o ${SLIB}oct.o
${SLIB}octarr.o: ${SLIB}octarr.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}octarr.c -o ${SLIB}octarr.o
${SLIB}plugin.o: ${SLIB}plugin.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}plugin.c -o ${SLIB}plugin.o
${SLIB}rdwr.o: ${SLIB}rdwr.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}rdwr.c -o ${SLIB}rdwr.o
${SLIB}sexp.o: ${SLIB}sexp.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}sexp.c -o ${SLIB}sexp.o
${SLIB}varr.o: ${SLIB}varr.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}varr.c -o ${SLIB}varr.o
${SLIB}verify.o: ${SLIB}verify.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}verify.c -o ${SLIB}verify.o
${SLIB}wrappers.o: ${SLIB}wrappers.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}wrappers.c -o ${SLIB}wrappers.o
test_bcexp.o: ./test_bcexp.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ./test_bcexp.c
../ando/testRunner.o: ../ando/src/testRunner.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ../ando/src/testRunner.c -o ../ando/testRunner.o
#
#
clean:
	rm -rf ${LM} ${OBJS}
#
run: all
	${LM}
#