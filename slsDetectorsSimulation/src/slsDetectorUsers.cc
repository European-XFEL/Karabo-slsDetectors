#include <fstream>
#include <iostream>
#include <unordered_map>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp> 

#include "sls_simulation_defs.h"
#include "slsDetectorUsers.h"

namespace slsDetectorDefs {
    std::unordered_map<int, std::string> detector_type_string {
        {static_cast<int>(detectorType::GENERIC), "Undefined"},
        {static_cast<int>(detectorType::GOTTHARD), "Gotthard"},
        {static_cast<int>(detectorType::JUNGFRAU), "Jungfrau"}
    };
}


slsDetectorUsers::slsDetectorUsers(int& ret, int id) {
    m_id = 0;
    m_detectorType = static_cast<int>(detectorType::GET_DETECTOR_TYPE);
    m_online = true;
    m_status = 0; // idle
    m_filePath = "";
    m_fileName = "run";
    m_fileIndex = 0;
    m_flatFieldCorrectionDir = "";
    m_flatFieldCorrectionFile = "";
    m_enableFlatFieldCorrection = false;
    m_enableCountRateCorrection = false;
    m_enablePixelMaskCorrection = false;
    m_enableAngularConversion = false;
    m_enableWriteToFile = false;
    m_x0 = 0;
    m_y0 = 0;
    m_nx = 1280;
    m_ny = 1;
    m_bitDepth = 16;
    m_settings = 0; // standard
    m_thresholdEnergy = 0;
    m_beamEnergy = 0;
    m_exposureTime = 0.000010; // 10 us
    m_exposurePeriod = 0.001; // 1 ms
    m_delayAfterTrigger = 0.;
    m_numberOfGates = 0;
    m_numberOfFrames = 1;
    m_numberOfCycles = 0;
    m_timingMode = 0; // auto
    m_receiverMode = 0;
    m_probes = 0;
    m_measurements = 0;
    m_lock = 0; // unlocked
    m_ratecorr = 0.;
    m_configFileName = "";
    m_parametersFileName = "";
    m_flags = "";
    m_fineoff = 0.;
    m_hostname = "";
    for (int i = 0; i < 4; ++i)
        m_extsig[i] = "off";
    m_master = -1;
    m_sync = "none";
    m_settingsdir = "";
    m_caldir = "";
    m_port = 1952;
    m_stopport = 1953;
    m_darkimage = "none";
    m_gainimage = "none";
    m_badchannels = "none";
    m_threaded = 1; // threaded
    m_globaloff = 0.;
    m_angconv = "none";
    m_binsize = 0.001;
    m_angdir = 1.0;
    m_moveflag = 1;
    m_vhighvoltage = 120;
    m_detectorip = "";
    m_detectormac = "";
    m_rx_tcpport = SLS_RX_DEFAULT_PORT;
    m_rx_udpport = 50001;
    m_rx_hostname = "";
    m_rx_udpip = "";
    m_r_online = 1;
    m_r_lock = 0;

    m_keepRunning = true;
    m_neededFrames = 0;
    ret = pthread_create(&m_dataThread, NULL, dataWorker, (void*) this);
}

slsDetectorUsers::~slsDetectorUsers() {
    m_keepRunning = false;
    pthread_join(m_dataThread, NULL);
}

std::string slsDetectorUsers::getDetectorDeveloper() {
    return "XFEL-Simulation";
}

int slsDetectorUsers::setOnline(int const online) {
    if (online == 0)
        m_online = false;
    else if (online == 1)
        m_online = true;

    if (m_online)
        return 1;
    else
        return 0;
}

void slsDetectorUsers::startMeasurement() {
    this->startMeasurementNoWait();

    while (m_keepRunning) {
        // startMeasurement must block, till acquisition is done
        if (m_status != 5)
            break;
        else
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    }
}

int slsDetectorUsers::stopMeasurement() {
    m_status = 0; // idle
    m_fileIndex++; // increase file index
    this->toReceiver("stop");
}

int slsDetectorUsers::getDetectorStatus() {
    return m_status;
}

std::string slsDetectorUsers::getFilePath() {
    return m_filePath;
}

std::string slsDetectorUsers::setFilePath(std::string s) {
    m_filePath = s;
    return s;
}

std::string slsDetectorUsers::getFileName() {
    return m_fileName;
}

std::string slsDetectorUsers::setFileName(std::string s) {
    m_fileName = s;
    return s;
}

int slsDetectorUsers::getFileIndex() {
    return m_fileIndex;
}

int slsDetectorUsers::setFileIndex(int i) {
    m_fileIndex = i;
    return m_fileIndex;
}

std::string slsDetectorUsers::getFlatFieldCorrectionDir() {
    return m_flatFieldCorrectionDir;
}

std::string slsDetectorUsers::setFlatFieldCorrectionDir(std::string dir) {
    m_flatFieldCorrectionDir = dir;
    return m_flatFieldCorrectionDir;
}

std::string slsDetectorUsers::getFlatFieldCorrectionFile() {
    return m_flatFieldCorrectionFile;
}

int slsDetectorUsers::setFlatFieldCorrectionFile(std::string fname) {
    m_flatFieldCorrectionFile = fname;
    if (fname == "")
        return 0; // disable
    else
        return 1;
}

int slsDetectorUsers::enableFlatFieldCorrection(int i) {
    if (i == 0)
        m_enableFlatFieldCorrection = false; // disable
    else if (i == 1)
        m_enableFlatFieldCorrection = true; // enable

    if (m_enableFlatFieldCorrection)
        return 1;
    else
        return 0;
}

int slsDetectorUsers::enableCountRateCorrection(int i) {
    if (i == 0)
        m_enableCountRateCorrection = false; // disable
    else if (i == 1)
        m_enableCountRateCorrection = true; // enable

    if (m_enableCountRateCorrection)
        return 1;
    else
        return 0;
}

int slsDetectorUsers::enablePixelMaskCorrection(int i) {
    if (i == 0)
        m_enablePixelMaskCorrection = false; // disable
    else if (i == 1)
        m_enablePixelMaskCorrection = true; // enable

    if (m_enablePixelMaskCorrection)
        return 1;
    else
        return 0;
}

int slsDetectorUsers::enableAngularConversion(int i) {
    if (i == 0)
        m_enableAngularConversion = false; // disable
    else if (i == 1)
        m_enableAngularConversion = true; // enable

    if (m_enableAngularConversion)
        return 1;
    else
        return 0;
}

int slsDetectorUsers::enableWriteToFile(int i) {
    if (i == 0)
        m_enableWriteToFile = false; // disable
    else if (i == 1)
        m_enableWriteToFile = true; // enable

    if (m_enableWriteToFile)
        return 1;
    else
        return 0;
}

int slsDetectorUsers::setPositions(int nPos, double* pos) {
    m_pos.assign(pos, pos + nPos);
}

int slsDetectorUsers::getPositions(double* pos) {
    if (pos != NULL) {
        for (int i = 0; i < m_pos.size(); ++i)
            pos[i] = m_pos.at(i);
        return m_pos.size();
    } else {
        return 0;
    }
}

int slsDetectorUsers::setDetectorSize(int x0, int y0, int nx, int ny) {
    if (x0 >= 0)
        m_x0 = x0;
    if (y0 >= 0)
        m_y0 = y0;
    if (m_nx >= 0)
        m_nx = nx;
    if (m_ny >= 0)
        m_ny = ny;

    return 0;
}

int slsDetectorUsers::getDetectorSize(int &x0, int &y0, int &nx, int &ny) {
    x0 = m_x0;
    y0 = m_y0;
    nx = m_nx;
    ny = m_ny;

    return 0;
}

int slsDetectorUsers::getMaximumDetectorSize(int &nx, int &ny) {
    nx = 1280;
    ny = 1;
}

int slsDetectorUsers::setBitDepth(int i) {
    if (i >= 0)
        m_bitDepth = i;

    return m_bitDepth;
}

int slsDetectorUsers::setSettings(int isettings) {
    if (isettings >= 0)
        m_settings = isettings;

    return m_settings;
}

int slsDetectorUsers::getThresholdEnergy() {
    return m_thresholdEnergy;
}

int slsDetectorUsers::setThresholdEnergy(int e_eV) {
    if (e_eV >= 0)
        m_thresholdEnergy = e_eV;

    return m_thresholdEnergy;
}

int slsDetectorUsers::getBeamEnergy() {
    return m_beamEnergy;
}

int slsDetectorUsers::setBeamEnergy(int e_eV) {
    if (e_eV >= 0)
        m_beamEnergy = e_eV;

    return m_beamEnergy;
}

double slsDetectorUsers::setExposureTime(double t, bool inseconds) {
    if (t >= 0) {
        if (inseconds)
            m_exposureTime = t;
        else
            m_exposureTime = t / 1000000000.; // ns -> s
    }

    if (inseconds)
        return m_exposureTime;
    else
        return m_exposureTime * 1000000000.; // s -> ns
}

double slsDetectorUsers::setExposurePeriod(double t, bool inseconds) {
    if (t >= 0) {
        if (inseconds)
            m_exposurePeriod = t;
        else
            m_exposurePeriod = t / 1000000000.; // ns -> s
    }

    if (inseconds)
        return m_exposurePeriod;
    else
        return m_exposurePeriod * 1000000000.; // s -> ns
}

double slsDetectorUsers::setDelayAfterTrigger(double t, bool inseconds) {
    if (t >= 0) {
        if (inseconds)
            m_delayAfterTrigger = t;
        else
            m_delayAfterTrigger = t / 1000000000.; // ns -> s
    }

    if (inseconds)
        return m_delayAfterTrigger;
    else
        return m_delayAfterTrigger * 1000000000.; // s -> ns
}

int64_t slsDetectorUsers::setNumberOfGates(int64_t t) {
    if (t >= 0)
        m_numberOfGates = t;

    return m_numberOfGates;
}

int64_t slsDetectorUsers::setNumberOfFrames(int64_t t) {
    if (t >= 0)
        m_numberOfFrames = t;

    return m_numberOfFrames;
}

int64_t slsDetectorUsers::setNumberOfCycles(int64_t t) {
    if (t >= 0)
        m_numberOfCycles = t;

    return m_numberOfCycles;
}

int slsDetectorUsers::setTimingMode(int pol) {
    if (pol >= 0)
        m_timingMode = pol;

    return m_timingMode;
}

int slsDetectorUsers::dumpConfigurationFile(std::string const fname) {
    char line[MAX_LEN];
    char* args[] = {line};
    std::ofstream cfile;

    cfile.open(fname, std::ofstream::out | std::ofstream::trunc);
    if (cfile.is_open()) {
        cfile << "fname " << this->getFileName() << std::endl;
        cfile << "index " << this->getFileIndex() << std::endl;
        cfile << "dr " << this->setBitDepth() << std::endl;
        cfile << "settings " << this->getDetectorSettings(this->setSettings()) << std::endl;
        cfile << "threshold " << this->getThresholdEnergy() << std::endl;
        cfile << "exptime " << this->setExposureTime(-1, true) << std::endl;
        cfile << "period " << this->setExposurePeriod(-1, true) << std::endl;
        cfile << "delay " << this->setDelayAfterTrigger(-1, true) << std::endl;
        cfile << "gates " << this->setNumberOfGates() << std::endl;
        cfile << "frames " << this->setNumberOfFrames() << std::endl;
        cfile << "cycles " << this->setNumberOfCycles() << std::endl;
        cfile << "timing " << this->getTimingMode(m_timingMode) << std::endl;
        cfile << "fineoff " << m_fineoff << std::endl;
        cfile << "ratecorr " << m_ratecorr << std::endl;
        cfile << "flatfield " << this->getFlatFieldCorrectionFile() << std::endl;
        cfile << "badchannels " << m_badchannels << std::endl;

        // TODO more params?

        cfile.close();
        return 0;
    } else {
        return 1;
    }
}

int slsDetectorUsers::readConfigurationFile(std::string const fname) {
    char line[MAX_LEN];
    std::ifstream cfile;

    cfile.open(fname, std::ifstream::in);
    if (cfile.is_open()) {
        while (!cfile.eof()) {
            cfile.getline(line, MAX_LEN);
            std::vector<std::string> v;
            boost::algorithm::split(v, line, boost::algorithm::is_space());
            const int argc = v.size();
            char* argv[argc];
            for (int i = 0; i < argc; ++i)
                argv[i] = (char*) v.at(i).c_str();
            this->putCommand(argc, argv);
        }
        cfile.close();
    }

    return 0;
}

int slsDetectorUsers::dumpDetectorSetup(std::string const fname) {
    char line[MAX_LEN];
    char* args[] = {line};
    std::ofstream cfile;

    cfile.open(fname, std::ofstream::out | std::ofstream::trunc);
    if (cfile.is_open()) {
        cfile << "type " << getDetectorType() << "+" << std::endl;
        cfile << m_id << ":hostname " << m_hostname << std::endl;
        cfile << m_id << ":port " << m_port << std::endl;
        cfile << m_id << ":stopport " << m_stopport << std::endl;
        cfile << m_id << ":rx_tcpport " << m_rx_tcpport << std::endl;
        cfile << m_id << ":settingsdir " << m_settingsdir << std::endl;
        cfile << m_id << ":angdir " << m_angdir << std::endl;
        cfile << m_id << ":moveflag " << m_moveflag << std::endl;
        cfile << m_id << ":lock " << m_lock << std::endl;
        cfile << m_id << ":caldir " << m_caldir << std::endl;
        cfile << m_id << ":ffdir " << this->getFlatFieldCorrectionDir() << std::endl;
        for (int i = 0; i < 4; ++i)
            cfile << m_id << ":extsig:" << i << " " << m_extsig[i] << std::endl;
        cfile << m_id << ":detectorip " << m_detectorip << std::endl;
        cfile << m_id << ":detectormac " << m_detectormac << std::endl;
        cfile << m_id << ":rx_udpport " << m_rx_udpport << std::endl;
        cfile << m_id << ":rx_udpip " << m_rx_udpip << std::endl;
        cfile << m_id << ":rx_hostname " << m_rx_hostname << std::endl;
        cfile << m_id << ":outdir " << this->getFilePath() << std::endl;
        cfile << m_id << ":vhighvoltage " << m_vhighvoltage << std::endl;

        cfile << "master " << m_master << std::endl;
        cfile << "sync " << m_sync << std::endl;
        cfile << "badchannels " << m_badchannels << std::endl;
        cfile << "angconv " << m_angconv << std::endl;
        cfile << "globaloff " << m_globaloff << std::endl;
        cfile << "binsize " << m_binsize << std::endl;
        cfile << "threaded " << m_threaded << std::endl;

        // TODO more params?

        cfile.close();
        return 0;
    } else {
        return 1;
    }
}

int slsDetectorUsers::retrieveDetectorSetup(std::string const fname) {
    char line[MAX_LEN];
    std::ifstream cfile;

    cfile.open(fname, std::ifstream::in);
    if (cfile.is_open()) {
        while (!cfile.eof()) {
            cfile.getline(line, MAX_LEN);
            std::vector<std::string> v;
            boost::algorithm::split(v, line, boost::algorithm::is_space());
            const int argc = v.size();
            char* argv[argc];
            for (int i = 0; i < argc; ++i)
                argv[i] = (char*) v.at(i).c_str();
            this->putCommand(argc, argv);
        }
        cfile.close();
    }

    return 0;
}

int slsDetectorUsers::setDetectorType(int type) {
    m_detectorType = type;
    return type;
}

std::string slsDetectorUsers::getDetectorType() {
    int detType = m_detectorType;
    if (slsDetectorDefs::detector_type_string.count(detType) == 0) {
        detType = static_cast<int>(detectorType::GENERIC);
    }

    return slsDetectorDefs::detector_type_string[detType];
}

int slsDetectorUsers::setReceiverMode(int n) {
    if (n >= 0)
        m_receiverMode = n;

    return m_receiverMode;
}

int64_t slsDetectorUsers::getModuleFirmwareVersion() {
    return (int64_t) 0xFFFFFFFFFFFFFFFF;
}

int64_t slsDetectorUsers::getModuleSerialNumber(int imod) {
    return (int64_t) 0xFFFFFFFFFFFFFFFF;
}

int64_t slsDetectorUsers::getDetectorFirmwareVersion() {
    return (int64_t) 0x2F20151015;
}

int64_t slsDetectorUsers::getDetectorSerialNumber() {
    return (int64_t) 0x50C246D8DB;
}

int64_t slsDetectorUsers::getDetectorSoftwareVersion() {
    return (int64_t) 0x30020150622;
}

int64_t slsDetectorUsers::getThisSoftwareVersion() {
    return (int64_t) 0x94520150622;
}

std::string slsDetectorUsers::putCommand(int narg, char *args[], int pos) {
    if (narg == 0)
        return std::string(); // empty command

    std::string parameter, reply, value;
    std::size_t found;

    //    std::cout << "### command: " << args[0] << std::endl;

    parameter = args[0];
    found = parameter.find_first_of(":");
    if (found != std::string::npos) {
        // parameter has controller index, eg "0:"
        std::string id = parameter.substr(0, found);
        if (id != std::to_string(m_id))
            return std::string(); // command is not for this controller
        else
            parameter = parameter.substr(found + 1); // trim controller index
    }

    for (int i = 1; i < narg; ++i) {
        value.append(args[i]);
        value.append(" ");
    }
    boost::algorithm::trim_right(value);

    //    std::cout << "### parameter: " << parameter << " ### value: " << value
    //              << std::endl;

    reply = value;
    if (parameter == "config") {
        m_configFileName = value;
        this->readConfigurationFile(value);
    } else if (parameter == "parameters") {
        m_parametersFileName = value;
        this->retrieveDetectorSetup(value);
    } else if (parameter == "settings") {
        m_settings = this->getDetectorSettings(value);
    } else if (parameter == "threshold") {
        m_thresholdEnergy = std::stoi(value);
    } else if (parameter == "timing") {
        m_timingMode = this->getTimingMode(value);
    } else if (parameter == "outdir") {
        this->setFilePath(value);
    } else if (parameter == "fname") {
        this->setFileName(value);
    } else if (parameter == "index") {
        this->setFileIndex(std::stoi(value));
    } else if (parameter == "enablefwrite") {
        this->enableWriteToFile(std::stoi(value));
    } else if (parameter == "exptime") {
        this->setExposureTime(std::stod(value), true);
    } else if (parameter == "period") {
        this->setExposurePeriod(std::stod(value), true);
    } else if (parameter == "delay") {
        this->setDelayAfterTrigger(std::stod(value), true);
    } else if (parameter == "gates") {
        this->setNumberOfGates(std::stoll(value));
    } else if (parameter == "frames") {
        this->setNumberOfFrames(std::stoll(value));
    } else if (parameter == "cycles") {
        this->setNumberOfCycles(std::stoll(value));
    } else if (parameter == "probes") {
        m_probes = std::stoi(value);
    } else if (parameter == "measurements") {
        m_measurements = std::stoi(value);
    } else if (parameter == "dr") {
        this->setBitDepth(std::stoi(value));
    } else if (parameter == "flags") {
        m_flags = value;
    } else if (parameter == "lock") {
        m_lock = std::stoi(value);
    } else if (parameter == "flatfield") {
        if (value == "" || value == "none") {
            this->enableFlatFieldCorrection(0); // disable
        }
        this-> setFlatFieldCorrectionFile(value);
    } else if (parameter == "ratecorr") {
        m_ratecorr = std::stod(value);
        if (m_ratecorr < 0) {
            this->enableCountRateCorrection(0); // disable
        }
    } else if (parameter == "fineoff") {
        m_fineoff = std::stod(value);
    } else if (parameter == "positions") {
        std::vector<std::string> v;
        boost::algorithm::split(v, value, boost::algorithm::is_space());
        int nPos = std::stoi(v[0]);
        m_pos.clear();
        for (int i = 1; i < v.size(); ++i)
            m_pos.push_back(std::stod(v[i]));
        this->setPositions(nPos, m_pos.data());
    } else if (parameter == "status") {
        if (value == "start")
            this->startMeasurementNoWait();
        else if (value == "stop")
            this->stopMeasurement();
    } else if (parameter == "online") {
        this->setOnline(std::stoi(value));
    } else if (parameter == "type") {
        // Ignored
    } else if (parameter == "hostname") {
        m_hostname = value;
    } else if (parameter == "extsig:0") {
        m_extsig[0] = value;
    } else if (parameter == "extsig:1") {
        m_extsig[1] = value;
    } else if (parameter == "extsig:2") {
        m_extsig[2] = value;
    } else if (parameter == "extsig:3") {
        m_extsig[3] = value;
    } else if (parameter == "master") {
        m_master = std::stoi(value);
    } else if (parameter == "sync") {
        m_sync = value;
    } else if (parameter == "settingsdir") {
        m_settingsdir = value;
    } else if (parameter == "caldir") {
        m_caldir = value;
    } else if (parameter == "port") {
        m_port = std::stoi(value);
    } else if (parameter == "stopport") {
        m_stopport = std::stoi(value);
    } else if (parameter == "detectorip") {
        m_detectorip = value;
    } else if (parameter == "detectormac") {
        m_detectormac = value;
    } else if (parameter == "rx_tcpport") {
        m_rx_tcpport = std::stoi(value);
    } else if (parameter == "rx_udpport") {
        m_rx_udpport = std::stoi(value);
    } else if (parameter == "rx_hostname") {
        this->setRxHostname(value);
    } else if (parameter == "rx_udpip") {
        m_rx_udpip = value;
    } else if (parameter == "r_online") {
        m_r_online = std::stoi(value);
    } else if (parameter == "r_lock") {
        m_r_lock = std::stoi(value);
    } else if (parameter == "ffdir") {
        this->setFlatFieldCorrectionDir(value);
    } else if (parameter == "darkimage") {
        m_darkimage = value;
    } else if (parameter == "gainimage") {
        m_gainimage = value;
    } else if (parameter == "badchannels") {
        if (value == "" || value == "none") {
            this->enablePixelMaskCorrection(0); // disable
        }
        m_badchannels = value;
    } else if (parameter == "threaded") {
        m_threaded = std::stoi(value);
    } else if (parameter == "globaloff") {
        m_globaloff = std::stod(value);
    } else if (parameter == "angconv") {
        if (value == "" || value == "none") {
            this->enableAngularConversion(0); // disable
        }
        m_angconv = value;
    } else if (parameter == "binsize") {
        m_binsize = std::stod(value);
    } else if (parameter == "angdir") {
        m_angdir = std::stod(value);
    } else if (parameter == "moveflag") {
        m_moveflag = std::stoi(value);
    } else if (parameter == "vhighvoltage") {
        m_vhighvoltage = std::stoi(value);
    } else {
        reply = "";
    }

    // TODO more detector settings?

    // std::cout << "#### reply: " << reply << std::endl;
    return reply;
}

std::string slsDetectorUsers::getCommand(int narg, char *args[], int pos) {
    for (int i = 0; i < narg; ++i) {
        std::string cmdline = args[i];
        std::string parameter, value;
        std::size_t found;

        found = cmdline.find_first_of(" ");
        if (found == std::string::npos) {
            // parameter has no values
            parameter = cmdline;
        } else {
            parameter = cmdline.substr(0, found);
            value = cmdline.substr(found + 1);
        }

        found = parameter.find_first_of(":");
        if (found != std::string::npos) {
            // parameter has controller index, eg "0:"
            int id = std::stoi(parameter.substr(0, found));
            if (id != m_id)
                continue; // command is not for this controller
            else
                parameter = parameter.substr(found + 1); // trim controller index
        }

        //        std::cout << "### parameter: " << parameter << " ### value: " << value
        //                << std::endl;

        boost::format int2hex("%x");
        if (parameter == "config") {
            this->dumpDetectorSetup(value);
            return "";
        } else if (parameter == "parameters") {
            this->dumpDetectorSetup(value);
            return "";
        } else if (parameter == "settings") {
            return this->getDetectorSettings(this->setSettings());
        } else if (parameter == "threshold") {
            return std::to_string(m_thresholdEnergy);
        } else if (parameter == "timing") {
            return this->getTimingMode(m_timingMode);
        } else if (parameter == "outdir") {
            return this->getFilePath();
        } else if (parameter == "fname") {
            return this->getFileName();
        } else if (parameter == "index") {
            return std::to_string(this->getFileIndex());
        } else if (parameter == "enablefwrite") {
            return std::to_string(this->enableWriteToFile());
        } else if (parameter == "exptime") {
            return std::to_string(this->setExposureTime(-1, true)); // seconds
        } else if (parameter == "period") {
            return std::to_string(this->setExposurePeriod(-1, true)); // seconds
        } else if (parameter == "delay") {
            return std::to_string(this->setDelayAfterTrigger(-1, true)); // seconds
        } else if (parameter == "gates") {
            return std::to_string(this->setNumberOfGates());
        } else if (parameter == "frames") {
            return std::to_string(this->setNumberOfFrames());
        } else if (parameter == "cycles") {
            return std::to_string(this->setNumberOfCycles());
        } else if (parameter == "probes") {
            return std::to_string(m_probes);
        } else if (parameter == "measurements") {
            return std::to_string(m_measurements);
        } else if (parameter == "dr") {
            return std::to_string(this->setBitDepth());
        } else if (parameter == "flags") {
            return m_flags;
        } else if (parameter == "lock") {
            return std::to_string(m_lock);
        } else if (parameter == "lastclient") {
            return "localhost";
        } else if (parameter == "flatfield") {
            return this-> getFlatFieldCorrectionFile();
        } else if (parameter == "ratecorr") {
            return std::to_string(m_ratecorr);
        } else if (parameter == "fineoff") {
            return std::to_string(m_fineoff);
        } else if (parameter == "positions") {
            int nPos = m_pos.size();
            std::string reply = std::to_string(nPos);
            for (auto it = m_pos.cbegin(); it != m_pos.cend(); ++it)
                reply += ' ' + *it;
            return reply;
        } else if (parameter == "moduleversion") {
            std::string reply = boost::str(int2hex % this->getModuleFirmwareVersion());
            return reply;
        } else if (parameter == "modulenumber") {
            std::string reply = boost::str(int2hex % this->getModuleSerialNumber());
            return reply;
        } else if (parameter == "detectorversion") {
            std::string reply = boost::str(int2hex % this->getDetectorFirmwareVersion());
            return reply;
        } else if (parameter == "detectornumber") {
            std::string reply = boost::str(int2hex % this->getDetectorSerialNumber());
            return reply;
        } else if (parameter == "softwareversion") {
            std::string reply = boost::str(int2hex % this->getDetectorSoftwareVersion());
            return reply;
        } else if (parameter == "thisversion") {
            std::string reply = boost::str(int2hex % this->getThisSoftwareVersion());
            return reply;
        } else if (parameter == "acquire") {
            this->startMeasurementNoWait();
        } else if (parameter == "status") {
            return this->runStatusType(this->getDetectorStatus());
        } else if (parameter == "online" || parameter == "checkonline") {
            return std::to_string(this->setOnline());
        } else if (parameter == "type") {
            return this->getDetectorType();
        } else if (parameter == "hostname") {
            return m_hostname;
        } else if (parameter == "extsig:0") {
            return m_extsig[0];
        } else if (parameter == "extsig:1") {
            return m_extsig[1];
        } else if (parameter == "extsig:2") {
            return m_extsig[2];
        } else if (parameter == "extsig:3") {
            return m_extsig[3];
        } else if (parameter == "master") {
            return std::to_string(m_master);
        } else if (parameter == "sync") {
            return m_sync;
        } else if (parameter == "settingsdir") {
            return m_settingsdir;
        } else if (parameter == "caldir") {
            return m_caldir;
        } else if (parameter == "port") {
            return std::to_string(m_port);
        } else if (parameter == "stopport") {
            return std::to_string(m_stopport);
        } else if (parameter == "id") {
            return std::to_string(m_id);
        } else if (parameter == "detectorip") {
            return m_detectorip;
        } else if (parameter == "detectormac") {
            return m_detectormac;
        } else if (parameter == "rx_tcpport") {
            return std::to_string(m_rx_tcpport);
        } else if (parameter == "rx_udpport") {
            return std::to_string(m_rx_udpport);
        } else if (parameter == "rx_hostname") {
            return m_rx_hostname;
        } else if (parameter == "rx_udpip") {
            return m_rx_udpip;
        } else if (parameter == "r_online" || parameter == "r_checkonline") {
            return std::to_string(m_r_online);
        } else if (parameter == "r_lock") {
            return std::to_string(m_r_lock);
        } else if (parameter == "r_lastclient") {
            return "localhost";
        } else if (parameter == "ffdir") {
            return this->getFlatFieldCorrectionDir();
        } else if (parameter == "darkimage") {
            return m_darkimage;
        } else if (parameter == "gainimage") {
            return m_gainimage;
        } else if (parameter == "badchannels") {
            return m_badchannels;
        } else if (parameter == "threaded") {
            return std::to_string(m_threaded);
        } else if (parameter == "globaloff") {
            return std::to_string(m_globaloff);
        } else if (parameter == "angconv") {
            return m_angconv;
        } else if (parameter == "binsize") {
            return std::to_string(m_binsize);
        } else if (parameter == "angdir") {
            return std::to_string(m_angdir);
        } else if (parameter == "moveflag") {
            return std::to_string(m_moveflag);
        } else if (parameter == "vhighvoltage") {
            return std::to_string(m_vhighvoltage);
        } else if (parameter == "temp_adc") {
            return std::string("39°C");
        } else if (parameter == "temp_fpga") {
            return std::string("43°C");
        }
        // TODO more detector settings?
    }

    return std::string();
}

void slsDetectorUsers::configureReceiver() {
    // This is the minimal configuration of the receiver
    this->toReceiver("detectortype", std::to_string(m_detectorType));
    this->toReceiver("exptime", std::to_string(m_exposureTime));
    this->toReceiver("delay", std::to_string(m_delayAfterTrigger));
    this->toReceiver("period", std::to_string(m_exposurePeriod));
    this->toReceiver("outdir", m_filePath);
    this->toReceiver("fname", m_fileName);
    this->toReceiver("index", std::to_string(m_fileIndex));
    this->toReceiver("enablefwrite", std::to_string(m_enableWriteToFile));
    this->toReceiver("settings", std::to_string(m_settings));
}

void slsDetectorUsers::toReceiver(const std::string& command, const std::string& parameters) {
    std::string line;
    boost::asio::ip::tcp::iostream stream;

    // The entire sequence of I/O operations must complete within 3 seconds.
    // If an expiry occurs, the socket is automatically closed and the stream
    // becomes bad.
    stream.expires_after(boost::posix_time::seconds(3));

    // Establish a connection to the server.
    stream.connect(m_rx_hostname, std::to_string(m_rx_tcpport));

    if (parameters == "")
        line = command;
    else
        line = command + " " + parameters;
    line += ";";

    // Send the command
    stream << line;

    if (!stream) {
        std::cout << "Could not send " << command << " to receiver: " << stream.error().message() << std::endl;
    }
}

void slsDetectorUsers::startMeasurementNoWait() {
    this->configureReceiver();
    this->toReceiver("start");

    m_neededFrames = m_numberOfFrames * std::max(m_numberOfGates, 1l) * std::max(m_numberOfCycles, 1l);
    m_status = 5; // running
}

void slsDetectorUsers::setRxHostname(std::string rx_hostname) {
    m_rx_hostname = rx_hostname;
}

void* slsDetectorUsers::dataWorker(void* self) {
    slsDetectorUsers* detector = static_cast<slsDetectorUsers*> (self);

    while (detector->m_keepRunning) {
        long delay_us = 1000000 * std::max(detector->m_delayAfterTrigger, 0.);
        long exptime_us = 1000000 * std::max(detector->m_exposureTime, 0.);
        long period_us = 1000000 * std::max(detector->m_exposurePeriod, 0.);

        if (detector->m_status == 5 && detector->m_neededFrames <= 0) {
            //std::cout << "Stopping measurement" << std::endl;
            detector->stopMeasurement();
            continue;
        } else if (detector->m_status == 5) {
            boost::this_thread::sleep(boost::posix_time::microseconds(delay_us + exptime_us));
            --(detector->m_neededFrames);
            if (period_us > exptime_us)
                boost::this_thread::sleep(boost::posix_time::microseconds(period_us - exptime_us));
        } else {
            // Acquisition not running yet
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        }
    }
}
