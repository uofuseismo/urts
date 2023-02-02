#include <array>
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
        auto nSamples = static_cast<int> (miniSEEDRecord->numsamples);
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
        dataPacket.setSamplingRate(miniSEEDRecord->samprate);
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
    ClientImpl() :
        ClientImpl(512)
    {
    }
    explicit ClientImpl(const int seedLinkRecordSize)
    {
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
    void pollSEEDLink()
    {
        SLpacket *seedLinkPacket{nullptr};
        while (true)
        {
            auto returnValue = sl_collect_nb_size(mSEEDLinkConnection,
                                                  &seedLinkPacket,
                                                  mSEEDLinkRecordSize);
            // Deal with packet
            if (returnValue == SLPACKET)
            {
                auto miniSEEDRecord
                     = reinterpret_cast<char *> (seedLinkPacket->msrecord);
                try
                {
                    auto packet = miniSEEDToDataPacket(miniSEEDRecord,
                                                       mSEEDLinkRecordSize);
                }
                catch (const std::exception &e)
                {
                }

            }
            else
            {
            }
        }
    }
//private:
    SLCD *mSEEDLinkConnection{nullptr};
    class ClientOptions mOptions;
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
};
    
/// Constructor
Client::Client() :
    pImpl(std::make_unique<ClientImpl> ())
{
}

/// Constructor
Client::~Client() = default;

