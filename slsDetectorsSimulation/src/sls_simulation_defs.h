#ifndef SLS_SIMULATION_DEFS_H
#define SLS_SIMULATION_DEFS_H

#include <bitset>
#include <cstdint>
#include <vector>

#define SLS_RX_DEFAULT_PORT 1954

#define MAX_LEN 256


namespace slsDetectorDefs {
    /**  return values */
    enum { OK, FAIL };

#define MAX_NUM_PACKETS 512

    typedef struct {
            uint64_t frameNumber; /**< is the frame number */
            uint32_t expLength; /**< is the subframe number (32 bit eiger) or real time exposure time in 100ns (others) */
            uint32_t packetNumber; /**< is the packet number */
            uint64_t bunchId; /**< is the bunch id from beamline */
            uint64_t timestamp; /**< is the time stamp with 10 MHz clock */
            uint16_t modId; /**< is the unique module id (unique even for left, right, top, bottom) */
            uint16_t row; /**< is the row index in the complete detector system */
            uint16_t column; /**< is the column index in the complete detector system */
            uint16_t reserved; /**< is reserved */
            uint32_t debug; /**< is for debugging purposes */
            uint16_t roundRNumber; /**< is the round robin set number */
            uint8_t detType; /**< is the detector type see :: detectorType */
            uint8_t version; /**< is the version number of this structure format */
    } sls_detector_header;

    typedef std::bitset<MAX_NUM_PACKETS> sls_bitset;

    typedef struct {
            sls_detector_header detHeader; /**< is the detector header */
            sls_bitset packetsMask; /**< is the packets caught bit mask */
    } sls_receiver_header;

    /**
       detector settings indexes
    */
    enum detectorSettings {
        STANDARD,
        FAST,
        HIGHGAIN,
        DYNAMICGAIN,
        LOWGAIN,
        MEDIUMGAIN,
        VERYHIGHGAIN,
        DYNAMICHG0,
        FIXGAIN1,
        FIXGAIN2,
        FORCESWITCHG1,
        FORCESWITCHG2,
        VERYLOWGAIN,
        G1_HIGHGAIN,
        G1_LOWGAIN,
        G2_HIGHCAP_HIGHGAIN,
        G2_HIGHCAP_LOWGAIN,
        G2_LOWCAP_HIGHGAIN,
        G2_LOWCAP_LOWGAIN,
        G4_HIGHGAIN,
        G4_LOWGAIN,
        UNDEFINED = 200,
        UNINITIALIZED
    };

    /** Type of the detector */
    enum detectorType {
        GENERIC,
        EIGER,
        GOTTHARD,
        JUNGFRAU,
        CHIPTESTBOARD,
        MOENCH,
        MYTHEN3,
        GOTTHARD2,
    };

    /**
      use of the external signals
    */
    enum externalSignalFlag {
        TRIGGER_IN_RISING_EDGE,
        TRIGGER_IN_FALLING_EDGE,
        INVERSION_ON,
        INVERSION_OFF
    };

    /**
      communication mode using external signals
    */
    enum timingMode {
        AUTO_TIMING,
        TRIGGER_EXPOSURE,
        GATED,
        BURST_TRIGGER,
        TRIGGER_GATED,
        NUM_TIMING_MODES
    };

    /**
       detector dacs indexes
    */
    enum dacIndex {
        TEMPERATURE_ADC,
        TEMPERATURE_FPGA,
        TEMPERATURE_FPGAEXT,
        TEMPERATURE_10GE,
        TEMPERATURE_DCDC,
        TEMPERATURE_SODL,
        TEMPERATURE_SODR,
        TEMPERATURE_FPGA2,
        TEMPERATURE_FPGA3,
    };

    /** staus mask */
    enum runStatus {
        IDLE,
        ERROR,
        WAITING,
        RUN_FINISHED,
        TRANSMITTING,
        RUNNING,
        STOPPED
    };

}


namespace sls {
    template<class T>
    using Result = std::vector<T>;

    using Positions = const std::vector<int> &;

}

#endif
