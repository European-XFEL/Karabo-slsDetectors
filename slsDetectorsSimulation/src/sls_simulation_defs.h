#ifndef SLS_SIMULATION_DEFS_H
#define SLS_SIMULATION_DEFS_H

#include <bitset>
#include <cstdint>

#define SLS_RX_DEFAULT_PORT 1954

#define MAX_LEN 256


namespace slsReceiverDefs {
    /*
     * return values
     */
    enum  {
            OK, /**< function succeeded */
            FAIL, /**< function failed */
            FINISHED, /**< acquisition finished */
            FORCE_UPDATE
    };

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

#define MAX_NUM_PACKETS 512

    typedef std::bitset<MAX_NUM_PACKETS> sls_bitset;

    typedef struct {
            sls_detector_header detHeader; /**< is the detector header */
            sls_bitset packetsMask; /**< is the packets caught bit mask */
    } sls_receiver_header;

}


namespace slsDetectorDefs {
    /*
     * return values
     */
    enum {
            OK, /**< function succeeded */
            FAIL, /**< function failed */
            FINISHED, /**< acquisition finished */
            FORCE_UPDATE
    };

}


/*
 *   detector settings indexes
 */
enum class detectorSettings {
    GET_SETTINGS=-1,  /**< return current detector settings */
    STANDARD,         /**< standard settings */
    FAST,             /**< fast settings */
    HIGHGAIN,         /**< highgain  settings */
    DYNAMICGAIN,      /**< dynamic gain  settings */
    LOWGAIN,          /**< low gain  settings */
    MEDIUMGAIN,       /**< medium gain  settings */
    VERYHIGHGAIN,     /**< very high gain  settings */
    LOWNOISE,         /**< low noise settings */
    DYNAMICHG0,       /**< dynamic high gain 0 */
    FIXGAIN1,         /**< fix gain 1 */
    FIXGAIN2,         /**< fix gain 2 */
    FORCESWITCHG1,    /**< force switch gain 1 */
    FORCESWITCHG2,    /**< force switch gain 2 */
    VERYLOWGAIN,      /**< very low gain settings */
    UNDEFINED=200,    /**< undefined or custom  settings */
    UNINITIALIZED     /**< uninitialiazed (status at startup) */
};


/*
 * Type of the detector
 */
enum class detectorType {
    GET_DETECTOR_TYPE=-1,   /**< the detector will return its type */
    GENERIC,  /**< generic sls detector */
    MYTHEN, /**< mythen */
    PILATUS, /**< pilatus */
    EIGER, /**< eiger */
    GOTTHARD, /**< gotthard */
    PICASSO, /**< picasso */
    AGIPD, /**< agipd */
    MOENCH, /**< moench */
    JUNGFRAU, /**< jungfrau */
    JUNGFRAUCTB, /**< jungfrauCTBversion */
    PROPIX, /**< propix */
    MYTHEN3 /**< mythen 3 */
};

#endif
