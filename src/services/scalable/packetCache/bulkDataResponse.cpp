#include <iostream>
#include <string>
#include <limits>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "urts/services/scalable/packetCache/bulkDataResponse.hpp"
#include "urts/services/scalable/packetCache/dataResponse.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::PacketCache::BulkDataResponse"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::PacketCache;
namespace UDP = URTS::Broadcasts::Internal::DataPacket;

namespace
{

nlohmann::json toJSONObject(
    const BulkDataResponse &response)
{
    nlohmann::json obj;
    obj["MessageType"] = response.getMessageType();
    obj["MessageVersion"] = response.getMessageVersion();
    auto nDataResponses = response.getNumberOfDataResponses();
    obj["NumberOfDataResponses"] = nDataResponses;
    if (nDataResponses > 0)
    {
        nlohmann::json dataObjects;
        auto responsePtr = response.getDataResponsesPointer();
        for (int i = 0; i < nDataResponses; ++i)
        {
            nlohmann::json dataObject;
            auto nPackets = responsePtr[i].getNumberOfPackets();
            dataObject["NumberOfPackets"] = nPackets;
            auto packetsPointer = responsePtr[i].getPacketsPointer();
            if (nPackets > 0)
            {
                dataObject["Network"] = packetsPointer[0].getNetwork();
                dataObject["Station"] = packetsPointer[0].getStation();
                dataObject["Channel"] = packetsPointer[0].getChannel();
                dataObject["LocationCode"] = packetsPointer[0].getLocationCode();
                // Now the packets (these were sorted on time)
                nlohmann::json packetObjects;
                for (int ip = 0; ip < nPackets; ++ip)
                {
                    nlohmann::json packetObject;
                    packetObject["StartTime"]
                        = packetsPointer[ip].getStartTime().count();
                    packetObject["SamplingRate"]
                        = packetsPointer[ip].getSamplingRate();
                    auto nSamples = packetsPointer[ip].getNumberOfSamples();
                    if (nSamples > 0)
                    {
                        packetObject["Data"] = packetsPointer[ip].getData();
                    }
                    else
                    {
                       packetObject["Data"] = nullptr;
                    }
                    packetObjects.push_back(packetObject);
                } // Loop on packets
                // Add the packets to this response object
                dataObject["Packets"] = packetObjects;
            } 
            else // No packets
            {
                dataObject["Packets"] = nullptr;
            } // End check on packets > 0
            dataObject["Identifier"] = responsePtr[i].getIdentifier();
            dataObject["ReturnCode"]
                = static_cast<int> (responsePtr[i].getReturnCode());
            // Update the data responses with the object for this request
            dataObjects.push_back(dataObject);
        } // Loop on responses
        // Finally save all objects for all requests 
        obj["DataResponses"] = dataObjects;
    }
    else
    {
        obj["DataResponses"] = nullptr;
    }
    obj["Identifier"] = response.getIdentifier();
    obj["ReturnCode"] = static_cast<int> (response.getReturnCode());
    return obj;
}

BulkDataResponse objectToBulkDataResponse(const nlohmann::json &obj)
{
    BulkDataResponse response;
    if (obj["MessageType"] != response.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    auto nDataResponses = obj["NumberOfDataResponses"].get<int> ();
    if (nDataResponses > 0)
    {
        auto dataObjects = obj["DataResponses"];
        for (const auto &dataObject : dataObjects)
        {
            auto nPackets = dataObject["NumberOfPackets"].get<int> ();
            if (nPackets > 0)
            {
                DataResponse dataResponse;
                auto network = dataObject["Network"].get<std::string> ();
                auto station = dataObject["Station"].get<std::string> ();
                auto channel = dataObject["Channel"].get<std::string> ();
                auto locationCode
                    = dataObject["LocationCode"].get<std::string> ();
                std::vector<UDP::DataPacket> packets;
                auto packetObjects = dataObject["Packets"];
                packets.reserve(nPackets);
                for (const auto &packetObject : packetObjects)
                {
                    UDP::DataPacket packet;
                    packet.setNetwork(network);
                    packet.setStation(station);
                    packet.setChannel(channel);
                    packet.setLocationCode(locationCode);
                    packet.setSamplingRate(packetObject["SamplingRate"]
                                          .get<double> ());
                    auto startTime = packetObject["StartTime"].get<int64_t> (); 
                    std::chrono::microseconds startTimeMuS{startTime};
                    packet.setStartTime(startTimeMuS);
                    if (!packetObject["Data"].is_null())
                    {
                        std::vector<double> data = packetObject["Data"];
                        packet.setData(std::move(data));
                    }
                    packets.push_back(std::move(packet));
                }
                if (!packets.empty()){dataResponse.setPackets(packets);}
                dataResponse.setIdentifier(dataObject["Identifier"]
                                          .get<uint64_t> ());
                auto rc = static_cast<DataResponse::ReturnCode>
                          (dataObject["ReturnCode"].get<int> ());
                dataResponse.setReturnCode(rc);
                response.addDataResponse(std::move(dataResponse));
            }
        }
    }
    response.setIdentifier(obj["Identifier"].get<uint64_t> ());
    response.setReturnCode(static_cast<BulkDataResponse::ReturnCode>
                           (obj["ReturnCode"].get<int> ()));
    return response;
}

BulkDataResponse fromJSONMessage(const std::string &message)
{
    auto obj = nlohmann::json::parse(message);
    return objectToBulkDataResponse(obj);
}

BulkDataResponse fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    return objectToBulkDataResponse(obj);
}

}

class BulkDataResponse::BulkDataResponseImpl
{
public:
    std::vector<DataResponse> mResponses;
    uint64_t mIdentifier{0};
    BulkDataResponse::ReturnCode mReturnCode{BulkDataResponse::ReturnCode::Success};
};

/// C'tor
BulkDataResponse::BulkDataResponse() :
    pImpl(std::make_unique<BulkDataResponseImpl> ())
{
}

/// Copy c'tor
BulkDataResponse::BulkDataResponse(const BulkDataResponse &response)
{
    *this = response;
}

/// Move c'tor
BulkDataResponse::BulkDataResponse(BulkDataResponse &&response) noexcept
{
    *this = std::move(response);
}

/// Copy assignment
BulkDataResponse& BulkDataResponse::operator=(const BulkDataResponse &response)
{
    if (&response == this){return *this;}
    pImpl = std::make_unique<BulkDataResponseImpl> (*response.pImpl);
    return *this;
}

/// Move assignment
BulkDataResponse&
    BulkDataResponse::operator=(BulkDataResponse &&response) noexcept
{
    if (&response == this){return *this;}
    pImpl = std::move(response.pImpl);
    return *this;
}

/// Destructor
BulkDataResponse::~BulkDataResponse() = default;

/// Reset class
void BulkDataResponse::clear() noexcept
{
    pImpl = std::make_unique<BulkDataResponseImpl> ();
}

/// Data responses
void BulkDataResponse::addDataResponse(const DataResponse &response)
{
    pImpl->mResponses.push_back(response);
}

void BulkDataResponse::addDataResponse(DataResponse &&response)
{
    pImpl->mResponses.push_back(std::move(response));
}

std::vector<DataResponse> BulkDataResponse::getDataResponses() const noexcept
{
    return pImpl->mResponses;
}

const DataResponse *BulkDataResponse::getDataResponsesPointer() const noexcept
{
    return pImpl->mResponses.data();
}

int BulkDataResponse::getNumberOfDataResponses() const noexcept
{
    return static_cast<int> (pImpl->mResponses.size());
}

/// Identifier
void BulkDataResponse::setIdentifier(const uint64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

uint64_t BulkDataResponse::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Return code
void BulkDataResponse::setReturnCode(const ReturnCode code) noexcept
{
    pImpl->mReturnCode = code;
}

BulkDataResponse::ReturnCode BulkDataResponse::getReturnCode() const noexcept
{
    return pImpl->mReturnCode;
}

/// Message type
std::string BulkDataResponse::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string BulkDataResponse::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

/// Create JSON
std::string BulkDataResponse::toJSON(const int nIndent) const
{
    auto obj = toJSONObject(*this);
    return obj.dump(nIndent);
}

/// Create CBOR
std::string BulkDataResponse::toCBOR() const
{
    auto obj = toJSONObject(*this);
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

/// From JSON
void BulkDataResponse::fromJSON(const std::string &message)
{
    *this = fromJSONMessage(message);
}

/// From CBOR
void BulkDataResponse::fromCBOR(const std::string &data)
{
    fromCBOR(reinterpret_cast<const uint8_t *> (data.data()), data.size());
}

void BulkDataResponse::fromCBOR(const uint8_t *data, const size_t length)
{
    if (length == 0){throw std::invalid_argument("No data");}
    if (data == nullptr)
    {
        throw std::invalid_argument("data is NULL");
    }
    *this = fromCBORMessage(data, length);
}

///  Convert message
std::string BulkDataResponse::toMessage() const
{
    return toCBOR();
}

void BulkDataResponse::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());
}

void BulkDataResponse::fromMessage(const char *messageIn,
                                   const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    fromCBOR(message, length);
}


/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage> BulkDataResponse::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<BulkDataResponse> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    BulkDataResponse::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<BulkDataResponse> (); 
    return result;
}

