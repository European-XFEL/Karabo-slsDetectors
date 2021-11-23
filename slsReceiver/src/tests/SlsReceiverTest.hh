/*
 * File:   SlsReceiverTest.hh
 * Author: parenti
 *
 * Created on Nov 21, 2017, 9:37:15 AM
 */

#ifndef SLSRECEIVERTEST_HH
#define	SLSRECEIVERTEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/karabo.hpp>

class SlsReceiverTest : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(SlsReceiverTest);

    CPPUNIT_TEST(shouldCreateGotthardReceiver);
    CPPUNIT_TEST(shouldCreateJungfrauReceiver);

    CPPUNIT_TEST_SUITE_END();

public:
    SlsReceiverTest();
    virtual ~SlsReceiverTest();

private:
    void shouldCreateGotthardReceiver();
    void shouldCreateJungfrauReceiver();

private:
    karabo::util::Hash m_config;
};

#endif	/* SLSRECEIVERTEST_HH */

