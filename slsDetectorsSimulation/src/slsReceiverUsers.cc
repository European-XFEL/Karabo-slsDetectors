#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp> 

#include "sls_simulation_defs.h"
#include "slsReceiverUsers.h"

using namespace boost::asio::ip;

#define MAX_FRAMES_PER_FILE 20000

slsReceiverUsers::slsReceiverUsers(int argc, char *argv[], int &success) : m_sock(0) {

    m_receiverStarted = false;
    m_acquisitionStarted = false;
    m_rx_tcpport = SLS_RX_DEFAULT_PORT;
    m_frameCounter = 0;
    m_dataSize = sizeof(short) * SLS_CHANNELS;
    m_filePointer = NULL;

    m_startAcquisitionCallBack = NULL;
    m_pStartAcquisition = NULL;
    m_acquisitionFinishedCallBack = NULL;
    m_pAcquisitionFinished = NULL;
    m_rawDataReadyCallBack = NULL;
    m_pRawDataReady = NULL;

    for (int i = 0; i < argc; ++i) {
        std::string line = argv[i];
        std::vector<std::string> v;
        boost::algorithm::split(v, line, boost::algorithm::is_space());
        if (v[0] == "--rx_tcpport" && v.size() > 1)
            m_rx_tcpport = std::stoi(v[1]);
        // TODO more parameters?
    }
    
    m_keepRunning = true;
    success = 0; // OK
    success += pthread_create(&m_tcpThread, NULL, tcpWorker, (void*) this);
    
    srand(time(NULL));
}

slsReceiverUsers::~slsReceiverUsers() {
    m_keepRunning = false;
    this->stopTcpServer();
    pthread_join(m_tcpThread, NULL);
    if (m_sock) delete m_sock;
}

void slsReceiverUsers::closeFile(int p) {
    std::runtime_error e("slsReceiverUsers::closeFile not implemented");
    throw e;
}

int slsReceiverUsers::start() {
    m_receiverStarted = true;
    return 0;
}

void slsReceiverUsers::stop() {
    m_receiverStarted = false;
}

int64_t slsReceiverUsers::getReceiverVersion() {
    return int64_t(7022809911320);
}

void slsReceiverUsers::registerCallBackStartAcquisition(int (*func)(char* filepath, char* filename, uint64_t fileindex, uint32_t datasize, void*), void *arg) {
    m_startAcquisitionCallBack = func;
    m_pStartAcquisition = arg;
}

void slsReceiverUsers::registerCallBackAcquisitionFinished(void (*func)(uint64_t nf, void*), void *arg) {
    m_acquisitionFinishedCallBack = func;
    m_pAcquisitionFinished = arg;

}

void slsReceiverUsers::registerCallBackRawDataReady(void (*func)(uint64_t frameNumber, uint32_t expLength, uint32_t packetNumber, uint64_t bunchId, uint64_t timestamp,
        uint16_t modId, uint16_t xCoord, uint16_t yCoord, uint16_t zCoord, uint32_t debug, uint16_t roundRNumber, uint8_t detType, uint8_t version,
        char* datapointer, uint32_t datasize, void*), void *arg) {
    m_rawDataReadyCallBack = func;
    m_pRawDataReady = arg;
}

void* slsReceiverUsers::tcpWorker(void* self) {
    slsReceiverUsers* receiver = static_cast<slsReceiverUsers*> (self);

    boost::asio::io_service io_service;
    tcp::endpoint endpoint(tcp::v4(), receiver->m_rx_tcpport);
    tcp::acceptor acceptor(io_service, endpoint);
    
    while (receiver->m_keepRunning) {

        try {
            if (receiver->m_sock){
                delete receiver->m_sock;
                receiver->m_sock = NULL;
            }
            receiver->m_sock = new boost::asio::ip::tcp::socket(io_service);
            acceptor.accept(*receiver->m_sock);
                        
            while (receiver->m_keepRunning) {
                char data[MAX_LEN];
                memset(data, 0, MAX_LEN);
                
                boost::system::error_code error;
                // TODO: use async_read_some, as (synchronous) read_some cannot be canceled
                size_t length = receiver->m_sock->read_some(boost::asio::buffer(data), error);
                if (error) {
                    std::cout << "slsReceiverUsers::tcpWorker - TCP error: " << error << std::endl;
                    break;
                }
                //std::cout << "echo: " << data << std::endl;

                // Split lines
                std::vector<std::string> r;
                boost::algorithm::split(r, data, boost::algorithm::is_any_of(";"));

                for (auto it = r.begin(); it != r.end(); ++it) {
                    if (*it == "") continue; // Skip empty lines
                    //std::cout << "echo: " << *it << std::endl;

                    // Split command and parameters
                    std::vector<std::string> v;
                    boost::algorithm::split(v, *it, boost::algorithm::is_space());

                    if (v[0] == "bye") {
                        break;
                    } else if (v[0] == "start") {
                        if (receiver->m_acquisitionStarted) {
                            // Already started -> ignore
                            continue;
                        }
                        try {
                            if (receiver->m_startAcquisitionCallBack != NULL) {
                                char* filepath = const_cast<char*> (receiver->m_filePath.c_str());
                                char* filename = const_cast<char*> (receiver->m_fileName.c_str());
                                uint64_t fileindex = receiver->m_fileIndex;
                                uint32_t datasize = receiver->m_dataSize;
                                int result = receiver->m_startAcquisitionCallBack(filepath, filename, fileindex, datasize, receiver->m_pStartAcquisition);

                                if (receiver->m_enableWriteToFile) {
                                    receiver->m_currAcqFrameCounter = 0;
                                    receiver->m_currFileFirstFrame = 0;
                                    std::string fname = receiver->generateFileName();
                                    receiver->m_filePointer = fopen(fname.c_str(), "w");
                                }
                            }
                            receiver->m_acquisitionStarted = true;
                        } catch (std::exception e) {
                            std::cout << "slsReceiverUsers::tcpWorker: " << e.what() << std::endl;
                        }
                    } else if (v[0] == "stop") {
                        if (!receiver->m_acquisitionStarted) {
                            // Not started -> ignore
                            continue;
                        }

                        try {
                            if (receiver->m_acquisitionFinishedCallBack != NULL) {
                                receiver->m_acquisitionFinishedCallBack(receiver->m_frameCounter, receiver->m_pAcquisitionFinished);

                                if (receiver->m_enableWriteToFile) {
                                    fclose(receiver->m_filePointer);
                                }
                            }
                            receiver->m_acquisitionStarted = false;
                        } catch (std::exception e) {
                            std::cout << "slsReceiverUsers::tcpWorker: " << e.what() << std::endl;
                        }
                    } else if (v[0] == "rawdata") {
                        if (!receiver->m_acquisitionStarted) {
                            // Not started -> ignore
                            continue;
                        }

                        receiver->m_frameCounter += 1;
                        try {
                            if (receiver->m_rawDataReadyCallBack != NULL) {
                                const uint64_t frameNumber = receiver->m_frameCounter;
                                const uint32_t expLength = 0;
                                const uint32_t packetNumber = 2;
                                const uint64_t bunchId = 0;
                                const uint64_t timestamp = 0;
                                const uint16_t modId = 0 ;
                                const uint16_t xCoord = 0, yCoord = 0, zCoord = 0;
                                const uint32_t debug = 0;
                                const uint16_t roundRNumber = 0;
                                const uint8_t detType = 4; // Gotthard
                                const uint8_t version = 1;
                                char* dataPointer = receiver->m_data;
                                const uint32_t dataSize = receiver->m_dataSize;

                                const short gain = receiver->m_settings;
                                short* adc_and_gain = reinterpret_cast<short*>(dataPointer);

                                // default (high gain)
                                int baseline = 4781;
                                int noise = 156;
                                if (gain == 4) { // low gain
                                    baseline = 1962;
                                    noise = 113;
                                } else if (gain == 5) { // medium gain
                                    baseline = 2676;
                                    noise = 79;
                                } else if (gain == 6) { // very high gain
                                    baseline = 5431;
                                    noise = 187;
                                }

                                for (int i=0; i<SLS_CHANNELS; ++i) {
                                    adc_and_gain[i] = baseline + rand()%noise; // Generate random data
                                    adc_and_gain[i] |= (gain << 14); // Pack gain together with ADC value
                                }

                                // Pass frame to callback
                                receiver->m_rawDataReadyCallBack(frameNumber, expLength, packetNumber, bunchId, timestamp, modId, xCoord, yCoord,
                                        zCoord, debug, roundRNumber, detType, version, dataPointer, dataSize, receiver->m_pRawDataReady);

                                // Open new file if needed
                                if (receiver->m_enableWriteToFile) {
                                    ++receiver->m_currAcqFrameCounter;
                                    if (receiver->m_currAcqFrameCounter - receiver->m_currFileFirstFrame >= MAX_FRAMES_PER_FILE) {
                                        receiver->m_currFileFirstFrame = receiver->m_currAcqFrameCounter;
                                        std::string fname = receiver->generateFileName();
                                        fclose(receiver->m_filePointer);
                                        receiver->m_filePointer = fopen(fname.c_str(), "w");
                                    }
                                }

                                // Write frame to file
                                if (receiver->m_enableWriteToFile)
                                    fwrite(dataPointer, sizeof(char), dataSize, receiver->m_filePointer);
                            }
                        } catch (std::exception e) {
                            std::cout << "slsReceiverUsers::tcpWorker: " << e.what() << std::endl;
                        }
                    } else if (v[0] == "outdir" && v.size() > 1) {
                        receiver->m_filePath = v[1];
                    } else if (v[0] == "fname" && v.size() > 1) {
                        receiver->m_fileName = v[1];
                    } else if (v[0] == "index" && v.size() > 1) {
                        receiver->m_fileIndex = std::stoi(v[1]);
                    } else if (v[0] == "enablefwrite" && v.size() > 1) {
                        receiver->m_enableWriteToFile = std::stoi(v[1]);
                    } else if (v[0] == "settings" && v.size() > 1) {
                        receiver->m_settings = std::stoi(v[1]);
                    }
                }
            }
        } catch (std::exception e) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            std::cout << "slsReceiverUsers::tcpWorker: " << e.what() << std::endl;
        } catch (...) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            std::cout << "slsReceiverUsers::tcpWorker: " << "unknown exception" << std::endl;
        }
        
    }
}

void slsReceiverUsers::stopTcpServer() {
    boost::asio::ip::tcp::iostream s;

    try {
        // The entire sequence of I/O operations must complete within 5 seconds.
        // If an expiry occurs, the socket is automatically closed and the stream
        // becomes bad.
        s.expires_from_now(boost::posix_time::seconds(5));

        // Establish a connection to the server.
        s.connect("localhost", std::to_string(m_rx_tcpport));

        // Send the command
        s << "bye";
        s.flush();

    } catch (std::exception e) {
        std::cout << "slsReceiverUsers::stopTcpServer: " << e.what() << std::endl;
    }
}

std::string slsReceiverUsers::generateFileName() {

    // Create filename (without path and extension)
    boost::filesystem::path fileName = boost::str(boost::format("%s_d0_f%012d_%d") % m_fileName % m_currFileFirstFrame % m_fileIndex);

    // Prepend path and append extension
    fileName = (boost::filesystem::path(m_filePath) / fileName).replace_extension("raw");

    return fileName.string();
}
