/*
Copyright (c) <2013> <B.Hoessen>

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

#ifndef ASSERTTEST_H
#define	ASSERTTEST_H

#include <cppunit/extensions/HelperMacros.h>

class AssertTest : public CppUnit::TestFixture {
public:

    CPPUNIT_TEST_SUITE(AssertTest);
    CPPUNIT_TEST(testCondition);
    CPPUNIT_TEST(testEquality);
    CPPUNIT_TEST_SUITE_END();

    /**
     * Test the condition checks
     */
    void testCondition();

    /**
     * Test the equality checks
     */
    void testEquality();


};

#endif	/* ASSERTTEST_H */

