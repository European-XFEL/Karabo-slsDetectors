#include <fstream>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp> 

#include "sls_simulation_defs.h"
#include "Detector.h"

namespace slsDetectorDefs {
    std::unordered_map<int, std::string> detector_type_string {
        {static_cast<int>(detectorType::GENERIC), "Undefined"},
        {static_cast<int>(detectorType::GOTTHARD), "Gotthard"},
        {static_cast<int>(detectorType::JUNGFRAU), "Jungfrau"}
    };

    std::unordered_map<int, std::string> detector_settings_string {
        {static_cast<int>(detectorSettings::STANDARD), "standard"},
        {static_cast<int>(detectorSettings::FAST), "fast"},
        {static_cast<int>(detectorSettings::HIGHGAIN), "highgain"},
        {static_cast<int>(detectorSettings::DYNAMICGAIN), "dynamicgain"},
        {static_cast<int>(detectorSettings::LOWGAIN), "lowgain"},
        {static_cast<int>(detectorSettings::MEDIUMGAIN), "mediumgain"},
        {static_cast<int>(detectorSettings::VERYHIGHGAIN), "veryhighgain"},
        {static_cast<int>(detectorSettings::DYNAMICHG0), "dynamichg0"},
        {static_cast<int>(detectorSettings::FIXGAIN1), "fixgain1"},
        {static_cast<int>(detectorSettings::FIXGAIN2), "fixgain2"},
        {static_cast<int>(detectorSettings::FORCESWITCHG1), "forceswitchg1"},
        {static_cast<int>(detectorSettings::FORCESWITCHG2), "forceswitchg2"},
        {static_cast<int>(detectorSettings::VERYLOWGAIN), "verylowgain"},
        {static_cast<int>(detectorSettings::G1_HIGHGAIN), "g1_hg"},
        {static_cast<int>(detectorSettings::G1_LOWGAIN), "g1_lg"},
        {static_cast<int>(detectorSettings::G2_HIGHCAP_HIGHGAIN), "g2_hc_hg"},
        {static_cast<int>(detectorSettings::G2_HIGHCAP_LOWGAIN), "g2_hc_lg"},
        {static_cast<int>(detectorSettings::G2_LOWCAP_HIGHGAIN), "g2_lc_hg"},
        {static_cast<int>(detectorSettings::G2_LOWCAP_LOWGAIN), "g2_lc_lg"},
        {static_cast<int>(detectorSettings::G4_HIGHGAIN), "g4_hg"},
        {static_cast<int>(detectorSettings::G4_LOWGAIN), "g4_lg"},
        {static_cast<int>(detectorSettings::UNDEFINED), "undefined"},
        {static_cast<int>(detectorSettings::UNINITIALIZED), "uninitialized"},
    };

    std::unordered_map<std::string, int> detector_settings_from_string;

    std::unordered_map<int, std::string> timing_mode_string {
        {static_cast<int>(timingMode::AUTO_TIMING), "auto"},
        {static_cast<int>(timingMode::TRIGGER_EXPOSURE), "trigger"},
        {static_cast<int>(timingMode::GATED), "gating"},
        {static_cast<int>(timingMode::BURST_TRIGGER), "burst_trigger"},
        {static_cast<int>(timingMode::TRIGGER_GATED), "trigger_gating"},
        {static_cast<int>(timingMode::NUM_TIMING_MODES), ""},
    };

    std::unordered_map<std::string, int> timing_mode_from_string;

    std::unordered_map<int, std::string> run_status_string {
        {static_cast<int>(runStatus::IDLE), "idle"},
        {static_cast<int>(runStatus::ERROR), "error"},
        {static_cast<int>(runStatus::WAITING), "waiting"},
        {static_cast<int>(runStatus::RUN_FINISHED), "finished"},
        {static_cast<int>(runStatus::TRANSMITTING), "transmitting"},
        {static_cast<int>(runStatus::RUNNING), "running"},
        {static_cast<int>(runStatus::STOPPED), "stopped"},
    };

}

namespace sls {

    using namespace slsDetectorDefs;

    Detector::Detector(int shm_id) {

        m_shm_id = shm_id;

        this->freeSharedMemory();

        for (const auto& kv : detector_settings_string) {
            detector_settings_from_string[kv.second] = kv.first;
        }

        for (const auto& kv : timing_mode_string) {
            timing_mode_from_string[kv.second] = kv.first;
        }

        m_keepRunning = true;
        m_neededFrames = 0;
        const int ret = pthread_create(&m_dataThread, NULL, dataWorker, (void*) this);
        if (ret != 0) {
            throw std::runtime_error("pthread_create returned error code " + std::to_string(ret));
        }
    }

    Detector::~Detector() {
        m_keepRunning = false;
        pthread_join(m_dataThread, NULL);
    }

    void Detector::freeSharedMemory() {
        m_hostname = {};
        m_port = 1952;
        m_stopport = 1953;
        m_settings = {};
        m_detectorType = detectorType::GENERIC;
        m_status = runStatus::IDLE;
        m_filePath = {};
        m_fileName = {};
        m_fileIndex = {};
        m_enableWriteToFile = false;
        m_exposureTime = ns(10000); // 10 us
        m_exposurePeriod = ns(1000000); // 1 ms
        m_delayAfterTrigger = ns(0);
        m_numberOfFrames = 1;
        m_numberOfTriggers = 1;
        m_numberOfGates = 1;
        m_timingMode = timingMode::AUTO_TIMING;
        for (int i = 0; i < 4; ++i) {
            m_extsig[i] = "trigger_in_rising_edge";
        }
        m_settingspath = "";
        m_badchannels = "none";
        m_highvoltage = {};
        m_powerchip = {};
        m_udp_srcip = "";
        m_udp_srcmac = "";
        m_rx_hostname = "";
        m_rx_tcpport = SLS_RX_DEFAULT_PORT;
        m_udp_dstip = "";
        m_udp_dstport = 50001;
    }

    void Detector::loadConfig(const std::string &fname) {
        this->freeSharedMemory();
        this->loadParameters(fname);
    }

    void Detector::loadParameters(const std::string &fname) {
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
                for (int i = 0; i < argc; ++i) {
                    argv[i] = const_cast<char*>(v[i].c_str());
                }
                this->putCommand(argc, argv);
            }
            cfile.close();
        }
    }

    void Detector::loadParameters(const std::vector<std::string> &parameters) {
        for (const std::string& command : parameters) {
            std::vector<std::string> v;
            boost::algorithm::split(v, command, boost::algorithm::is_space());
            const int argc = v.size();
            char* argv[argc];
            for (int i = 0; i < argc; ++i) {
                argv[i] = const_cast<char*>(v[i].c_str());
            }
            this->putCommand(argc, argv);
        }
    }

    Result<std::string> Detector::getHostname(Positions pos) const {
        if (pos.size() == 0) {
            return m_hostname;
        } else {
            std::vector<std::string> hostname;
            for (const int p : pos) {
                if (p < m_hostname.size()) {
                    hostname.push_back(m_hostname[p]);
                } else {
                    throw std::runtime_error("Detector::getHostname : position not found!");
                }
            }
            return hostname;
        }
    }

    void Detector::setHostname(const std::vector<std::string> &hostname) {
        this->freeSharedMemory();
        const size_t size = hostname.size();
        m_hostname = hostname;
        m_settings = std::vector<slsDetectorDefs::detectorSettings>(size, detectorSettings::UNINITIALIZED);
        m_highvoltage = std::vector<int>(size, 0);
        m_powerchip = std::vector<bool>(size, false);
        m_filePath = std::vector<std::string>(size, "/tmp");
        m_fileName = std::vector<std::string>(size, "run");
        m_fileIndex = std::vector<int64_t>(size, 0);
    };

    Result<int64_t> Detector::getFirmwareVersion(Positions pos) const {
        const int64_t ver = 0x200724; // 2020.07.24
        if (pos.size() == 0) {
            return {};
        } else {
            return std::vector<int64_t>(pos.size(), ver);
        }
    }

    Result<int64_t> Detector::getDetectorServerVersion(Positions pos) const {
        const int64_t ver = 0x201117; // 2011.11.17
        if (pos.size() == 0) {
            return {};
        } else {
            return std::vector<int64_t>(pos.size(), ver);
        }
    }

    Result<int64_t> Detector::getSerialNumber(Positions pos) const {
        const int64_t sn_base = 0xFFF20000;
        if (pos.size() == 0) {
            return {};
        } else {
            std::vector<int64_t> sn;
            for (const auto p : pos) {
                sn.push_back(sn_base + p);
            }
            return sn;
        }
    }

    Result<int64_t> Detector::getReceiverVersion(Positions pos) const {
        const int64_t ver = 0x201125; // 2020.11.25
        if (pos.size() == 0) {
            return {ver};
        } else {
            return std::vector<int64_t>(pos.size(), ver);
        }
    }

    Result<slsDetectorDefs::detectorType> Detector::getDetectorType(Positions pos) const {
        if (pos.size() == 0) {
            return {m_detectorType};
        } else {
            return std::vector<slsDetectorDefs::detectorType>(pos.size(), m_detectorType);
        }
    }

    std::vector<slsDetectorDefs::detectorSettings> Detector::getSettingsList() const {
        if (m_detectorType == JUNGFRAU) {
            return {DYNAMICGAIN, DYNAMICHG0, FIXGAIN1, FIXGAIN2, FORCESWITCHG1, FORCESWITCHG2};
        } else if (m_detectorType == GOTTHARD) {
            return {DYNAMICGAIN, HIGHGAIN, LOWGAIN, MEDIUMGAIN, VERYHIGHGAIN};
        } else if (m_detectorType == GOTTHARD2) {
            return {DYNAMICGAIN, FIXGAIN1, FIXGAIN2};
        } else {
            return {};
        }
    }

    Result<slsDetectorDefs::detectorSettings> Detector::getSettings(Positions pos) const {
        if (pos.size() == 0) {
            return m_settings;
        } else {
            std::vector<slsDetectorDefs::detectorSettings> settings;
            for (const int p : pos) {
                if (p < m_settings.size()) {
                    settings.push_back(m_settings[p]);
                } else {
                    throw std::runtime_error("Detector::getSettings : position not found!");
                }
            }
            return settings;
        }
    }

    void Detector::setSettings(slsDetectorDefs::detectorSettings value, Positions pos) {
        if (pos.size() == 0) {
            m_settings.assign(this->size(), value);
        } else {
            for (const int p : pos) {
                if (p < m_settings.size()) {
                    m_settings[p] = value;
                } else {
                    throw std::runtime_error("Detector::setSettings : position not found!");
                }
            }
        }
    }

    Result<int64_t> Detector::getNumberOfFrames(Positions pos) const {
        if (pos.size() == 0) {
            return {m_numberOfFrames};
        } else {
            return std::vector<int64_t>(pos.size(), m_numberOfFrames);
        }
    }

    void Detector::setNumberOfFrames(int64_t value) {
        if (value >= 0) m_numberOfFrames = value;
    }

    Result<int64_t> Detector::getNumberOfTriggers(Positions pos) const {
        if (pos.size() == 0) {
            return {m_numberOfTriggers};
        } else {
            return std::vector<int64_t>(pos.size(), m_numberOfTriggers);
        }
    }

    void Detector::setNumberOfTriggers(int64_t value) {
        if (value >= 0) m_numberOfTriggers = value;
    }

    Result<int64_t> Detector::getNumberOfGates(Positions pos) const {
        if (pos.size() == 0) {
            return {m_numberOfGates};
        } else {
            return std::vector<int64_t>(pos.size(), m_numberOfGates);
        }
    }

    void Detector::setNumberOfGates(int64_t value) {
        if (value >= 0) m_numberOfGates = value;
    }

    Result<ns> Detector::getExptime(Positions pos) const {
        if (pos.size() == 0) {
            return {m_exposureTime};
        } else {
            return std::vector<ns>(pos.size(), m_exposureTime);
        }
    }

    void Detector::setExptime(ns t, Positions pos) {
        m_exposureTime = t;
    }

    Result<ns> Detector::getPeriod(Positions pos) const {
        if (pos.size() == 0) {
            return {m_exposurePeriod};
        } else {
            return std::vector<ns>(pos.size(), m_exposurePeriod);
        }
    }

    void Detector::setPeriod(ns t, Positions pos) {
        m_exposurePeriod = t;
    }

    Result<ns> Detector::getDelayAfterTrigger(Positions pos) const {
        if (pos.size() == 0) {
            return {m_delayAfterTrigger};
        } else {
            return std::vector<ns>(pos.size(), m_delayAfterTrigger);
        }
    }

    void Detector::setDelayAfterTrigger(ns value, Positions pos) {
        m_delayAfterTrigger = value;
    }

    Result<int> Detector::getHighVoltage(Positions pos) const {
        if (pos.size() == 0) {
            return m_highvoltage;
        } else {
            std::vector<int> highvoltage;
            for (const int p : pos) {
                if (p < m_highvoltage.size()) {
                    highvoltage.push_back(m_highvoltage[p]);
                } else {
                    throw std::runtime_error("Detector::getHighVoltage : position not found!");
                }
            }
            return highvoltage;
        }
    }

    void Detector::setHighVoltage(int value, Positions pos) {
        if (pos.size() == 0) {
            m_highvoltage.assign(this->size(), value);
        } else {
            for (const int p : pos) {
                if (p < m_highvoltage.size()) {
                    m_highvoltage[p] = value;
                } else {
                    throw std::runtime_error("Detector::setHighVoltage : position not found!");
                }
            }
        }
    }

    Result<bool> Detector::getPowerChip(Positions pos) const {
        if (pos.size() == 0) {
            return m_powerchip;
        } else {
            std::vector<bool> powerchip;
            for (const int p : pos) {
                if (p < m_powerchip.size()) {
                    powerchip.push_back(m_powerchip[p]);
                } else {
                    throw std::runtime_error("Detector::getPowerChip : position not found!");
                }
            }
            return powerchip;
        }
    }

    void Detector::setPowerChip(bool on, Positions pos) {
       if (pos.size() == 0) {
            m_powerchip.assign(this->size(), on);
        } else {
            for (const int p : pos) {
                if (p < m_powerchip.size()) {
                    m_powerchip[p] = on;
                } else {
                    throw std::runtime_error("Detector::setPowerChip : position not found!");
                }
            }
        }
    }


    std::vector<slsDetectorDefs::dacIndex> Detector::getTemperatureList() const {
        if (m_detectorType == JUNGFRAU || m_detectorType == GOTTHARD || m_detectorType == GOTTHARD2) {
            return {dacIndex::TEMPERATURE_ADC, dacIndex::TEMPERATURE_FPGA};
        } else {
            return {};
        }
    }

    Result<int> Detector::getTemperature(slsDetectorDefs::dacIndex index, Positions pos) const {
        return std::vector<int>(pos.size(), 40);
    }

    void Detector::acquire() {
        this->startMeasurementNoWait();

        while (m_keepRunning) {
            // startMeasurement must block, till acquisition is done
            if (m_status != slsDetectorDefs::runStatus::RUNNING)
                break;
            else
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        }
    }

    void Detector::stopDetector() {
        m_status = slsDetectorDefs::runStatus::IDLE;
        for (int i = 0; i < this->size() ; ++i) {
            m_fileIndex[i] += 1; // increase file index
        }
        this->toReceiver("stop");
    }

    Result<slsDetectorDefs::runStatus> Detector::getDetectorStatus(Positions pos) const {
        if (pos.size() == 0) {
            return {m_status};
        } else {
            return std::vector<slsDetectorDefs::runStatus>(pos.size(), m_status);
        }
    }

    Result<std::string> Detector::getFilePath(Positions pos) const {
        if (pos.size() == 0) {
            return m_filePath;
        } else {
            std::vector<std::string> fpath;
            for (const int p : pos) {
                if (p < m_filePath.size()) {
                    fpath.push_back(m_filePath[p]);
                } else {
                    throw std::runtime_error("Detector::getFilePath : position not found!");
                }
            }
            return fpath;
        }
    }

    void Detector::setFilePath(const std::string &fpath, Positions pos) {
        if (pos.size() == 0) {
            m_filePath.assign(this->size(), fpath);
        } else {
            for (const int p : pos) {
                if (p < m_filePath.size()) {
                    m_filePath[p] = fpath;
                } else {
                    throw std::runtime_error("Detector::setFilePath : position not found!");
                }
            }
        }
    }

    Result<std::string> Detector::getFileNamePrefix(Positions pos) const {
        if (pos.size() == 0) {
            return m_fileName;
        } else {
            std::vector<std::string> fname;
            for (const int p : pos) {
                if (p < m_fileName.size()) {
                    fname.push_back(m_fileName[p]);
                } else {
                    throw std::runtime_error("Detector::getFileName : position not found!");
                }
            }
            return fname;
        }
    }

    void Detector::setFileNamePrefix(const std::string &fname, Positions pos) {
        if (pos.size() == 0) {
            m_fileName.assign(this->size(), fname);
        } else {
            for (const int p : pos) {
                if (p < m_fileName.size()) {
                    m_fileName[p] = fname;
                } else {
                    throw std::runtime_error("Detector::setFileName : position not found!");
                }
            }
        }
    }

    Result<int64_t> Detector::getAcquisitionIndex(Positions pos) const {
        if (pos.size() == 0) {
            return m_fileIndex;
        } else {
            std::vector<int64_t> findex;
            for (const int p : pos) {
                if (p < m_fileIndex.size()) {
                    findex.push_back(m_fileIndex[p]);
                } else {
                    throw std::runtime_error("Detector::getAcquisitionIndex : position not found!");
                }
            }
            return findex;
        }
    }

    void Detector::setAcquisitionIndex(int64_t i, Positions pos) {
        if (pos.size() == 0) {
            m_fileIndex.assign(this->size(), i);
        } else {
            for (const int p : pos) {
                if (p < m_fileIndex.size()) {
                    m_fileIndex[p] = i;
                } else {
                    throw std::runtime_error("Detector::setAcquisitionIndex : position not found!");
                }
            }
        }
    }

    std::vector<slsDetectorDefs::timingMode> Detector::getTimingModeList() const {
        if (m_detectorType == GOTTHARD || m_detectorType == JUNGFRAU || m_detectorType == GOTTHARD2) {
            return {timingMode::AUTO_TIMING, timingMode::TRIGGER_EXPOSURE};
        } else {
            return {};
        }
    }

    Result<slsDetectorDefs::timingMode> Detector::getTimingMode(Positions pos) const {
        if (pos.size() == 0) {
            return {m_timingMode};
        } else {
            return std::vector<slsDetectorDefs::timingMode>(pos.size(), m_timingMode);
        }
    }

    void Detector::setTimingMode(slsDetectorDefs::timingMode value, Positions pos) {
        m_timingMode = value;
    }

    Result<int> Detector::getTemperatureEvent(Positions pos) const {
        return std::vector<int>(pos.size(), 0);
    }

    void Detector::resetTemperatureEvent(Positions pos) {
    }

    Result<uint32_t> Detector::readRegister(uint32_t addr, Positions pos) const {
        if (m_register.find(addr) == m_register.end()) {
            return std::vector<uint32_t>(pos.size(), 0);
        } else {
            const uint32_t& val = m_register.find(addr)->second;
            return std::vector<uint32_t>(pos.size(), val);
        }
    }

    void Detector::writeRegister(uint32_t addr, uint32_t val, Positions pos) {
        m_register[addr] = val;
    }

    int Detector::dumpDetectorSetup(std::string const fname) {
        char line[MAX_LEN];
        char* args[] = {line};
        std::ofstream cfile;

        cfile.open(fname, std::ofstream::out | std::ofstream::trunc);
        if (cfile.is_open()) {
            cfile << "hostname ";
            for (int i = 0; i < m_hostname.size(); ++ i) {
                cfile << m_hostname[i] << "+";
            }
            cfile << std::endl;

            cfile << "port " << m_port << std::endl;
            cfile << "stopport " << m_stopport << std::endl;
            cfile << "rx_tcpport " << m_rx_tcpport << std::endl;
            cfile << "settingspath " << m_settingspath << std::endl; // used to be "settingsdir"
            for (int i = 0; i < 4; ++i)
                cfile << "extsig " << i << " " << m_extsig[i] << std::endl;
            cfile << "udp_srcip " << m_udp_srcip << std::endl; // used to be "detectorip"
            cfile << "udp_srcmac " << m_udp_srcmac << std::endl; // used to be "detectormac"
            cfile << "udp_dstport " << m_udp_dstport << std::endl;
            cfile << "udp_dstip " << m_udp_dstip << std::endl; // used to be "rx_udpip"
            cfile << "rx_hostname " << m_rx_hostname << std::endl;
            cfile << "badchannels " << m_badchannels << std::endl;

            for (int i = 0; i < m_hostname.size(); ++ i) {
                cfile << i << ":settings " << detector_settings_string[m_settings[i]] << std::endl;
                cfile << i << ":highvoltage " << m_highvoltage[i] << std::endl;
                cfile << i << ":fpath " << m_filePath[i] << std::endl;
                cfile << i << ":fname " << m_fileName[i] << std::endl;
                cfile << i << ":findex " << m_fileIndex[i] << std::endl;
            }

            cfile.close();
            return 0;
        } else {
            return 1;
        }
    }

    int Detector::retrieveDetectorSetup(std::string const fname) {
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

    slsDetectorDefs::detectorType Detector::setDetectorType(slsDetectorDefs::detectorType type) {
        m_detectorType = type;
        return type;
    }

    std::string Detector::putCommand(int narg, char *args[]) {
    // A definition of commands can be found in:
    // https://slsdetectorgroup.github.io/devdoc/commandline.html#commands
        if (narg == 0) {
            return std::string(); // empty command
        }

        std::string parameter, reply, value;
        std::size_t found;

        // std::cout << "### command: " << args[0] << std::endl;

        int pos = -1; // means write to "all" controllers
        parameter = args[0];
        found = parameter.find_first_of(":");
        if (found != std::string::npos) {
            // parameter has controller index, eg "0:"
            pos = std::stoi(parameter.substr(0, found));
            parameter = parameter.substr(found + 1); // trim controller index
        }

        if (pos >= this->size()) {
            return std::string(); // controller not defined
        }

        for (int i = 1; i < narg; ++i) {
            value.append(args[i]);
            value.append(" ");
        }
        boost::algorithm::trim_right(value);

        // std::cout << "### parameter: " << parameter << " ### value: " << value
        //          << std::endl;

        reply = value;
        if (parameter == "config") {
            this->loadConfig(value);
        } else if (parameter == "parameters") {
            this->retrieveDetectorSetup(value);
        } else if (parameter == "settings") {
            const slsDetectorDefs::detectorSettings settings = static_cast<slsDetectorDefs::detectorSettings>(detector_settings_from_string[value]);
            if (pos > 0) {
                m_settings[pos] = settings;
            } else {
                m_settings.assign(this->size(), settings);
            }
        } else if (parameter == "timing") {
            m_timingMode = static_cast<timingMode>(timing_mode_from_string[value]);
        } else if (parameter == "fpath") {
            if (pos > 0) {
                m_filePath[pos] = value;
            } else {
                m_filePath.assign(this->size(), value);
            }
        } else if (parameter == "fname") {
            if (pos > 0) {
                m_fileName[pos] = value;
            } else {
                m_fileName.assign(this->size(), value);
            }
        } else if (parameter == "findex") {
            const int64_t findex = std::stoll(value);
            if (pos > 0) {
                m_fileIndex[pos] = findex;
            } else {
                m_fileIndex.assign(this->size(), findex);
            }
        } else if (parameter == "fwrite") {
            m_enableWriteToFile = std::stoi(value);
        } else if (parameter == "exptime") {
            // Default unit on command line is second
            const long exptime_ns = 1.e9 * std::stof(value);
            m_exposureTime = ns(exptime_ns);
        } else if (parameter == "period") {
            // Default unit on command line is second
            const long period_ns = 1.e9 * std::stof(value);
            m_exposurePeriod = ns(period_ns);
        } else if (parameter == "delay") {
            // Default unit on command line is second
            const long delay_ns = 1.e9 * std::stof(value);
            m_delayAfterTrigger = ns(delay_ns);
        } else if (parameter == "gates") {
            this->setNumberOfGates(std::stoll(value));
        } else if (parameter == "frames") {
            this->setNumberOfFrames(std::stoll(value));
        } else if (parameter == "triggers") {
            this->setNumberOfTriggers(std::stoll(value));
        } else if (parameter == "status") {
            if (value == "start")
                this->startMeasurementNoWait();
            else if (value == "stop")
                this->stopDetector();
        } else if (parameter == "type") {
            // Ignored
        } else if (parameter == "hostname") {
            this->freeSharedMemory();
            boost::algorithm::trim_if(value, boost::algorithm::is_any_of("+"));
            std::vector<std::string> hostname;
            boost::algorithm::split(hostname, value, boost::algorithm::is_any_of("+"));
            this->setHostname(hostname);
        } else if (parameter == "extsig") {
            if (narg >= 3) {
                const int n_signal = std::stoi(args[1]);
                if (0 <= n_signal && n_signal <= 3) {
                    m_extsig[n_signal] = args[2];
                }
            }
        } else if (parameter == "settingspath") {
            m_settingspath = value;
        } else if (parameter == "port") {
            m_port = std::stoi(value);
        } else if (parameter == "stopport") {
            m_stopport = std::stoi(value);
        } else if (parameter == "udp_srcip") {
            m_udp_srcip = value;
        } else if (parameter == "udp_srcmac") {
            m_udp_srcmac = value;
        } else if (parameter == "rx_tcpport") {
            m_rx_tcpport = std::stoi(value);
        } else if (parameter == "udp_dstport") {
            m_udp_dstport = std::stoi(value);
        } else if (parameter == "rx_hostname") {
            this->setRxHostname(value);
        } else if (parameter == "udp_dstip") {
            m_udp_dstip = value;
        } else if (parameter == "badchannels") {
            m_badchannels = value;
        } else if (parameter == "highvoltage") {
            int highvoltage = std::stoi(value);
            if (pos > 0) {
                m_highvoltage[pos] = highvoltage;
            } else {
                m_highvoltage.assign(this->size(), highvoltage);
            }
        } else {
            reply = "";
        }

        // std::cout << "#### reply: " << reply << std::endl;
        return reply;
    }

    std::string Detector::getCommand(int narg, char *args[]) {
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

            int pos = 0;
            found = parameter.find_first_of(":");
            if (found != std::string::npos) {
                // parameter has controller index, eg "0:"
                pos = std::stoi(parameter.substr(0, found));
                parameter = parameter.substr(found + 1); // trim controller index
            }

            if (pos >= this->size()) {
                return std::string(); // controller not defined
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
                return detector_settings_string[m_settings[pos]];
            } else if (parameter == "timing") {
                return timing_mode_string[static_cast<int>(m_timingMode)];
            } else if (parameter == "timinglist") {
                std::vector<slsDetectorDefs::timingMode> timinglist = this->getTimingModeList();
                std::stringstream ss;
                for (const auto& mode : timinglist) {
                    ss << timing_mode_string[mode] << " ";
                }
                return ss.str();
            } else if (parameter == "fpath") {
                return m_filePath[pos];
            } else if (parameter == "fname") {
                return m_fileName[pos];
            } else if (parameter == "findex") {
                return std::to_string(m_fileIndex[pos]);
            } else if (parameter == "fwrite") {
                return std::to_string(m_enableWriteToFile);
            } else if (parameter == "exptime") {
                const float exptime_s = m_exposureTime.count() * 1.e-9; // ns -> s
                return std::to_string(exptime_s);
            } else if (parameter == "period") {
                const float period_s = m_exposurePeriod.count() * 1.e-9; // ns -> s
                return std::to_string(period_s);
            } else if (parameter == "delay") {
                const float delay_s = m_delayAfterTrigger.count() * 1.e-9; // ns -> s
                return std::to_string(delay_s);
            } else if (parameter == "gates") {
                return std::to_string(m_numberOfGates);
            } else if (parameter == "frames") {
                return std::to_string(m_numberOfFrames);
            } else if (parameter == "triggers") {
                return std::to_string(m_numberOfTriggers);
            } else if (parameter == "lastclient") {
                return "localhost";
            } else if (parameter == "firmwareversion") {
                std::string reply = boost::str(int2hex % this->getFirmwareVersion()[pos]);
                return reply;
            } else if (parameter == "serialnumber") {
                std::string reply = boost::str(int2hex % this->getSerialNumber()[pos]);
                return reply;
            } else if (parameter == "detectorserverversion") {
                std::string reply = boost::str(int2hex % this->getDetectorServerVersion()[pos]);
                return reply;
            } else if (parameter == "clientversion") {
                std::string reply = boost::str(int2hex % this->getClientVersion());
                return reply;
            } else if (parameter == "rx_version") {
                std::string reply = boost::str(int2hex % this->getReceiverVersion()[pos]);
                return reply;
            } else if (parameter == "acquire") {
                this->startMeasurementNoWait();
            } else if (parameter == "status") {
                return slsDetectorDefs::run_status_string[m_status];
            } else if (parameter == "type") {
                return slsDetectorDefs::detector_type_string[m_detectorType];
            } else if (parameter == "hostname") {
                std::stringstream ss;
                for (const auto& host : m_hostname) {
                    ss << host << "+";
                }
                return ss.str();
            } else if (parameter == "extsig") {
                if (narg >= 2) {
                    const int n_signal = std::stoi(args[1]);
                    if (0 <= n_signal && n_signal <= 3) {
                        return m_extsig[n_signal];
                    }
                }
                return "";
            } else if (parameter == "settingspath") {
                return m_settingspath;
            } else if (parameter == "port") {
                return std::to_string(m_port);
            } else if (parameter == "stopport") {
                return std::to_string(m_stopport);
            } else if (parameter == "udp_srcip") {
                return m_udp_srcip;
            } else if (parameter == "udp_srcmac") {
                return m_udp_srcmac;
            } else if (parameter == "rx_tcpport") {
                return std::to_string(m_rx_tcpport);
            } else if (parameter == "udp_dstport") {
                return std::to_string(m_udp_dstport);
            } else if (parameter == "rx_hostname") {
                return m_rx_hostname;
            } else if (parameter == "udp_dstip") {
                return m_udp_dstip;
            } else if (parameter == "rx_lastclient") {
                return "localhost";
            } else if (parameter == "badchannels") {
                return m_badchannels;
            } else if (parameter == "highvoltage") {
                return std::to_string(m_highvoltage[pos]);
            } else if (parameter == "temp_adc") {
                const int temperature = this->getTemperature(dacIndex::TEMPERATURE_ADC)[pos];
                return std::to_string(temperature);
            } else if (parameter == "temp_fpga") {
                const int temperature = this->getTemperature(dacIndex::TEMPERATURE_FPGA)[pos];
                return std::to_string(temperature);
            }
        }

        return std::string();
    }

    void Detector::configureReceiver() {
        // This is the minimal configuration of the receiver
        this->toReceiver("detectortype", std::to_string(m_detectorType));
        this->toReceiver("exptime", std::to_string(1.e-9 * m_exposureTime.count())); // s
        this->toReceiver("delay", std::to_string(1.e-9 * m_delayAfterTrigger.count())); // s
        this->toReceiver("period", std::to_string(1.e-9 * m_exposurePeriod.count())); // s
        this->toReceiver("fpath", m_filePath[0]); // XXX support for multiple receivers
        this->toReceiver("fname", m_fileName[0]); // XXX
        this->toReceiver("findex", std::to_string(m_fileIndex[0])); // XXX
        this->toReceiver("fwrite", std::to_string(m_enableWriteToFile));
        if (m_settings.size() > 0) {
            this->toReceiver("settings", std::to_string(m_settings[0]));
        } else {
            this->toReceiver("settings", std::to_string(detectorSettings::UNINITIALIZED));
        }
    }

    void Detector::toReceiver(const std::string& command, const std::string& parameters) {
        std::string line;
        boost::asio::ip::tcp::iostream stream;

        // The entire sequence of I/O operations must complete within 3 seconds.
        // If an expiry occurs, the socket is automatically closed and the stream
        // becomes bad.
#if BOOST_VERSION >= 106800 // i.e. Karabo >= 2.11
        stream.expires_after(std::chrono::duration<int>(3));
#else
        stream.expires_after(boost::posix_time::seconds(3));
#endif

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

    void Detector::startMeasurementNoWait() {
        this->configureReceiver();
        this->toReceiver("start");

        m_neededFrames = m_numberOfFrames * std::max(m_numberOfGates, 1l) * std::max(m_numberOfTriggers, 1l);
        m_status = slsDetectorDefs::runStatus::RUNNING;
    }

    void Detector::setRxHostname(std::string rx_hostname) {
        m_rx_hostname = rx_hostname;
    }

    void* Detector::dataWorker(void* self) {
        Detector* detector = static_cast<Detector*> (self);

        while (detector->m_keepRunning) {
            long delay_us = detector->m_delayAfterTrigger.count() / 1000;
            long exptime_us = detector->m_exposureTime.count() / 1000;
            long period_us = detector->m_exposurePeriod.count() / 1000;

            if (detector->m_status == slsDetectorDefs::runStatus::RUNNING && detector->m_neededFrames <= 0) {
                //std::cout << "Stopping measurement" << std::endl;
                detector->stopDetector();
                continue;
            } else if (detector->m_status == slsDetectorDefs::runStatus::RUNNING) {
                boost::this_thread::sleep(boost::posix_time::microseconds(delay_us + exptime_us));
                --(detector->m_neededFrames);
                if (period_us > exptime_us)
                    boost::this_thread::sleep(boost::posix_time::microseconds(period_us - exptime_us));
            } else {
                // Acquisition not running yet
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            }
        }

        return nullptr;
    }

} // namespace sls
