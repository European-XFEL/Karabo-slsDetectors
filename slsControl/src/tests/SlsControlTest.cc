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
    m_configOk.set("detectorHostName", std::vector<std::string>({"blah1", "blah2"}));
    m_configOk.set("udpSrcIp", std::vector<std::string>({"1.2.3.4", "2.3.4.5"}));
    m_configOk.set("rxHostname", std::vector<std::string>({"1halb", "2halb"}));
    m_configOk.set("rxTcpPort", std::vector<unsigned short>({1956, 1957}));
    m_configOk.set("udpDstIp", std::vector<std::string>({"4.3.2.1", "5.4.3.2"}));
    m_configOk.set("udpDstPort", std::vector<unsigned short>({50002, 50003}));

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
