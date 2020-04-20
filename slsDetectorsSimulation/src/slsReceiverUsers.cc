#include <iostream>
#include <unordered_map>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp> 

#include "sls_simulation_defs.h"
#include "slsReceiverUsers.h"

using namespace boost::asio::ip;

#define MAX_FRAMES_PER_FILE 20000

#define WARN_MISSING_PARAMETER(command) std::cout << "WARN " << command << " command need additional parameter!" << std::endl;

namespace slsDetectorDefs {
    std::unordered_map<int, const int> channels {
        {static_cast<int>(detectorType::GET_DETECTOR_TYPE), 0}, // UNDEFINED
        {static_cast<int>(detectorType::GOTTHARD), 1280},
        {static_cast<int>(detectorType::JUNGFRAU), 1024 * 512}
    };


    std::unordered_map<int, const std::vector<int>> generic_baseline_noise {
        {static_cast<int>(detectorSettings::UNINITIALIZED), {81, 12}}
    };


    std::unordered_map<int, const std::vector<int>> gotthard_baseline_noise {
        {static_cast<int>(detectorSettings::UNINITIALIZED), {4781, 156}}, // Default: HIGHGAIN
        {static_cast<int>(detectorSettings::LOWGAIN), {1962, 113}},
        {static_cast<int>(detectorSettings::MEDIUMGAIN), {2676, 79}},
        {static_cast<int>(detectorSettings::HIGHGAIN), {4781, 156}},
        {static_cast<int>(detectorSettings::VERYHIGHGAIN), {5431, 187}}
    };


    std::unordered_map<int, std::unordered_map<int, const std::vector<int>>> baseline_noise {
        {static_cast<int>(detectorType::GENERIC), generic_baseline_noise},
        {static_cast<int>(detectorType::GOTTHARD), gotthard_baseline_noise}
        //{static_cast<int>(detectorType::JUNGFRAU), jungfrau_baseline_noise} // TODO
    };
}


slsReceiverUsers::slsReceiverUsers(int argc, char *argv[], int &success) : m_acceptor(0), m_sock(0) {
    m_receiverStarted = false;
    m_acquisitionStarted = false;
    m_delay_us = 0;
    m_exptime_us = 10;
    m_period_us = 1000000;
    m_rx_tcpport = SLS_RX_DEFAULT_PORT;
    m_settings = static_cast<int>(detectorSettings::UNINITIALIZED);
    m_frameCounter = 0;
    m_detectorType = static_cast<int>(detectorType::GET_DETECTOR_TYPE); // UNDEFINED
    m_dataSize = 0;
    m_data = NULL;
    m_filePointer = NULL;

    m_startAcquisitionCallBack = NULL;
    m_pStartAcquisition = NULL;
    m_acquisitionFinishedCallBack = NULL;
    m_pAcquisitionFinished = NULL;
    m_rawDataReadyCallBack = NULL;
    m_pRawDataReady = NULL;

    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--rx_tcpport") == 0 || strcmp(argv[i], "-t") == 0) {
            if (++i < argc) { // port number is the next argument
                m_rx_tcpport = std::stoi(argv[i]);
            }
        }
    }

    m_keepRunning = true;
    success = 0; // OK

    try {
        tcp::endpoint endpoint(tcp::v4(), m_rx_tcpport);
        m_acceptor = new tcp::acceptor(m_io_service, endpoint);

        success += pthread_create(&m_tcpThread, NULL, tcpWorker, (void*) this);
    } catch (const std::exception& e) {
        ++success;
        std::cout << "slsReceiverUsers::slsReceiverUsers: " << e.what() << std::endl;
    }

    srand(time(NULL));
}

slsReceiverUsers::~slsReceiverUsers() {
    m_keepRunning = false;
    this->stopTcpServer();
    pthread_join(m_tcpThread, NULL);
    if (m_sock) delete m_sock;
    if (m_acceptor) delete m_acceptor;
    if (m_data) delete[] m_data;
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

void slsReceiverUsers::registerCallBackRawDataReady(void (*func)(char* metadata, char* datapointer, uint32_t datasize, void*), void *arg) {
    m_rawDataReadyCallBack = func;
    m_pRawDataReady = arg;
}

void* slsReceiverUsers::tcpWorker(void* self) {
    slsReceiverUsers* receiver = static_cast<slsReceiverUsers*> (self);

    while (receiver->m_keepRunning) {

        try {
            if (receiver->m_sock){
                delete receiver->m_sock;
                receiver->m_sock = NULL;
            }
            receiver->m_sock = new tcp::socket(receiver->m_io_service);
            receiver->m_acceptor->accept(*receiver->m_sock);

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
                    // std::cout << "echo: " << *it << std::endl; // XXX

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
                                // call registered start function
                                char* filepath = const_cast<char*> (receiver->m_filePath.c_str());
                                char* filename = const_cast<char*> (receiver->m_fileName.c_str());
                                const uint64_t fileindex = receiver->m_fileIndex;
                                const int channels = slsDetectorDefs::channels[receiver->m_detectorType];
                                const uint32_t datasize = channels * sizeof(short); // sample data size
                                receiver->m_startAcquisitionCallBack(filepath, filename, fileindex, datasize, receiver->m_pStartAcquisition);

                                if (receiver->m_enableWriteToFile) {
                                    receiver->m_currAcqFrameCounter = 0;
                                    receiver->m_currFileFirstFrame = 0;
                                    std::string fname = receiver->generateFileName();
                                    receiver->m_filePointer = fopen(fname.c_str(), "w");
                                }
                            }

                            // start providing data in a thread
                            const int ret = pthread_create(&receiver->m_dataThread, NULL, dataWorker, self);
                            if (ret != 0) {
                                std::cout << "slsReceiverUsers::tcpWorker: cannot create data thread. ret="
                                    << ret << std::endl;
                                continue;
                            }

                            receiver->m_acquisitionStarted = true;
                        } catch (const std::exception& e) {
                            std::cout << "slsReceiverUsers::tcpWorker: " << e.what() << std::endl;
                        }
                    } else if (v[0] == "stop") {
                        if (!receiver->m_acquisitionStarted) {
                            // Not started -> ignore
                            continue;
                        }

                        try {
                            if (receiver->m_acquisitionFinishedCallBack != NULL) {
                                // call registerd stop function
                                receiver->m_acquisitionFinishedCallBack(receiver->m_frameCounter, receiver->m_pAcquisitionFinished);

                                if (receiver->m_enableWriteToFile) {
                                    fclose(receiver->m_filePointer);
                                }
                            }
                            receiver->m_acquisitionStarted = false;

                            // Wait for data thread to quit
                            pthread_join(receiver->m_dataThread, NULL);

                        } catch (const std::exception& e) {
                            std::cout << "slsReceiverUsers::tcpWorker: " << e.what() << std::endl;
                        }
                    } else if (v[0] == "exptime") {
                        if (v.size() == 1) {
                            WARN_MISSING_PARAMETER(v[0])
                            continue;
                        }

                        receiver->m_exptime_us = 1000000 * std::stof(v[1]);
                    } else if (v[0] == "delay") {
                        if (v.size() == 1) {
                            WARN_MISSING_PARAMETER(v[0])
                            continue;
                        }

                        receiver->m_delay_us = 1000000 * std::stof(v[1]);
                    } else if (v[0] == "period") {
                        if (v.size() == 1) {
                            WARN_MISSING_PARAMETER(v[0])
                            continue;
                        }

                        receiver->m_period_us = 1000000 * std::stof(v[1]);
                    } else if (v[0] == "detectortype") {
                        if (v.size() == 1) {
                            WARN_MISSING_PARAMETER(v[0])
                            continue;
                        }

                        const int detectorType = std::stoi(v[1]);
                        if (detectorType == receiver->m_detectorType)
                            continue; // nothing to be done

                        if (receiver->m_data) { // free memory
                            receiver->m_dataSize = 0;
                            delete[] receiver->m_data;
                            receiver->m_data = NULL;
                        }

                        // Allocate memory for two samples
                        const int channels = slsDetectorDefs::channels[detectorType];
                        receiver->m_dataSize = 2 * channels * sizeof(short);
                        receiver->m_data = new char[receiver->m_dataSize];

                        // Update detector type
                        receiver->m_detectorType = detectorType;
                    } else if (v[0] == "outdir") {
                        if (v.size() == 1) {
                            WARN_MISSING_PARAMETER(v[0])
                            continue;
                        }

                        receiver->m_filePath = v[1];
                    } else if (v[0] == "fname") {
                        if (v.size() == 1) {
                            WARN_MISSING_PARAMETER(v[0])
                            continue;
                        }

                        receiver->m_fileName = v[1];
                    } else if (v[0] == "index") {
                        if (v.size() == 1) {
                            WARN_MISSING_PARAMETER(v[0])
                            continue;
                        }

                        receiver->m_fileIndex = std::stoi(v[1]);
                    } else if (v[0] == "enablefwrite") {
                        if (v.size() == 1) {
                            WARN_MISSING_PARAMETER(v[0])
                            continue;
                        }

                        receiver->m_enableWriteToFile = std::stoi(v[1]);
                    } else if (v[0] == "settings") {
                        if (v.size() == 1) {
                            WARN_MISSING_PARAMETER(v[0])
                            continue;
                        }

                        int gain = std::stoi(v[1]);
                        if (gain == receiver->m_settings)
                            continue; // nothing to be done

                        // Get baseline/noise map for detector type
                        int detType = receiver->m_detectorType;
                        if (slsDetectorDefs::baseline_noise.count(detType) == 0) {
                            std::cout << "WARN Cannot find baseline/noise for this detectorType -> using generic"
                                << std::endl;
                            detType = static_cast<int>(detectorType::GENERIC);
                        }

                        // Get baseline/noise values for gain settings
                        if (slsDetectorDefs::baseline_noise[detType].count(gain) == 0) {
                            std::cout << "WARN Cannot find baseline/noise for this gain -> using default"
                                << std::endl;
                            gain = static_cast<int>(detectorSettings::UNINITIALIZED);
                        }
                        const int baseline = slsDetectorDefs::baseline_noise[detType][gain][0];
                        const int noise = slsDetectorDefs::baseline_noise[detType][gain][1];

                        // Generate random data
                        const int channels = slsDetectorDefs::channels[receiver->m_detectorType];
                        short* adc_and_gain = reinterpret_cast<short*>(receiver->m_data);
                        for (int i = 0; i < 2 * channels; ++i) {
                            adc_and_gain[i] = baseline + rand()%noise; // Generate random data
                            adc_and_gain[i] |= (gain << 14); // Pack gain together with ADC value
                        }

                        // Update settings
                        receiver->m_settings = gain;
                    }
                }
            }
        } catch (const std::exception& e) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            std::cout << "slsReceiverUsers::tcpWorker: " << e.what() << std::endl;
        } catch (...) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            std::cout << "slsReceiverUsers::tcpWorker: " << "unknown exception" << std::endl;
        }
        
    }
}

void* slsReceiverUsers::dataWorker(void* self) {
    slsReceiverUsers* receiver = static_cast<slsReceiverUsers*> (self);

    // Fill-up header
    slsReceiverDefs::sls_detector_header* detectorHeader = &(receiver->m_header.detHeader);

    detectorHeader->expLength = 0;
    detectorHeader->packetNumber = 2;
    detectorHeader->bunchId = 0;
    detectorHeader->timestamp = 0;
    detectorHeader->modId = 0 ;
    detectorHeader->row = 0;
    detectorHeader->column = 0;
    detectorHeader->reserved = 0;
    detectorHeader->debug = 0;
    detectorHeader->roundRNumber = 0;
    detectorHeader->detType = receiver->m_detectorType;
    detectorHeader->version = 1;

    const int channels = slsDetectorDefs::channels[receiver->m_detectorType];
    const uint32_t dataSize = channels * sizeof(short);
    char *dataPointer, *metadata;

    while (receiver->m_acquisitionStarted) {
        receiver->m_frameCounter += 1;
        detectorHeader->frameNumber = receiver->m_frameCounter;
        receiver->m_header.packetsMask.reset();

        // randomly access m_data, which is twice as large as one sample
        dataPointer = receiver->m_data + sizeof(short) * rand() % channels;

        // Pass frame and metadata to callback
        metadata = reinterpret_cast<char*>(&receiver->m_header);
        receiver->m_rawDataReadyCallBack(metadata, dataPointer, dataSize, receiver->m_pRawDataReady);

        if (receiver->m_enableWriteToFile) {
            ++receiver->m_currAcqFrameCounter;

            if (receiver->m_currAcqFrameCounter - receiver->m_currFileFirstFrame >= MAX_FRAMES_PER_FILE) {
            // Open new file if needed
                receiver->m_currFileFirstFrame = receiver->m_currAcqFrameCounter;
                std::string fname = receiver->generateFileName();
                fclose(receiver->m_filePointer);
                receiver->m_filePointer = fopen(fname.c_str(), "w");
            }

            // Write frame to file
            fwrite(dataPointer, sizeof(char), dataSize, receiver->m_filePointer);
        }

        // Note: Subtract 1 us to take somehow into account dead times. A more sophisticated
        //       way would be to iteratively correct the sleep time in the loop.
        const long acq_time_us = receiver->m_delay_us + receiver->m_exptime_us - 1;
        if (acq_time_us > 0)
            boost::this_thread::sleep(boost::posix_time::microseconds(acq_time_us));

        const long sleep_time_us = receiver->m_period_us - receiver->m_delay_us - receiver->m_exptime_us - 1;
        if (sleep_time_us > 0)
            boost::this_thread::sleep(boost::posix_time::microseconds(sleep_time_us));

    }
}

void slsReceiverUsers::stopTcpServer() {
    tcp::iostream s;

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

    } catch (const std::exception& e) {
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
