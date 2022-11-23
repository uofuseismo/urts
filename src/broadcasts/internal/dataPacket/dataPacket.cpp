#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <nlohmann/json.hpp>
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "private/isEmpty.hpp"

#define MESSAGE_TYPE "URTS::Broadcasts::Internal::DataPacket::DataPacket"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Broadcasts::Internal::DataPacket;

namespace
{

nlohmann::json toJSONObject(const DataPacket &packet)
{
    nlohmann::json obj;
    obj["MessageType"] = packet.getMessageType();
    obj["MessageVersion"] = packet.getMessageVersion();
    obj["Network"] = packet.getNetwork();
    obj["Station"] = packet.getStation();
    obj["Channel"] = packet.getChannel();
    obj["LocationCode"] = packet.getLocationCode();
    obj["StartTime"] = packet.getStartTime().count();
    obj["SamplingRate"] = packet.getSamplingRate();
    if (packet.haveSamplingRate() && packet.getNumberOfSamples() > 0)
    {
        obj["EndTime"] = packet.getEndTime().count();
    }
    else
    {
        obj["EndTime"] = nullptr;
    }
    if (packet.getNumberOfSamples() > 0)
    {
        obj["Data"] = packet.getData();
    }
    else
    {
        obj["Data"] = nullptr;
    }
    return obj;
}

void objectToDataPacket(const nlohmann::json &obj, DataPacket *packet)
{
    packet->clear();
    if (obj["MessageType"] != packet->getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    // Essential stuff
    packet->setNetwork(obj["Network"].get<std::string> ());
    packet->setStation(obj["Station"].get<std::string> ());
    packet->setChannel(obj["Channel"].get<std::string> ());
    packet->setLocationCode(obj["LocationCode"].get<std::string> ());
    packet->setSamplingRate(obj["SamplingRate"].get<double> ());
    auto startTime = obj["StartTime"].get<int64_t> ();
    std::chrono::microseconds startTimeMuS{startTime};
    packet->setStartTime(startTimeMuS);
    std::vector<double> data = obj["Data"]; //.get<std::vector<T>>
    if (!data.empty()){packet->setData(std::move(data));}
}

/*
void fromJSONMessage(const std::string &message, DataPacket *packet)
{
    auto obj = nlohmann::json::parse(message);
    objectToDataPacket(obj, packet);
}
*/

void fromCBORMessage(const uint8_t *message, const size_t length,
                     DataPacket *packet)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    objectToDataPacket(obj, packet);
}

}

class DataPacket::DataPacketImpl
{
public:
    void updateEndTime()
    {
        mEndTimeMicroSeconds = mStartTimeMicroSeconds;
        if (!mData.empty() && mSamplingRate > 0)
        {
            auto nSamples = static_cast<int> (mData.size());
            auto traceDuration
                = std::round( ((nSamples - 1)/mSamplingRate)*1000000 );
            auto iTraceDuration = static_cast<int64_t> (traceDuration);
            std::chrono::microseconds traceDurationMuS{iTraceDuration};
            mEndTimeMicroSeconds = mStartTimeMicroSeconds + traceDurationMuS;
        }
    }
    std::vector<double> mData;
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    /// Start time in microseconds (10e-6)
    std::chrono::microseconds mStartTimeMicroSeconds{0};
    /// End time in microseconds (10e-6)
    std::chrono::microseconds mEndTimeMicroSeconds{0};
    double mSamplingRate{0};
};

/// Clear class
void DataPacket::clear() noexcept
{
    pImpl->mData.clear();
    pImpl->mNetwork.clear();
    pImpl->mStation.clear();
    pImpl->mChannel.clear();
    pImpl->mLocationCode.clear();
    constexpr std::chrono::microseconds zeroMuS{0};
    pImpl->mStartTimeMicroSeconds = zeroMuS;
    pImpl->mEndTimeMicroSeconds = zeroMuS;
    pImpl->mSamplingRate = 0;
}

/// C'tor
DataPacket::DataPacket() :
    pImpl(std::make_unique<DataPacketImpl> ())
{
}

/// Copy c'tor 
DataPacket::DataPacket(const DataPacket &packet)
{
    *this = packet;
}

/// Move c'tor
DataPacket::DataPacket(DataPacket &&packet) noexcept
{
    *this = std::move(packet);
}

/// Copy assignment
DataPacket& DataPacket::operator=(const DataPacket &packet)
{
    if (&packet == this){return *this;}
    pImpl = std::make_unique<DataPacketImpl> (*packet.pImpl);
    return *this;
}

/// Move assignment
DataPacket& DataPacket::operator=(DataPacket &&packet) noexcept
{
    if (&packet == this){return *this;}
    pImpl = std::move(packet.pImpl);
    return *this;
}

/// Destructor
DataPacket::~DataPacket() = default;

/// Network
void DataPacket::setNetwork(const std::string &network)
{
    if (::isEmpty(network)){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string DataPacket::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set yet");}
    return pImpl->mNetwork;
}

bool DataPacket::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void DataPacket::setStation(const std::string &station)
{
    if (::isEmpty(station)){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = station;
}

std::string DataPacket::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set yet");}
    return pImpl->mStation;
}

bool DataPacket::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void DataPacket::setChannel(const std::string &channel)
{
    if (::isEmpty(channel)){throw std::invalid_argument("Channel is empty");}
    pImpl->mChannel = channel;
}

std::string DataPacket::getChannel() const
{
    if (!haveChannel()){throw std::runtime_error("Channel not set yet");}
    return pImpl->mChannel;
}

bool DataPacket::haveChannel() const noexcept
{
    return !pImpl->mChannel.empty();
}

/// Location code
void DataPacket::setLocationCode(const std::string &location)
{
    if (::isEmpty(location)){throw std::invalid_argument("Location is empty");}
    pImpl->mLocationCode = location;
}

std::string DataPacket::getLocationCode() const
{
    if (!haveLocationCode())
    {
        throw std::runtime_error("Location code not set yet");
    }
    return pImpl->mLocationCode;
}

bool DataPacket::haveLocationCode() const noexcept
{
    return !pImpl->mLocationCode.empty();
}

/// Sampling rate
void DataPacket::setSamplingRate(const double samplingRate) 
{
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("samplingRate = "
                                  + std::to_string(samplingRate)
                                  + " must be positive");
    }
    pImpl->mSamplingRate = samplingRate;
    pImpl->updateEndTime();
}

double DataPacket::getSamplingRate() const
{
    if (!haveSamplingRate()){throw std::runtime_error("Sampling rate not set");}
    return pImpl->mSamplingRate;
}

bool DataPacket::haveSamplingRate() const noexcept
{
    return (pImpl->mSamplingRate > 0);     
}

/// Number of samples
int DataPacket::getNumberOfSamples() const noexcept
{
    return static_cast<int> (pImpl->mData.size());
}

/// Start time
void DataPacket::setStartTime(const double startTime) noexcept
{
    auto iStartTimeMuS = static_cast<int64_t> (std::round(startTime*1.e6));
    std::chrono::microseconds startTimeMuS{iStartTimeMuS};
    setStartTime(startTimeMuS);
}

void DataPacket::setStartTime(
    const std::chrono::microseconds &startTime) noexcept
{
    pImpl->mStartTimeMicroSeconds = startTime;
    pImpl->updateEndTime();
}

std::chrono::microseconds DataPacket::getStartTime() const noexcept
{
    return pImpl->mStartTimeMicroSeconds;
}

std::chrono::microseconds DataPacket::getEndTime() const
{
    if (!haveSamplingRate())
    {   
        throw std::runtime_error("Sampling rate note set");
    }   
    if (getNumberOfSamples() < 1)
    {   
        throw std::runtime_error("No samples in signal");
    }   
    return pImpl->mEndTimeMicroSeconds;
}

/// Sets the data
void DataPacket::setData(std::vector<double> &&x) noexcept
{
    pImpl->mData = std::move(x);
    pImpl->updateEndTime();
}

template<typename U>
void DataPacket::setData(const std::vector<U> &x) noexcept
{
    setData(x.size(), x.data());
}

template<typename U>
void DataPacket::setData(const int nSamples, const U *__restrict__ x)
{
    // Invalid
    if (nSamples < 0){throw std::invalid_argument("nSamples not positive");}
    pImpl->mData.resize(nSamples);
    pImpl->updateEndTime();
    // No data so nothing to do
    if (nSamples == 0){return;}
    if (x == nullptr){throw std::invalid_argument("x is NULL");}
    auto *__restrict__ dPtr = pImpl->mData.data();
    std::copy(x, x + nSamples, dPtr);
}

/// Gets the data
std::vector<double> DataPacket::getData() const noexcept
{
    return pImpl->mData;
}

const double* DataPacket::getDataPointer() const noexcept
{
    return pImpl->mData.data();
}

/// Message format
std::string DataPacket::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage> DataPacket::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<DataPacket> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    DataPacket::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<DataPacket> (); 
    return result;
}

///  Convert message
std::string DataPacket::toMessage() const
{
    return toCBOR();
}

void DataPacket::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());   
}

void DataPacket::fromMessage(const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    fromCBOR(message, length);
}

/// From CBOR
void DataPacket::fromCBOR(const std::string &data)
{
    fromCBOR(reinterpret_cast<const uint8_t *> (data.data()), data.size());
}

void DataPacket::fromCBOR(const uint8_t *data, const size_t length)
{
    if (length == 0){throw std::invalid_argument("No data");}
    if (data == nullptr)
    {
        throw std::invalid_argument("data is NULL");
    }
    DataPacket packet;
    fromCBORMessage(data, length, &packet);
    *this = std::move(packet);
}

/// To CBOR
std::string DataPacket::toCBOR() const
{
    auto obj = toJSONObject(*this);
    auto message = nlohmann::json::to_cbor(obj);
    std::string result(message.begin(), message.end());
    return result;
}

/// To JSON
std::string DataPacket::toJSON(const int nIndent) const
{
    auto obj = toJSONObject(*this);
    auto result = obj.dump(nIndent);
    return result;
}

/// Message version
std::string DataPacket::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///--------------------------------------------------------------------------///
///                               Template Instantiation                     ///
///--------------------------------------------------------------------------///
template void URTS::Broadcasts::Internal::DataPacket::DataPacket::setData(const std::vector<double> &) noexcept;
template void URTS::Broadcasts::Internal::DataPacket::DataPacket::setData(const std::vector<float> &) noexcept;
template void URTS::Broadcasts::Internal::DataPacket::DataPacket::setData(const std::vector<int> &) noexcept;
template void URTS::Broadcasts::Internal::DataPacket::DataPacket::setData(const std::vector<int64_t> &) noexcept;
template void URTS::Broadcasts::Internal::DataPacket::DataPacket::setData(const std::vector<int16_t> &) noexcept;

template void URTS::Broadcasts::Internal::DataPacket::DataPacket::setData(const int, const double *);
template void URTS::Broadcasts::Internal::DataPacket::DataPacket::setData(const int, const float *);
template void URTS::Broadcasts::Internal::DataPacket::DataPacket::setData(const int, const int *);
template void URTS::Broadcasts::Internal::DataPacket::DataPacket::setData(const int, const int64_t *);
template void URTS::Broadcasts::Internal::DataPacket::DataPacket::setData(const int, const int16_t *);
