#ifndef SLS_SIMULATION_DEFS_H
#define SLS_SIMULATION_DEFS_H

#define SLS_RX_DEFAULT_PORT 1954

#define SLS_CHANNELS 1280

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
}

namespace slsDetectorDefs {
    /*
     * return values
     */
    enum  {
            OK, /**< function succeeded */
            FAIL, /**< function failed */
            FINISHED, /**< acquisition finished */
            FORCE_UPDATE
    };
}

#endif
