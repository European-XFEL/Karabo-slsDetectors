/*
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef SLS_RECEIVER_H
#define SLS_RECEIVER_H

#include <cstdint>
#include <cstdio>

extern "C" {
#include <pthread.h>
}

#include <boost/asio.hpp>

#include "sls_simulation_defs.h"

namespace sls {

    // forward declarations
    class Receiver;


    class session {
        // Mainly from boost's async_tcp_echo_server.cpp example
       public:
        session(boost::asio::io_context& io_context, Receiver* receiver);

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
        server(boost::asio::io_context& io_context, short port, Receiver* receiver);

        void handle_accept(session* new_session, const boost::system::error_code& ec);

       private:
        boost::asio::io_context& m_io_context;
        boost::asio::ip::tcp::acceptor m_acceptor;
        Receiver* m_receiver;
    };


    class Receiver {
       public:
        Receiver(uint16_t port = 1954);

        ~Receiver();

        int64_t getReceiverVersion();

        void registerCallBackStartAcquisition(void (*func)(const slsDetectorDefs::startCallbackHeader, void*),
                                              void* arg);

        void registerCallBackAcquisitionFinished(void (*func)(const slsDetectorDefs::endCallbackHeader, void*),
                                                 void* arg);

        void registerCallBackRawDataReady(void (*func)(slsDetectorDefs::sls_receiver_header&,
                                                       const slsDetectorDefs::dataCallbackHeader, char*, size_t&,
                                                       void*),
                                          void* arg);

        void processCommand(const std::string& command);

       private:
        void (*m_startAcquisitionCallBack)(const slsDetectorDefs::startCallbackHeader, void*);
        void* m_pStartAcquisition;
        void (*m_acquisitionFinishedCallBack)(const slsDetectorDefs::endCallbackHeader, void*);
        void* m_pAcquisitionFinished;
        void (*m_rawDataReadyCallBack)(slsDetectorDefs::sls_receiver_header&, const slsDetectorDefs::dataCallbackHeader,
                                       char*, size_t&, void*);
        void* m_pRawDataReady;

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

        uint64_t m_acqIndex;
        uint64_t m_frameIndex;
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
        boost::asio::io_context m_io_context;
        server* m_server;
        pthread_t m_dataThread, m_ioServThread;
        static void* dataWorker(void* self);
        static void* ioServWorker(void* self);
        std::string generateFileName();
        void setGain(int gain);
    };

} // namespace sls

#endif
