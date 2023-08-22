/*
 *
 * Author: <Iryna.Kozlova@xfel.eu>
 *
 * Created on July 12, 2013, 11:30 AM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_SLSCONTROL_HH
#define KARABO_SLSCONTROL_HH


#include <karabo/karabo.hpp>

#ifndef SLS_SIMULATION
#include <sls/Detector.h>
#else
#include "../slsDetectorsSimulation/Detector.h"
#endif

#include "../common/version.hh"  // provides SLSDETECTORS_PACKAGE_VERSION

/**
 * The main Karabo namespace
 */
namespace karabo {

    class SlsControl : public karabo::core::Device<> {
    public:

        KARABO_CLASSINFO(SlsControl, "SlsControl",
            SLSDETECTORS_PACKAGE_VERSION)

        explicit SlsControl(const karabo::util::Hash& config);

        virtual ~SlsControl();

        static void expectedParameters(karabo::util::Schema& expected);

        virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration) override;

    private: // Slots
        void start();
        void stop();
        void reset();

    private: // Functions
        void initialize();

        void connect(const boost::system::error_code& ec);

        void acquireBlocking(const boost::system::error_code& ec);

        void startPoll();
        void stopPoll();
        void pollHardware(const boost::system::error_code& ec);
        void pollOnce(karabo::util::Hash& h);
        virtual void pollDetectorSpecific(karabo::util::Hash& h) {
        };

        void sendBaseConfiguration();
        void sendInitialConfiguration();
        void sendConfiguration(const karabo::util::Hash& configHash);
        virtual void configureDetectorSpecific(const karabo::util::Hash& configHash) {
        };

        virtual void powerOn() {
        };

        bool isServerOnline(const std::string& host, unsigned short port, std::string& errorMsg);
        bool areDetectorsOnline();
        bool areReceiversOnline();
        bool ping(std::string host);

        void createTmpDir();

    protected:
#ifdef SLS_SIMULATION
        slsDetectorDefs::detectorType m_detectorType;
#endif
        std::shared_ptr<sls::Detector> m_SLS;
        unsigned int m_numberOfModules;
        std::vector<int> m_positions;

        void sendConfiguration(const std::string& command, const std::string& parameters = "", int pos = -1);
        void createCalibrationAndSettings(const std::string& settings);

    private: // Members
        const unsigned short m_defaultPort = 1952;

        bool m_connect;
        const unsigned int m_reconnectTime = 5000;
        boost::asio::deadline_timer m_connect_timer;

        bool m_isConfigured; // modules can be polled only after hostnames are set
        bool m_firstPoll;
        bool m_poll;
        boost::asio::deadline_timer m_poll_timer;

        boost::asio::deadline_timer m_acquire_timer;

        std::string m_tmpDir;
        unsigned int m_shm_id; // shared memory segment index
    };

} /* namespace karabo */

#endif /* KARABO_SLSCONTROL_HH */
