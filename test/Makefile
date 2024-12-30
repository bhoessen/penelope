LIBRARY_NAME=penelope
CC=g++
cc=gcc
CONF=Debug
#the directory that will contain the binary
DIST_DIR=dist/${CONF}
#directory containing the objects files and the dependencies
BUILD_DIR=build/${CONF}


BASECPPFLAGS=-g -Wall -Wextra -Werror -DDEBUG
LDLIBS_LOCATION=../dist/${CONF}

ifeq (${CONF},Valgrind)
    CPPFLAGS = -DVALGRIND ${BASECPPFLAGS}
else
    CPPFLAGS=${BASECPPFLAGS}
endif


BASELDFLAGS=-lpthread -lgomp -lcppunit -l${LIBRARY_NAME}
LDFLAGS=-flto -rdynamic ${BASELDFLAGS}


TARGET=${DIST_DIR}/${LIBRARY_NAME}-test
SRCFILES=$(wildcard src/*.cpp)
OSRCFILES=$(SRCFILES:.cpp=.o)
OBJECTFILES=$(addprefix ${BUILD_DIR}/, ${OSRCFILES})

all: ${TARGET}

${LIBRARY_NAME}-test: Makefile ${TARGET}

gdb: ${TARGET}
	@echo "Launching gdb on ${TARGET}"
	@sh -c "LD_LIBRARY_PATH=${LDLIBS_LOCATION}:$$LD_LIBRARY_PATH export LD_LIBRARY_PATH; gdb ${TARGET}"

test: ${TARGET} 
	@echo "Launching ${TARGET}"
	@sh -c "LD_LIBRARY_PATH=${LDLIBS_LOCATION}:$$LD_LIBRARY_PATH ${TARGET}"

coverage: CPPFLAGS = --coverage $(BASECPPFLAGS)
coverage: LDFLAGS = -lgcov ${BASELDFLAGS}
coverage: .cov ${TARGET} test
	@rm -rf tmp.info app.info
#	capture the information about the library itself
	@lcov --directory ../build --capture --output-file cpp.info -b ../ > /dev/null
#	capture the information about the template in the library and the tests
	@lcov --directory ./build --capture --output-file tpp.info -b ./ > /dev/null
#	remove the information about the includes
	@lcov -r tpp.info "/usr*" --output-file atpp.info > /dev/null
	@lcov -r cpp.info "/usr*" --output-file acpp.info > /dev/null
#	remove the information about the different test
	@lcov -r atpp.info "`pwd`/*" --output-file tpp.info > /dev/null
	@lcov -r acpp.info "`pwd`/*" --output-file cpp.info > /dev/null
#	generate the related html
	@genhtml  --output-directory coverage ./tpp.info ./cpp.info -t ${TARGET} > ./coverage.log
#	check that the code coverage didn't drop
	@cat ./coverage.log | grep 'lines' | sed 's/.*: \(.*\)%.*/\1/' > .tmp.cov
	@echo `cat .cov` " <= " `cat .tmp.cov` > .calc
	@if [ "`cat .calc | bc`" != "1" ]; then echo "code coverage has drop from " `cat .cov` "to" `cat .tmp.cov`; false; fi;
#	save the current coverage for next usage
	@mv .tmp.cov .cov

.cov:
	@echo "0.0" > $@

valgrind:
	@make CONF=Valgrind
	@sh -c "LD_LIBRARY_PATH=${LDLIBS_LOCATION}:$$LD_LIBRARY_PATH valgrind --trace-children=yes --child-silent-after-fork=yes --leak-check=full --show-reachable=yes --track-origins=yes --error-exitcode=1 --db-attach=yes ./dist/Valgrind/${LIBRARY_NAME}-test"

${TARGET}: ${OBJECTFILES}
	@mkdir -p ${DIST_DIR}
	g++ -o ${TARGET} ${OBJECTFILES} -L${LDLIBS_LOCATION} ${LDFLAGS}

${DIST_DIR}:
	@mkdir -p ${DIST_DIR}

${BUILD_DIR}:
	@mkdir -p ${BUILD_DIR}

${BUILD_DIR}/%.o: %.cpp Makefile
	@mkdir -p `echo "$@" | sed "s/\(.*\)\/.*\.o/\1/"`
	@rm -f $@.d
	g++ ${CPPFLAGS} -c -I../include -MMD -MP -MF $@.d -o $@ $<

clean:
	rm -rf build dist acpp.info atpp.info cpp.info tpp.info coverage.log .tmp.cov .calc .*~

cleaner:
	rm -rf .cov coverage

#load dependencies definitions
DEPFILES=$(wildcard $(addsuffix .d, ${OBJECTFILES}))
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
