/*
 * $Id: SlsDetector.hh 12381 2014-01-15 16:27:07Z parenti $
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

    class SlsControl : public karabo::core::Device<karabo::core::CameraFsm> {
    public:

        KARABO_CLASSINFO(SlsControl, "SlsControl", "2.2")

        /**
         * Constructor providing the initial configuration in form of a Hash object.
         * If this class is constructed using the configuration system the Hash object will
         * already be validated using the information of the expectedParameters function.
         * The configuration is provided in a key/value fashion. 
         */
        SlsControl(const karabo::util::Hash& config);

        virtual ~SlsControl();

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::util::Schema& expected);


        /**
         * Implement this function if you need to handle the reconfigured data (e.g. send to a hardware)
         * NOTE: (a) the incomingReconfiguration was validated before
         *       (b) if you can not apply the requested reconfiguration modify the incomingReconfiguration to
         *           be in sync with your current state
         *       (c) if you need not to handle the reconfigured data, there is no need to implement this function.
         *           the reconfiguration will automatically be applied to the current state.
         * @param incomingReconfiguration An externally requested reconfiguration
         */
        virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);


    private: // State-machine call-backs (override)

        void initializationStateOnEntry();

        void acquireAction();

        void stopAction();

        void errorFoundAction(const std::string& user, const std::string& detail);

        void resetAction();

        bool connectGuard() {
            return this->isDetectorOnline();
        }

    private: // Functions

        std::string getDetectorStatus();

        void stopMeasurement();

        bool isHostOnline(std::string host, unsigned short port);
        bool isDetectorOnline();
        bool isReceiverOnline();

        virtual void powerOn(){};

        void pollHardware();

        void connect();

        void initialize();

        void acquire();

        void createTmpDir();

        void createCalibrationAndSettings(const std::string& settings);

        void sendInitialConfiguration();

        void sendConfiguration(const karabo::util::Hash& configHash);

        void getPathsByTag(std::vector<std::string >& paths, const std::string& tags);

    	virtual const char* getCalibrationString() const = 0;

   	virtual const char* getSettingsString() const = 0;

    protected:

        void sendConfiguration(const std::string& command, const std::string& parameters = "");

    private: // Members

        bool m_isOnline;
        bool m_isConfigured;
        slsDetectorUsers* m_SLS;

        unsigned int m_detIdx;
        std::string m_tmpDir;

        boost::thread m_acquisitionThread;

        bool m_doPolling; // polling thread will run until this is true
        boost::thread m_pollThread;

        bool m_doConnect; // connectThread will run until this is set to false
        boost::thread m_connectThread;

    };

} /* namespace karabo */

#endif /* KARABO_SLSCONTROL_HH */
