#ifndef SLS_DETECTOR_USERS_H
#define SLS_DETECTOR_USERS_H

#include <string>
#include <vector>

#include <pthread.h>

#include <boost/asio.hpp>

class detectorData;

class slsDetectorUsers {
public:

    slsDetectorUsers(int& ret, int id = 0);

    virtual ~slsDetectorUsers();

    std::string getDetectorDeveloper();

    int setOnline(int const online = -1);

    void startMeasurement();

    int stopMeasurement();

    int getDetectorStatus();

    std::string getFilePath();

    std::string setFilePath(std::string s);

    std::string getFileName();

    std::string setFileName(std::string s);

    int getFileIndex();

    int setFileIndex(int i);

    std::string getFlatFieldCorrectionDir();

    std::string setFlatFieldCorrectionDir(std::string dir);

    std::string getFlatFieldCorrectionFile();

    int setFlatFieldCorrectionFile(std::string fname = "");

    int enableFlatFieldCorrection(int i = -1);

    int enableCountRateCorrection(int i = -1);

    int enablePixelMaskCorrection(int i = -1);

    int enableAngularConversion(int i = -1);

    int enableWriteToFile(int i = -1);

    int setPositions(int nPos, double *pos);

    int getPositions(double *pos = NULL);

    int setDetectorSize(int x0 = -1, int y0 = -1, int nx = -1, int ny = -1);

    int getDetectorSize(int &x0, int &y0, int &nx, int &ny);

    int getMaximumDetectorSize(int &nx, int &ny);

    int setBitDepth(int i = -1);

    int setSettings(int isettings = -1);

    int getThresholdEnergy();

    int setThresholdEnergy(int e_eV);

    int getBeamEnergy();

    int setBeamEnergy(int e_eV);

    double setExposureTime(double t = -1, bool inseconds = false);

    double setExposurePeriod(double t = -1, bool inseconds = false);

    double setDelayAfterTrigger(double t = -1, bool inseconds = false);

    int64_t setNumberOfGates(int64_t t = -1);

    int64_t setNumberOfFrames(int64_t t = -1);

    int64_t setNumberOfCycles(int64_t t = -1);

    int setTimingMode(int pol = -1);

    int dumpConfigurationFile(std::string const fname);

    int readConfigurationFile(std::string const fname);

    int dumpDetectorSetup(std::string const fname);

    // Not part of original slsDetectorUsers
    int retrieveDetectorSetup(std::string const fname);

    // Different signature than original slsDetectorUsers
    int setDetectorType(int type);

    std::string getDetectorType();

    int setReceiverMode(int n = -1);

    void registerDataCallback(int( *userCallback)(detectorData* d, int f, int s, void*), void *pArg) {
        std::runtime_error e("slsDetectorUsers::registerDataCallback not implemented");
        throw e;
    }

    void registerRawDataCallback(int( *userCallback)(double* p, int n, void*), void *pArg) {
        std::runtime_error e("slsDetectorUsers::registerRawDataCallback not implemented");
        throw e;
    }

    virtual void initDataset(int refresh) {
    }

    virtual void addFrame(double *data, double pos, double i0, double t, std::string fname, double var) {
    }

    virtual void finalizeDataset(double *a, double *v, double *e, int &np) {
    }

    int64_t getModuleFirmwareVersion();

    int64_t getModuleSerialNumber(int imod = -1);

    int64_t getDetectorFirmwareVersion();

    int64_t getDetectorSerialNumber();

    int64_t getDetectorSoftwareVersion();

    int64_t getThisSoftwareVersion();

    void registerAcquisitionFinishedCallback(int( *func)(double, int, void*), void *pArg) {
        std::runtime_error e("slsDetectorUsers::registerAcquisitionFinishedCallback not implemented");
        throw e;
    }

    void registerGetPositionCallback(double (*func)(void*), void *arg) {
        std::runtime_error e("slsDetectorUsers::registerGetPositionCallback not implemented");
        throw e;
    }

    void registerConnectChannelsCallback(int (*func)(void*), void *arg) {
        std::runtime_error e("slsDetectorUsers::registerConnectChannelsCallback not implemented");
        throw e;
    }

    void registerDisconnectChannelsCallback(int (*func)(void*), void *arg) {
        std::runtime_error e("slsDetectorUsers::registerDisconnectChannelsCallback not implemented");
        throw e;
    }

    void registerGoToPositionCallback(int (*func)(double, void*), void *arg) {
        std::runtime_error e("slsDetectorUsers::registerGoToPositionCallback not implemented");
        throw e;
    }

    void registerGoToPositionNoWaitCallback(int (*func)(double, void*), void *arg) {
        std::runtime_error e("slsDetectorUsers::registerGoToPositionNoWaitCallback not implemented");
        throw e;
    }

    void registerGetI0Callback(double (*func)(int, void*), void *arg) {
        std::runtime_error e("slsDetectorUsers::registerGetI0Callback not implemented");
        throw e;
    }

    std::string putCommand(int narg, char *args[], int pos = -1);

    std::string getCommand(int narg, char *args[], int pos = -1);

    /************************************************************************

                             STATIC FUNCTIONS

     *********************************************************************/

    static std::string runStatusType(int s) {	\
    switch (s) {				\
    case 0: return std::string("idle");		\
    case 1: return std::string("error");	\
    case 2: return std::string("waiting");	\
    case 3: return std::string("finished");	\
    case 4: return std::string("data");		\
    case 5: return std::string("running");	\
    default: return std::string("unknown");	\
                            }
    };

    static int getDetectorSettings(std::string s) {	\
    if (s == "standard") return 0;			\
    if (s == "fast") return 1;				\
    if (s == "highgain") return 2;			\
    if (s == "dynamicgain") return 3;			\
    if (s == "lowgain") return 4;			\
    if (s == "mediumgain") return 5;			\
    if (s == "veryhighgain") return 6;			\
    return -1;
    };

    static std::string getDetectorSettings(int s) {	\
    switch (s) {					\
    case 0: return std::string("standard");		\
    case 1: return std::string("fast");			\
    case 2: return std::string("highgain");		\
    case 3: return std::string("dynamicgain");		\
    case 4: return std::string("lowgain");		\
    case 5: return std::string("mediumgain");		\
    case 6: return std::string("veryhighgain");		\
    default: return std::string("undefined");		\
                            }
    };

    static std::string getTimingMode(int f) {		\
    switch (f) {					\
    case 0: return std::string("auto");			\
    case 1: return std::string("trigger");		\
    case 2: return std::string("ro_trigger");		\
    case 3: return std::string("gating");		\
    case 4: return std::string("triggered_gating");	\
    default: return std::string("unknown");		\
                            }
    };

    static int getTimingMode(std::string s) {	\
    if (s == "auto") return 0;			\
    if (s == "trigger") return 1;		\
    if (s == "ro_trigger") return 2;		\
    if (s == "gating") return 3;		\
    if (s == "triggered_gating") return 4;	\
    return -1;
    };

private: // Simulation properties

    int m_id;
    int m_detectorType;
    bool m_online;
    int m_status;
    std::string m_filePath;
    std::string m_fileName;
    int m_fileIndex;
    std::string m_flatFieldCorrectionDir;
    std::string m_flatFieldCorrectionFile;
    bool m_enableFlatFieldCorrection;
    bool m_enableCountRateCorrection;
    bool m_enablePixelMaskCorrection;
    bool m_enableAngularConversion;
    bool m_enableWriteToFile;
    std::vector<double> m_pos;
    int m_x0, m_y0, m_nx, m_ny;
    int m_bitDepth;
    int m_settings;
    int m_thresholdEnergy;
    int m_beamEnergy;
    double m_exposureTime;
    double m_exposurePeriod;
    double m_delayAfterTrigger;
    int64_t m_numberOfGates;
    int64_t m_numberOfFrames;
    int64_t m_numberOfCycles;
    int m_timingMode;
    int m_receiverMode;
    int m_probes;
    int m_measurements;
    int m_lock;
    double m_ratecorr;
    std::string m_configFileName;
    std::string m_parametersFileName;
    std::string m_flags;
    double m_fineoff;
    std::string m_extsig[4];
    int m_master;
    std::string m_sync;
    std::string m_hostname;
    std::string m_settingsdir;
    std::string m_caldir;
    int m_port;
    int m_stopport;
    std::string m_darkimage;
    std::string m_gainimage;
    std::string m_badchannels;
    int m_threaded;
    double m_globaloff;
    std::string m_angconv;
    double m_binsize;
    double m_angdir;
    int m_moveflag;
    int m_vhighvoltage;
    std::string m_detectorip;
    std::string m_detectormac;
    int m_rx_tcpport;
    int m_rx_udpport;
    std::string m_rx_hostname;
    std::string m_rx_udpip;
    int m_r_online;
    int m_r_lock;

private:
    void configureReceiver();
    void toReceiver(const std::string& command, const std::string& parameters = "");

    void startMeasurementNoWait();
    void setRxHostname(std::string rx_hostname);

    bool m_keepRunning;
    pthread_t m_dataThread;
    static void* dataWorker(void* self);

    int64_t m_neededFrames;

};

#endif
