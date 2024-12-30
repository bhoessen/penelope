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

#ifndef CLASSMACRO_H
#define	CLASSMACRO_H

#include <stddef.h>

/**
 * The purpose of this class is to be able to demangle the name of a class
 * easily
 */
class DemanglerHelper {
private:
    /** The name of the last class that we demangled */
    char* name;
    /** The size of the buffer name */
    size_t sz;
public:

    /** Create a new DemanglerHelper */
    DemanglerHelper();
    
    /** Destructor */
    ~DemanglerHelper();

    /**
     * Retrieve the demangled name of a given class
     * @param nm the mangled name of a class
     * @return a string representing the demangled name of the class or a empty
     *         string if we weren't able to demangle the name
     */
    char* getName(const char* nm);
    
    /** The static demangler that will be used in the macro */
    static DemanglerHelper singleton;

};



#ifndef __CLASS__
#ifdef DEBUG
#define __CLASS__  DemanglerHelper::singleton.getName(typeid(*this).name())
#else
#define __CLASS__ typeid(*this).name()
#endif /* DEBUG */
#endif

#endif	/* CLASSMACRO_H */

