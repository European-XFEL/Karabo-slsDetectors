/*
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef SLS_DETECTOR_H
#define SLS_DETECTOR_H

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#include <pthread.h>
}

#include <boost/asio.hpp>

#include "sls_simulation_defs.h"

namespace sls {
    using ns = std::chrono::nanoseconds;

    // forward declarations
    class detectorData;


    class Detector {
       public:
        Detector(int shm_id = 0);

        virtual ~Detector();

        void freeSharedMemory();

        void loadConfig(const std::string& fname);

        void loadParameters(const std::string& fname);

        void loadParameters(const std::vector<std::string>& parameters);

        Result<std::string> getHostname(Positions pos = {}) const;

        void setHostname(const std::vector<std::string>& hostname);

        int getShmId() const {
            return m_shm_id;
        }

        std::string getPackageVersion() const {
            return "7.0.1";
        } // 7.0.1 API

        std::string getClientVersion() const {
            return "7.0.1";
        }

        Result<int64_t> getFirmwareVersion(Positions pos = {}) const;

        Result<std::string> getDetectorServerVersion(Positions pos = {}) const;

        Result<int64_t> getSerialNumber(Positions pos = {}) const;

        Result<std::string> getReceiverVersion(Positions pos = {}) const;

        Result<slsDetectorDefs::detectorType> getDetectorType(Positions pos = {}) const;

        int size() const {
            return m_hostname.size();
        }

        bool empty() const;

        /** list of possible settings for this detector */
        std::vector<slsDetectorDefs::detectorSettings> getSettingsList() const;

        Result<slsDetectorDefs::detectorSettings> getSettings(Positions pos = {}) const;

        void setSettings(slsDetectorDefs::detectorSettings value, Positions pos = {});

        /**************************************************
         *                                                *
         *    Callbacks                                   *
         *                                                *
         * ************************************************/

        void registerAcquisitionFinishedCallback(void (*func)(double, int, void*), void* pArg) {
            throw std::runtime_error("Detector::registerAcquisitionFinishedCallback not implemented");
        }

        void registerDataCallback(void (*func)(detectorData*, uint64_t, uint32_t, void*), void* pArg) {
            throw std::runtime_error("Detector::registerDataCallback not implemented");
        }

        /**************************************************
         *                                                *
         *    Acquisition Parameters                      *
         *                                                *
         * ************************************************/

        Result<int64_t> getNumberOfFrames(Positions pos = {}) const;

        void setNumberOfFrames(int64_t value);

        Result<int64_t> getNumberOfTriggers(Positions pos = {}) const;

        void setNumberOfTriggers(int64_t value);

        Result<int64_t> getNumberOfGates(Positions pos = {}) const;

        void setNumberOfGates(int64_t value);

        Result<ns> getExptime(Positions pos = {}) const;

        void setExptime(ns t, Positions pos = {});

        Result<ns> getPeriod(Positions pos = {}) const;

        void setPeriod(ns t, Positions pos = {});

        Result<ns> getDelayAfterTrigger(Positions pos = {}) const;

        void setDelayAfterTrigger(ns value, Positions pos = {});

        Result<int> getHighVoltage(Positions pos = {}) const;

        void setHighVoltage(int value, Positions pos = {});

        Result<bool> getPowerChip(Positions pos = {}) const;

        void setPowerChip(bool on, Positions pos = {});

        std::vector<slsDetectorDefs::dacIndex> getTemperatureList() const;

        Result<int> getTemperature(slsDetectorDefs::dacIndex index, Positions pos = {}) const;

        /**************************************************
         *                                                *
         *    Acquisition                                 *
         *                                                *
         * ************************************************/

        void acquire();

        void stopDetector(Positions pos = {});

        Result<slsDetectorDefs::runStatus> getDetectorStatus(Positions pos = {}) const;

        Result<std::string> getFilePath(Positions pos = {}) const;

        void setFilePath(const std::string& fpath, Positions pos = {});

        Result<std::string> getFileNamePrefix(Positions pos = {}) const;

        void setFileNamePrefix(const std::string& fname, Positions pos = {});

        Result<int64_t> getAcquisitionIndex(Positions pos = {}) const;

        void setAcquisitionIndex(int64_t i, Positions pos = {});

        std::vector<slsDetectorDefs::timingMode> getTimingModeList() const;

        Result<slsDetectorDefs::timingMode> getTimingMode(Positions pos = {}) const;

        void setTimingMode(slsDetectorDefs::timingMode value, Positions pos = {});

        /**************************************************
         *                                                *
         *    Jungfrau Specific                           *
         *                                                *
         * ************************************************/

        Result<int> getTemperatureEvent(Positions pos = {}) const;

        void resetTemperatureEvent(Positions pos = {});

        /**************************************************
         *                                                *
         *    Advanced                                    *
         *                                                *
         * ************************************************/

        Result<uint32_t> readRegister(uint32_t addr, Positions pos = {}) const;

        void writeRegister(uint32_t addr, uint32_t val, Positions pos = {});

        // Not available in "real" Detector
        slsDetectorDefs::detectorType setDetectorType(slsDetectorDefs::detectorType type);

       private: // Simulation properties
        int m_shm_id;
        slsDetectorDefs::detectorType m_detectorType;
        slsDetectorDefs::runStatus m_status;
        std::vector<std::string> m_filePath;
        std::vector<std::string> m_fileName;
        std::vector<int64_t> m_fileIndex;
        bool m_enableWriteToFile;
        std::vector<slsDetectorDefs::detectorSettings> m_settings;
        ns m_exposureTime;
        ns m_exposurePeriod;
        ns m_delayAfterTrigger;
        int64_t m_numberOfFrames;
        int64_t m_numberOfTriggers;
        int64_t m_numberOfGates;
        slsDetectorDefs::timingMode m_timingMode;
        std::string m_extsig[4];
        std::vector<std::string> m_hostname;
        std::string m_settingspath;
        int m_port;
        int m_stopport;
        std::string m_badchannels;
        std::vector<int> m_highvoltage;
        std::vector<bool> m_powerchip;
        std::string m_udp_srcip; // detector UDP/IP
        int m_rx_tcpport;
        std::string m_rx_hostname;
        std::string m_udp_dstip;                           // receiver UDP/IP
        int m_udp_dstport;                                 // receiver UDP port
        std::string m_udp_srcmac;                          // detector UDP MAC
        std::unordered_map<uint32_t, uint32_t> m_register; // XXX std::vector

       private:
        int dumpDetectorSetup(std::string const fname);
        int retrieveDetectorSetup(std::string const fname);

        std::string putCommand(int narg, char* args[]);
        std::string getCommand(int narg, char* args[]);

        void configureReceiver();
        void toReceiver(const std::string& command, const std::string& parameters = "");

        void startMeasurementNoWait();
        void setRxHostname(std::string rx_hostname);

        bool m_keepRunning;
        pthread_t m_dataThread;
        static void* dataWorker(void* self);

        int64_t m_neededFrames;
    };

} // namespace sls
#endif
