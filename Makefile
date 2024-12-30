LIBRARY_NAME=penelope

#available configurations are: Release, Debug
CONF=Release

CC=g++
cc=gcc
DIST_DIR=dist/${CONF}
BUILD_DIR=build/${CONF}
AR=ar

LIB-STATIC=lib${LIBRARY_NAME}.a
LIB-SHARED=lib${LIBRARY_NAME}.so
LIBRARY=${LIB-SHARED}

BINARY=${LIBRARY_NAME}


SRCFILES=$(wildcard src/*.cpp)
HEADERS=$(wildcard include/${LIBRARY_NAME}/*.h)
OSRCFILES=$(SRCFILES:.cpp=.o)
OBJECTFILES=$(addprefix ${BUILD_DIR}/, ${OSRCFILES})

#we need to separate the files needed for the binary as they are not needed
#for the library
BINFILES=$(wildcard src/bin/*.cpp)
BINHEADERS=$(wildcard src/bin/*.h)
OBINFILES=$(BINFILES:.cpp=.o)
OBJECTBINFILES=$(addprefix ${BUILD_DIR}/, ${OBINFILES})

PIC=-fPIC
WARNINGS=-Wall -Wextra -Werror -Wshadow -Woverloaded-virtual -fno-strict-aliasing
DEFINES=
ifeq (${CONF},Release)
#we are in release mode, so be sure to optimize everything!
  PIC=
  OPTIM_FLAGS=-O3
  BASECPPFLAGS=-fopenmp ${WARNINGS}
  BASELDFLAGS=-flto
  DEFINES=-D RELEASE
else
#we are in a debug mode, so do not optimize anything!
  OPTIM_FLAGS=-O0
  BASECPPFLAGS=-fopenmp ${WARNINGS} -g -DDEBUG
  BASELDFLAGS=-rdynamic 
endif


ifeq (${CONF},Coverage)
  CPPFLAGS = --coverage $(BASECPPFLAGS)
  LDFLAGS = -lgcov -fprofile-arcs ${BASELDFLAGS}
else
  CPPFLAGS=${BASECPPFLAGS}
  LDFLAGS=${BASELDFLAGS} -lpthread -lgomp
endif

SHARED=

all: ${BINARY} 

${LIB-SHARED}: LIBRARY = ${LIB-SHARED}
${LIB-SHARED}: SHARED =
${LIB-SHARED}: PIC = -fPIC
${LIB-SHARED}: ${DIST_DIR}/${LIB-SHARED}


${LIB-STATIC}: LIBRARY = ${LIB-STATIC}
${LIB-STATIC}: SHARED = -static
${LIB-STATIC}: ${DIST_DIR}/${LIB-STATIC}

	
help:
	@echo "This Makefile provide a clean way to compile ${LIBRARY_NAME}"
	@echo "Several targets have been made to simplify the life of the devs"
	@echo "   all:      create both static and shared versions of ${LIBRARY_NAME}"
	@echo "   help:     provide a help message"
	@echo "   metrics:  use cppncss to compute the cyclomatic complexity of ${LIBRARY_NAME}"
	@echo "   rats:     checks statically the code against some well known security issues"
	@echo "   doc:      create the html documentation about this project"
	@echo "   clean:    remove the builds and dist"
	@echo "   cleaner:  remove the builds, dists and logs"
	@echo "   ${LIB-STATIC}: the static version of the library"
	@echo "   ${LIB-SHARED}: the shared version of the library"
	@echo "   SatELite: create the static satelite pre-processor binary"
	@echo "   archive:  an archive that will contain everything needed to distribute ${LIBRARY_NAME}"
	@echo ""
	@echo " There are also some variables that can be used to select the build process"
	@echo "   SHARED:   when creating the binary (target ${LIBRARY_NAME}), leave empty" 
	@echo "             for a shared binary, or \"-static\" for a static binary"

.PHONY: test checks clean cleaner valgrind tasklist

checks:
	@make test
	@make valgrind
	@make coverage
	@make rats
	@make static-check
	@make doc
	@make metrics

valgrind:
	@echo "[valgrind]"
	@make CONF=Valgrind ${LIBRARY}
	@make -C test CONF=Valgrind $@

coverage:
	@echo "[coverage]"
	@make CONF=Coverage ${LIBRARY}
	@make -C test CONF=Coverage $@

#${BINARY}: PIC =
${BINARY}: Makefile ${DIST_DIR}/${BINARY}
	@cp ${DIST_DIR}/${BINARY} ./

${DIST_DIR}/${BINARY}: Makefile ${DIST_DIR} ${OBJECTFILES} ${OBJECTBINFILES} ${HEADERS} ${BINHEADERS}
	${CC} -o ${DIST_DIR}/${BINARY} ${OBJECTFILES} ${OBJECTBINFILES} ${SHARED} ${LDFLAGS}

archive: Makefile Doxyfile configuration.ini ${HEADERS} ${BINHEADERS} ${SRCFILES} cleaner
	tar czvf ${LIBRARY_NAME}-`date +"%y-%m-%d"`.tar.gz Makefile Doxyfile configuration.ini include src

test:
	@make CONF=Debug ${LIBRARY}
	@make -C test $@

${LIBRARY_NAME}-test:
	@make CONF=Debug ${LIBRARY}
	@make -C test $@

#create the documentation about the library
#it can be downloaded here: http://sourceforge.net/projects/doxygen/
doc: Makefile doxygen.log
	@if [ "`more doxygen.log`" != "" ]; then \
	cat doxygen.log | perl -pe 's/Warning:/\e[1;31mWarning:\e[0m/g' | perl -pe 's/^.*.h\:[0-9]*/\e[33m$$&\e[0m/g'; false; \
	fi

doxygen.log: Doxyfile ${HEADERS}
	@echo "[doc]"
	@mkdir -p doc
	@(more Doxyfile; echo "STRIP_FROM_PATH = `pwd`"; echo "PROJECT_NAME = ${LIBRARY_NAME}") > .Doxyfile
	@doxygen .Doxyfile > /dev/null
	@rm .Doxyfile
	
#create the task list of things that still needs to be done
tasklist: .todo
	@more .todo
	
.todo: Makefile ${HEADERS} ${SRCFILES}
	@grep "TODO" ${HEADERS} ${SRCFILES} > .todo

#cppncss compute the cyclomatic complexity measure
#it ca be downloaded here: http://cppncss.sourceforge.net/
metrics: Makefile ${SRCFILES} ${HEADERS}
	@cppncss -r -p="`pwd`" -f=$@ ${SRCFILES} ${HEADERS}

#rats checks for unsecure code
#it can be downloaded here: https://www.fortify.com/ssa-elements/threat-intelligence/rats.html
rats: Makefile ${SRCFILES} ${HEADERS}
	@echo "[rats]"
	@rats -l c -r -w 3 --resultsonly ${SRCFILES} ${BINFILES} ${HEADERS} ${BINHEADERS} > rats.log
	@if [ "`more rats.log`" != "" ]; then more rats.log | perl -pe 's/High/\e[1;31mHigh\e[0m/g' | perl -pe 's/Low/\e[32mLow\e[0m/g' | perl -pe 's/Medium/\e[33mMedium\e[0m/g'; false; fi

static-check: Makefile ${SRCFILES}
	@echo "[static-check]"
	@cppcheck --enable=performance,portability,information,unusedFunction,missingInclude,style -I include/ ${SRCFILES} 2>err.log
	@if [ "`more err.log`" != "" ]; then printf "\033[31m"; cat err.log; printf "\033[0m" false; fi

${DIST_DIR}/%.so: Makefile ${DIST_DIR} ${OBJECTFILES}
	${CC} -shared ${LDFLAGS} -o $@ -fPIC ${OBJECTFILES}
	
${DIST_DIR}/%.a: Makefile ${DIST_DIR} ${OBJECTFILES}
	${AR} -rcs $@ ${OBJECTFILES}

${DIST_DIR}:
	@mkdir -p ${DIST_DIR}

${BUILD_DIR}:
	@mkdir -p ${BUILD_DIR}
	
${BUILD_DIR}/%.o: %.cpp Makefile
	@mkdir -p `echo "$@" | sed "s/\(.*\)\/.*\.o/\1/"`
	@rm -f $@.d
	${CC} ${CPPFLAGS} ${PIC} ${OPTIM_FLAGS} ${DEFINES} -c -Iinclude -MMD -MP -MF $@.d -o $@ $<
	
clean:
	@echo "[cleaning]"
	rm -rf dist build .todo penelope
	@make -C test $@

cleaner: clean
	rm -rf doc metrics doxygen.log rats.log err.log
	@make -C test $@

#load dependencies definitions
DEPFILES=$(wildcard $(addsuffix .d, ${OBJECTFILES}))
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
