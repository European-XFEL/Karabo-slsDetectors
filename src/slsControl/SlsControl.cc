/*
 *
 * Author: <Iryna.Kozlova@xfel.eu>
 *
 * Created on July 12, 2013, 11:30 AM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

#include "SlsControl.hh"

USING_KARABO_NAMESPACES
namespace fs = boost::filesystem;

namespace karabo {

    SlsControl::SlsControl(const Hash& config) : Device<>(config),
            m_numberOfModules(0), m_connect(false), m_connect_timer(EventLoop::getIOService()),
            m_isConfigured(false), m_firstPoll(true), m_poll(false),
            m_poll_timer(EventLoop::getIOService()),
            m_acquire_timer(EventLoop::getIOService()) {
        KARABO_INITIAL_FUNCTION(initialize);

        KARABO_SLOT(start);
        KARABO_SLOT(stop);
        KARABO_SLOT(reset);

        this->createTmpDir(); // Create temporary directory
    }

    SlsControl::~SlsControl() {
        // Stop deadline timers
        m_connect = false;
        m_connect_timer.cancel();
        this->stopPoll();

        try {
            bool success = true;
            std::string baseName("/dev/shm/slsDetectorPackage_multi_" + karabo::util::toString(m_shm_id));
            success &= fs::remove(baseName);
            for (size_t i = 0; i < m_numberOfModules; ++i) {
                success &= fs::remove(baseName + "_sls_" + karabo::util::toString(i));
            }
            if (success) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Deleted shared memory segment " << m_shm_id;
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Could not remove shared memory segment " << m_shm_id;
            }

            // Remove temporary directory and its content
            success = fs::remove_all(m_tmpDir);
            if (success) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Removed temporary dir " << m_tmpDir;
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Could not remove temporary dir " << m_tmpDir;
            }
        } catch (const std::exception& e) {
            KARABO_LOG_FRAMEWORK_ERROR << "Exception in ~SlsControl: " << e.what();
        }

    }

    void SlsControl::expectedParameters(Schema& expected) {

#ifdef SLS_SIMULATION
        BOOL_ELEMENT(expected).key("setDetOffline")
                .displayedName("Set Detector Offline")
                .description("Simulate the fact that the detector is offline")
                .assignmentOptional().defaultValue(false)
                .reconfigurable()
                .expertAccess()
                .commit();
#endif

        OVERWRITE_ELEMENT(expected).key("state")
                .setNewOptions(State::UNKNOWN, State::INIT, State::ERROR, State::ON, State::DISABLED, State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected).key("start")
                .displayedName("Start")
                .description("Starts acquisition")
                .allowedStates(State::ON)
                .commit();

        SLOT_ELEMENT(expected).key("stop")
                .displayedName("Stop")
                .description("Stops acquisition")
                .allowedStates(State::ACQUIRING)
                .commit();

        SLOT_ELEMENT(expected).key("reset")
                .displayedName("Reset")
                .description("Resets the device in case of an error")
                .allowedStates(State::ERROR)
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("detectorHostName")
                .alias("hostname")
                // No "sls" tag... will be processed differently
                .displayedName("Detector Hostname")
                .description("The hostnames (or IP addresses) of all modules.")
                .assignmentMandatory()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("udpSrcIp")
                .alias("udp_srcip") // was "detectorip"
                // No "sls" tag... will be processed differently
                .displayedName("Detector UDP/IP")
                .description("IP address of the detector (source) UDP interface. "
                "Must be in the same subnet as the destination UDP/IP. "
                "Used to be named \"detectorIp\".")
                .assignmentMandatory()
                .commit();

        // Arbitrary, but must be not in use in the same subnet
        VECTOR_STRING_ELEMENT(expected).key("udpSrcMac")
                .alias("udp_srcmac") // was "detectormac"
                .tags("sls")
                .displayedName("Detector UDP MAC")
                .description("MAC address of the detector (source) UDP interface. "
                "Arbitrary (e.g. 00:aa:bb:cc:dd:ee), but must be not in use in the same subnet. "
                "Used to be named \"detectorMac\"")
                .assignmentOptional().defaultValue({})
                .commit();

        VECTOR_UINT16_ELEMENT(expected).key("detectorHostPort")
                .alias("port")
                .tags("sls")
                .displayedName("detectorHostPort")
                .description("Detector Host Port. Will use 1952 if left empty.")
                .assignmentOptional().defaultValue({})
                .commit();

        VECTOR_UINT16_ELEMENT(expected).key("detectorHostStopPort")
                .alias("stopport")
                .tags("sls")
                .displayedName("detectorHostStopPort")
                .description("Detector Host Stop Port. Will use 1953 if left empty.")
                .assignmentOptional().noDefaultValue()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("rxHostname")
                .alias("rx_hostname")
                // No "sls" tag... will be processed differently
                .displayedName("RX Hostname")
                .description("The hostname or IP of the host where the receiver device is running. "
                "It is used for TCP communication between control and receiver, to configure the receiver. "
                "It updates the receiver with detector parameters. It also resets any prior receiver "
                "property (not on detector).")
                .assignmentMandatory()
                .commit();

        VECTOR_UINT16_ELEMENT(expected).key("rxTcpPort")
                .alias("rx_tcpport")
                // No "sls" tag... will be processed differently
                .displayedName("RX TCP Port")
                .description("The TCP port for client-receiver communication. Default is 1954. "
                "Must be different if multiple receivers run on the same host.")
                .assignmentMandatory()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("udpDstIp")
                .alias("udp_dstip") // was rx_udpip
                // No "sls" tag... will be processed differently
                .displayedName("RX UDP/IP")
                .description("The IP address of the network interface on the receiver's host, "
                "which is receiving data from the detector. "
                "Used to be named \"rxUdpIp\"")
                .assignmentMandatory()
                .commit();

        VECTOR_UINT16_ELEMENT(expected).key("udpDstPort")
                .alias("udp_dstport") // was "rx_udpport"
                // No "sls" tag... will be processed differently
                .displayedName("RX UDP/IP Port")
                .description("Port number of the receiver (destination) UDP interface. Default is 50001. "
                "Used to be named \"rxUdpPort\"")
                .assignmentMandatory()
                .commit();

        STRING_ELEMENT(expected).key("settings")
                .alias("settings")
                .tags("sls")
                .displayedName("Settings")
                .description("Detector settings.")
                .assignmentOptional().defaultValue("dynamicgain") // OVERWRITE in derived class
                .options({"dynamicgain", "lowgain", "mediumgain", "highgain", "veryhighgain"}) // OVERWRITE in derived class
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        // [AP] settingspath & fformat are internally set and not visible to the user

        NODE_ELEMENT(expected).key("dataStorage")
                .displayedName("Data Storage")
                .commit();

        std::vector<unsigned char> dataStorageEnableOptions = {0u, 1u};
        UINT8_ELEMENT(expected).key("dataStorage.enable")
                .alias("fwrite") // was "enablefwrite"
                .tags("sls")
                .displayedName("Enable")
                .description("Enable file write on the receiver host.")
                .assignmentOptional().defaultValue(0)
                .options(dataStorageEnableOptions)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        STRING_ELEMENT(expected).key("dataStorage.filePath")
                .alias("fpath") // was "outdir"
                .tags("sls")
                .displayedName("File Path")
                .description("The path (on receiver) for saving data to file.")
                .assignmentOptional().defaultValue("/tmp")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        STRING_ELEMENT(expected).key("dataStorage.fileName")
                .alias("fname")
                .tags("sls")
                .displayedName("File Name")
                .description("The name prefix for saving data to file.")
                .assignmentOptional().defaultValue("run")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT32_ELEMENT(expected).key("dataStorage.fileIndex")
                .alias("findex") // was "index"
                .tags("sls")
                .displayedName("File Start Index")
                .description("The starting index for saving data to file. It "
                "will be automatically updated after each acquisition.")
                .assignmentOptional().defaultValue(0)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        VECTOR_INT16_ELEMENT(expected).key("lock")
                .alias("lock")
                .tags("sls")
                .displayedName("Lock")
                .description("Lock the detector to one IP.")
                .assignmentOptional().defaultValue({0})
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        UINT32_ELEMENT(expected).key("highVoltageMax")
                .description("Max value allowed for 'highVoltage'. Higher values will be rejected by the device. "
                " Used to be named \"vHighVoltageMax\".")
                .assignmentOptional().defaultValue(200)
                .unit(Unit::VOLT)
                .reconfigurable()
                .adminAccess()
                .commit();

        VECTOR_UINT32_ELEMENT(expected).key("highVoltage")
                .alias("highvoltage") // was "vhighvoltage"
                .tags("sls")
                .displayedName("High Voltage")
                .description("High voltage to the sensor. "
                " Used to be named \"vHighVoltage\".")
                .assignmentOptional().defaultValue({90})
                .unit(Unit::VOLT)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT32_ELEMENT(expected).key("dynamicRange")
                .alias("dr")
                .tags("sls")
                .displayedName("Dynamic Range")
                .description("Dymanic range. Used to be named \"bitDepth\".")
                .assignmentOptional().defaultValue(16)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        FLOAT_ELEMENT(expected).key("exposureTime")
                .alias("exptime")
                .tags("sls")
                .displayedName("Exposure Time")
                .description("The exposure time.")
                .assignmentOptional().defaultValue(1.e-5) // 10 us
                // .minExc(0.) // Set in the derived classes
                .unit(Unit::SECOND)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        FLOAT_ELEMENT(expected).key("exposurePeriod")
                .alias("period")
                .tags("sls")
                .displayedName("Exposure Period")
                .description("The period between frames.")
                .assignmentOptional().defaultValue(0.1) // 100 ms
                // .minExc(0.) // Set in the derived classes
                .unit(Unit::SECOND)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        VECTOR_FLOAT_ELEMENT(expected).key("delayAfterTrigger")
                .alias("delay")
                .tags("sls")
                .displayedName("Delay After Trigger")
                .description("The delay after trigger.")
                .assignmentOptional().defaultValue({0.})
                .unit(Unit::SECOND)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT64_ELEMENT(expected).key("numberOfFrames")
                .alias("frames")
                .tags("sls")
                .displayedName("Number of Frames")
                .description("Number of frames per acquisition. In trigger mode, number of frames per trigger.")
                .assignmentOptional().defaultValue(1)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT64_ELEMENT(expected).key("numberOfTriggers")
                .alias("triggers")
                .tags("sls")
                .displayedName("Number of Triggers")
                .description("Number of triggers per acquisition. Used to be named \"numberOfCycles\".")
                .assignmentOptional().defaultValue(1)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

// Mythen3 only
//        INT64_ELEMENT(expected).key("numberOfGates")
//                .alias("gates")
//                .tags("sls")
//                .displayedName("Number of Gates")
//                .description("Number Of Gates")
//                .assignmentOptional().defaultValue(0)
//                .reconfigurable()
//                .allowedStates(State::ON)
//                .commit();

        STRING_ELEMENT(expected).key("timing")
                .alias("timing")
                .tags("sls")
                .displayedName("Timing Mode")
                .description("The timing mode of the detector.")
                .assignmentOptional().defaultValue("auto")
                .options({"auto", "gating", "trigger", "ro_trigger", "triggered_gating"}) // OVERWRITE in derived class
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        FLOAT_ELEMENT(expected).key("acquisitionTime")
                .displayedName("Acquisition Time")
                .description("Acquisition time. Depending on trigger mode, it will set different parameters on the"
                "detector in order to have the acquisition running for the given time.")
                .assignmentOptional().noDefaultValue()
                .unit(Unit::SECOND)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        FLOAT_ELEMENT(expected).key("triggerPeriod")
                .displayedName("Ext. Trigger Period")
                .description("External trigger period. Used together with acquisitionTime to setup detector acquisition"
                " parameters.")
                .assignmentOptional().defaultValue(0.100)
                .unit(Unit::SECOND)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        // Read-only properties

        const std::vector<std::string> interfaces = {"Trigger"};
        VECTOR_STRING_ELEMENT(expected).key("interfaces")
                .expertAccess()
                .readOnly().initialValue(interfaces)
                .commit();

        STRING_ELEMENT(expected).key("clientVersion")
                .displayedName("SLS Library Version")
                .description("SLS software version, in the format [0xYYMMDD]. "
                "Used to be named \"thisVersion\".")
                .readOnly()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("firmwareVersion")
                .displayedName("Firmware Version")
                .displayedName("Firmware Version")
                .description("Fimware version of the detectors, in the format [0xYYMMDD]"
                //" or an increasing 2 digit number for Eiger"
                ". Used to be named \"detectorVersion\".")
                .readOnly()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("detServerVersion")
                .displayedName("Detector Server Version")
                .description("On-board detector server software versions, "
                "in the format [0xYYMMDD]. "
                "Used to be named \"softwareVersion\".")
                .readOnly()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("serialNumber")
                .displayedName("Serial Number")
                .description("Serial number of the detectors. "
                "Used to be named \"detectorNumber\".")
                .readOnly()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("receiverVersion")
                .displayedName("Receiver Version")
                .description("Receiver versions, in the format [0xYYMMDD].")
                .readOnly()
                .commit();

        UINT32_ELEMENT(expected).key("pollingInterval")
                .displayedName("Polling Interval")
                .description("The interval for polling the detector module for status. This "
                "interval also determines the maximum time needed to indicate that a detector "
                "module is offline or not reachable anymore.")
                .assignmentOptional().defaultValue(10)
                .unit(Unit::SECOND)
                .minInc(2)
                .maxInc(600)
                .reconfigurable()
                .commit();

    }

    void SlsControl::start() {
        this->updateState(State::ACQUIRING);
        m_acquire_timer.expires_from_now(boost::posix_time::milliseconds(0));
        m_acquire_timer.async_wait(karabo::util::bind_weak(&SlsControl::acquireBlocking, this, boost::asio::placeholders::error));
    }

    void SlsControl::stop() {
        KARABO_LOG_FRAMEWORK_DEBUG << "In stop";

        m_SLS->stopDetector();
        KARABO_LOG_INFO << "Acquisition stopped";
        this->set("status", "Acquisition stopped");

        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting stop";
        this->updateState(State::ON);
    }

    void SlsControl::reset() {
        if (this->areReceiversOnline()) {
            try {
                this->updateState(State::INIT);
                KARABO_LOG_INFO << "Connected to detector(s)";
                this->set("status", "Connected to detector(s)");

                this->sendBaseConfiguration();
                this->sendInitialConfiguration();
                const Hash& config = this->getCurrentConfiguration();
                this->configureDetectorSpecific(config);

                this->updateState(State::ON);
            } catch (const std::exception& e) {
                const std::string msg = std::string("Exception in 'reset': ") + e.what();
                this->updateState(State::ERROR, Hash("status", msg));
                KARABO_LOG_FRAMEWORK_ERROR << msg;
            }
        }
    }

    void SlsControl::initialize() {
        try {
            m_numberOfModules = this->get<std::vector<std::string> >("detectorHostName").size();
            m_positions.resize(m_numberOfModules);
            for (size_t i = 0; i < m_numberOfModules; ++i) {
                m_positions[i] = i;
            }
        } catch (const karabo::util::Exception &e) {
            KARABO_LOG_FRAMEWORK_ERROR << "Exception in initialize: " << e;
            m_numberOfModules = 0;
        }

        // TODO this code should be moved to connect, so that it can be retried
        KARABO_LOG_FRAMEWORK_DEBUG << "Creating m_SLS...";
        std::hash<std::string> hash_fn;
        m_shm_id = (0x7FFFFFFF & hash_fn(m_tmpDir)); // Make unique index from UUID
        KARABO_LOG_FRAMEWORK_INFO << "Shared memory segment index: " << m_shm_id;
        try {
            std::shared_ptr<sls::Detector> detector(new sls::Detector(m_shm_id));
            m_SLS = std::move(detector);
        } catch (const std::exception& e) {
            KARABO_LOG_FRAMEWORK_DEBUG << "    failed!";
            this->set("status", "Failed to create sls::Detector");
            return;
        }
        KARABO_LOG_FRAMEWORK_DEBUG << "    done!";

        m_connect = true;
        m_connect_timer.expires_from_now(boost::posix_time::milliseconds(0));
        m_connect_timer.async_wait(karabo::util::bind_weak(&SlsControl::connect, this, boost::asio::placeholders::error));
    }

    void SlsControl::connect(const boost::system::error_code& ec) {
        KARABO_LOG_FRAMEWORK_DEBUG << "In connect";
        if (ec) {
            return;
        }

        bool detectorOnline, receiverOnline;

        detectorOnline = this->areDetectorsOnline();
        if (detectorOnline) {
            // Only check receiver if detector is online
            receiverOnline = this->areReceiversOnline();
        } else {
            receiverOnline = true;
        }

        if (detectorOnline) {
            if (receiverOnline) {
                KARABO_LOG_INFO << "Connected to detector(s)";
                this->set("status", "Connected to detector(s)");
                try {
                    this->updateState(State::INIT);

                    this->sendBaseConfiguration();
                    this->sendInitialConfiguration();
                    const Hash& config = this->getCurrentConfiguration();
                    this->configureDetectorSpecific(config);
#ifdef SLS_SIMULATION
                    m_SLS->setDetectorType(m_detectorType);
#endif
                    this->updateState(State::ON);
                } catch (const std::exception& e) {
                    const std::string msg = std::string("Exception in 'connect': ") + e.what();
                    this->updateState(State::ERROR, Hash("status", msg));
                    KARABO_LOG_FRAMEWORK_ERROR << msg;
                }
            } else { // !receiverOnline
                this->updateState(State::ERROR);
            }

            this->startPoll();
        } else if (m_connect) {
            if (this->getState() != State::UNKNOWN) this->updateState(State::UNKNOWN);
            m_connect_timer.expires_at(m_connect_timer.expires_at() + boost::posix_time::milliseconds(m_reconnectTime));
            m_connect_timer.async_wait(karabo::util::bind_weak(&SlsControl::connect, this, boost::asio::placeholders::error));
        } else {
            return;
        }
    }

    void SlsControl::acquireBlocking(const boost::system::error_code& ec) {
        KARABO_LOG_FRAMEWORK_DEBUG << "In acquireBlocking";
        if (ec) {
            return;
        }

        this->set("status", "Acquisition started");
        // The following is a blocking function: it will only return when the
        // acquisition is over!
        // If the detector is powered off during an acquisition, this thread
        // will never return!
        m_SLS->acquire();
        this->set("status", "Acquisition finished (or stopped)");

        this->updateState(State::ON);
        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting acquireBlocking";
    }

    void SlsControl::startPoll() {
        m_poll = true;
        m_poll_timer.expires_from_now(boost::posix_time::seconds(1));
        m_poll_timer.async_wait(karabo::util::bind_weak(&SlsControl::pollHardware, this, boost::asio::placeholders::error));
    }

    void SlsControl::stopPoll() {
        m_poll = false;
        m_poll_timer.cancel();
    }

    void SlsControl::pollHardware(const boost::system::error_code& ec) {
        KARABO_LOG_FRAMEWORK_DEBUG << "In pollHardware";
        if (ec) {
            return;
        }

        if (!this->areDetectorsOnline()) {
            // stops polling and tries to reconnect
            this->updateState(State::UNKNOWN);
            m_firstPoll = true;
            m_connect = true;
            m_connect_timer.expires_from_now(boost::posix_time::milliseconds(0));
            m_connect_timer.async_wait(karabo::util::bind_weak(&SlsControl::connect, this, boost::asio::placeholders::error));
            return;
        }

        if (!this->areReceiversOnline() && this->getState() != State::ERROR) {
            this->updateState(State::ERROR);
        }

        if (m_isConfigured) {
            // Can only poll after the base cfg (i.e. host) has been applied
            Hash h;

            if (m_firstPoll) {
                this->pollOnce(h);

                // Poll only once
                m_firstPoll = false;
            }

            // Poll detector specific parameters
            this->pollDetectorSpecific(h);

            if (!h.empty()) {
                this->set(h); // bulk set
            }
        }

        if (m_poll) {
            m_poll_timer.expires_at(m_poll_timer.expires_at() + boost::posix_time::seconds(this->get<unsigned int>("pollingInterval")));
            m_poll_timer.async_wait(karabo::util::bind_weak(&SlsControl::pollHardware, this, boost::asio::placeholders::error));
            return;
        }

    }


    void SlsControl::pollOnce(karabo::util::Hash& h) {
        std::stringstream ss;
        ss << std::hex << std::showbase << m_SLS->getClientVersion();
        h.set("clientVersion", ss.str());

        std::vector<std::string> version;
        for (const int64_t& v : m_SLS->getFirmwareVersion(m_positions)) {
            ss.str(""); // clear content
            ss << std::hex << std::showbase << v;
            version.push_back(ss.str());
        }
        h.set("firmwareVersion", version);

        h.set<std::vector<std::string>>("detServerVersion", m_SLS->getDetectorServerVersion(m_positions));

        version.clear(); // clear content
        for (const int64_t& v : m_SLS->getSerialNumber(m_positions)) {
            ss.str(""); // clear content
            ss << std::hex << std::showbase << v;
            version.push_back(ss.str());
        }
        h.set("serialNumber", version);

        h.set<std::vector<std::string>>("receiverVersion", m_SLS->getReceiverVersion(m_positions));
    }

    void SlsControl::sendBaseConfiguration() {
        // Get detector and receiver information
        const auto hostnames = this->get<std::vector<std::string> >("detectorHostName");
        const auto udp_src_ips = this->get<std::vector<std::string> >("udpSrcIp");

        const auto rx_hostnames = this->get<std::vector<std::string> > ("rxHostname");
        const auto rx_tcpports = this->get<std::vector<unsigned short> >("rxTcpPort");
        const auto udp_dst_ips = this->get<std::vector<std::string> >("udpDstIp");
        const auto udp_dst_ports = this->get<std::vector<unsigned short> >("udpDstPort");

        // Check that all vectors have the same size
        if (hostnames.size() != m_numberOfModules) {
            throw KARABO_PARAMETER_EXCEPTION(std::string("detectorHostName has wrong size: " + toString(hostnames.size()) + "!= " + toString(m_numberOfModules)));
        } else if (udp_src_ips.size() != m_numberOfModules) {
            throw KARABO_PARAMETER_EXCEPTION(std::string("udpSrcIp has wrong size: " + toString(udp_src_ips.size()) + "!= " + toString(m_numberOfModules)));
        } else if (rx_hostnames.size() != m_numberOfModules) {
            throw KARABO_PARAMETER_EXCEPTION(std::string("rxHostname has wrong size: " + toString(rx_hostnames.size()) + "!= " + toString(m_numberOfModules)));
        } else if (rx_tcpports.size() != m_numberOfModules) {
            throw KARABO_PARAMETER_EXCEPTION(std::string("rxTcpPort has wrong size: " + toString(rx_tcpports.size()) + "!= " + toString(m_numberOfModules)));
        } else if (udp_dst_ips.size() != m_numberOfModules) {
            throw KARABO_PARAMETER_EXCEPTION(std::string("udpDstIp has wrong size: " + toString(udp_dst_ips.size()) + "!= " + toString(m_numberOfModules)));
        } else if (udp_dst_ports.size() != m_numberOfModules) {
            throw KARABO_PARAMETER_EXCEPTION(std::string("udpDstPort has wrong size: " + toString(udp_dst_ports.size()) + "!= " + toString(m_numberOfModules)));
        }

        const std::string fname = m_tmpDir + "/base.config";
        std::ofstream configFile(fname.c_str(), std::ofstream::trunc);
        if (configFile.is_open() && !m_isConfigured) {
            configFile << "hostname ";
            for (auto hostname : hostnames) {
                configFile << hostname << "+";
            }
            configFile << std::endl;

            for (size_t i = 0; i < m_numberOfModules; ++i) {
                // Please note: order of the parameter matters!
                configFile << i << ":udp_dstport " << udp_dst_ports[i] << std::endl;
                configFile << i << ":rx_tcpport " << rx_tcpports[i] << std::endl;
                configFile << i << ":udp_srcip " << udp_src_ips[i] << std::endl;
                configFile << i << ":udp_dstip " << udp_dst_ips[i] << std::endl;
                configFile << i << ":rx_hostname " << rx_hostnames[i] << std::endl;
            }
            configFile.close();

            m_SLS->loadConfig(fname); // This will also free SLS shared memory

            this->powerOn();

            m_isConfigured = true;
        } else if (!configFile.is_open()) {
            throw KARABO_RECONFIGURE_EXCEPTION("Could not open file " + fname + "for writing");
        }
    }

    void SlsControl::sendInitialConfiguration() {
        // Get current configuration
        const Hash configHash = this->getCurrentConfiguration("sls");

       // Send some more parameters, not from configuration hash
        this->sendConfiguration("settingspath", m_tmpDir); // settings directory
        this->sendConfiguration("fformat", "binary"); // file format

        // Send all other parameters, from configuration hash
        this->sendConfiguration(configHash);

        KARABO_LOG_FRAMEWORK_DEBUG << "Configuration done";
        this->set("status", "Configuration done");

        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting SlsControl::sendInitialConfiguration";
    }

    void SlsControl::sendConfiguration(const karabo::util::Hash& configHash) {
        KARABO_LOG_FRAMEWORK_DEBUG << "Entering SlsControl::sendConfiguration";

        // Check that detector and receiver are online
        const State& state = this->getState();
        if (state == State::UNKNOWN || state == State::ERROR) {
            KARABO_LOG_ERROR << "sendConfiguration(): detector or receiver is not online. Aborting!";
            return;
        }

        Hash flat;
        Hash::flatten(configHash, flat);

        const Schema& fullSchema = this->getFullSchema();
        std::string key, alias;
        for (auto it = flat.begin(); it != flat.end(); ++it) {

            key = it->getKey();
            if (fullSchema.isAccessReadOnly(key)) {
                KARABO_LOG_FRAMEWORK_DEBUG << "SlsControl::sendConfiguration - key: " << key << " is Read-Only -> Skip";
                continue;
            }
            alias = this->getAliasFromKey<std::string >(key);
            const Types::ReferenceType type = configHash.getType(key);

            KARABO_LOG_FRAMEWORK_DEBUG << "SlsControl::sendConfiguration - Key: " << key << " Type: " << type;

            if (Types::isSimple(type)) {
                if (type == Types::FLOAT || type == Types::DOUBLE) {
                    // We have to convert the value to fixed floating-point notation
                    std::stringstream ss;
                    ss.precision(9);
                    ss << std::fixed << configHash.getAs<double>(key);
                    this->sendConfiguration(alias, ss.str());
                } else {
                    const std::string value = configHash.getAs<std::string>(key);
                    this->sendConfiguration(alias, value);
                }

            } else if (Types::isVector(type)) {
                // XXX Here we possibly have to use std::to_string for
                // vectors of floate/doubles -> see simple types
                const auto values = configHash.getAs<std::string, std::vector>(key);
                if (values.size() == 0) {
                    continue; // ignore key
                } else if (values.size() == 1) {
                    // send same value to all
                    this->sendConfiguration(alias, values[0]);
                } else if (values.size() == m_numberOfModules) {
                    for (size_t i = 0; i < values.size(); ++i) {
                        this->sendConfiguration(alias, values[i], i);
                    }
                } else {
                    KARABO_LOG_ERROR << "SlsControl::sendConfiguration error: " << key << " has "
                            << values.size() << " elements but modules are " << m_numberOfModules;
                    continue;
                }
            } else {
                KARABO_LOG_WARN << "SlsControl::sendConfiguration received parameter " << key
                        << " is neither simple nor vector";
                continue;
            }
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::sendConfiguration";
    }

    // Return true if a TCP server is running on host:port
    bool SlsControl::isServerOnline(const std::string& host, unsigned short port, std::string& errorMsg) {
        KARABO_LOG_FRAMEWORK_DEBUG << "Entering SlsControl::isServerOnline";

        if (!this->ping(host)) {
            errorMsg = host + " is not pingable";
            return false;
        }
        else {
            errorMsg = "";
            return true;
        }

        bool online;
        const std::string portString = std::to_string(port);

        try {
            boost::asio::ip::tcp::iostream s;

            // The entire sequence of I/O operations must complete within 5 seconds.
            // If an expiry occurs, the socket is automatically closed and the stream
            // becomes bad.
            s.expires_after(std::chrono::duration<int>(5));

            // Establish a connection to the server.
            s.connect(host, portString);

            if (s) {
                errorMsg = "";
                online = true;
            } else {
                errorMsg = host + ":" + portString + " is offline";
                online = false;
            }
        } catch (const std::exception& e) {
            // Check failed
            errorMsg = "Cannot connect to " + host + ":" + portString + ". " + e.what();
            online = false;
        }

        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting SlsControl::isServerOnline";
        return online;
    }

    bool SlsControl::areDetectorsOnline() {
        KARABO_LOG_FRAMEWORK_DEBUG << "Entering SlsControl::areDetectorsOnline";

        const auto hosts = this->get<std::vector<std::string> >("detectorHostName");
        std::vector<unsigned short> ports;
        try {
            ports = this->get<std::vector<unsigned short> >("detectorHostPort");
            ports.resize(hosts.size(), m_defaultPort); // If size mismatch: extend with default / truncate (as needed)
        } catch (const karabo::util::ParameterException& e) { // key not found
            ports.assign(hosts.size(), m_defaultPort); // Default value
        }

#ifndef SLS_SIMULATION
        bool online = true;
        std::string errorMsg;
        for (size_t i = 0; i < hosts.size(); ++i) {
            if (!this->isServerOnline(hosts[i], ports[i], errorMsg)) {
                online = false;
                break;
            }
        }

        const std::string status = this->get<std::string>("status");
        if (!online && (this->getState() != State::UNKNOWN || status == "")) { // only once
            KARABO_LOG_ERROR << errorMsg;
            this->set("status", errorMsg);
        }

#else
        bool online = !(this->get<bool>("setDetOffline"));
#endif

        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting SlsControl::areDetectorsOnline";
        return online;

    }

    bool SlsControl::areReceiversOnline() {
        /* Note: When called on 5.0.1 receiver, the latter will print out an error like:
         *     17:17:49.184 ERROR: TCP socket read 0 bytes instead of 4 bytes
         *     17:17:49.184 ERROR: TCP socket sent 0 bytes instead of 1000 bytes
         *     17:17:49.185 ERROR: Accept failed
         * This is because a connection is established, but no data exchanged.
         */

        KARABO_LOG_FRAMEWORK_DEBUG << "Entering SlsControl::areReceiversOnline";

        const auto hosts = this->get<std::vector<std::string> >("rxHostname");
        const auto ports = this->get<std::vector<unsigned short> >("rxTcpPort");
        if (ports.size() < hosts.size()) {
            KARABO_LOG_FRAMEWORK_ERROR << "len(rxTcpPort) < len(rxHostname)";
        }

#ifndef SLS_SIMULATION
        bool online = true;
        std::string errorMsg;
        for (size_t i = 0; i < hosts.size(); ++i) {
            if (!this->isServerOnline(hosts[i], ports[i], errorMsg)) {
                online = false;
                break;
            }
        }

        const std::string status = this->get<std::string>("status");
        if (!online && (this->getState() != State::ERROR || status == "")) { // only once
            KARABO_LOG_ERROR << errorMsg;
            this->set("status", errorMsg);
        }

#else
        bool online = true;
#endif

        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting SlsControl::areReceiversOnline";
        return online;

    }

    // Return true if host is pingable.
    // This function could still return success if the host is in a protected
    // network, but in the latter case the device will fail in the
    // configuration step.
    bool SlsControl::ping(std::string host) {
        const std::string command = std::string("ping -c1 -s1 -W2 ") + host
        + " > /dev/null 2>&1";

        const int err = system(command.c_str());
        return (err == 0);
    }

    void SlsControl::createTmpDir() {
        KARABO_LOG_FRAMEWORK_DEBUG << "Entering SlsControl::createTmpDir";

        if (fs::exists("/dev/shm")) {
            char tmpDir[] = "/dev/shm/slsXXXXXX";
            m_tmpDir = mkdtemp(tmpDir); // Use volatile memory for temporary files
        } else {
            char tmpDir[] = "/tmp/slsXXXXXX";
            m_tmpDir = mkdtemp(tmpDir); // Fall back to /tmp directory
        }

        KARABO_LOG_FRAMEWORK_DEBUG << "Created temporary dir" << m_tmpDir;
    }

    void SlsControl::createCalibrationAndSettings(const std::string& settings) {
        KARABO_LOG_FRAMEWORK_DEBUG << "Entering SlsControl::createCalibrationAndSettings";

        const std::string calibrationDir = m_tmpDir + "/" + settings;

        if (!fs::exists(calibrationDir)) {
            // Create calibration and settings directory
            fs::create_directory(calibrationDir);

            KARABO_LOG_FRAMEWORK_DEBUG << "Created calibration dir" << calibrationDir;
        }

        // In case the detector needs calibration and setting files
        // (not the case for Gotthard, Jungfrau, Gotthard-II),
        // the derived class should - in addition - do something like:
        //
        // const std::string fname = calibrationDir + "/calibration.sn";
        // if (!fs::exists(fname)) {
        //     // Create calibration file
        //     std::ofstream fstr;
        //     fstr.open(fname.c_str());
        //
        //     if (fstr.is_open()) {
        //         fstr << "227 5.6\n"
        //         fstr.close();
        //
        //     } else {
        //         throw KARABO_RECONFIGURE_EXCEPTION("Could not open file " + fname + "for writing");
        //     }
        // }

        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting SlsControl::createCalibrationAndSettings";
    }

    void SlsControl::sendConfiguration(const std::string& command, const std::string& parameters, int pos) {
        KARABO_LOG_DEBUG << "Entering SlsControl::sendConfiguration";

        // Check that detector and receiver are online
        const State& state = this->getState();
        if (state == State::UNKNOWN || state == State::ERROR) {
            KARABO_LOG_FRAMEWORK_ERROR << "sendConfiguration(): detector or receiver is not online. Aborting!";
            return;
        }

        if (command.size() == 0) {
            KARABO_LOG_FRAMEWORK_WARN << "SlsControl::sendConfiguration skip empty command";
            return;
        }

        if (command == "settings") {
            // Create calibration and settings files (if needed)
            this->createCalibrationAndSettings(parameters);
        }

        std::stringstream command_and_parameters;
        if (pos >= 0) {
            command_and_parameters << pos << ":"; // position
        }
        command_and_parameters << command;
        if (parameters.size() > 0) {
            command_and_parameters << " " << parameters;
        }

        m_SLS->loadParameters(std::vector<std::string>({command_and_parameters.str()}));

        KARABO_LOG_FRAMEWORK_DEBUG << "Sent configuration: " << command_and_parameters.str();

        KARABO_LOG_DEBUG << "Quitting SlsControl::sendConfiguration";
    }

    void SlsControl::preReconfigure(Hash& incomingReconfiguration) {
        KARABO_LOG_FRAMEWORK_DEBUG << "Entering SlsControl::preReconfigure";

        KARABO_LOG_FRAMEWORK_DEBUG << "Incoming reconfiguration: \n" << incomingReconfiguration;

        if (incomingReconfiguration.has("highVoltage")) {
            // validate
            const auto highVoltageMax = incomingReconfiguration.has("highVoltageMax") ? incomingReconfiguration.get<unsigned int>("highVoltageMax") : this->get<unsigned int>("highVoltageMax");
            const auto highVoltage = incomingReconfiguration.get<std::vector<unsigned int>>("highVoltage");
            for (auto value : highVoltage) {
                if (value > highVoltageMax) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Discarding 'highVoltage', as it contains values higher than highVoltageMax (" << highVoltageMax << ")";
                    incomingReconfiguration.erase("highVoltage");
                }
            }
        }

        const Hash h1 = this->filterByTags(incomingReconfiguration, "sls");
        KARABO_LOG_FRAMEWORK_DEBUG << "Filtered reconfiguration: \n" << h1;

        this->sendConfiguration(h1);

        // Send detector specific configuration
        this->configureDetectorSpecific(incomingReconfiguration);

        Hash h2;
        if (incomingReconfiguration.has("acquisitionTime")) {
            const float acquisitionTime = incomingReconfiguration.get<float>("acquisitionTime");
            const std::string timing = incomingReconfiguration.has("timing") ? incomingReconfiguration.get<std::string>("timing") : this->get<std::string>("timing");

            if (timing == "trigger") {
                // External Trigger mode
                const float triggerPeriod = incomingReconfiguration.has("triggerPeriod") ? incomingReconfiguration.get<float>("triggerPeriod") : this->get<float>("triggerPeriod");

                if (acquisitionTime >= triggerPeriod) {
                    long long numberOfTriggers = std::floor(acquisitionTime / triggerPeriod);
                    h2.set("numberOfTriggers", numberOfTriggers);
                } else {
                    h2.set("numberOfTriggers", 1ll);

                    float exposureTime = incomingReconfiguration.has("exposureTime") ? incomingReconfiguration.get<float>("exposureTime") : this->get<float>("exposureTime");
                    float exposurePeriod = incomingReconfiguration.has("exposurePeriod") ? incomingReconfiguration.get<float>("exposurePeriod") : this->get<float>("exposurePeriod");

                    if (exposureTime >= acquisitionTime) {
                        exposureTime = acquisitionTime;
                        h2.set("exposureTime", exposureTime);
                    }

                    if (exposurePeriod >= acquisitionTime) {
                        exposurePeriod = acquisitionTime;
                        h2.set("exposurePeriod", exposurePeriod);
                    }

                    const long long numberOfFrames = acquisitionTime / std::max(exposureTime, exposurePeriod);
                    h2.set("numberOfFrames", numberOfFrames);
                }

                // Update device itself, then send parameters to detector
                this->set(h2);
                this->sendConfiguration(h2);

            } else if (timing == "auto") {
                // Internal Trigger mode
                h2.set("numberOfTriggers", 1ll);

                float exposureTime = incomingReconfiguration.has("exposureTime") ? incomingReconfiguration.get<float>("exposureTime") : this->get<float>("exposureTime");
                float exposurePeriod = incomingReconfiguration.has("exposurePeriod") ? incomingReconfiguration.get<float>("exposurePeriod") : this->get<float>("exposurePeriod");

                if (exposureTime >= acquisitionTime) {
                    exposureTime = acquisitionTime;
                    h2.set("exposureTime", exposureTime);
                }

                if (exposurePeriod >= acquisitionTime) {
                    exposurePeriod = acquisitionTime;
                    h2.set("exposurePeriod", exposurePeriod);
                }

                const long long numberOfFrames = acquisitionTime / std::max(exposureTime, exposurePeriod);
                h2.set("numberOfFrames", numberOfFrames);

                // Update device itself, then send parameters to detector
                this->set(h2);
                this->sendConfiguration(h2);

            } else {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION(std::string("Do not know how to set acquisitionTime in case of timing=")
                        + timing);
            }

        }

        if (incomingReconfiguration.has("pollingInterval") && m_poll) {
            // Stop and restart polling, such that new pollingInterval will be applied
            m_poll = false;
            m_poll_timer.cancel();
            m_poll = true;
            m_poll_timer.expires_from_now(boost::posix_time::seconds(1));
            m_poll_timer.async_wait(karabo::util::bind_weak(&SlsControl::pollHardware, this, boost::asio::placeholders::error));
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::preReconfigure";
    }

} /* namespace karabo */
