/*
 *
 * Author: <Iryna.Kozlova@xfel.eu>
 * 
 * Created on July 12, 2013, 11:30 AM
 *
 * Copyright (c) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_SLSCONTROL_HH
#define KARABO_SLSCONTROL_HH


#include <karabo/karabo.hpp>

#ifndef SLS_SIMULATION
#include <slsdetectors/slsDetectorUsers.h>
#else
#include <slssimulation/slsDetectorUsers.h>
#endif

/**
 * The main Karabo namespace
 */
namespace karabo {

    class SlsControl : public karabo::core::Device<> {
    public:

        KARABO_CLASSINFO(SlsControl, "SlsControl", "2.2")

        SlsControl(const karabo::util::Hash& config);

        virtual ~SlsControl();

        static void expectedParameters(karabo::util::Schema& expected);

        virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);

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
        void getPathsByTag(std::vector<std::string >& paths, const std::string& tags);

        void sendBaseConfiguration();
        void sendInitialConfiguration();
        void sendConfiguration(const karabo::util::Hash& configHash);
        virtual void configureDetectorSpecific(const karabo::util::Hash& configHash) {
        };

        virtual void powerOn() {
        };

        bool isHostOnline(std::string host, unsigned short port);
        bool areDetectorsOnline();
        bool areReceiversOnline();

        void createTmpDir();
        void createCalibrationAndSettings(const std::string& settings);

        virtual const char* getCalibrationString() const = 0;
        virtual const char* getSettingsString() const = 0;

    protected:
#ifdef SLS_SIMULATION
        int m_detectorType;
#endif
        slsDetectorUsers* m_SLS;
        unsigned int m_numberOfModules;
        void sendConfiguration(const std::string& command, const std::string& parameters = "", int pos = -1);

    private: // Members
        const unsigned short m_defaultPort = 1952;

        bool m_connect;
        const unsigned int m_reconnectTime = 5000;
        boost::asio::deadline_timer m_connect_timer;

        bool m_firstPoll;
        bool m_poll;
        boost::asio::deadline_timer m_poll_timer;

        boost::asio::deadline_timer m_acquire_timer;

        std::string m_tmpDir;
        unsigned int m_id; // multi-detector index
    };

} /* namespace karabo */

#endif /* KARABO_SLSCONTROL_HH */
