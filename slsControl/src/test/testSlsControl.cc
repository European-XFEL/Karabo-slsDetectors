/*
 * Author: parenti
 *
 * Created on September 09, 2022, 10:02 AM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "SlsControl.hh"

#include <boost/shared_ptr.hpp>
#include <gtest/gtest.h>
#include <thread>
#include <utility>

#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/PluginLoader.hh"


#define DEVICE_SERVER_ID "testDeviceSrvCpp"
#define TEST_DEVICE_ID_0 "testGotthard2ControlFail"
#define TEST_DEVICE_ID_1 "testGotthardControlFail"
#define TEST_DEVICE_ID_2 "testJFControlFail"
#define TEST_DEVICE_ID_3 "testGotthard2Control"
#define TEST_DEVICE_ID_4 "testGotthardControl"
#define TEST_DEVICE_ID_5 "testJFControl"

#define LOG_PRIORITY     "FATAL"  // Can also be "DEBUG", "INFO" or "ERROR"

#define DEV_CLI_TIMEOUT_SEC 2


/**
 * @brief Test fixture for the SlsControl device class.
 */
class SlsControlFixture: public testing::Test {
protected:

    SlsControlFixture() = default;

    void SetUp( ) {
        m_eventLoopThread = std::thread(&karabo::net::EventLoop::work);

        // Load the library dynamically
        const karabo::util::Hash& pluginConfig = karabo::util::Hash("pluginDirectory", ".");
        karabo::util::PluginLoader::create("PluginLoader", pluginConfig)->update();

        // Instantiate C++ Device Server.
        karabo::util::Hash config("serverId", DEVICE_SERVER_ID,
                                  "scanPlugins", true,
                                  "Logger.priority", LOG_PRIORITY);
        m_deviceSrv = karabo::core::DeviceServer::create("DeviceServer", config);
        m_deviceSrv->finalizeInternalInitialization();
        // Instantiate Device Client.
        m_deviceCli = boost::make_shared<karabo::core::DeviceClient>();
    }

    void TearDown( ) {
        m_deviceCli.reset();
        m_deviceSrv.reset();
        karabo::net::EventLoop::stop();
        m_eventLoopThread.join();
    }

    void instantiateTestDevice0(const karabo::util::Hash& devSpecificCfg) {
        karabo::util::Hash devCfg("deviceId", TEST_DEVICE_ID_0);
        devCfg.merge(devSpecificCfg);

        std::pair<bool, std::string> success =
            m_deviceCli->instantiate(DEVICE_SERVER_ID, "Gotthard2Control",
                                     devCfg, DEV_CLI_TIMEOUT_SEC);

        ASSERT_FALSE(success.first)
            << "Error instantiating '" << TEST_DEVICE_ID_0 << "':\n"
            << success.second;
    }

    void instantiateTestDevice1(const karabo::util::Hash& devSpecificCfg) {
        karabo::util::Hash devCfg("deviceId", TEST_DEVICE_ID_1);
        devCfg.merge(devSpecificCfg);

        std::pair<bool, std::string> success =
            m_deviceCli->instantiate(DEVICE_SERVER_ID, "GotthardControl",
                                     devCfg, DEV_CLI_TIMEOUT_SEC);

        ASSERT_FALSE(success.first)
            << "Error instantiating '" << TEST_DEVICE_ID_1 << "':\n"
            << success.second;
    }

    void instantiateTestDevice2(const karabo::util::Hash& devSpecificCfg) {
        karabo::util::Hash devCfg("deviceId", TEST_DEVICE_ID_2);
        devCfg.merge(devSpecificCfg);

        std::pair<bool, std::string> success =
            m_deviceCli->instantiate(DEVICE_SERVER_ID, "JungfrauControl",
                                     devCfg, DEV_CLI_TIMEOUT_SEC);

        ASSERT_FALSE(success.first)
            << "Error instantiating '" << TEST_DEVICE_ID_2 << "':\n"
            << success.second;
    }

    void instantiateTestDevice3(const karabo::util::Hash& devSpecificCfg) {
        karabo::util::Hash devCfg("deviceId", TEST_DEVICE_ID_3);
        devCfg.set("detectorHostName", std::vector<std::string>({"blah1", "blah2"}));
        devCfg.set("udpSrcIp", std::vector<std::string>({"1.2.3.4", "2.3.4.5"}));
        devCfg.set("rxHostname", std::vector<std::string>({"1halb", "2halb"}));
        devCfg.set("rxTcpPort", std::vector<unsigned short>({1956, 1957}));
        devCfg.set("udpDstIp", std::vector<std::string>({"4.3.2.1", "5.4.3.2"}));
        devCfg.set("udpDstPort", std::vector<unsigned short>({50002, 50003}));
        devCfg.merge(devSpecificCfg);

        std::pair<bool, std::string> success =
            m_deviceCli->instantiate(DEVICE_SERVER_ID, "Gotthard2Control",
                                     devCfg, DEV_CLI_TIMEOUT_SEC);

        ASSERT_TRUE(success.first)
            << "Error instantiating '" << TEST_DEVICE_ID_3 << "':\n"
            << success.second;
    }

    void instantiateTestDevice4(const karabo::util::Hash& devSpecificCfg) {
        karabo::util::Hash devCfg("deviceId", TEST_DEVICE_ID_4);
        devCfg.set("detectorHostName", std::vector<std::string>({"blah1", "blah2"}));
        devCfg.set("udpSrcIp", std::vector<std::string>({"1.2.3.4", "2.3.4.5"}));
        devCfg.set("rxHostname", std::vector<std::string>({"1halb", "2halb"}));
        devCfg.set("rxTcpPort", std::vector<unsigned short>({1956, 1957}));
        devCfg.set("udpDstIp", std::vector<std::string>({"4.3.2.1", "5.4.3.2"}));
        devCfg.set("udpDstPort", std::vector<unsigned short>({50002, 50003}));
        devCfg.merge(devSpecificCfg);

        std::pair<bool, std::string> success =
            m_deviceCli->instantiate(DEVICE_SERVER_ID, "GotthardControl",
                                     devCfg, DEV_CLI_TIMEOUT_SEC);

        ASSERT_TRUE(success.first)
            << "Error instantiating '" << TEST_DEVICE_ID_4 << "':\n"
            << success.second;
    }

    void instantiateTestDevice5(const karabo::util::Hash& devSpecificCfg) {
        karabo::util::Hash devCfg("deviceId", TEST_DEVICE_ID_5);
        devCfg.set("detectorHostName", std::vector<std::string>({"blah1", "blah2"}));
        devCfg.set("udpSrcIp", std::vector<std::string>({"1.2.3.4", "2.3.4.5"}));
        devCfg.set("rxHostname", std::vector<std::string>({"1halb", "2halb"}));
        devCfg.set("rxTcpPort", std::vector<unsigned short>({1956, 1957}));
        devCfg.set("udpDstIp", std::vector<std::string>({"4.3.2.1", "5.4.3.2"}));
        devCfg.set("udpDstPort", std::vector<unsigned short>({50002, 50003}));
        devCfg.merge(devSpecificCfg);

        std::pair<bool, std::string> success =
            m_deviceCli->instantiate(DEVICE_SERVER_ID, "JungfrauControl",
                                     devCfg, DEV_CLI_TIMEOUT_SEC);

        ASSERT_TRUE(success.first)
            << "Error instantiating '" << TEST_DEVICE_ID_5 << "':\n"
            << success.second;
    }

    void deinstantiateTestDevice() {
        ASSERT_NO_THROW(
            m_deviceCli->killDevice(TEST_DEVICE_ID_3, DEV_CLI_TIMEOUT_SEC))
        << "Failed to deinstantiate device '" << TEST_DEVICE_ID_3 << "'";

        ASSERT_NO_THROW(
            m_deviceCli->killDevice(TEST_DEVICE_ID_4, DEV_CLI_TIMEOUT_SEC))
        << "Failed to deinstantiate device '" << TEST_DEVICE_ID_4 << "'";

        ASSERT_NO_THROW(
            m_deviceCli->killDevice(TEST_DEVICE_ID_5, DEV_CLI_TIMEOUT_SEC))
        << "Failed to deinstantiate device '" << TEST_DEVICE_ID_5 << "'";
    }

    std::thread m_eventLoopThread;

    karabo::core::DeviceServer::Pointer m_deviceSrv;
    karabo::core::DeviceClient::Pointer m_deviceCli;
};


// TODO: Give the test case a proper name (not "testScaffold")
TEST_F(SlsControlFixture, testScaffold){

    // TODO: Provide a non-empty config for the device under test.
    instantiateTestDevice0(karabo::util::Hash());
    instantiateTestDevice1(karabo::util::Hash());
    instantiateTestDevice2(karabo::util::Hash());
    instantiateTestDevice3(karabo::util::Hash());
    instantiateTestDevice4(karabo::util::Hash());
    instantiateTestDevice5(karabo::util::Hash());

    // TODO: Define a test body.

    deinstantiateTestDevice();
}
