#include "ClassMacro.h"
#include <cxxabi.h>

DemanglerHelper::DemanglerHelper() : name(NULL), sz(1024) {
    name = new char[sz];
    name[0] = 0;
    name[sz - 1] = 0;
}

DemanglerHelper::~DemanglerHelper() {
    delete[](name);
}

char* DemanglerHelper::getName(const char* nm) {
    int status = 0;
    size_t tmp = sz;
    name = abi::__cxa_demangle(nm, name, &tmp, &status);
    if (status != 0) {
        name[0] = 0;
    } else {
        if (tmp > sz) {
            sz = tmp;
        }
    }
    return name;
}

DemanglerHelper DemanglerHelper::singleton;
