#include <iostream>
#include <string>
#include <limits>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "urts/services/scalable/packetCache/dataResponse.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::PacketCache::DataResponse"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::PacketCache;
namespace UDP = URTS::Broadcasts::Internal::DataPacket;

namespace
{

nlohmann::json toJSONObject(
    const DataResponse &response)
{
    nlohmann::json obj;
    obj["MessageType"] = response.getMessageType();
    obj["MessageVersion"] = response.getMessageVersion();
    const auto &packetsReference = response.getPacketsReference();
#ifndef NDEBUG
    assert(static_cast<int> (packetsReference.size()) ==
           response.getNumberOfPackets());
#endif
    auto nPackets = response.getNumberOfPackets();
    obj["NumberOfPackets"] = nPackets;
    auto packetsPointer = response.getPacketsPointer();
    if (nPackets > 0)
    {
        obj["Network"] = packetsPointer[0].getNetwork();
        obj["Station"] = packetsPointer[0].getStation();
        obj["Channel"] = packetsPointer[0].getChannel();
        obj["LocationCode"] = packetsPointer[0].getLocationCode();
        // Now the packets (these were sorted on time)
        nlohmann::json packetObjects;
	for (const auto &packet : packetsReference)
        {
            nlohmann::json packetObject;
            packetObject["StartTime"] = packet.getStartTime().count();
            packetObject["SamplingRate"] = packet.getSamplingRate();
            auto nSamples = packet.getNumberOfSamples();
            if (nSamples > 0)
            {
                packetObject["Data"] = packet.getData();
            }
            else
            {
                packetObject["Data"] = nullptr;
            } 
            packetObjects.push_back(packetObject);
        }
        obj["Packets"] = packetObjects;
    }
    else
    {
        obj["Packets"] = nullptr;
    }
    obj["Identifier"] = response.getIdentifier();
    obj["ReturnCode"] = static_cast<int> (response.getReturnCode());
    return obj;
}

DataResponse objectToDataResponse(const nlohmann::json &obj)
{
    DataResponse response;
    if (obj["MessageType"] != response.getMessageType())
    {   
        throw std::invalid_argument("Message has invalid message type");
    }   
    auto nPackets = obj["NumberOfPackets"].get<int> ();
    if (nPackets > 0)
    {
        auto network = obj["Network"].get<std::string> ();
        auto station = obj["Station"].get<std::string> ();
        auto channel = obj["Channel"].get<std::string> ();
        auto locationCode = obj["LocationCode"].get<std::string> ();
        std::vector<UDP::DataPacket> packets;
        auto packetObjects = obj["Packets"];
        packets.reserve(nPackets);
        for (const auto &packetObject : packetObjects)
        {
            UDP::DataPacket packet;
            packet.setNetwork(network);
            packet.setStation(station);
            packet.setChannel(channel);
            packet.setLocationCode(locationCode);
            packet.setSamplingRate(packetObject["SamplingRate"].get<double> ());
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
        response.setPackets(std::move(packets));
    }
    response.setIdentifier(obj["Identifier"].get<uint64_t> ());
    response.setReturnCode(static_cast<DataResponse::ReturnCode>
                           (obj["ReturnCode"].get<int> ()));
    return response;
}

DataResponse fromJSONMessage(const std::string &message)
{
    auto obj = nlohmann::json::parse(message);
    return objectToDataResponse(obj);
}

DataResponse fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    return objectToDataResponse(obj);
}

void checkPackets(const std::vector<UDP::DataPacket> &packets)
{
    std::string name;
    for (const auto &packet : packets)
    {
        if (!packet.haveNetwork())
        {
            throw std::invalid_argument("Network not set on packet");
        }
        if (!packet.haveStation())
        {
            throw std::invalid_argument("Station not set on packet");
        }
        if (!packet.haveChannel())
        {
            throw std::invalid_argument("Channel not set on packet");
        }
        if (!packet.haveLocationCode())
        {
            throw std::invalid_argument("Location code not set on packet");
        }
        if (!packet.haveSamplingRate())
        {
            throw std::invalid_argument("Sampling rate not set");
        }
        // Check that names are consistent
        auto tempName = packet.getNetwork() + "." + packet.getStation() + "." 
                      + packet.getChannel() + "." + packet.getLocationCode();
        if (!name.empty()) 
        {
            if (tempName != name)
            {
                throw std::runtime_error("Inconsistent SNCL");
            }
        }
        else
        {
            name = tempName;
        }
    }
}

}

class DataResponse::DataResponseImpl
{
public:
    void sortPackets()
    {
        if (!std::is_sorted(mPackets.begin(),
                            mPackets.end(),
                            [](const UDP::DataPacket &a,
                               const UDP::DataPacket &b)
                            {
                                return a.getStartTime() < b.getStartTime();
                            }))
        {
            std::sort(mPackets.begin(), mPackets.end(),
                      [](const UDP::DataPacket &a,
                         const UDP::DataPacket &b)
                       {
                           return a.getStartTime() < b.getStartTime();
                       });
        }
    }
    std::vector<UDP::DataPacket> mPackets;
    uint64_t mIdentifier{0};
    ReturnCode mReturnCode{ReturnCode::Success};
};

/// C'tor
DataResponse::DataResponse() :
    pImpl(std::make_unique<DataResponseImpl> ())
{
}

/// Copy c'tor
DataResponse::DataResponse(const DataResponse &response)
{
    *this = response;
}

/// Move c'tor
DataResponse::DataResponse(DataResponse &&response) noexcept
{
    *this = std::move(response);
}

/// Copy assignment
DataResponse& DataResponse::operator=(const DataResponse &response)
{
    if (&response == this){return *this;}
    pImpl = std::make_unique<DataResponseImpl> (*response.pImpl);
    return *this;
}

/// Move assignment
DataResponse& DataResponse::operator=(DataResponse &&response) noexcept
{
    if (&response == this){return *this;}
    pImpl = std::move(response.pImpl);
    return *this;
}

/// Destructor
DataResponse::~DataResponse() = default;

/// Reset class
void DataResponse::clear() noexcept
{
    pImpl = std::make_unique<DataResponseImpl> ();
}

/// Packets
void DataResponse::setPackets(
    const std::vector<UDP::DataPacket> &packets)
{
    checkPackets(packets);
    pImpl->mPackets = packets;
    pImpl->sortPackets();
}

void DataResponse::setPackets(std::vector<UDP::DataPacket> &&packets)
{
    checkPackets(packets);
    pImpl->mPackets = std::move(packets);
    pImpl->sortPackets();
}

int DataResponse::getNumberOfPackets() const noexcept
{
    return static_cast<int> (pImpl->mPackets.size());
}

std::vector<UDP::DataPacket> DataResponse::getPackets() const noexcept
{
    return pImpl->mPackets;
}

const UDP::DataPacket *DataResponse::getPacketsPointer() const noexcept
{
    return pImpl->mPackets.data();
}

const std::vector<UDP::DataPacket>
&DataResponse::getPacketsReference() const noexcept
{
    return pImpl->mPackets;
}

/// Identifier
void DataResponse::setIdentifier(const uint64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

uint64_t DataResponse::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Return code
void DataResponse::setReturnCode(const ReturnCode code) noexcept
{
    pImpl->mReturnCode = code;
}

DataResponse::ReturnCode DataResponse::getReturnCode() const noexcept
{
    return pImpl->mReturnCode;
}

/// Message type
std::string DataResponse::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string DataResponse::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

/// Create JSON
std::string DataResponse::toJSON(const int nIndent) const
{
    auto obj = toJSONObject(*this);
    return obj.dump(nIndent);
}

/// Create CBOR
std::string DataResponse::toCBOR() const
{
    auto obj = toJSONObject(*this);
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

/// From JSON
void DataResponse::fromJSON(const std::string &message)
{
    *this = fromJSONMessage(message);
}

/// From CBOR
void DataResponse::fromCBOR(const std::string &data)
{
    fromCBOR(reinterpret_cast<const uint8_t *> (data.data()), data.size());
}

void DataResponse::fromCBOR(const uint8_t *data, const size_t length)
{
    if (length == 0){throw std::invalid_argument("No data");}
    if (data == nullptr)
    {
        throw std::invalid_argument("data is NULL");
    }
    *this = fromCBORMessage(data, length);
}

///  Convert message
std::string DataResponse::toMessage() const
{
    return toCBOR();
}

void DataResponse::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());
}

void DataResponse::fromMessage(const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    fromCBOR(message, length);
}


/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage> DataResponse::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<DataResponse> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    DataResponse::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<DataResponse> (); 
    return result;
}

