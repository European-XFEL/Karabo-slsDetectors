/*
 * File:   SlsDetectorTest.cc
 * Author: parenti
 *
 * Created on Nov 16, 2017, 4:31:16 PM
 */

#include "SlsControlTest.hh"

using namespace std;

USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(SlsControlTest);

SlsControlTest::SlsControlTest() {
    m_configOk.set("deviceId", "testdevice");
    m_configOk.set("detectorHostName", "blah");
    m_configOk.set("detectorIp", "1.2.3.4");
    m_configOk.set("rxHostname", "halb");
    m_configOk.set("rxUdpIp", "4.3.2.1");

    m_configNotOk.set("deviceId", "testdevice");
}

SlsControlTest::~SlsControlTest() {
}

void SlsControlTest::setUp() {
}

void SlsControlTest::tearDown() {
}

void SlsControlTest::shouldCreateGotthardControl() {
    BaseDevice::Pointer device = BaseDevice::create("GotthardControl", m_configOk);

    CPPUNIT_ASSERT_EQUAL(string("GotthardControl"), (device->getClassInfo()).getClassName());
}

void SlsControlTest::shouldNotCreateGotthardControl() {
    // Must fail, as mandatory parameters are not provided
    CPPUNIT_ASSERT_THROW(BaseDevice::Pointer device = BaseDevice::create("GotthardControl", m_configNotOk),
            karabo::util::ParameterException);
}

void SlsControlTest::shouldCreateJungfrauControl() {
    BaseDevice::Pointer device = BaseDevice::create("JungfrauControl", m_configOk);

    CPPUNIT_ASSERT_EQUAL(string("JungfrauControl"), (device->getClassInfo()).getClassName());
}

void SlsControlTest::shouldNotCreateJungfrauControl() {
    // Must fail, as mandatory parameters are not provided
    CPPUNIT_ASSERT_THROW(BaseDevice::Pointer device = BaseDevice::create("JungfrauControl", m_configNotOk),
            karabo::util::ParameterException);
}
