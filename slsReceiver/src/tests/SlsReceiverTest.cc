/*
 * File:   SlsReceiverTest.cc
 * Author: parenti
 *
 * Created on Nov 21, 2017, 9:37:15 AM
 */

#include "SlsReceiverTest.hh"
#include <karabo/karabo.hpp>

using namespace std;

USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(SlsReceiverTest);

SlsReceiverTest::SlsReceiverTest() {
    m_config.set("deviceId", "testdevice");
}

SlsReceiverTest::~SlsReceiverTest() {
}

void SlsReceiverTest::setUp() {
}

void SlsReceiverTest::tearDown() {
}

void SlsReceiverTest::shouldCreateGotthardReceiver() {
    BaseDevice::Pointer device = BaseDevice::create("GotthardReceiver", m_config);

    CPPUNIT_ASSERT_EQUAL(string("GotthardReceiver"), (device->getClassInfo()).getClassName());
}

void SlsReceiverTest::shouldCreateJungfrauReceiver() {
    BaseDevice::Pointer device = BaseDevice::create("JungfrauReceiver", m_config);

    CPPUNIT_ASSERT_EQUAL(string("JungfrauReceiver"), (device->getClassInfo()).getClassName());
}
