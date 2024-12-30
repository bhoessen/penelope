/*
Copyright (c) <2012> <B.Hoessen>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
 */

#ifndef ASSERTS_H
#define	ASSERTS_H


#ifdef DEBUG

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <assert.h>

extern unsigned long int __nb_Asserts;

#ifdef WIN32
#define PENELOPE_TRACE(_a) {}
#else

#include <execinfo.h>
#include <cxxabi.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_FRAMES 100

//Code for PENELOPE_TRACE obtained at:
//http://mykospark.net/2009/09/runtime-backtrace-in-c-with-name-demangling/
//In order to work, binaries must be compiled with the -rdynamic option
#define PENELOPE_TRACE(outputStream) {void* addresses[MAX_FRAMES];\
    int bt_size = backtrace(addresses, MAX_FRAMES);\
    char** symbols = backtrace_symbols(addresses, bt_size);\
    for (int x = 0; x < bt_size; ++x) {\
        size_t dem_size;\
        int status;\
        char temp[512];\
        char* demangled;\
        if (1 == sscanf(symbols[x], "%*[^(]%*[^_]%511[^)+]", temp)) {\
          if (NULL != (demangled = abi::__cxa_demangle(temp, NULL, &dem_size, &status))) {\
            outputStream << "#" << x << " " << demangled << "\n";\
            free(demangled);\
          }\
        }else if (1 == sscanf(symbols[x], "%511s", temp)) {\
          outputStream << "#" << x << " " << temp << "\n";\
        }else {\
          outputStream << "#" << x << " " << symbols[x] << "\n";\
        }\
    }\
    free(symbols);\
}

#endif /* WIN32 */

namespace penelope {

    template<typename T>
    void checkEquality(const T& expected, const T& actual, const char* nameEx, const char* nameAct, const char* fileName, int lineNb, std::ostream& out = std::cerr) {
        __nb_Asserts++;
        if (expected != actual) {
            out << nameEx << " == " << nameAct << " is not respected\n";
            out << "Expected: " << expected << "\n";
            out << "Actual:   " << actual << "\n";
            out << "In " << fileName << ":" << lineNb << "\n";
            PENELOPE_TRACE(out);
            assert((expected) == (actual));
        }
    }

    inline void checkTruth(bool condition, const char* nameCond, const char* fileName, int lineNb, std::ostream& out = std::cerr) {
        __nb_Asserts++;
        if (!(condition)) {
            out << nameCond << " is not respected\n";
            out << "In " << fileName << ":" << lineNb << "\n";
            PENELOPE_TRACE(out);
            assert(condition);
        }

    }
}


#define ASSERT_TRUE(condition) {penelope::checkTruth(condition, #condition, __FILE__, __LINE__);}

#define ASSERT_EQUAL(expected, actual) {penelope::checkEquality(expected, actual, #expected, #actual, __FILE__, __LINE__);}

#define ASSERT_NB_ASSERT __nb_Asserts

#else

#define ASSERT_EQUAL(expected, actual)

#define ASSERT_TRUE(condition)

#define ASSERT_NB_ASSERT 0

#define PENELOPE_TRACE(_a) {}

#endif

#endif	/* ASSERTS_H */

