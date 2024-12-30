#include "../../include/penelope/utils/Options.h"

#include "OptionTest.h"

#include <string.h>

CPPUNIT_TEST_SUITE_REGISTRATION(OptionTest);

using namespace penelope;

void OptionTest::testString(){
    StringOption testStringOption("TEST", "string", "The value of the string for test", NULL);
    CPPUNIT_ASSERT(NULL == (const char*)testStringOption);

    CPPUNIT_ASSERT(!testStringOption.parse("-alpha"));
    CPPUNIT_ASSERT(NULL == (const char*)testStringOption);

    CPPUNIT_ASSERT(testStringOption.parse("-string="));
    CPPUNIT_ASSERT_EQUAL(std::string(""), std::string((const char*)testStringOption));

    CPPUNIT_ASSERT(testStringOption.parse("-string=bla"));
    CPPUNIT_ASSERT_EQUAL(std::string("bla"), std::string((const char*)testStringOption));

}

void OptionTest::testDouble(){

    DoubleOption testDoubleOption("TEST", "double", "The value of the double for test", 42.0);
    CPPUNIT_ASSERT_EQUAL(42.0, (double)testDoubleOption);

    CPPUNIT_ASSERT(!testDoubleOption.parse("-alpha"));
    CPPUNIT_ASSERT_EQUAL(42.0, (double)testDoubleOption);

    CPPUNIT_ASSERT(!testDoubleOption.parse("-double="));
    CPPUNIT_ASSERT_EQUAL(42.0, (double)testDoubleOption);

    CPPUNIT_ASSERT(testDoubleOption.parse("-double=2.5"));
    CPPUNIT_ASSERT_EQUAL(2.5, (double)testDoubleOption);

    CPPUNIT_ASSERT(testDoubleOption.parse("-double=-0.1"));
    CPPUNIT_ASSERT_EQUAL(-0.1, (double)testDoubleOption);


}

void OptionTest::testBool(){

    BoolOption testBoolOption("TEST", "bool", "The value of the boolean for test", false);
    CPPUNIT_ASSERT(!(bool)testBoolOption);

    CPPUNIT_ASSERT(!testBoolOption.parse("-alpha"));
    CPPUNIT_ASSERT(!(bool)testBoolOption);
    
    CPPUNIT_ASSERT(testBoolOption.parse("-bool"));
    CPPUNIT_ASSERT((bool)testBoolOption);
    
    CPPUNIT_ASSERT(testBoolOption.parse("-no-bool"));
    CPPUNIT_ASSERT(!(bool)testBoolOption);
    
}

void OptionTest::testInt(){
    IntOption testIntOption("TEST", "int", "The value of the integer for test", 42 );
    CPPUNIT_ASSERT_EQUAL(42, (int)testIntOption);

    CPPUNIT_ASSERT(!testIntOption.parse("-alpha"));
    CPPUNIT_ASSERT_EQUAL(42, (int)testIntOption);

    CPPUNIT_ASSERT(!testIntOption.parse("-int="));
    CPPUNIT_ASSERT_EQUAL(42, (int)testIntOption);

    CPPUNIT_ASSERT(testIntOption.parse("-int=12"));
    CPPUNIT_ASSERT_EQUAL(12, (int)testIntOption);
    
    CPPUNIT_ASSERT(testIntOption.parse("-int=-12"));
    CPPUNIT_ASSERT_EQUAL(-12, (int)testIntOption);

}
