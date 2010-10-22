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
INCLUDE_OBJ = -I../../../libtest/ando/include -I../../../include -I. -I.. -I ../../../
SLIB=../../lib/
TESTRUNNER_O = ../../../libtest/ando/testRunner.o
TESTRUNNER_C = ../../../libtest/ando/src/testRunner.c

OBJS = \
	${TESTRUNNER_O} \
	test_open.o
	
CFLAG_LM  = ${CFLAG} -ggdb -Wall
CFLAG_OBJ = ${CFLAG} -ggdb -Wall

#
LIB_X = -L/usr/local/spocp/lib -lspocpclient

LM = ./test

#
all: ${LM}
#
${LM}: ${OBJS}
	${CC} ${CFLAG_LM} ${CFLAG_OBJ} ${INCLUDE_OBJ} -o ${LM} ${OBJS} ${LIB_X}
#
${TESTRUNNER_O}: ${TESTRUNNER_C}
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ${TESTRUNNER_C} -o ${TESTRUNNER_O}
test_open.o: ./test_open.c
	${CC} ${CFLAG_OBJ} ${INCLUDE_OBJ} -c ./test_open.c
#
#
clean:
	rm -rf ${LM} ${OBJS}
#
run: all
	${LM}
#