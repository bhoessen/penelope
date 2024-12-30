#include "VecTest.h"

#include "../../include/penelope/utils/Vec.h"

using namespace penelope;

CPPUNIT_TEST_SUITE_REGISTRATION(VecTest);

void VecTest::testCreation(){
    vec<int> a;
    CPPUNIT_ASSERT_EQUAL(0, a.size());
    int m = 8;
    for(int i = 0; i<m; i++){
        a.push(i);
    }
    CPPUNIT_ASSERT_EQUAL(m, a.size());

    for(int i=0; i<m; i++){
        CPPUNIT_ASSERT_EQUAL(i, a[i]);
    }
    CPPUNIT_ASSERT_EQUAL(m, a.size());

    for(int i=0; i<m; i++){
        a.pop();
        CPPUNIT_ASSERT_EQUAL(m-i-1, a.size());
    }

}

void VecTest::testEquality(){
    vec<int> a, b;
    CPPUNIT_ASSERT(a == b);
    int m = 3;
    for(int i=0; i<m; i++){
        a.push(i);
        b.push(i);
    }
    CPPUNIT_ASSERT(a == b);
    b.push(4);
    CPPUNIT_ASSERT(!(a == b));
    a.push(2);
    CPPUNIT_ASSERT(!(a == b));

}

void VecTest::testComparison(){
    vec<int> a,b;
    CPPUNIT_ASSERT(!(a<b));
    b.push(1);
    CPPUNIT_ASSERT(a<b);
    a.push(0);
    CPPUNIT_ASSERT(a<b);
    a.push(0);
    CPPUNIT_ASSERT(!(a<b));

}
