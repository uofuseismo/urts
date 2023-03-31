#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <nlohmann/json.hpp>
#include "urts/broadcasts/internal/probabilityPacket/probabilityPacket.hpp"
#include "private/isEmpty.hpp"

#define MESSAGE_TYPE "URTS::Broadcasts::Internal::ProbabilityPacket::ProbabilityPacket"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Broadcasts::Internal::ProbabilityPacket;

namespace
{

nlohmann::json toJSONObject(const ProbabilityPacket &packet)
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
    obj["Algorithm"] = packet.getAlgorithm();
    obj["PositiveClassName"] = packet.getPositiveClassName();
    obj["NegativeClassName"] = packet.getNegativeClassName();
    obj["OriginalChannels"] = packet.getOriginalChannels();
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

ProbabilityPacket objectToProbabilityPacket(const nlohmann::json &obj)
{
    ProbabilityPacket packet;
    if (obj["MessageType"] != packet.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    packet.setNetwork(obj["Network"].get<std::string> ());
    packet.setStation(obj["Station"].get<std::string> ());
    packet.setChannel(obj["Channel"].get<std::string> ());
    packet.setLocationCode(obj["LocationCode"].get<std::string> ());
    packet.setSamplingRate(obj["SamplingRate"].get<double> ());
    packet.setAlgorithm(obj["Algorithm"].get<std::string> ());
    packet.setPositiveClassName(obj["PositiveClassName"].get<std::string> ());
    packet.setNegativeClassName(obj["NegativeClassName"].get<std::string> ());
    std::vector<std::string> originalChannels = obj["OriginalChannels"];
    if (!originalChannels.empty())
    {
        packet.setOriginalChannels(originalChannels);
    }
    auto startTime = obj["StartTime"].get<int64_t> ();
    std::chrono::microseconds startTimeMuS{startTime};
    packet.setStartTime(startTimeMuS);
    std::vector<double> data = obj["Data"];
    if (!data.empty()){packet.setData(std::move(data));}
    return packet;
}

ProbabilityPacket fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    return ::objectToProbabilityPacket(obj);
}

}

class ProbabilityPacket::ProbabilityPacketImpl
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
    std::vector<std::string> mOriginalChannels;
    std::vector<double> mData;
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    std::string mAlgorithm;
    std::string mPositiveClassName;
    std::string mNegativeClassName;
    /// Start time in microseconds (10e-6)
    std::chrono::microseconds mStartTimeMicroSeconds{0};
    /// End time in microseconds (10e-6)
    std::chrono::microseconds mEndTimeMicroSeconds{0};
    double mSamplingRate{0};
};

/// Clear class
void ProbabilityPacket::clear() noexcept
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
ProbabilityPacket::ProbabilityPacket() :
    pImpl(std::make_unique<ProbabilityPacketImpl> ())
{
}

/// Copy c'tor 
ProbabilityPacket::ProbabilityPacket(const ProbabilityPacket &packet)
{
    *this = packet;
}

/// Move c'tor
ProbabilityPacket::ProbabilityPacket(ProbabilityPacket &&packet) noexcept
{
    *this = std::move(packet);
}

/// Copy assignment
ProbabilityPacket&
ProbabilityPacket::operator=(const ProbabilityPacket &packet)
{
    if (&packet == this){return *this;}
    pImpl = std::make_unique<ProbabilityPacketImpl> (*packet.pImpl);
    return *this;
}

/// Move assignment
ProbabilityPacket&
ProbabilityPacket::operator=(ProbabilityPacket &&packet) noexcept
{
    if (&packet == this){return *this;}
    pImpl = std::move(packet.pImpl);
    return *this;
}

/// Destructor
ProbabilityPacket::~ProbabilityPacket() = default;

/// Network
void ProbabilityPacket::setNetwork(const std::string &network)
{
    if (::isEmpty(network)){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string ProbabilityPacket::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set yet");}
    return pImpl->mNetwork;
}

bool ProbabilityPacket::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void ProbabilityPacket::setStation(const std::string &station)
{
    if (::isEmpty(station)){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = station;
}

std::string ProbabilityPacket::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set yet");}
    return pImpl->mStation;
}

bool ProbabilityPacket::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void ProbabilityPacket::setChannel(const std::string &channel)
{
    if (::isEmpty(channel)){throw std::invalid_argument("Channel is empty");}
    pImpl->mChannel = channel;
}

std::string ProbabilityPacket::getChannel() const
{
    if (!haveChannel()){throw std::runtime_error("Channel not set yet");}
    return pImpl->mChannel;
}

bool ProbabilityPacket::haveChannel() const noexcept
{
    return !pImpl->mChannel.empty();
}

/// Location code
void ProbabilityPacket::setLocationCode(const std::string &location)
{
    if (::isEmpty(location)){throw std::invalid_argument("Location is empty");}
    pImpl->mLocationCode = location;
}

std::string ProbabilityPacket::getLocationCode() const
{
    if (!haveLocationCode())
    {
        throw std::runtime_error("Location code not set yet");
    }
    return pImpl->mLocationCode;
}

bool ProbabilityPacket::haveLocationCode() const noexcept
{
    return !pImpl->mLocationCode.empty();
}

/// Sampling rate
void ProbabilityPacket::setSamplingRate(const double samplingRate) 
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

double ProbabilityPacket::getSamplingRate() const
{
    if (!haveSamplingRate()){throw std::runtime_error("Sampling rate not set");}
    return pImpl->mSamplingRate;
}

bool ProbabilityPacket::haveSamplingRate() const noexcept
{
    return (pImpl->mSamplingRate > 0);     
}

/// Original packets
void ProbabilityPacket::setOriginalChannels(
    const std::vector<std::string> &originalChannels) noexcept
{
    pImpl->mOriginalChannels = originalChannels;
}

std::vector<std::string> ProbabilityPacket::getOriginalChannels() const noexcept
{
    return pImpl->mOriginalChannels;
}

/// Algorithm
void ProbabilityPacket::setAlgorithm(const std::string &algorithm) noexcept
{
    pImpl->mAlgorithm= algorithm;
}

std::string ProbabilityPacket::getAlgorithm() const noexcept
{
    return pImpl->mAlgorithm;
}

/// Positive class
void ProbabilityPacket::setPositiveClassName(
    const std::string &className) noexcept
{
    pImpl->mPositiveClassName = className;
}

std::string ProbabilityPacket::getPositiveClassName() const noexcept
{
    return pImpl->mPositiveClassName;
}

/// Negative class
void ProbabilityPacket::setNegativeClassName(
    const std::string &className) noexcept
{
    pImpl->mNegativeClassName = className;
}

std::string ProbabilityPacket::getNegativeClassName() const noexcept
{
    return pImpl->mNegativeClassName;
}

/// Number of samples
int ProbabilityPacket::getNumberOfSamples() const noexcept
{
    return static_cast<int> (pImpl->mData.size());
}

/// Start time
void ProbabilityPacket::setStartTime(const double startTime) noexcept
{
    auto iStartTimeMuS = static_cast<int64_t> (std::round(startTime*1.e6));
    std::chrono::microseconds startTimeMuS{iStartTimeMuS};
    setStartTime(startTimeMuS);
}

void ProbabilityPacket::setStartTime(
    const std::chrono::microseconds &startTime) noexcept
{
    pImpl->mStartTimeMicroSeconds = startTime;
    pImpl->updateEndTime();
}

std::chrono::microseconds ProbabilityPacket::getStartTime() const noexcept
{
    return pImpl->mStartTimeMicroSeconds;
}

std::chrono::microseconds ProbabilityPacket::getEndTime() const
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
void ProbabilityPacket::setData(std::vector<double> &&x) noexcept
{
    pImpl->mData = std::move(x);
    pImpl->updateEndTime();
}

template<typename U>
void ProbabilityPacket::setData(const std::vector<U> &x) noexcept
{
    auto nSamples = static_cast<int> (x.size());
    pImpl->mData.resize(nSamples);
    pImpl->updateEndTime();
    if (nSamples == 0){return;}
    std::copy(x.begin(), x.end(), pImpl->mData.begin());
}

/// Gets the data
std::vector<double> ProbabilityPacket::getData() const noexcept
{
    return pImpl->mData;
}

const std::vector<double> &ProbabilityPacket::getDataReference() const noexcept
{
    return pImpl->mData;
}

/// Message format
std::string ProbabilityPacket::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string ProbabilityPacket::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage> ProbabilityPacket::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<ProbabilityPacket> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    ProbabilityPacket::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<ProbabilityPacket> (); 
    return result;
}

///  Convert message
std::string ProbabilityPacket::toMessage() const
{
    auto obj = ::toJSONObject(*this);
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

void ProbabilityPacket::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());   
}

void ProbabilityPacket::fromMessage(const char *messageIn, const size_t length)
{
    if (length == 0){throw std::invalid_argument("No data");}
    if (messageIn == nullptr)
    {
        throw std::invalid_argument("Message is NULL");
    }
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

///--------------------------------------------------------------------------///
///                               Template Instantiation                     ///
///--------------------------------------------------------------------------///
template void URTS::Broadcasts::Internal::ProbabilityPacket::ProbabilityPacket::setData(const std::vector<double> &) noexcept;
template void URTS::Broadcasts::Internal::ProbabilityPacket::ProbabilityPacket::setData(const std::vector<float> &) noexcept;
template void URTS::Broadcasts::Internal::ProbabilityPacket::ProbabilityPacket::setData(const std::vector<int> &) noexcept;
template void URTS::Broadcasts::Internal::ProbabilityPacket::ProbabilityPacket::setData(const std::vector<int64_t> &) noexcept;
template void URTS::Broadcasts::Internal::ProbabilityPacket::ProbabilityPacket::setData(const std::vector<int16_t> &) noexcept;
