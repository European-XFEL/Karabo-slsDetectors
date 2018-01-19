/*
 * $Id: SlsDetector.cc 12381 2014-01-15 16:27:07Z parenti $
 *
 * Author: <Iryna.Kozlova@xfel.eu>
 * 
 * Created on July 12, 2013, 11:30 AM
 *
 * Copyright (c) 2010-2012 European XFEL GmbH Hamburg. All rights reserved.
 */

#include <fstream>

#include <boost/range/algorithm/count.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

#include "SlsControl.hh"

USING_KARABO_NAMESPACES
namespace fs=boost::filesystem;

namespace karabo {

    SlsControl::SlsControl(const Hash& config) : Device<CameraFsm>(config),
    m_isOnline(false), m_isConfigured(false), m_SLS(NULL), m_detIdx(0), m_doPolling(true) {
        createTmpDir(); // Create temporary directory
    }

    SlsControl::~SlsControl() {
        // Stop threads
        m_doPolling = false;
        m_doConnect = false;

        // Join threads
        if (m_pollThread.joinable()) m_pollThread.join();
        if (m_connectThread.joinable()) m_connectThread.join();

        // Remove temporary directory and its content
        fs::remove_all(m_tmpDir);

        if (m_SLS != NULL)
            delete m_SLS;

        KARABO_LOG_DEBUG << "Removed temporary dir" << m_tmpDir;
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

        OVERWRITE_ELEMENT(expected).key("connectCamera")
                .setNowAdminAccess() // Auto-connect -> Hide
                .commit();

        OVERWRITE_ELEMENT(expected).key("trigger")
                .setNewDescription("Trigger command is not available.")
                .setNowAdminAccess() // Hide it to everybody except admin
                .commit();

        STRING_ELEMENT(expected).key("detectorType")
                .displayedName("detectorType")
                .description("Type of slsDetector (Gotthard, Mythen, etc.)")
                .assignmentOptional().defaultValue("Gotthard+") // OVERWRITE in derived class
                .options("Gotthard+") // OVERWRITE in derived class
                .commit();

        STRING_ELEMENT(expected).key("detectorHostName")
                .alias("hostname")
                // No "sls" tag... will be processed differently
                .displayedName("detectorHostName")
                .description("Detector Host Name")
                .assignmentMandatory()
                .commit();

        STRING_ELEMENT(expected).key("detectorIp")
                .alias("detectorip")
                // No "sls" tag... will be processed differently
                .displayedName("detectorIp")
                .description("Detector IP. Must be on the same subnet as the receiver.")
                .assignmentMandatory()
                .commit();

        UINT16_ELEMENT(expected).key("detectorHostPort")
                .alias("port")
                .tags("sls")
                .displayedName("detectorHostPort")
                .description("Detector Host Port")
                .assignmentOptional().defaultValue(1952)
                .commit();

        UINT16_ELEMENT(expected).key("detectorHostStopPort")
                .alias("stopport")
                .tags("sls")
                .displayedName("detectorHostStopPort")
                .description("Detector Host Stop Port")
                .assignmentOptional().defaultValue(1953)
                .commit();

        STRING_ELEMENT(expected).key("rxHostname")
                .alias("rx_hostname")
                // No "sls" tag... will be processed differently
                .displayedName("rxHostname")
                .description("Receiver Hostname")
                .assignmentMandatory()
                .commit();

        STRING_ELEMENT(expected).key("settings")
                .alias("settings")
                .tags("sls")
                .displayedName("settings")
                .description("Settings")
                .assignmentOptional().defaultValue("dynamicgain") // OVERWRITE in derived class
                .options("dynamicgain lowgain mediumgain highgain veryhighgain") // OVERWRITE in derived class
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        // [AP] settingsdir, caldir, ffdir are internally set and not visible to the user

        NODE_ELEMENT(expected).key("dataStorage")
                .displayedName("Data Storage")
                .commit();

        UINT8_ELEMENT(expected).key("dataStorage.enable")
                .alias("enablefwrite")
                .tags("sls")
                .displayedName("Enable")
                .assignmentOptional().defaultValue(0)
                .options("0,1")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        PATH_ELEMENT(expected).key("dataStorage.filePath")
                .alias("outdir")
                .tags("sls")
                .displayedName("File Path")
                .description("The path (on receiver) for saving data to file")
                .isDirectory()
                .assignmentOptional().defaultValue("/tmp")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        STRING_ELEMENT(expected).key("dataStorage.fileName")
                .alias("fname")
                .tags("sls")
                .displayedName("File Name")
                .description("The name for saving data to file")
                .assignmentOptional().defaultValue("run")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT32_ELEMENT(expected).key("dataStorage.fileIndex")
                .alias("index")
                .tags("sls")
                .displayedName("File Start Index")
                .description("The starting index for saving data to file. It "
                "will be automatically updated after each acquisition.")
                .assignmentOptional().defaultValue(0)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT16_ELEMENT(expected).key("online")
                .alias("online")
                .tags("sls")
                .displayedName("online")
                .description("Online")
                .assignmentOptional().defaultValue(1)
                .options("0 1")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT16_ELEMENT(expected).key("angDir")
                .alias("angdir")
                .tags("sls")
                .displayedName("angDir")
                .description("angDir")
                .assignmentOptional().defaultValue(1)
                .options("1 -1")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT16_ELEMENT(expected).key("moveFlag")
                .alias("moveflag")
                .tags("sls")
                .displayedName("moveFlag")
                .description("moveFlag")
                .assignmentOptional().defaultValue(0)
                .options("0 1")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT16_ELEMENT(expected).key("lock")
                .alias("lock")
                .tags("sls")
                .displayedName("lock")
                .description("lock")
                .assignmentOptional().defaultValue(0)
                .options("0 1")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        // Only ExtSig:0 is used in gotthard
        // In principle also ExtSig:1, ExtSig:2 and ExtSig:3 could be present
        STRING_ELEMENT(expected).key("extSig0")
                .alias("extsig:0")
                .tags("sls")
                .displayedName("extSig0")
                .description("Ext Sig 0")
                .assignmentOptional().defaultValue("off")
                .options("off gate_in_active_high gate_in_active_low trigger_in_rising_edge "
                "trigger_in_falling_edge ro_trigger_in_rising_edge ro_trigger_in_falling_edge "
                "gate_out_active_high gate_out_active_low trigger_out_rising_edge "
                "trigger_out_falling_edge ro_trigger_out_rising_edge ro_trigger_out_falling_edge sync")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        // TODO: darkimage, gainimage?

        // TODO: vthreshold, vcalibration, vtrimbit, vpreamp, vshaper1, vshaper2 ?

        UINT32_ELEMENT(expected).key("vHighVoltage")
                .alias("vhighvoltage")
                .tags("sls")
                .displayedName("vHighVoltage")
                .description("V High Voltage")
                .options("0 90 110 120 150 180 200")
                .assignmentOptional().defaultValue(0)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        // vapower, vddpower, vshpower, viopower, vref_ds, vcascn_pb, vcascp_pb, vout_cm, vcasc_out, vin_cm, vref_comp,
        // ib_test_c TODO
        // reg a d?
        // clkdivider, setlenght, waitstates, totdivider, totdutycycle TODO
        // setup, trimbits ?

        INT16_ELEMENT(expected).key("master")
                .alias("master")
                .tags("sls global")
                .displayedName("master")
                .description("master")
                .assignmentOptional().defaultValue(-1)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        STRING_ELEMENT(expected).key("sync")
                .alias("sync")
                .tags("sls global")
                .displayedName("sync")
                .description("Sync")
                .assignmentOptional().defaultValue("none")
                .options("none gating trigger complementary")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        // Set internally: headerbefore="none", headerafter="none", headerbeforepar="none", headerafterpar="none"

        // TODO badchannels: create bad channel file on the fly in /dev/shm or /tmp?
        PATH_ELEMENT(expected).key("badChannels")
                .alias("badchannels")
                .tags("sls")
                .displayedName("Bad Channels")
                .description("Sets the bad channel file to fname. Bad channels will be "
                "omitted in the .dat file. Use 'none' to unset.")
                .assignmentOptional().defaultValue("none")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        // TODO angconv: create angconv file on the fly in /dev/shm or /tmp?
        PATH_ELEMENT(expected).key("angConv")
                .alias("angconv")
                .tags("sls")
                .displayedName("Angular Conversion")
                .description("Sets the file with the coefficients for angular conversion. Use 'none' "
                "to disable angular conversion.")
                .assignmentOptional().defaultValue("none")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        FLOAT_ELEMENT(expected).key("globalOff")
                .alias("globaloff")
                .tags("sls")
                .description("Global Offset")
                .unit(Unit::DEGREE)
                .assignmentOptional().defaultValue(0.)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        FLOAT_ELEMENT(expected).key("binSize")
                .alias("binsize")
                .tags("sls")
                .displayedName("binSize")
                .description("Bin Size")
                .unit(Unit::DEGREE)
                .assignmentOptional().defaultValue(0.001)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT16_ELEMENT(expected).key("threaded")
                .alias("threaded")
                .tags("sls")
                .displayedName("threaded")
                .description("threaded")
                .assignmentOptional().defaultValue(1)
                .options("0 1")
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        STRING_ELEMENT(expected).key("detectorDeveloper")
                .displayedName("detectorDeveloper")
                .description("Detector Developer. (useful to define subset of working functions)")
                .readOnly()
                .commit();

        STRING_ELEMENT(expected).key("flatFieldCorrectionFile")
                .alias("flatfield")
                .tags("sls")
                .displayedName("FlatFieldCorrectionFile")
                .description("Flat field corrections file name. Use 'none' to disable corrections.")
                .assignmentOptional().defaultValue("none")
                .reconfigurable()
                .commit();

        std::vector<double> positions;
        VECTOR_DOUBLE_ELEMENT(expected).key("positions")
                .alias("positions")
                .tags("sls")
                .displayedName("Positions")
                .description("Positions for the acquisition. Usage: pos1, pos2, pos3, ...")
                .assignmentOptional().defaultValue(positions)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        VECTOR_INT32_ELEMENT(expected).key("maximumDetectorSize")
                .displayedName("MaximumDetectorSize")
                .description("Maximum detector size")
                .readOnly()
                .commit();

        std::vector<int> roi(1, 0); // "roi 0" -> Disable ROI
        VECTOR_INT32_ELEMENT(expected).key("roi")
                .alias("roi")
                .tags("sls")
                .displayedName("ROI")
                .description("Set the ROI. Usage: i, xmin, xmax, ymin, ymax (where i is the number of ROIs). To disable use: 0.")
                .assignmentOptional().defaultValue(roi)
                .minSize(1).maxSize(5)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT32_ELEMENT(expected).key("bitDepth")
                .alias("dr")
                .tags("sls")
                .displayedName("BitDepth")
                .description("Bit Depth")
                .assignmentOptional().defaultValue(16)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        FLOAT_ELEMENT(expected).key("exposureTime")
                .alias("exptime")
                .tags("sls")
                .displayedName("ExposureTime")
                .description("exposure time value")
                .assignmentOptional().defaultValue(0.001)
                .unit(Unit::SECOND)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        FLOAT_ELEMENT(expected).key("exposurePeriod")
                .alias("period")
                .tags("sls")
                .displayedName("ExposurePeriod")
                .description("exposure period")
                .assignmentOptional().defaultValue(1.)
                .unit(Unit::SECOND)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        FLOAT_ELEMENT(expected).key("delayAfterTrigger")
                .alias("delay")
                .tags("sls")
                .displayedName("DelayAfterTrigger")
                .description("delay after trigger")
                .assignmentOptional().defaultValue(0.)
                .unit(Unit::SECOND)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT64_ELEMENT(expected).key("numberOfGates")
                .alias("gates")
                .tags("sls")
                .displayedName("NumberOfGates")
                .description("Number Of Gates")
                .assignmentOptional().defaultValue(0)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT64_ELEMENT(expected).key("numberOfFrames")
                .alias("frames")
                .tags("sls")
                .displayedName("NumberOfFrames")
                .description("Number Of Frames")
                .assignmentOptional().defaultValue(1)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT64_ELEMENT(expected).key("numberOfCycles")
                .alias("cycles")
                .tags("sls")
                .displayedName("NumberOfCycles")
                .description("Number Of Cycles")
                .assignmentOptional().defaultValue(1)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        STRING_ELEMENT(expected).key("timing")
                .alias("timing")
                .tags("sls")
                .displayedName("TimingMode")
                .description("The timing mode of the detector")
                .assignmentOptional().defaultValue("auto")
                .options("auto gating trigger ro_trigger triggered_gating")
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

        STRING_ELEMENT(expected).key("detectorNumber")
                .alias("detectornumber")
                .tags("sls readOnConnect")
                .displayedName("Detector Number")
                .description("Returns the serial number of the module (normally the MAC address).")
                .readOnly()
                .commit();

        STRING_ELEMENT(expected).key("detectorVersion")
                .alias("detectorversion")
                .tags("sls readOnConnect")
                .displayedName("Detector Version")
                .description("Returns the version of the controller firmware.")
                .readOnly()
                .commit();

        STRING_ELEMENT(expected).key("softwareVersion")
                .alias("softwareversion")
                .tags("sls readOnConnect")
                .displayedName("Detector SW Version")
                .description("Returns the version of the software running on the detector.")
                .readOnly()
                .commit();

        STRING_ELEMENT(expected).key("thisVersion")
                .alias("thisversion")
                .tags("sls readOnConnect")
                .displayedName("Control SW Version")
                .description("Returns the version of the control software which is being used.")
                .readOnly()
                .commit();

        UINT32_ELEMENT(expected).key("pollingInterval")
                .displayedName("Polling Interval")
                .description("The interval for polling the laser front-end for status.")
                .assignmentOptional().defaultValue(20)
                .unit(Unit::SECOND)
                .minInc(5)
                .maxInc(600)
                .reconfigurable()
                .commit();

        STRING_ELEMENT(expected).key("tempAdc")
                .alias("temp_adc")
                .tags("sls poll")
                .displayedName("ADC Temperature")
                .description("Returns the ADC temperature.")
                .readOnly()
                .commit();

        STRING_ELEMENT(expected).key("tempFpga")
                .alias("temp_fpga")
                .tags("sls poll")
                .displayedName("FPGA Temperature")
                .description("Returns the FPGA temperature.")
                .readOnly()
                .commit();

    }

    void SlsControl::initializationStateOnEntry() {
        KARABO_LOG_DEBUG << "Entering SlsControl::initializationStateOnEntry";

        // Create new m_SLS detector object
        if (m_SLS == NULL)
            m_SLS = new slsDetectorUsers(m_detIdx);

        // Start threads
        m_connectThread = boost::thread(boost::bind(&karabo::SlsControl::connect, this));
        m_pollThread = boost::thread(boost::bind(&karabo::SlsControl::pollHardware, this));

        KARABO_LOG_DEBUG << "Quitting SlsControl::initializationStateOnEntry";
    }

    std::string SlsControl::getDetectorStatus() {
        // m_SLS->getCommand is sometimes more reliable than m_SLS->getDetectorStatus
        // In slsDetectorsPackage 2.0.3 m_SLS->getCommand can hang if Gotthard is not responsive
        char* args[1];
        args[0] = const_cast<char*>("status");
        return m_SLS->getCommand(1, args, m_detIdx);
    }

    void SlsControl::stopMeasurement() {
        // m_SLS->putCommand is sometimes more reliable than m_SLS->stopMeasurement
        // In slsDetectorsPackage 2.0.3 m_SLS->getCommand can hang if Gotthard is not responsive
        char* args[2];
        args[0] = const_cast<char*>("status");
        args[1] = const_cast<char*>("stop");
        m_SLS->putCommand(2, args, m_detIdx);

        // Get current file index and update device
        args[0] = const_cast<char*>("index");
        int index = boost::lexical_cast<int>(m_SLS->getCommand(1, args, m_detIdx));
        this->set("dataStorage.fileIndex", index);

    }

    bool SlsControl::isHostOnline(std::string host, unsigned short port) {
        KARABO_LOG_DEBUG << "Entering SlsControl::isHostOnline";

        bool online;
        std::string portString = boost::lexical_cast<std::string >(port);

        try {
            boost::asio::ip::tcp::iostream s;

            // The entire sequence of I/O operations must complete within 5 seconds.
            // If an expiry occurs, the socket is automatically closed and the stream
            // becomes bad.
            s.expires_from_now(boost::posix_time::seconds(5));

            // Establish a connection to the server.
            s.connect(host, portString);

            if (s) {
                online = true;
            } else {
                online = false;
            }
        } catch (...) {
            // Check failed
            online = false;
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::isHostOnline";
        return online;
    }

    bool SlsControl::isDetectorOnline() {
        KARABO_LOG_DEBUG << "Entering SlsControl::isDetectorOnline";

        std::string host = get<std::string>("detectorHostName");
        unsigned short port = get<unsigned short>("detectorHostPort");

#ifndef SLS_SIMULATION
        bool online = isHostOnline(host, port);
#else
        bool online = !(this->get<bool>("setDetOffline"));
#endif

        KARABO_LOG_DEBUG << "Quitting SlsControl::isDetectorOnline";
        return online;

    }

    bool SlsControl::isReceiverOnline() {
        KARABO_LOG_DEBUG << "Entering SlsControl::isReceiverOnline";

        std::string host = get<std::string>("rxHostname");
        unsigned short port = get<unsigned short>("rxTcpPort");

#ifndef SLS_SIMULATION
        bool online = isHostOnline(host, port);
#else
        bool online = true;
#endif

        KARABO_LOG_DEBUG << "Quitting SlsControl::isReceiverOnline";
        return online;

    }

    void SlsControl::pollHardware() {
        KARABO_LOG_DEBUG << "Entering SlsControl::pollHardware";

        bool detectorOnline, receiverOnline;
        bool first = true;
        // To ensure that polling will happen first time:
        unsigned int counter = 1000000;
        while (m_doPolling) {

            if (this->getState() == State::ACQUIRING) {
                // Polling during acquisition would interfere with it
                boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                continue;
            }

            unsigned int pollingInterval = get<unsigned int >("pollingInterval");

            if (m_isOnline && counter >= pollingInterval) {
                KARABO_LOG_DEBUG << "Polling...";

                counter = 0;

                // Paths to be polled
                std::vector<std::string> paths;

                getPathsByTag(paths, "poll");

                if (first) {
                    // Run only once
                    first = false;

                    std::vector<std::string> onConnectPaths;
                    getPathsByTag(onConnectPaths, "readOnConnect");
                    paths.insert(paths.end(), onConnectPaths.begin(), onConnectPaths.end());
                }

                char* args[1];
                for (std::vector<std::string>::iterator it = paths.begin(); it != paths.end(); ++it) {
                    try {
                        std::string alias = getAliasFromKey<std::string >(*it); // key=*it
                        args[0] = const_cast<char*>(alias.c_str());
                        std::string reply = m_SLS->getCommand(1, args, m_detIdx);
                        set(*it, reply); // key=*it
                        KARABO_LOG_DEBUG << "Command: " << args[0] << ". Reply: " << reply;
                    } catch (const karabo::util::Exception& e) {
                        KARABO_LOG_ERROR << "Exception caught in SlsControl::pollHardware(): " << e.userFriendlyMsg() << ". " << e.detailedMsg();
                    } catch (...) {
                        KARABO_LOG_ERROR << "Exception caught in SlsControl::pollHardware()";
                    }
                }

            }

            // Sleep for awhile
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            ++counter;
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::pollHardware";
    }

    void SlsControl::connect() {
        KARABO_LOG_DEBUG << "Entering SlsControl::connectThread()";

        bool detectorOnline, receiverOnline;
        while (m_doConnect) {

            if (this->getState() == State::ACQUIRING) {
                // Polling during acquisition would interfere with it
                boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                continue;
            }

	    this->powerOn();
            detectorOnline = isDetectorOnline();
            if (detectorOnline) {
                // Only check receiver if detector is online
                receiverOnline = isReceiverOnline();
            } else {
                receiverOnline = true;
            }

            KARABO_LOG_DEBUG << "SlsControl::connectThread online: " << detectorOnline <<
                    " " << receiverOnline;

            if (detectorOnline) {
                if (this->getState() == State::UNKNOWN) {
                    KARABO_LOG_DEBUG << "Detector is online";
                    m_isConfigured = false; // m_SLS must be reconfigured
                    this->execute("connectCamera"); // Change state to ON
                }

                if (receiverOnline) {
                    m_isOnline = true;
                    if (this->getState() == State::ERROR) {
                        KARABO_LOG_DEBUG << "Receiver is online";
                    }
                    if (!m_isConfigured) {
                        // Configure detector and receiver
                        this->initialize();
                    }

                } else {
                    m_isOnline = false;
                    m_isConfigured = false; // m_SLS must be reconfigured
                    if (this->getState() != State::ERROR) {
                        this->execute("errorFound", "Receiver is offline", ""); // Change state to ERROR
                    }
                }

            } else { // !detectorOnline
                if (this->getState() != State::UNKNOWN) {
                    KARABO_LOG_DEBUG << "Detector is offline";
                    m_isOnline = false;
                    m_isConfigured = false; // m_SLS must be reconfigured
                    this->disconnectCamera(); // Change state to UNKNOWN
                }
            }

            // Sleep for a while
            boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
        }
    }

    void SlsControl::initialize() {
        KARABO_LOG_INFO << "Entering SlsControl::initialize";

        try {
            sendInitialConfiguration();
            m_isConfigured = true;
        } catch (const Exception& e) {
            this->execute("errorFound", e.userFriendlyMsg(), e.detailedMsg());
            m_isConfigured = false; // m_SLS must be reconfigured
            return;
        } catch (std::exception& e) {
            this->execute("errorFound", e.what(), "");
            m_isConfigured = false; // m_SLS must be reconfigured
            return;
        } catch (...) {
            this->execute("errorFound", "Exception caught in SlsControl::initialize()", "");
            m_isConfigured = false; // m_SLS must be reconfigured
            return;
        }

        try {
            // Get some properties
            std::string detectorDeveloper = m_SLS->getDetectorDeveloper();
            KARABO_LOG_INFO << "detectorDeveloper: " << detectorDeveloper;
            set("detectorDeveloper", detectorDeveloper);

            int nx, ny;
            std::vector<int> maximumDetectorSize;
            m_SLS->getMaximumDetectorSize(nx, ny);
            maximumDetectorSize.push_back(nx);
            maximumDetectorSize.push_back(ny);
            KARABO_LOG_INFO << "maximumDetectorSize: " << nx << "x" << ny;
            set("maximumDetectorSize", maximumDetectorSize);
        } catch (const Exception& e) {
            this->execute("errorFound", e.userFriendlyMsg(), e.detailedMsg());
            return;
        } catch (std::exception& e) {
            this->execute("errorFound", e.what(), "");
            return;
        } catch (...) {
            this->execute("errorFound", "Exception caught in SlsControl::initialize()", "");
            return;
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::initialize";
    }

    void SlsControl::acquire() {
        KARABO_LOG_DEBUG << "Entering SlsControl::acquire";

        try {
            // Start measurement and acquires
            // This is blocking call -> must be started in a thread
            KARABO_LOG_INFO << "SlsControl::acquire starting measurement";
            m_SLS->startMeasurement();
            KARABO_LOG_DEBUG << "SlsControl::acquire measurement done!";

            if (this->getState() == State::ACQUIRING) {
                // Acquisition ended, must trigger transition to ON state
                this->execute("stop");
                KARABO_LOG_DEBUG << "SlsControl::acquire executed 'stop'.";
            }

        } catch (const karabo::util::Exception& e) {
            this->execute("errorFound", e.userFriendlyMsg(), e.detailedMsg());
        } catch (std::exception& e) {
            this->execute("errorFound", e.what(), "");
        } catch (...) {
            this->execute("errorFound", "Unknown exception in SlsControl::acquire", "");
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::acquire";
    }

    void SlsControl::createTmpDir() {
        KARABO_LOG_DEBUG << "Entering SlsControl::createTmpDir";

        if (fs::exists("/dev/shm")) {
            char tmpDir[] = "/dev/shm/slsXXXXXX";
            m_tmpDir = mkdtemp(tmpDir); // Use volatile memory for temporary files
        } else {
            char tmpDir[] = "/tmp/slsXXXXXX";
            m_tmpDir = mkdtemp(tmpDir); // Fall back to /tmp directory
        }

        KARABO_LOG_DEBUG << "Created temporary dir" << m_tmpDir;

    }

    void SlsControl::createCalibrationAndSettings(const std::string& settings) {
        KARABO_LOG_DEBUG << "Entering SlsControl::createCalibrationAndSettings";

        std::string m_calibrationDir = m_tmpDir + "/" + settings;
        std::string calibrationFileName = m_calibrationDir + "/calibration.sn";
        std::string settingsFileName = m_calibrationDir + "/settings.sn";

        if (!fs::exists(m_calibrationDir)) {
            // Create calibration and settings directory
            fs::create_directory(m_calibrationDir);

            KARABO_LOG_DEBUG << "Created calibration dir" << m_calibrationDir;
        }

        if (!fs::exists(calibrationFileName)) {
            // Create calibration file
            std::ofstream calibrationFile;
            calibrationFile.open(calibrationFileName.c_str());

            if (calibrationFile.is_open()) {
                std::string calibrationString = this->getCalibrationString();
                calibrationFile << calibrationString;
                calibrationFile.close();

                KARABO_LOG_DEBUG << "Created calibration file " << calibrationFileName;
            }
        }

        if (!fs::exists(settingsFileName)) {
            // Create settings file
            std::ofstream settingsFile;
            settingsFile.open(settingsFileName.c_str());

            if (settingsFile.is_open()) {
                std::string settingsString = this->getSettingsString();
                settingsFile << settingsString;
                settingsFile.close();

                KARABO_LOG_DEBUG << "Created settings file " << settingsFileName;
            }
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::createCalibrationAndSettings";
    }

    void SlsControl::sendConfiguration(const karabo::util::Hash& configHash) {
        KARABO_LOG_DEBUG << "Entering SlsControl::sendConfiguration";

        // Check that detector and receiver are online
        if (!m_isOnline) {
            KARABO_LOG_ERROR << "sendConfiguration(): detector or receiver is not online. Aborting!";
            return;
        }

        Hash flat;
        Hash::flatten(configHash, flat);

        for (Hash::const_iterator it = flat.begin(); it != flat.end(); ++it) {
            std::string key, alias, value;

            try {
                key = it->getKey();
                alias = getAliasFromKey<std::string >(key);
                value = configHash.getAs<std::string>(key);
                Types::ReferenceType type = configHash.getType(key);

                KARABO_LOG_DEBUG << "SlsControl::sendConfiguration - Key: " << key << " Type: " << type;

                if (Types::isSimple(type)) {
                    KARABO_LOG_DEBUG << "######## SIMPLE TYPE #########";
                    sendConfiguration(alias, value);

                } else if (Types::isVector(type)) {
                    KARABO_LOG_DEBUG << "######## VECTOR TYPE #########";

                    if (key == "positions") {
                        // In case of "positions", vector length must be prepended to vector itself
                        if (value.size() == 0) {
                            // No parameters
                            sendConfiguration(alias, "0");
                        } else {
                            // Prepend number of (comma-separated) parameters
                            std::string parameters = toString(boost::count(value, ',') + 1) + "," + value;
                            sendConfiguration(alias, parameters);
                        }
                    } else {
                        sendConfiguration(alias, value);
                    }

                } else {
                    KARABO_LOG_WARN << "SlsControl::sendConfiguration received parameter " << key
                            << " is neither simple nor vector";
                    continue;
                }
            } catch (const karabo::util::Exception& e) {
                KARABO_LOG_ERROR << "SlsControl::sendConfiguration caught exception: " << e.userFriendlyMsg() << " " << e.detailedMsg();
                continue;
            } catch (std::exception& e) {
                KARABO_LOG_ERROR << "SlsControl::sendConfiguration caught exception: " << e.what();
                continue;
            } catch (...) {
                KARABO_LOG_ERROR << "SlsControl::sendConfiguration caught exception";
                continue;
            }
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::sendConfiguration";
    }

    void SlsControl::sendConfiguration(const std::string& command, const std::string& parameters) {
        KARABO_LOG_DEBUG << "Entering SlsControl::sendConfiguration";

        // Check that detector and receiver are online
        if (!m_isOnline) {
            KARABO_LOG_ERROR << "sendConfiguration(): detector or receiver is not online. Aborting!";
            return;
        }

        if (command.size() == 0) {
            KARABO_LOG_WARN << "SlsControl::sendConfiguration skip empty command";
            return;
        }

        if (command == "settings") {
            // Create calibration and settings files (if needed)
            createCalibrationAndSettings(parameters);
        }

        std::vector<std::string > tokens;
        boost::split(tokens, parameters, boost::is_any_of(", "));

        int narg = tokens.size() + 1;
        char* args[narg];
        args[0] = (char*) command.c_str();
        for (int i = 0; i < tokens.size(); ++i)
            args[i + 1] = const_cast<char*>(tokens.at(i).c_str());

        try {
            std::string reply = m_SLS->putCommand(narg, args, m_detIdx);
            KARABO_LOG_DEBUG << "Command: " << args[0] << " " << parameters << ". Reply: " << reply;
        } catch (...) {
            KARABO_LOG_ERROR << "Could not send configuration";
            throw; // Calling function must take care
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::sendConfiguration";
    }

    void SlsControl::sendInitialConfiguration() {
        KARABO_LOG_DEBUG << "Entering SlsControl::sendInitialConfiguration";

        // Check that detector and receiver are online
        if (!m_isOnline) {
            KARABO_LOG_ERROR << "sendConfiguration(): detector or receiver is not online. Aborting!";
            return;
        }

        try {

            // Free the shared memory
            char* args[1];
            args[0] = const_cast<char*>("free");
            m_SLS->getCommand(1, args, m_detIdx);

            // Get detector and receiver information
            std::string type = get<std::string >("detectorType");
            std::string hostname = get<std::string >("detectorHostName");
            std::string detectorip = get<std::string >("detectorIp");

            std::string rx_hostname = get<std::string >("rxHostname");
            std::string rx_udpip = get<std::string >("rxUdpIp");

            const std::string fname = m_tmpDir + "/base.config";
            std::ofstream configFile(fname.c_str(), std::ofstream::trunc);
            if (configFile.is_open()) {
                configFile << "type " << type << std::endl;
                configFile << m_detIdx << ":hostname " << hostname << std::endl;
                configFile << m_detIdx << ":detectorip " << detectorip << std::endl;
                configFile << m_detIdx << ":rx_udpip " << rx_udpip << std::endl;
                configFile << m_detIdx << ":rx_hostname " << rx_hostname << std::endl;
                configFile.close();

                m_SLS->readConfigurationFile(fname);
            }

            // Get current configuration
            Hash configHash = getCurrentConfiguration("sls");

            // Send some more parameters, not from configuration hash
            sendConfiguration("settingsdir", m_tmpDir); // settings directory
            sendConfiguration("caldir", m_tmpDir); // calibrations directory

            std::string ffdir = std::getenv("HOME");
            sendConfiguration("ffdir", ffdir); // flat field corrections directory

            sendConfiguration("extsig:1", "off");
            sendConfiguration("extsig:2", "off");
            sendConfiguration("extsig:3", "off");

            sendConfiguration("headerbefore", "none");
            sendConfiguration("headerafter", "none");
            sendConfiguration("headerbeforepar", "none");
            sendConfiguration("headerafterpar", "none");

            sendConfiguration("fineoff", "0.");

            sendConfiguration("startscript", "none");
            sendConfiguration("startscriptpar", "none");
            sendConfiguration("stopscript", "none");
            sendConfiguration("stopscriptpar", "none");

            sendConfiguration("scriptbefore", "none");
            sendConfiguration("scriptbeforepar", "none");
            sendConfiguration("scriptafter", "none");
            sendConfiguration("scriptafterpar", "none");

            sendConfiguration("scan0script", "0");
            sendConfiguration("scan0par", "0");
            sendConfiguration("scan0prec", "0");
            sendConfiguration("scan0steps", "0");

            sendConfiguration("scan1script", "0");
            sendConfiguration("scan1par", "0");
            sendConfiguration("scan1prec", "0");
            sendConfiguration("scan1steps", "0");

            // Send all other parameters, from configuration hash
            sendConfiguration(configHash);

            KARABO_LOG_DEBUG << "Configuration done";
        } catch (...) {
            KARABO_LOG_ERROR << "Failed to send initial configuration";
            throw; // Re-throw (calling function must take care)
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::sendInitialConfiguration";

    }

    void SlsControl::getPathsByTag(std::vector<std::string >& paths, const std::string& tags) {
        karabo::util::Schema schema = this->getFullSchema();
        karabo::util::Hash parameters = schema.getParameterHash();
        karabo::util::Hash filteredParameters = this->filterByTags(parameters, tags);

        // N.B. this->getCurrentConfiguration(tags)) dose not return parameters with no value set                                          

        filteredParameters.getPaths(paths);
    }

    void SlsControl::acquireAction() {
        KARABO_LOG_INFO << "SlsControl::acquireAction";

        // Check that detector and receiver are online, and m_SLS is configured
        // Trigger error if not
        if (!m_isOnline) {
            this->execute("errorFound", "acquireAction(): detector or receiver is not online", "");
            return;
        } else if (!m_isConfigured) {
            this->execute("errorFound", "acquireAction(): m_SLS is not configured", "");
            return;
        }

        try {
            const std::string filePath = this->get<std::string >("dataStorage.filePath");
            if (fs::is_regular_file(filePath)) {
                throw KARABO_IO_EXCEPTION(std::string("Path ") + filePath + " exists but is a file");
            } else if (fs::is_directory(filePath)) {
                // Check that directory is writable
                if (access(filePath.c_str(), W_OK) != 0) {
                    throw KARABO_IO_EXCEPTION(std::string("Cannot write into directory ") + filePath);
                }
            } else {
                // Output directory does not exist -> try to create it
                bool success = fs::create_directories(filePath);
                if (!success) {
                    throw KARABO_IO_EXCEPTION(std::string("Cannot create directory ") + filePath);
                }
            }

            std::string runStatus = this->getDetectorStatus();

            if (runStatus != "idle") {
                KARABO_LOG_WARN << "Cannot start acquisition. Run status is "
                        << runStatus << ".";
                return;
            }

        } catch (const karabo::util::Exception& e) {
            this->execute("errorFound", e.userFriendlyMsg(), e.detailedMsg());
            return;
        } catch (std::exception& e) {
            this->execute("errorFound", e.what(), "");
            return;
        } catch (...) {
            this->execute("errorFound", "Unknown exception in SlsControl::acquireAction", "");
            return;
        }

        if (m_acquisitionThread.joinable()) {
            // Acquisition thread is already running
            KARABO_LOG_WARN << "Cannot start acquisition. Thread is already running.";
            return;
        }

        // Start acquisition (in a thread)
        m_acquisitionThread = boost::thread(boost::bind(&SlsControl::acquire, this));

        KARABO_LOG_DEBUG << "Quitting SlsControl::acquireAction";
    }

    void SlsControl::stopAction() {
        KARABO_LOG_INFO << "SlsControl::stopAction";

        // Check that detector and receiver are online
        if (!m_isOnline) {
            KARABO_LOG_ERROR << "stopAction(): detector or receiver is not online. Aborting!";
            return;
        }

        try {
            this->stopMeasurement();

            std::string runStatus = this->getDetectorStatus();
            if (runStatus == "error") {
                this->execute("errorFound", "getDetectorStatus returned error", "");
            }

        } catch (const karabo::util::Exception& e) {
            KARABO_LOG_ERROR << "Cannot get detector status: " <<
                    e.userFriendlyMsg() << " " << e.detailedMsg();
            return;
        } catch (std::exception& e) {
            KARABO_LOG_ERROR << "Cannot get detector status: " <<
                    e.what();
            return;
        } catch (...) {
            KARABO_LOG_ERROR << "Cannot get detector status";
            return;
        }

        if (m_acquisitionThread.joinable()) {
            // Join acquisition thread
            m_acquisitionThread.join();
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::stopAction";
    }

    void SlsControl::errorFoundAction(const std::string& user, const std::string& detail) {
        KARABO_LOG_ERROR << "[short] " << user;
        KARABO_LOG_ERROR << "[detailed] " << detail;
    }

    void SlsControl::resetAction() {
        KARABO_LOG_INFO << "SlsControl::resetAction";
        if (!m_isConfigured) {
            this->initialize(); // Configure detector & receiver
        }
    }

    void SlsControl::preReconfigure(Hash& incomingReconfiguration) {
        KARABO_LOG_DEBUG << "Entering SlsControl::preReconfigure";

        KARABO_LOG_DEBUG << "Incoming reconfiguration: \n" << incomingReconfiguration;
        Hash h1 = this->filterByTags(incomingReconfiguration, "sls");
        KARABO_LOG_DEBUG << "Filtered reconfiguration: \n" << h1;

        sendConfiguration(h1);

        Hash h2;
        if (incomingReconfiguration.has("acquisitionTime")) {
            float acquisitionTime = incomingReconfiguration.get<float>("acquisitionTime");
            std::string timing = incomingReconfiguration.has("timing") ? incomingReconfiguration.get<std::string>("timing") : this->get<std::string>("timing");

            if (timing == "trigger") {
                // External Trigger mode
                float triggerPeriod = incomingReconfiguration.has("triggerPeriod") ? incomingReconfiguration.get<float>("triggerPeriod") : this->get<float>("triggerPeriod");
                long long numberOfCycles;

                if (acquisitionTime >= triggerPeriod) {
                    numberOfCycles = std::floor(acquisitionTime / triggerPeriod);
                    h2.set("numberOfCycles", numberOfCycles);
                } else {
                    h2.set("numberOfCycles", 1ll);

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

                    long long numberOfFrames = acquisitionTime / std::max(exposureTime, exposurePeriod);
                    h2.set("numberOfFrames", numberOfFrames);
                }

                // Update device itself, then send parameters to detector
                this->set(h2);
                sendConfiguration(h2);

            } else if (timing == "auto") {
                // Internal Trigger mode
                h2.set("numberOfCycles", 1ll);

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

                long long numberOfFrames = acquisitionTime / std::max(exposureTime, exposurePeriod);
                h2.set("numberOfFrames", numberOfFrames);

                // Update device itself, then send parameters to detector
                this->set(h2);
                sendConfiguration(h2);

            } else {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION(std::string("Do not know how to set acquisitionTime in case of timing=")
                        + timing);
            }

        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::preReconfigure";
    }

} /* namespace karabo */
