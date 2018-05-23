/*
 *
 * Author: <Iryna.Kozlova@xfel.eu>
 * 
 * Created on July 12, 2013, 11:30 AM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

#include "SlsControl.hh"

USING_KARABO_NAMESPACES
namespace fs = boost::filesystem;

namespace karabo {

    SlsControl::SlsControl(const Hash& config) : Device<>(config), m_SLS(NULL),
    m_connect(false), m_connect_timer(EventLoop::getIOService()),
    m_firstPoll(true), m_poll(false), m_poll_timer(EventLoop::getIOService()),
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

        if (m_SLS != NULL) {
            delete m_SLS;
        }

        // Remove temporary directory and its content
        fs::remove_all(m_tmpDir);
        KARABO_LOG_FRAMEWORK_DEBUG << "Removed temporary dir" << m_tmpDir;
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
                .setNewOptions(State::UNKNOWN, State::INIT, State::ERROR, State::ON, State::ACQUIRING)
                .setNewDefaultValue(State::INIT)
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
                .displayedName("detectorHostName")
                .description("Detector Host Name")
                .assignmentMandatory()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("detectorIp")
                .alias("detectorip")
                // No "sls" tag... will be processed differently
                .displayedName("detectorIp")
                .description("Detector IP. Must be on the same subnet as the receiver.")
                .assignmentMandatory()
                .commit();

        VECTOR_UINT16_ELEMENT(expected).key("detectorHostPort")
                .alias("port")
                .tags("sls")
                .displayedName("detectorHostPort")
                .description("Detector Host Port. Will use 1952 if left empty.")
                .assignmentOptional().noDefaultValue()
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
                .displayedName("rxHostname")
                .description("Receiver Hostname")
                .assignmentMandatory()
                .commit();

        VECTOR_UINT16_ELEMENT(expected).key("rxTcpPort")
                .alias("rx_tcpport")
                // No "sls" tag... will be processed differently
                .displayedName("rxTcpPort")
                .description("Receiver TCP Port")
                .assignmentMandatory()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("rxUdpIp")
                .alias("rx_udpip")
                // No "sls" tag... will be processed differently
                .displayedName("rxUdpIp")
                .description("Receiver UDP IP")
                .assignmentMandatory()
                .commit();

        VECTOR_UINT16_ELEMENT(expected).key("rxUdpPort")
                .alias("rx_udpport")
                // No "sls" tag... will be processed differently
                .displayedName("rxUdpPort")
                .description("Receiver UDP Port")
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

        VECTOR_INT16_ELEMENT(expected).key("online")
                .alias("online")
                .tags("sls")
                .displayedName("online")
                .description("Sets the detector in online (1) or offline (0) mode.")
                .assignmentOptional().defaultValue({1})
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        VECTOR_INT16_ELEMENT(expected).key("angDir")
                .alias("angdir")
                .tags("sls")
                .displayedName("angDir")
                .description("Sets the angular direction of the detector (1 means channel number"
                "in the same direction as the angular encoder, -1 different direction).")
                .assignmentOptional().defaultValue({1})
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        VECTOR_INT16_ELEMENT(expected).key("moveFlag")
                .alias("moveflag")
                .tags("sls")
                .displayedName("moveFlag")
                .description("Related to a single controller d. 1 if the detector modules move"
                "with the angular encoder, 0 if they are static (useful for multidetector systems)")
                .assignmentOptional().defaultValue({0})
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        VECTOR_INT16_ELEMENT(expected).key("lock")
                .alias("lock")
                .tags("sls")
                .displayedName("lock")
                .description("lock")
                .assignmentOptional().defaultValue({0})
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

        VECTOR_UINT32_ELEMENT(expected).key("vHighVoltage")
                .alias("vhighvoltage")
                .tags("sls")
                .displayedName("vHighVoltage")
                .description("Sets the DAC value of the high voltage. Options: 0 90 110 120 150 180 200")
                .assignmentOptional().defaultValue({90})
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
                .tags("sls")
                .displayedName("master")
                .description("Sets the master of a multi-controller detector to the controller "
                "with index i. -1 removes master.")
                .assignmentOptional().defaultValue(-1)
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        STRING_ELEMENT(expected).key("sync")
                .alias("sync")
                .tags("sls")
                .displayedName("sync")
                .description("Sets the synchronization mode of the various controller within "
                "a detector structure")
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
                .description("Sets the bad channel filename. Bad channels will be "
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

        VECTOR_FLOAT_ELEMENT(expected).key("globalOff")
                .alias("globaloff")
                .tags("sls")
                .displayedName("Global Offset")
                .description("Sets the offset of the beamline i.e. angular position of channel"
                "0 when angular encoder at 0.")
                .unit(Unit::DEGREE)
                .assignmentOptional().defaultValue({0.})
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        VECTOR_FLOAT_ELEMENT(expected).key("binSize")
                .alias("binsize")
                .tags("sls")
                .displayedName("Bin Size")
                .description("Sets the size of the angular bins for angular conversion.")
                .unit(Unit::DEGREE)
                .assignmentOptional().defaultValue({0.001})
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        INT16_ELEMENT(expected).key("threaded")
                .alias("threaded")
                .tags("sls")
                .displayedName("Threaded")
                .description("Avoid changing it. Sets if the data are written to disk in parallel "
                "with the acquisition (1) or after the acquisition (0).")
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

        PATH_ELEMENT(expected).key("flatFieldCorrectionFile")
                .alias("flatfield")
                .tags("sls")
                .displayedName("Flat-Field Correction File")
                .description("Flat field corrections file name. Use 'none' to disable corrections.")
                .assignmentOptional().defaultValue("none")
                .reconfigurable()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("positions")
                .alias("positions")
                .tags("sls")
                .displayedName("Positions")
                .description("Positions for the acquisition. Usage: n pos1 pos2 ... posn")
                .assignmentOptional().defaultValue({"0"})
                .reconfigurable()
                .allowedStates(State::ON)
                .commit();

        VECTOR_INT32_ELEMENT(expected).key("maximumDetectorSize")
                .displayedName("MaximumDetectorSize")
                .description("Maximum detector size")
                .readOnly()
                .commit();

        // Detector returns error - also from command line interface...
        //        VECTOR_STRING_ELEMENT(expected).key("roi")
        //                .alias("roi")
        //                .tags("sls")
        //                .displayedName("ROI")
        //                .description("Set the ROI. Usage: i, xmin, xmax, ymin, ymax (where i is the number of ROIs). To disable use: 0.")
        //                .assignmentOptional().defaultValue({"0"})
        //                .minSize(1).maxSize(5)
        //                .reconfigurable()
        //                .allowedStates(State::ON)
        //                .commit();

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

        VECTOR_STRING_ELEMENT(expected).key("detectorNumber")
                .alias("detectornumber")
                .tags("sls readOnConnect")
                .displayedName("Detector Number")
                .description("Returns the serial number of the module (normally the MAC address).")
                .readOnly()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("detectorVersion")
                .alias("detectorversion")
                .tags("sls readOnConnect")
                .displayedName("Detector Version")
                .description("Returns the version of the controller firmware.")
                .readOnly()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("softwareVersion")
                .alias("softwareversion")
                .tags("sls readOnConnect")
                .displayedName("Detector SW Version")
                .description("Returns the version of the software running on the detector.")
                .readOnly()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("thisVersion")
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

        VECTOR_STRING_ELEMENT(expected).key("tempAdc")
                .alias("temp_adc")
                .tags("sls poll")
                .displayedName("ADC Temperature")
                .description("Returns the ADC temperature.")
                .readOnly()
                .commit();

        VECTOR_STRING_ELEMENT(expected).key("tempFpga")
                .alias("temp_fpga")
                .tags("sls poll")
                .displayedName("FPGA Temperature")
                .description("Returns the FPGA temperature.")
                .readOnly()
                .commit();

    }

    void SlsControl::start() {
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

            this->updateState(State::ACQUIRING);
            m_acquire_timer.expires_from_now(boost::posix_time::milliseconds(0));
            m_acquire_timer.async_wait(karabo::util::bind_weak(&SlsControl::acquireBlocking, this, boost::asio::placeholders::error));
        } catch (karabo::util::Exception& e) {
            this->updateState(State::ERROR);
            KARABO_LOG_ERROR << e;
        }
    }

    void SlsControl::stop() {
        KARABO_LOG_FRAMEWORK_DEBUG << "In stop";
        m_SLS->stopMeasurement();
        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting stop";
        this->updateState(State::ON);
    }

    void SlsControl::reset() {
        if (this->areReceiversOnline()) {
            try {
                this->updateState(State::ON);
                KARABO_LOG_INFO << "Connected to detector(s)";
                this->sendBaseConfiguration();
                this->sendInitialConfiguration();
            } catch (karabo::util::Exception& e) {
                this->updateState(State::ERROR);
                KARABO_LOG_FRAMEWORK_ERROR << e;
            }
        } else {
            KARABO_LOG_ERROR << "Receiver(s) are offline";
        }
    }

    void SlsControl::initialize() {
        try {
            m_numberOfModules = this->get<std::vector<std::string> >("detectorHostName").size();
        } catch (const karabo::util::Exception &e) {
            KARABO_LOG_FRAMEWORK_ERROR << "SlsControl::initializationStateOnEntry - " << e;
            m_numberOfModules = 0;
        }

        KARABO_LOG_FRAMEWORK_DEBUG << "Creating m_SLS...";
        m_SLS = new slsDetectorUsers(0);
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
                this->updateState(State::ON);
                KARABO_LOG_INFO << "Connected to detector(s)";
                try {
                    this->sendBaseConfiguration();
                    this->sendInitialConfiguration();
                } catch (karabo::util::Exception& e) {
                    this->updateState(State::ERROR);
                    KARABO_LOG_FRAMEWORK_ERROR << e;
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

        KARABO_LOG_FRAMEWORK_DEBUG << "Stop polling, as it would interfere with acquisition";
        this->stopPoll();

        m_SLS->startMeasurement(); // Blocking function - will return when acquisition is over!

        KARABO_LOG_FRAMEWORK_DEBUG << "Restart polling";
        this->startPoll();
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
            KARABO_LOG_ERROR << "Detector(s) went offline";
            this->updateState(State::UNKNOWN);
            m_connect = true;
            m_connect_timer.expires_from_now(boost::posix_time::milliseconds(0));
            m_connect_timer.async_wait(karabo::util::bind_weak(&SlsControl::connect, this, boost::asio::placeholders::error));
            return;
        }

        if (!this->areReceiversOnline() && this->getState() != State::ERROR) {
            KARABO_LOG_ERROR << "Receiver(s) went offline";
            this->updateState(State::ERROR);
        }

        // Paths to be polled
        std::vector<std::string> paths;
        this->getPathsByTag(paths, "poll");

        if (m_firstPoll) {
            // Run only once
            m_firstPoll = false;

            std::vector<std::string> onConnectPaths;
            this->getPathsByTag(onConnectPaths, "readOnConnect");
            paths.insert(paths.end(), onConnectPaths.begin(), onConnectPaths.end());
        }

        char* args[1];
        Hash h;
        for (auto it = paths.begin(); it != paths.end(); ++it) {
            try {
                std::string alias = this->getAliasFromKey<std::string >(*it); // key=*it
                auto type = this->getValueType(*it);
                args[0] = const_cast<char*> (alias.c_str());
                if (Types::isSimple(type)) {
                    std::string reply = m_SLS->getCommand(1, args, 0);
                    h.set(*it, reply);
                    KARABO_LOG_FRAMEWORK_DEBUG << "Command: " << args[0] << ". Reply: " << reply;
                } else { // isVector
                    std::vector<std::string> reply;
                    for (size_t i = 0; i < m_numberOfModules; ++i) {
                        reply.push_back(m_SLS->getCommand(1, args, i));
                    }
                    h.set(*it, reply);
                    KARABO_LOG_FRAMEWORK_DEBUG << "Command: " << args[0] << ". Reply: " << toString(reply);
                }
            } catch (const karabo::util::Exception& e) {
                KARABO_LOG_ERROR << "Exception caught in SlsControl::pollHardware(): " << e;
            }
        }
        if (!h.empty()) {
            this->set(h); // bulk set
        }

        if (m_poll) {
            m_poll_timer.expires_at(m_poll_timer.expires_at() + boost::posix_time::seconds(this->get<unsigned int>("pollingInterval")));
            m_poll_timer.async_wait(karabo::util::bind_weak(&SlsControl::pollHardware, this, boost::asio::placeholders::error));
            return;
        }

    }

    void SlsControl::getPathsByTag(std::vector<std::string >& paths, const std::string& tags) {
        karabo::util::Schema schema = this->getFullSchema();
        karabo::util::Hash parameters = schema.getParameterHash();
        karabo::util::Hash filteredParameters = this->filterByTags(parameters, tags);

        // N.B. this->getCurrentConfiguration(tags)) dose not return parameters with no value set                                          

        filteredParameters.getPaths(paths);
    }

    void SlsControl::sendBaseConfiguration() {
        // Get detector and receiver information
        auto hostnames = this->get<std::vector<std::string> >("detectorHostName");
        auto detectorips = this->get<std::vector<std::string> >("detectorIp");

        auto rx_hostnames = this->get<std::vector<std::string> > ("rxHostname");
        auto rx_tcpports = this->get<std::vector<unsigned short> >("rxTcpPort");
        auto rx_udpips = this->get<std::vector<std::string> >("rxUdpIp");
        auto rx_udpports = this->get<std::vector<unsigned short> >("rxUdpPort");

        // Check that all vectors have the same size
        if (hostnames.size() != m_numberOfModules) {
            throw KARABO_PARAMETER_EXCEPTION(std::string("detectorHostName has wrong size: " + toString(hostnames.size()) + "!= " + toString(m_numberOfModules)));
        } else if (detectorips.size() != m_numberOfModules) {
            throw KARABO_PARAMETER_EXCEPTION(std::string("detectorIp has wrong size: " + toString(detectorips.size()) + "!= " + toString(m_numberOfModules)));
        } else if (rx_hostnames.size() != m_numberOfModules) {
            throw KARABO_PARAMETER_EXCEPTION(std::string("rxHostname has wrong size: " + toString(rx_hostnames.size()) + "!= " + toString(m_numberOfModules)));
        } else if (rx_tcpports.size() != m_numberOfModules) {
            throw KARABO_PARAMETER_EXCEPTION(std::string("rxTcpPort has wrong size: " + toString(rx_tcpports.size()) + "!= " + toString(m_numberOfModules)));
        } else if (rx_udpips.size() != m_numberOfModules) {
            throw KARABO_PARAMETER_EXCEPTION(std::string("rxUdpIp has wrong size: " + toString(rx_udpips.size()) + "!= " + toString(m_numberOfModules)));
        } else if (rx_udpports.size() != m_numberOfModules) {
            throw KARABO_PARAMETER_EXCEPTION(std::string("rxUdpPort has wrong size: " + toString(rx_udpports.size()) + "!= " + toString(m_numberOfModules)));
        }

        const std::string fname = m_tmpDir + "/base.config";
        std::ofstream configFile(fname.c_str(), std::ofstream::trunc);
        if (configFile.is_open()) {
            configFile << "hostname ";
            for (auto hostname : hostnames) {
                configFile << hostname << "+";
            }
            configFile << std::endl;

            for (size_t i = 0; i < m_numberOfModules; ++i) {
                // Please note: order of the parameter matters!
                configFile << i << ":rx_udpport " << rx_udpports[i] << std::endl;
                configFile << i << ":rx_tcpport " << rx_tcpports[i] << std::endl;
                configFile << i << ":detectorip " << detectorips[i] << std::endl;
                configFile << i << ":rx_udpip " << rx_udpips[i] << std::endl;
                configFile << i << ":rx_hostname " << rx_hostnames[i] << std::endl;
            }
            configFile.close();

            int success = m_SLS->readConfigurationFile(fname); // This will also free SLS shared memory
            if (success != 0) {
                throw KARABO_RECONFIGURE_EXCEPTION("m_SLS->readConfigurationFile returned code " + toString(success));
            }
            this->powerOn();
        } else {
            throw KARABO_RECONFIGURE_EXCEPTION("Could not open file " + fname + "for writing");
        }
    }

    void SlsControl::sendInitialConfiguration() {
        try {
            // Get current configuration
            Hash configHash = this->getCurrentConfiguration("sls");

            // Send some more parameters, not from configuration hash
            this->sendConfiguration("settingsdir", m_tmpDir); // settings directory
            this->sendConfiguration("caldir", m_tmpDir); // calibrations directory

            std::string ffdir = std::getenv("HOME");
            this->sendConfiguration("ffdir", ffdir); // flat field corrections directory

            this->sendConfiguration("extsig:1", "off");
            this->sendConfiguration("extsig:2", "off");
            this->sendConfiguration("extsig:3", "off");

            this->sendConfiguration("headerbefore", "none");
            this->sendConfiguration("headerafter", "none");
            this->sendConfiguration("headerbeforepar", "none");
            this->sendConfiguration("headerafterpar", "none");

            this->sendConfiguration("fineoff", "0.");

            this->sendConfiguration("startscript", "none");
            this->sendConfiguration("startscriptpar", "none");
            this->sendConfiguration("stopscript", "none");
            this->sendConfiguration("stopscriptpar", "none");

            this->sendConfiguration("scriptbefore", "none");
            this->sendConfiguration("scriptbeforepar", "none");
            this->sendConfiguration("scriptafter", "none");
            this->sendConfiguration("scriptafterpar", "none");

            this->sendConfiguration("scan0script", "0");
            this->sendConfiguration("scan0par", "0");
            this->sendConfiguration("scan0prec", "0");
            this->sendConfiguration("scan0steps", "0");

            this->sendConfiguration("scan1script", "0");
            this->sendConfiguration("scan1par", "0");
            this->sendConfiguration("scan1prec", "0");
            this->sendConfiguration("scan1steps", "0");

            this->sendConfiguration("fileformat", "binary");

            // Send all other parameters, from configuration hash
            this->sendConfiguration(configHash);

            KARABO_LOG_FRAMEWORK_DEBUG << "Configuration done";
        } catch (...) {
            KARABO_LOG_FRAMEWORK_ERROR << "Failed to send initial configuration";
            throw; // Re-throw (calling function must take care)
        }

        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting SlsControl::sendInitialConfiguration";
    }

    void SlsControl::sendConfiguration(const karabo::util::Hash& configHash) {
        KARABO_LOG_FRAMEWORK_DEBUG << "Entering SlsControl::sendConfiguration";

        // Check that detector and receiver are online
        if (this->getState() != State::ON) {
            KARABO_LOG_ERROR << "sendConfiguration(): detector or receiver is not online. Aborting!";
            return;
        }

        Hash flat;
        Hash::flatten(configHash, flat);

        const Schema& fullSchema = this->getFullSchema();
        std::string key, alias;
        for (auto it = flat.begin(); it != flat.end(); ++it) {

            try {
                key = it->getKey();
                if (fullSchema.isAccessReadOnly(key)) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "SlsControl::sendConfiguration - key: " << key << " is Read-Only -> Skip";
                    continue;
                }
                alias = this->getAliasFromKey<std::string >(key);
                Types::ReferenceType type = configHash.getType(key);

                KARABO_LOG_FRAMEWORK_DEBUG << "SlsControl::sendConfiguration - Key: " << key << " Type: " << type;

                if (Types::isSimple(type)) {
                    std::string value = configHash.getAs<std::string>(key);
                    this->sendConfiguration(alias, value);

                } else if (Types::isVector(type)) {
                    auto values = configHash.getAs<std::string, std::vector>(key);
                    if (values.size() == 0) {
                        continue; // ignore key
                    } else if (values.size() == 1) {
                        // send same value to all
                        for (size_t i = 0; i < m_numberOfModules; ++i) {
                            this->sendConfiguration(alias, values[0], i);
                        }
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
            } catch (const karabo::util::Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "SlsControl::sendConfiguration caught exception: " << e;
                continue;
            }
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::sendConfiguration";
    }

    bool SlsControl::isHostOnline(std::string host, unsigned short port) {
        KARABO_LOG_FRAMEWORK_DEBUG << "Entering SlsControl::isHostOnline";

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

        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting SlsControl::isHostOnline";
        return online;
    }

    bool SlsControl::areDetectorsOnline() {
        KARABO_LOG_FRAMEWORK_DEBUG << "Entering SlsControl::areDetectorsOnline";

        auto hosts = this->get<std::vector<std::string> >("detectorHostName");
        std::vector<unsigned short> ports;
        try {
            ports = this->get<std::vector<unsigned short> >("detectorHostPort");
            ports.resize(hosts.size(), m_defaultPort); // If size mismatch: extend with default / truncate (as needed)
        } catch (karabo::util::ParameterException) { // key not found
            ports.assign(hosts.size(), m_defaultPort); // Default value
        }

#ifndef SLS_SIMULATION
        bool online = true;
        for (size_t i = 0; i < hosts.size(); ++i) {
            if (!this->isHostOnline(hosts[i], ports[i])) {
                if (this->getState() != State::UNKNOWN) { // only once
                    KARABO_LOG_ERROR << hosts[i] << ":" << ports[i] << " is offline.";
                }
                online = false;
                break;
            }
        }
#else
        bool online = !(this->get<bool>("setDetOffline"));
#endif

        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting SlsControl::areDetectorsOnline";
        return online;

    }

    bool SlsControl::areReceiversOnline() {
        KARABO_LOG_FRAMEWORK_DEBUG << "Entering SlsControl::areReceiversOnline";

        auto hosts = get<std::vector<std::string> >("rxHostname");
        auto ports = get<std::vector<unsigned short> >("rxTcpPort");
        if (ports.size() < hosts.size()) {
            KARABO_LOG_FRAMEWORK_ERROR << "len(rxTcpPort) < len(rxHostname)";
        }

#ifndef SLS_SIMULATION
        bool online = true;
        for (size_t i = 0; i < hosts.size(); ++i) {
            if (!this->isHostOnline(hosts[i], ports[i])) {
                if (this->getState() != State::ERROR) { // only once
                    KARABO_LOG_ERROR << hosts[i] << ":" << ports[i] << " is offline.";
                }
                online = false;
                break;
            }
        }
#else
        bool online = true;
#endif

        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting SlsControl::areReceiversOnline";
        return online;

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

        std::string calibrationDir = m_tmpDir + "/" + settings;
        std::string calibrationFileName = calibrationDir + "/calibration.sn";
        std::string settingsFileName = calibrationDir + "/settings.sn";

        if (!fs::exists(calibrationDir)) {
            // Create calibration and settings directory
            fs::create_directory(calibrationDir);

            KARABO_LOG_FRAMEWORK_DEBUG << "Created calibration dir" << calibrationDir;
        }

        if (!fs::exists(calibrationFileName)) {
            // Create calibration file
            std::ofstream calibrationFile;
            calibrationFile.open(calibrationFileName.c_str());

            if (calibrationFile.is_open()) {
                std::string calibrationString = this->getCalibrationString();
                calibrationFile << calibrationString;
                calibrationFile.close();

                KARABO_LOG_FRAMEWORK_DEBUG << "Created calibration file " << calibrationFileName;
            } else {
                throw KARABO_RECONFIGURE_EXCEPTION("Could not open file " + calibrationFileName + "for writing");
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

                KARABO_LOG_FRAMEWORK_DEBUG << "Created settings file " << settingsFileName;
            } else {
                throw KARABO_RECONFIGURE_EXCEPTION("Could not open file " + settingsFileName + "for writing");
            }
        }

        KARABO_LOG_FRAMEWORK_DEBUG << "Quitting SlsControl::createCalibrationAndSettings";
    }

    void SlsControl::sendConfiguration(const std::string& command, const std::string& parameters, int pos) {
        KARABO_LOG_DEBUG << "Entering SlsControl::sendConfiguration";

        // Check that detector and receiver are online
        if (this->getState() != State::ON) {
            KARABO_LOG_FRAMEWORK_ERROR << "sendConfiguration(): detector or receiver is not online. Aborting!";
            return;
        }

        if (command.size() == 0) {
            KARABO_LOG_FRAMEWORK_WARN << "SlsControl::sendConfiguration skip empty command";
            return;
        }

        try {
            if (command == "settings") {
                // Create calibration and settings files (if needed)
                this->createCalibrationAndSettings(parameters);
            }

            std::vector<std::string > tokens;
            boost::split(tokens, parameters, boost::is_any_of(", "));

            int narg = tokens.size() + 1;
            char* args[narg];
            args[0] = (char*) command.c_str();
            for (size_t i = 0; i < tokens.size(); ++i)
                args[i + 1] = const_cast<char*> (tokens.at(i).c_str());

            std::string reply = m_SLS->putCommand(narg, args, pos);
            KARABO_LOG_FRAMEWORK_DEBUG << "Pos: " << pos << ". Command: " << args[0] << " " << parameters << ". Reply: " << reply;
        } catch (...) {
            KARABO_LOG_FRAMEWORK_ERROR << "Could not send configuration";
            throw; // Calling function must take care
        }

        KARABO_LOG_DEBUG << "Quitting SlsControl::sendConfiguration";
    }

    void SlsControl::preReconfigure(Hash& incomingReconfiguration) {
        KARABO_LOG_FRAMEWORK_DEBUG << "Entering SlsControl::preReconfigure";

        KARABO_LOG_FRAMEWORK_DEBUG << "Incoming reconfiguration: \n" << incomingReconfiguration;
        Hash h1 = this->filterByTags(incomingReconfiguration, "sls");
        KARABO_LOG_FRAMEWORK_DEBUG << "Filtered reconfiguration: \n" << h1;

        this->sendConfiguration(h1);

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
                this->sendConfiguration(h2);

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
