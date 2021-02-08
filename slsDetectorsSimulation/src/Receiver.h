#ifndef SLS_RECEIVER_H
#define SLS_RECEIVER_H

#include <cstdint>
#include <cstdio>

extern "C" {
#include <pthread.h>
}

#include "sls_simulation_defs.h"

namespace sls {

    // forward declarations
    class Receiver;


    class session {
    // Mainly from boost's async_tcp_echo_server.cpp example
    public:
        session(boost::asio::io_service& io_service, Receiver* receiver);

        boost::asio::ip::tcp::socket& socket() {
            return m_socket;
        }

        void start();

        void handle_read(const boost::system::error_code& ec);

    private:
        boost::asio::ip::tcp::socket m_socket;
        boost::asio::streambuf m_streambuf;
        Receiver* m_receiver;
    };


    class server {
    // Mainly from boost's async_tcp_echo_server.cpp example
    public:
        server(boost::asio::io_service& io_service, short port, Receiver* receiver);

        void handle_accept(session* new_session, const boost::system::error_code& ec);

    private:
        boost::asio::io_service& m_io_service;
        boost::asio::ip::tcp::acceptor m_acceptor;
        Receiver* m_receiver;
    };


    class Receiver {
    public:
        Receiver(int argc, char *argv[]);
        Receiver(int tcpip_port_no = 1954);

        ~Receiver();

        int64_t getReceiverVersion();

        void registerCallBackStartAcquisition(int (*func)(std::string filepath, std::string filename, uint64_t fileindex, uint32_t datasize, void*), void *arg);

        void registerCallBackAcquisitionFinished(void (*func)(uint64_t nf, void*), void *arg);

        void registerCallBackRawDataReady(void (*func)(char* metadata, char* datapointer, uint32_t datasize, void*), void *arg);

        void processCommand(const std::string& command);

    private:
        int (*m_startAcquisitionCallBack)(std::string, std::string, uint64_t, uint32_t, void*);
        void *m_pStartAcquisition;
        void (*m_acquisitionFinishedCallBack)(uint64_t, void*);
        void *m_pAcquisitionFinished;
        void (*m_rawDataReadyCallBack)(char*, char*, uint32_t, void*);
        void *m_pRawDataReady;

    private: // Simulation properties
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
        slsDetectorDefs::sls_receiver_header m_header;
        int m_detectorType;
        char* m_data;
        uint32_t m_dataSize;
        int m_fileWriteOption;
        FILE* m_filePointer;

    private:
        boost::asio::io_service m_io_service;
        server* m_server;
        pthread_t m_dataThread, m_ioServThread;
        static void* dataWorker(void* self);
        static void* ioServWorker(void* self);
        std::string generateFileName();
        void setGain(int gain);

    };

} // namespace sls

#endif
