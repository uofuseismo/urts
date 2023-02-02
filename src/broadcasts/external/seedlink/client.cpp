#include <array>
#include <mutex>
#include <cmath>
#include <string>
#include <umps/logging/standardOut.hpp>
#include </usr/local/include/libmseed.h>
#include <libslink.h>
#include <slplatform.h>
#include "urts/broadcasts/external/seedlink/client.hpp"
#include "urts/broadcasts/external/seedlink/clientOptions.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"

using namespace URTS::Broadcasts::External::SEEDLink;
namespace UDataPacket = URTS::Broadcasts::Internal::DataPacket;

namespace
{
/// @brief Unpacks a miniSEED record.
[[nodiscard]]
UDataPacket::DataPacket
    miniSEEDToDataPacket(char *msRecord,
                         const int seedLinkRecordSize = 512)
{
    UDataPacket::DataPacket dataPacket;
    constexpr int8_t verbose{0};
    constexpr uint32_t flags{MSF_UNPACKDATA};
    MS3Record *miniSEEDRecord{nullptr};
    auto returnValue = msr3_parse(msRecord, seedLinkRecordSize,
                                  &miniSEEDRecord, flags,
                                  verbose);
    if (returnValue == 0)
    {
        // SNCL
        std::string network(8, '\0');
        std::string station{8, '\0'};
        std::string channel{8, '\0'};
        std::string location{8, '\0'};
        returnValue = ms_sid2nslc(miniSEEDRecord->sid,
                                  network.data(), station.data(),
                                  location.data(), channel.data());
        if (returnValue == 0)
        {
            dataPacket.setNetwork(network);
            dataPacket.setStation(station);
            dataPacket.setChannel(channel);
            dataPacket.setLocationCode(location); 
        }
        else
        {
            throw std::runtime_error("Failed to unpack SNCL");
        }
        // Sampling rate
        dataPacket.setSamplingRate(miniSEEDRecord->samprate);
        // Start time (convert from nanoseconds to microseconds)
        std::chrono::microseconds startTime
        {
            static_cast<int64_t> (std::round(miniSEEDRecord->starttime*1.e-3))
        };
        dataPacket.setStartTime(startTime);
        // Data
        auto nSamples = static_cast<int> (miniSEEDRecord->numsamples);
        if (nSamples > 0)
        {
            if (miniSEEDRecord->sampletype == 'i')
            {
                const auto data
                     = reinterpret_cast<const int *>
                       (miniSEEDRecord->datasamples);
                dataPacket.setData(nSamples, data);
            }
            else if (miniSEEDRecord->sampletype == 'f')
            {
                const auto data
                     = reinterpret_cast<const float *>
                       (miniSEEDRecord->datasamples);
                dataPacket.setData(nSamples, data);
            }
            else if (miniSEEDRecord->sampletype == 'd')
            {
                const auto data
                     = reinterpret_cast<const double *> 
                       (miniSEEDRecord->datasamples); 
                dataPacket.setData(nSamples, data);
            }
            else
            {
                throw std::runtime_error("Unhandled sample type");
            }
        }
    }
    else
    {
        if (returnValue < 0)
        {
            throw std::runtime_error("libmseed error detected");
        }
        throw std::runtime_error(
             "Insufficient data.  Number of additional bytes estimated is "
           + std::to_string(returnValue));
    }
    return dataPacket;
}
}

class Client::ClientImpl
{
public:
    ClientImpl(std::shared_ptr<UMPS::Logging::ILog> logger = nullptr) :
        ClientImpl(512, logger)
    {
    }
    ClientImpl(const int seedLinkRecordSize,
              std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mLogger(logger)
    {
        if (logger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> ();
        }
        if (seedLinkRecordSize == 128 ||
            seedLinkRecordSize == 256 ||
            seedLinkRecordSize == 512)
        {
            mSEEDLinkRecordSize = 512;
        }
        else
        {
            throw std::invalid_argument(
                "SEEDLink record size must be 128, 256, or 512");
        }
    }
    /// Destructor
    ~ClientImpl()
    {
        disconnect();
    }
    /// Terminate the SEED link client connection
    void disconnect()
    {
        if (mSEEDLinkConnection)
        {
            if (mSEEDLinkConnection->link != -1)
            {
                sl_disconnect(mSEEDLinkConnection);
            }
            mSEEDLinkConnection = nullptr;
        }
    }
    /// Connect to the SEED link client
    void connect()
    {
        disconnect();
        mSEEDLinkConnection = sl_newslcd();
    }
    /// Terminate SEEDLink connection
    void terminate()
    {
        if (!mSEEDLinkConnection->terminate)
        {
            sl_terminate(mSEEDLinkConnection);
        }
    }
    /// Gets the latest packets from the SEEDLink server.
    void getPackets()
    {
        std::vector<UDataPacket::DataPacket> dataPackets;
        dataPackets.reserve(std::max(1024, mMaxPacketsRead));
        SLpacket *seedLinkPacket{nullptr};
        while (true)
        {
            auto returnValue = sl_collect_nb_size(mSEEDLinkConnection,
                                                  &seedLinkPacket,
                                                  mSEEDLinkRecordSize);
            // Deal with packet
            if (returnValue == SLPACKET)
            {
	        auto packetType = sl_packettype(seedLinkPacket);
                // I really only care about data packets
                if (packetType == SLDATA)
                {
	            //auto sequenceNumber = sl_sequence(seedLinkPacket);
                    auto miniSEEDRecord
                        = reinterpret_cast<char *> (seedLinkPacket->msrecord);
                    try
                    {
                        auto packet = miniSEEDToDataPacket(miniSEEDRecord,
                                                           mSEEDLinkRecordSize);
                        dataPackets.push_back(std::move(packet));
                    }
                    catch (const std::exception &e)
                    {
                        mLogger->error("Skipping packet.  Unpacking failed with: "
                                     + std::string(e.what()));
                    }
                }
            }
            else
            {
                break;
            }
        }
        // Now put the data into
        auto nRead = static_cast<int> (dataPackets.size());
        if (nRead > 0)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mDataPackets = std::move(dataPackets); 
            mMaxPacketsRead = std::max(nRead, mMaxPacketsRead);
        }
    }
//private:
    mutable std::mutex mMutex;
    SLCD *mSEEDLinkConnection{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr}; 
    ClientOptions mOptions;
    std::vector<UDataPacket::DataPacket> mDataPackets;
    const std::array<std::string, 10> PacketType{ "Data",
                                                  "Detection",
                                                  "Calibration",
                                                  "Timing",
                                                  "Message",
                                                  "General",
                                                  "Request",
                                                  "Info",
                                                  "Info (terminated)",
                                                  "KeepAlive" };
    int mSEEDLinkRecordSize{512};
    int mMaxPacketsRead{0};
    bool mConnected{false};
};
    
/// Constructor
Client::Client() :
    pImpl(std::make_unique<ClientImpl> ())
{
}

/// Constructor
Client::~Client() = default;

/// Is connected?
bool Client::isConnected() const noexcept
{
    return pImpl->mConnected;
}
