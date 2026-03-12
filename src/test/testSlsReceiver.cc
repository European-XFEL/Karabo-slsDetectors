/*
 * Author: parenti
 *
 * Created on September 09, 2022, 10:02 AM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include <gtest/gtest.h>

#include <memory>
#include <thread>
#include <utility>

#include "../slsReceiver/SlsReceiver.hh"
#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/util/PluginLoader.hh"


#define DEVICE_SERVER_ID "testDeviceSrvCpp"
#define TEST_DEVICE_ID_0 "testGotthard2Receiver"
#define TEST_DEVICE_ID_1 "testJFReceiver"

#define LOG_PRIORITY "FATAL" // Can also be "DEBUG", "INFO" or "ERROR"

#define DEV_CLI_TIMEOUT_SEC 2


/**
 * @brief Test fixture for the SlsControl device class.
 */
class SlsControlFixture : public testing::Test {
   protected:
    SlsControlFixture() = default;

    void SetUp() {
        m_eventLoopThread = std::thread(&karabo::net::EventLoop::work);

        // Load the library dynamically
        const karabo::data::Hash& pluginConfig = karabo::data::Hash("pluginDirectory", ".");
        karabo::util::PluginLoader::create("PluginLoader", pluginConfig)->update();

        // Instantiate C++ Device Server.
        karabo::data::Hash config("serverId", DEVICE_SERVER_ID, "log.level", LOG_PRIORITY);
        m_deviceSrv = karabo::core::DeviceServer::create("DeviceServer", config);
        m_deviceSrv->finalizeInternalInitialization();
        // Instantiate Device Client.
        m_deviceCli = std::make_shared<karabo::core::DeviceClient>();
    }

    void TearDown() {
        m_deviceCli.reset();
        m_deviceSrv.reset();
        karabo::net::EventLoop::stop();
        m_eventLoopThread.join();
    }

    void instantiateTestDevice0(const karabo::data::Hash& devSpecificCfg) {
        karabo::data::Hash devCfg("deviceId", TEST_DEVICE_ID_0, "rxTcpPort", 1954);
        devCfg.merge(devSpecificCfg);

        std::pair<bool, std::string> success =
              m_deviceCli->instantiate(DEVICE_SERVER_ID, "Gotthard2Receiver", devCfg, DEV_CLI_TIMEOUT_SEC);

        ASSERT_TRUE(success.first) << "Error instantiating '" << TEST_DEVICE_ID_0 << "':\n" << success.second;
    }

    void instantiateTestDevice1(const karabo::data::Hash& devSpecificCfg) {
        karabo::data::Hash devCfg("deviceId", TEST_DEVICE_ID_1, "rxTcpPort", 3954);
        devCfg.merge(devSpecificCfg);

        std::pair<bool, std::string> success =
              m_deviceCli->instantiate(DEVICE_SERVER_ID, "JungfrauReceiver", devCfg, DEV_CLI_TIMEOUT_SEC);

        ASSERT_TRUE(success.first) << "Error instantiating '" << TEST_DEVICE_ID_1 << "':\n" << success.second;
    }

    void deinstantiateTestDevice() {
        ASSERT_NO_THROW(m_deviceCli->killDevice(TEST_DEVICE_ID_0, DEV_CLI_TIMEOUT_SEC))
              << "Failed to deinstantiate device '" << TEST_DEVICE_ID_0 << "'";

        ASSERT_NO_THROW(m_deviceCli->killDevice(TEST_DEVICE_ID_1, DEV_CLI_TIMEOUT_SEC))
              << "Failed to deinstantiate device '" << TEST_DEVICE_ID_1 << "'";
    }

    std::thread m_eventLoopThread;

    karabo::core::DeviceServer::Pointer m_deviceSrv;
    karabo::core::DeviceClient::Pointer m_deviceCli;
};


TEST_F(SlsControlFixture, testSlsReceiver) {
    // TODO: Provide a non-empty config for the device under test.
    instantiateTestDevice0(karabo::data::Hash());
    instantiateTestDevice1(karabo::data::Hash());

    // TODO: Define a test body.

    deinstantiateTestDevice();
}
