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

#ifndef COOPERATIONTEST_H
#define	COOPERATIONTEST_H


#include <cppunit/extensions/HelperMacros.h>
#include "../../include/penelope/utils/Vec.h"
#include "../../include/penelope/core/SolverTypes.h"

class CooperationTest : public CppUnit::TestFixture {
public:

    CPPUNIT_TEST_SUITE(CooperationTest);
    CPPUNIT_TEST(testdp10);
    CPPUNIT_TEST(testdp04s);
    CPPUNIT_TEST(testdp04u);
    CPPUNIT_TEST(testaaai10);
    CPPUNIT_TEST_SUITE_END();

    void testdp10();
    void testdp04s();
    void testdp04u();
    void testaaai10();


private:

    bool solve(const char* fileName, int nbThreads = 1);

};

#endif	/* COOPERATIONTEST_H */

