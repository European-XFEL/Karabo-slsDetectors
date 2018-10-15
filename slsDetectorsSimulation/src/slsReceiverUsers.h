#ifndef SLS_RECEIVER_USERS_H
#define SLS_RECEIVER_USERS_H

#include <cstdint>
#include <cstdio>

#include <pthread.h>

#include "sls_simulation_defs.h"

class slsReceiver;

class slsReceiverUsers {
public:
    slsReceiverUsers(int argc, char *argv[], int &success);

    ~slsReceiverUsers();

    void closeFile(int p);

    int start();

    void stop();

    int64_t getReceiverVersion();

    void registerCallBackStartAcquisition(int (*func)(char* filepath, char* filename, uint64_t fileindex, uint32_t datasize, void*), void *arg);

    void registerCallBackAcquisitionFinished(void (*func)(uint64_t nf, void*), void *arg);

    void registerCallBackRawDataReady(void (*func)(char* metadata, char* datapointer, uint32_t datasize, void*), void *arg);

    slsReceiver* receiver;

private:
    int (*m_startAcquisitionCallBack)(char*, char*, uint64_t, uint32_t, void*);
    void *m_pStartAcquisition;
    void (*m_acquisitionFinishedCallBack)(uint64_t, void*);
    void *m_pAcquisitionFinished;
    void (*m_rawDataReadyCallBack)(char*, char*, uint32_t, void*);
    void *m_pRawDataReady;

private: // Simulation properties
    bool m_receiverStarted;
    bool m_acquisitionStarted;

    std::string m_filePath;
    std::string m_fileName;
    uint64_t m_fileIndex;
    bool m_enableWriteToFile;
    int m_rx_tcpport;
    int m_settings;

    uint64_t m_frameCounter;
    int m_currAcqFrameCounter;
    int m_currFileFirstFrame;
    slsReceiverDefs::sls_receiver_header m_header;
    char m_data[sizeof(short) * SLS_CHANNELS];
    uint32_t m_dataSize;
    int m_fileWriteOption;
    FILE* m_filePointer;

private:
    boost::asio::ip::tcp::socket* m_sock;
    bool m_keepRunning;
    pthread_t m_tcpThread;
    static void* tcpWorker(void* self);
    void stopTcpServer();
    std::string generateFileName();

};

#endif
