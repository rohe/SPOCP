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
	${SLIB}atom.o \
	${SLIB}basic.o \
	${SLIB}boundary.o \
	${SLIB}boundarysl.o \
	${SLIB}branch.o \
	${SLIB}bsdrb.o \
	${SLIB}dset.o \
	${SLIB}hash.o \
	${SLIB}hashfunc.o \
	${SLIB}index.o \
	${SLIB}junc.o \
	${SLIB}ll.o \
	${SLIB}log.o \
	${SLIB}oct.o \
	${SLIB}octarr.o \
	${SLIB}range.o \
	${SLIB}rangestore.o \
	${SLIB}rdb.o \
	${SLIB}ruleinst.o \
	${SLIB}sexp.o \
	${SLIB}sha1.o \
	${SLIB}skiplist.o \
	${SLIB}ssn.o \
	${SLIB}ssn_b.o \
	${SLIB}varr.o \
	${SLIB}verify.o \
	${SLIB}wrappers.o \
	../ando/testRunner.o \
	test_ssn.o

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
${SLIB}atom.o: ${SLIB}atom.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}atom.c -o ${SLIB}atom.o
${SLIB}basic.o: ${SLIB}basic.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}basic.c -o ${SLIB}basic.o
${SLIB}boundary.o: ${SLIB}boundary.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}boundary.c -o ${SLIB}boundary.o
${SLIB}boundarysl.o: ${SLIB}boundarysl.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}boundarysl.c -o ${SLIB}boundarysl.o
${SLIB}branch.o: ${SLIB}branch.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}branch.c -o ${SLIB}branch.o
${SLIB}bsdrb.o: ${SLIB}bsdrb.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}bsdrb.c -o ${SLIB}bsdrb.o
${SLIB}dset.o: ${SLIB}dset.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}dset.c -o ${SLIB}dset.o
${SLIB}hash.o: ${SLIB}hash.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}hash.c -o ${SLIB}hash.o
${SLIB}hashfunc.o: ${SLIB}hashfunc.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}hashfunc.c -o ${SLIB}hashfunc.o
${SLIB}index.o: ${SLIB}index.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}index.c -o ${SLIB}index.o
${SLIB}junc.o: ${SLIB}junc.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}junc.c -o ${SLIB}junc.o
${SLIB}ll.o: ${SLIB}ll.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}ll.c -o ${SLIB}ll.o
${SLIB}log.o: ${SLIB}log.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}log.c -o ${SLIB}log.o
${SLIB}oct.o: ${SLIB}oct.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}oct.c -o ${SLIB}oct.o
${SLIB}octarr.o: ${SLIB}octarr.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}octarr.c -o ${SLIB}octarr.o
${SLIB}range.o: ${SLIB}range.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}range.c -o ${SLIB}range.o
${SLIB}rangestore.o: ${SLIB}rangestore.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}rangestore.c -o ${SLIB}rangestore.o
${SLIB}rdb.o: ${SLIB}rdb.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}rdb.c -o ${SLIB}rdb.o
${SLIB}ruleinst.o: ${SLIB}ruleinst.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}ruleinst.c -o ${SLIB}ruleinst.o
${SLIB}sexp.o: ${SLIB}sexp.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}sexp.c -o ${SLIB}sexp.o
${SLIB}sha1.o: ${SLIB}sha1.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}sha1.c -o ${SLIB}sha1.o
${SLIB}skiplist.o: ${SLIB}skiplist.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}skiplist.c -o ${SLIB}skiplist.o
${SLIB}ssn.o: ${SLIB}ssn.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}ssn.c -o ${SLIB}ssn.o
${SLIB}ssn_b.o: ${SLIB}ssn_b.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}ssn_b.c -o ${SLIB}ssn_b.o
${SLIB}varr.o: ${SLIB}varr.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}varr.c -o ${SLIB}varr.o
${SLIB}verify.o: ${SLIB}verify.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}verify.c -o ${SLIB}verify.o
${SLIB}wrappers.o: ${SLIB}wrappers.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${SLIB}wrappers.c -o ${SLIB}wrappers.o
../ando/testRunner.o: ../ando/src/testRunner.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ../ando/src/testRunner.c -o ../ando/testRunner.o
test_ssn.o: ./test_ssn.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ./test_ssn.c

clean:
	rm -rf ${LM} ${OBJS}
#
run: all
	${LM}
#