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
	${SLIB}bsdrb.o \
	${SLIB}check.o \
	${SLIB}ll.o \
	${SLIB}log.o \
	${SLIB}oct.o \
	${SLIB}rdb.o \
	${SLIB}ruleinst.o \
	${SLIB}sha1.o \
	${SLIB}varr.o \
	${SLIB}wrappers.o \
	../ando/testRunner.o \
	test_check.o\
	
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
${SLIB}bsdrb.o: ${SLIB}bsdrb.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}bsdrb.c -o ${SLIB}bsdrb.o
${SLIB}check.o: ${SLIB}check.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}check.c -o ${SLIB}check.o
${SLIB}ll.o: ${SLIB}ll.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}ll.c -o ${SLIB}ll.o
${SLIB}log.o: ${SLIB}log.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}log.c -o ${SLIB}log.o
${SLIB}oct.o: ${SLIB}oct.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}oct.c -o ${SLIB}oct.o
${SLIB}rdb.o: ${SLIB}rdb.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}rdb.c -o ${SLIB}rdb.o
${SLIB}ruleinst.o: ${SLIB}ruleinst.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}ruleinst.c -o ${SLIB}ruleinst.o
${SLIB}sha1.o: ${SLIB}sha1.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}sha1.c -o ${SLIB}sha1.o
${SLIB}varr.o: ${SLIB}varr.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}varr.c -o ${SLIB}varr.o
${SLIB}wrappers.o: ${SLIB}wrappers.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}wrappers.c -o ${SLIB}wrappers.o
../ando/testRunner.o: ../ando/src/testRunner.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ../ando/src/testRunner.c -o ../ando/testRunner.o
test_check.o: ./test_check.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ./test_check.c
#
#
clean:
	rm -rf ${LM} ${OBJS}
#
run: all
	${LM}
#