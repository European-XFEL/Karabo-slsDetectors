#include <iostream>
#include <unordered_map>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp> 

#include "Receiver.h"

using namespace boost::asio::ip;

#define MAX_FRAMES_PER_FILE 20000


namespace slsDetectorDefs {
    std::unordered_map<int, const int> channels {
        {static_cast<int>(detectorType::GENERIC), 0}, // UNDEFINED
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

sls::session::session(boost::asio::io_service& io_service, Receiver* receiver) :
        m_socket(io_service), m_receiver(receiver) {
}

void sls::session::start() {
    boost::asio::async_read_until(m_socket, m_streambuf, ";",
        boost::bind(&session::handle_read, this,
        boost::asio::placeholders::error));
}

void sls::session::handle_read(const boost::system::error_code& ec) {
    if (!ec) {
        std::istream reply(&m_streambuf);
        std::string command;
        std::getline(reply, command, ';');
        boost::algorithm::trim_if(command, boost::algorithm::is_any_of(" \n\r"));

        m_receiver->processCommand(command); // process received command

        boost::asio::async_read_until(m_socket, m_streambuf, ";",
            boost::bind(&session::handle_read, this,
            boost::asio::placeholders::error));

    } else {
        delete this;
    }
}

sls::server::server(boost::asio::io_service& io_service, short port, Receiver* receiver) :
        m_io_service(io_service), m_acceptor(io_service,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
        m_receiver(receiver) {
    session* new_session = new session(m_io_service, m_receiver);
    m_acceptor.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
        boost::asio::placeholders::error));
}

void sls::server::handle_accept(session* new_session, const boost::system::error_code& ec) {
    if (!ec) {
        new_session->start();
        new_session = new session(m_io_service, m_receiver);
        m_acceptor.async_accept(new_session->socket(),
            boost::bind(&server::handle_accept, this, new_session,
            boost::asio::placeholders::error));
    } else {
        delete new_session;
    }
}

sls::Receiver::Receiver(int argc, char *argv[]) : m_filePath("/tmp"), m_fileName("run") {
    m_acquisitionStarted = false;
    m_delay_us = 0;
    m_exptime_us = 10;
    m_period_us = 1000; // 1 ms
    m_rx_tcpport = SLS_RX_DEFAULT_PORT;
    m_settings = static_cast<int>(slsDetectorDefs::detectorSettings::UNINITIALIZED);
    m_frameCounter = 0;
    m_detectorType = static_cast<int>(slsDetectorDefs::detectorType::GENERIC); // UNDEFINED
    m_fileIndex = 0;
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

    try {
        m_server = new server(m_io_service, m_rx_tcpport, this);
        pthread_create(&m_ioServThread, NULL, ioServWorker, (void*) this);
    } catch (const std::exception& e) {
        std::cout << "Receiver::Receiver: " << e.what() << std::endl;
        throw; // re-throw
    }

    srand(time(NULL));
}

sls::Receiver::Receiver(int tcpip_port_no) {
    const std::string tcpip_port_no_str = std::to_string(tcpip_port_no);
    const int argc = 3;
    char* argv[argc];
    argv[0] = const_cast<char*>("ignored"); //  First parameter will be ignored
    argv[1] = const_cast<char*>("--rx_tcpport");
    argv[2] = const_cast<char*>(tcpip_port_no_str.c_str());

    Receiver(argc, argv);
}

sls::Receiver::~Receiver() {
    m_io_service.stop();
    pthread_join(m_ioServThread, NULL);
    if (m_data) delete[] m_data;
    if (m_server) delete m_server;
}

int64_t sls::Receiver::getReceiverVersion() {
    return int64_t(7022809911320);
}

void sls::Receiver::registerCallBackStartAcquisition(int (*func)(std::string filepath, std::string filename, uint64_t fileindex, uint32_t datasize, void*), void *arg) {
    m_startAcquisitionCallBack = func;
    m_pStartAcquisition = arg;
}

void sls::Receiver::registerCallBackAcquisitionFinished(void (*func)(uint64_t nf, void*), void *arg) {
    m_acquisitionFinishedCallBack = func;
    m_pAcquisitionFinished = arg;

}

void sls::Receiver::registerCallBackRawDataReady(void (*func)(char* metadata, char* datapointer, uint32_t datasize, void*), void *arg) {
    m_rawDataReadyCallBack = func;
    m_pRawDataReady = arg;
}

void* sls::Receiver::dataWorker(void* self) {
    Receiver* receiver = static_cast<Receiver*> (self);

    // Fill-up header
    slsDetectorDefs::sls_detector_header* detectorHeader = &(receiver->m_header.detHeader);

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
        if (acq_time_us > 0) {
            boost::this_thread::sleep(boost::posix_time::microseconds(acq_time_us));
        }

        const long sleep_time_us = receiver->m_period_us - receiver->m_delay_us - receiver->m_exptime_us - 1;
        if (sleep_time_us > 0) {
            boost::this_thread::sleep(boost::posix_time::microseconds(sleep_time_us));
        }

    }

    return nullptr;
}

void* sls::Receiver::ioServWorker(void* self) {
    Receiver* receiver = static_cast<Receiver*> (self);
    receiver->m_io_service.run();
    return nullptr;
}

std::string sls::Receiver::generateFileName() {

    // Create filename (without path and extension)
    boost::filesystem::path fileName = boost::str(boost::format("%s_d0_f%012d_%d") % m_fileName % m_currFileFirstFrame % m_fileIndex);

    // Prepend path and append extension
    fileName = (boost::filesystem::path(m_filePath) / fileName).replace_extension("raw");

    return fileName.string();
}

void sls::Receiver::processCommand(const std::string& command) {
    // std::cout << "processCommand: command=" << command << std::endl;

    // Split command and parameters
    std::vector<std::string> v;
    boost::algorithm::split(v, command, boost::algorithm::is_space());

    if (v[0] == "start") {
        if (m_acquisitionStarted) {
            // Already started -> ignore
            return;
        }

        const int channels = slsDetectorDefs::channels[m_detectorType];
        if (channels <= 0) {
            std::cout << "Not started: undefined/unknown detector type" << std::endl;
            return;
        }

        try {
            if (m_startAcquisitionCallBack != NULL) {
                // call registered start function
                const uint32_t datasize = channels * sizeof(short); // sample data size
                m_startAcquisitionCallBack(m_filePath, m_fileName, m_fileIndex, datasize, m_pStartAcquisition);

                if (m_enableWriteToFile) {
                    m_currAcqFrameCounter = 0;
                    m_currFileFirstFrame = 0;
                    std::string fname = this->generateFileName();
                    m_filePointer = fopen(fname.c_str(), "w");
                }
            }

            // start providing data in a thread
            m_acquisitionStarted = true;
            const int ret = pthread_create(&m_dataThread, NULL, Receiver::dataWorker, this);
            if (ret != 0) {
                std::cout << "Receiver::processCommand: cannot create data thread. ret="
                    << ret << std::endl;
                return;
            }

        } catch (const std::exception& e) {
            std::cout << "Receiver::processCommand: " << e.what() << std::endl;
        }

    } else if (v[0] == "stop") {
        if (!m_acquisitionStarted) {
            // Not started -> ignore
            return;
        }

        try {
            if (m_acquisitionFinishedCallBack != NULL) {
                // call registerd stop function
                m_acquisitionFinishedCallBack(m_frameCounter, m_pAcquisitionFinished);

                if (m_enableWriteToFile) {
                    fclose(m_filePointer);
                }
            }
            m_acquisitionStarted = false;

            // Wait for data thread to quit
            pthread_join(m_dataThread, NULL);

        } catch (const std::exception& e) {
            std::cout << "Receiver::processCommand: " << e.what() << std::endl;
        }

    } else if (v[0] == "exptime") {
        if (v.size() == 2) {
            m_exptime_us = 1.0e6 * std::stof(v[1]); // s -> us
            std::cout << "Receiver::processCommand: exptime=" << m_exptime_us << " us" << std::endl;
        }

    } else if (v[0] == "delay") {
        if (v.size() == 2) {
            m_delay_us = 1.0e6 * std::stof(v[1]); // s -> us
            std::cout << "Receiver::processCommand: delay=" << m_delay_us << " us" << std::endl;
        }

    } else if (v[0] == "period") {
        if (v.size() == 2) {
            m_period_us = 1.0e6 * std::stof(v[1]); // s -> us
            std::cout << "Receiver::processCommand: period=" << m_period_us << " us" << std::endl;
        }

    } else if (v[0] == "detectortype") {
        if (v.size() == 2) {
            const int detectorType = std::stoi(v[1]);
            if (detectorType != m_detectorType) {
                if (m_data) { // free memory
                    m_dataSize = 0;
                    delete[] m_data;
                    m_data = NULL;
                }

                // Allocate memory for two samples
                const int channels = slsDetectorDefs::channels[detectorType];
                m_dataSize = 2 * channels * sizeof(short);
                m_data = new char[m_dataSize];

                // Update detector type
                m_detectorType = detectorType;
            }

            // apply "settings"
            this->setGain(m_settings);

            std::cout << "Receiver::processCommand: detectortype=" << m_detectorType << std::endl;
        }

    } else if (v[0] == "fpath") {
        if (v.size() == 2) {
            m_filePath = v[1];
            std::cout << "Receiver::processCommand: fpath=" << m_filePath << std::endl;
        }

    } else if (v[0] == "fname") {
        if (v.size() == 2) {
            m_fileName = v[1];
            std::cout << "Receiver::processCommand: fname=" << m_fileName << std::endl;
        }

    } else if (v[0] == "findex") {
        if (v.size() == 2) {
            m_fileIndex = std::stoi(v[1]);
            std::cout << "Receiver::processCommand: findex=" << m_fileIndex << std::endl;
        }

    } else if (v[0] == "fwrite") {
        if (v.size() == 2) {
            m_enableWriteToFile = std::stoi(v[1]);
            std::cout << "Receiver::processCommand: fwrite=" << m_enableWriteToFile << std::endl;
        }

    } else if (v[0] == "settings") {
        if (v.size() == 2) {
            int gain = std::stoi(v[1]);
            this->setGain(gain);
            std::cout << "Receiver::processCommand: settings=" << m_settings << std::endl;
        }
    }

}

void sls::Receiver::setGain(int gain) {
    if (gain == m_settings) {
        // Nothing to be done
        return;
    }

    // Get baseline/noise map for detector type
    int detType = m_detectorType;
    if (slsDetectorDefs::baseline_noise.count(detType) == 0) {
        std::cout << "WARN Cannot find baseline/noise for this detectorType -> using generic"
                  << std::endl;
        detType = static_cast<int>(slsDetectorDefs::detectorType::GENERIC);
    }

    // Get baseline/noise values for gain settings
    if (slsDetectorDefs::baseline_noise[detType].count(gain) == 0) {
        std::cout << "WARN Cannot find baseline/noise for this gain -> using default"
                  << std::endl;
        gain = static_cast<int>(slsDetectorDefs::detectorSettings::UNINITIALIZED);
    }
    const int baseline = slsDetectorDefs::baseline_noise[detType][gain][0];
    const int noise = slsDetectorDefs::baseline_noise[detType][gain][1];

    // Generate random data
    const int channels = slsDetectorDefs::channels[m_detectorType];
    short* adc_and_gain = reinterpret_cast<short*>(m_data);
    for (int i = 0; i < 2 * channels; ++i) {
        adc_and_gain[i] = baseline + rand()%noise; // Generate random data
        adc_and_gain[i] |= (gain << 14); // Pack gain together with ADC value
    }

    // Update settings
    m_settings = gain;
}
