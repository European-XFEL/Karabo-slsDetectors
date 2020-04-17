#ifndef SLS_RECEIVER_USERS_H
#define SLS_RECEIVER_USERS_H

#include <cstdint>
#include <cstdio>

extern "C" {
#include <pthread.h>
}

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

    long m_delay_us;
    long m_exptime_us;
    long m_period_us;

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
    int m_detectorType;
    char* m_data;
    uint32_t m_dataSize;
    int m_fileWriteOption;
    FILE* m_filePointer;

private:
    boost::asio::io_service m_io_service;
    boost::asio::ip::tcp::acceptor* m_acceptor;
    boost::asio::ip::tcp::socket* m_sock;
    bool m_keepRunning;
    pthread_t m_tcpThread, m_dataThread;
    static void* tcpWorker(void* self);
    static void* dataWorker(void* self);
    void stopTcpServer();
    std::string generateFileName();

};

#endif
