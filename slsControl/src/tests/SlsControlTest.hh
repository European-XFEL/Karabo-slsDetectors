/*
 * File:   SlsDetectorTest.hh
 * Author: parenti
 *
 * Created on Nov 16, 2017, 4:31:16 PM
 */

#ifndef SLSCONTROLTEST_HH
#define	SLSCONTROLTEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/karabo.hpp>

class SlsControlTest : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(SlsControlTest);

    CPPUNIT_TEST(shouldCreateGotthardControl);
    CPPUNIT_TEST(shouldNotCreateGotthardControl);
    CPPUNIT_TEST(shouldCreateGotthard2Control);
    CPPUNIT_TEST(shouldNotCreateGotthard2Control);
    CPPUNIT_TEST(shouldCreateJungfrauControl);
    CPPUNIT_TEST(shouldNotCreateJungfrauControl);

    CPPUNIT_TEST_SUITE_END();

public:
    SlsControlTest();
    virtual ~SlsControlTest();

private:
    void shouldCreateGotthardControl();
    void shouldNotCreateGotthardControl();
    void shouldCreateGotthard2Control();
    void shouldNotCreateGotthard2Control();
    void shouldCreateJungfrauControl();
    void shouldNotCreateJungfrauControl();

private:
    karabo::util::Hash m_configOk;
    karabo::util::Hash m_configNotOk;
};

#endif	/* SLSCONTROLTEST_HH */

