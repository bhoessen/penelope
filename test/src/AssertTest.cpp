#define NDEBUG
#include "../../include/penelope/utils/Asserts.h"

#include "AssertTest.h"

#include <string.h>
#include <sstream>

CPPUNIT_TEST_SUITE_REGISTRATION(AssertTest);

using namespace penelope;

void AssertTest::testCondition(){
    {
        std::stringstream out;
        checkTruth(true, "true", __FILE__, __LINE__, out);
        out.flush();
        std::string msg(out.str());
        CPPUNIT_ASSERT_EQUAL(std::string(""), msg);
    }

    {
        std::stringstream out;
        checkTruth(false, "false", "test.cpp",42, out);
        out.flush();
        std::string msg(out.str());
        CPPUNIT_ASSERT_EQUAL(std::string("false is not respected\n"
            "In test.cpp:42\n"
            "#0 penelope::checkTruth(bool, char const*, char const*, int, std::ostream&)\n"), 
            msg.substr(0,114));
    }
    
}

void AssertTest::testEquality(){
    {
        std::stringstream out;
        int tmp = 3;
        checkEquality(tmp, tmp, "tmp", "tmp", __FILE__, __LINE__, out);
        out.flush();
        std::string msg(out.str());
        CPPUNIT_ASSERT_EQUAL(std::string(""), msg);
    }

    {
        std::stringstream out;
        int tmp = 3;
        checkEquality(tmp, tmp+1, "tmp", "tmp+1","test.cpp", 42, out);
        out.flush();
        std::string msg(out.str());
        CPPUNIT_ASSERT_EQUAL(std::string("tmp == tmp+1 is not respected\n"
            "Expected: 3\n"
            "Actual:   4\n"
            "In test.cpp:42\n"
            "#0 void penelope::checkEquality<int>(int const&, int const&, char const*, char const*, char const*, int, std::ostream&)\n"),
            msg.substr(0,189));
    }

}

