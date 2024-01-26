#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "urts/broadcasts/internal/pick/pick.hpp"
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"
#include "private/isEmpty.hpp"

#define MESSAGE_TYPE "URTS::Broadcasts::Internal::Pick"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Broadcasts::Internal::Pick;

namespace
{

nlohmann::json toJSONObject(const Pick &pick)
{
    nlohmann::json obj;
    // Essential stuff (this will throw): 
    // Network/Station/Channel/Location
    obj["MessageType"] = pick.getMessageType();
    obj["MessageVersion"] = pick.getMessageVersion();
    obj["Network"] = pick.getNetwork();
    obj["Station"] = pick.getStation();
    obj["Channel"] = pick.getChannel();
    obj["LocationCode"] = pick.getLocationCode();
    // Pick time
    obj["Time"] = static_cast<int64_t> (pick.getTime().count());
    // Identifier
    obj["Identifier"] = pick.getIdentifier();
    // Non-essential stuff:
    if (pick.haveLowerAndUpperUncertaintyBound())
    {
        auto bounds = pick.getLowerAndUpperUncertaintyBound();
        nlohmann::json jsonBounds;
        jsonBounds["LowerPercentile"] = bounds.first.getPercentile();
        jsonBounds["LowerPerturbation"]
            = bounds.first.getPerturbation().count();
        jsonBounds["UpperPercentile"] = bounds.second.getPercentile();
        jsonBounds["UpperPerturbation"]
            = bounds.second.getPerturbation().count();
        obj["UncertaintyBounds"] = jsonBounds;
    }
    else
    {
        obj["UncertaintyBounds"] = nullptr;
    }
    // Original channels
    auto originalChannels = pick.getOriginalChannels();
    if (!originalChannels.empty())
    {
        obj["OriginalChannels"] = originalChannels;
    }
    else
    {
        obj["OriginalChannels"] = nullptr;
    }
    // Phase hint
    auto phaseHint = pick.getPhaseHint();
    if (!phaseHint.empty())
    {
        obj["PhaseHint"] = phaseHint;
    }
    else
    {
        obj["PhaseHint"] = nullptr;
    }
    // First motion
    obj["FirstMotion"] = static_cast<int> (pick.getFirstMotion()); 
    // Review
    obj["ReviewStatus"] = static_cast<int> (pick.getReviewStatus());
    // Algorithm
    obj["ProcessingAlgorithms"] = pick.getProcessingAlgorithms();
    return obj;
}

Pick objectToPick(const nlohmann::json &obj)
{
    Pick pick;
    if (obj["MessageType"] != pick.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    // Essential stuff
    pick.setNetwork(obj["Network"].get<std::string> ());
    pick.setStation(obj["Station"].get<std::string> ());
    pick.setChannel(obj["Channel"].get<std::string> ());
    pick.setLocationCode(obj["LocationCode"].get<std::string> ());
    auto pickTime = std::chrono::microseconds {obj["Time"].get<int64_t> ()};
    pick.setTime(pickTime);
    pick.setIdentifier(obj["Identifier"].get<uint64_t> ());
    // Optional stuff
    if (!obj["OriginalChannels"].is_null())
    {
        std::vector<std::string> originalChannels = obj["OriginalChannels"];
        if (!originalChannels.empty())
        {
            pick.setOriginalChannels(originalChannels);
        }
    }
    pick.setFirstMotion(
        static_cast<Pick::FirstMotion> (obj["FirstMotion"].get<int> ()));
    pick.setReviewStatus(
        static_cast<Pick::ReviewStatus> (obj["ReviewStatus"].get<int> ()));
    if (!obj["UncertaintyBounds"].is_null())
    {
        auto jsonBounds = obj["UncertaintyBounds"];
        auto lowerPercentile = jsonBounds["LowerPercentile"].get<double> ();
        std::chrono::microseconds lowerPerturbation
            {jsonBounds["LowerPerturbation"].get<int64_t> ()};
        UncertaintyBound lowerBound;
        lowerBound.setPercentile(lowerPercentile);
        lowerBound.setPerturbation(lowerPerturbation);
 
        auto upperPercentile = jsonBounds["UpperPercentile"].get<double> ();
        std::chrono::microseconds upperPerturbation
            {jsonBounds["UpperPerturbation"].get<int64_t> ()};
        UncertaintyBound upperBound;
        upperBound.setPercentile(upperPercentile);
        upperBound.setPerturbation(upperPerturbation);

        pick.setLowerAndUpperUncertaintyBound(
            std::pair {lowerBound, upperBound} );
    }
    if (!obj["PhaseHint"].is_null())
    {
        pick.setPhaseHint(obj["PhaseHint"].get<std::string> ());
    }
    if (!obj["ProcessingAlgorithms"].is_null())
    {
        pick.setProcessingAlgorithms(obj["ProcessingAlgorithms"]
                                       .get<std::vector<std::string>> ());
    }
    return pick;
}

Pick fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    return ::objectToPick(obj);
}

}

class Pick::PickImpl
{
public:
    std::vector<std::string> mProcessingAlgorithms;
    std::vector<std::string> mOriginalChannels;
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    std::string mPhaseHint;
    std::string mAlgorithm{"unspecified"};
    std::pair<UncertaintyBound, UncertaintyBound> mUncertaintyBounds;
    std::chrono::microseconds mTime{0};
    uint64_t mIdentifier{0};
    Pick::FirstMotion mFirstMotion{Pick::FirstMotion::Unknown};
    Pick::ReviewStatus mReviewStatus{Pick::ReviewStatus::Automatic};
    bool mHaveUncertaintyBounds{false};
    bool mHaveTime{false};
    bool mHaveIdentifier{false};
};

/// C'tor
Pick::Pick() :
    pImpl(std::make_unique<PickImpl> ())
{
}

/// Copy c'tor
Pick::Pick(const Pick &pick)
{
    *this = pick;
}

/// Move c'tor
Pick::Pick(Pick &&pick) noexcept
{
    *this = std::move(pick);
}

/// Copy assignment
Pick& Pick::operator=(const Pick &pick)
{
    if (&pick == this){return *this;}
    pImpl = std::make_unique<PickImpl> (*pick.pImpl);
    return *this;
}

/// Move assignment
Pick& Pick::operator=(Pick &&pick) noexcept
{
    if (&pick == this){return *this;}
    pImpl = std::move(pick.pImpl);
    return *this;
}

/// Destructor
Pick::~Pick() = default;

/// Reset the class 
void Pick::clear() noexcept
{
    pImpl = std::make_unique<PickImpl> ();
}

/// Network
void Pick::setNetwork(const std::string &network)
{
    if (::isEmpty(network)){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string Pick::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set yet");}
    return pImpl->mNetwork;
}

bool Pick::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void Pick::setStation(const std::string &station)
{
    if (::isEmpty(station)){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = station;
}

std::string Pick::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set yet");}
    return pImpl->mStation;
}

bool Pick::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void Pick::setChannel(const std::string &channel)
{
    if (::isEmpty(channel)){throw std::invalid_argument("Channel is empty");}
    pImpl->mChannel = channel;
}

std::string Pick::getChannel() const
{
    if (!haveChannel()){throw std::runtime_error("Channel not set yet");}
    return pImpl->mChannel;
}

bool Pick::haveChannel() const noexcept
{
    return !pImpl->mChannel.empty();
}

/// Location code
void Pick::setLocationCode(const std::string &location)
{
    if (::isEmpty(location))
    {
        throw std::invalid_argument("Location code is empty");
    }
    pImpl->mLocationCode = location;
}

std::string Pick::getLocationCode() const
{
    if (!haveLocationCode())
    {
        throw std::runtime_error("Location code not set yet");
    }
    return pImpl->mLocationCode;
}

bool Pick::haveLocationCode() const noexcept
{
    return !pImpl->mLocationCode.empty();
}

/// Pick time
void Pick::setTime(const double time) noexcept
{
    auto pickTime
       = std::chrono::microseconds{static_cast<int64_t> (std::round(time*1.e6))};
    setTime(pickTime);
}

void Pick::setTime(const std::chrono::microseconds &time) noexcept
{
    pImpl->mTime = time;
    pImpl->mHaveTime = true;
}

std::chrono::microseconds Pick::getTime() const
{
    if (!haveTime()){throw std::runtime_error("Time not yet set");}
    return pImpl->mTime;
}

bool Pick::haveTime() const noexcept
{
    return pImpl->mHaveTime;
}

/// Pick identifier
void Pick::setIdentifier(const uint64_t id) noexcept
{
    pImpl->mIdentifier = id;
    pImpl->mHaveIdentifier = true;
}

uint64_t Pick::getIdentifier() const
{
    if (!haveIdentifier()){throw std::runtime_error("Identifier not yet set");}
    return pImpl->mIdentifier;
}

bool Pick::haveIdentifier() const noexcept
{
    return pImpl->mHaveIdentifier;
}

/// First motion
void Pick::setFirstMotion(const Pick::FirstMotion firstMotion) noexcept
{
    pImpl->mFirstMotion = firstMotion;
}

Pick::FirstMotion Pick::getFirstMotion() const noexcept
{
    return pImpl->mFirstMotion;
}

/// Review status
void Pick::setReviewStatus(const Pick::ReviewStatus status) noexcept
{
    pImpl->mReviewStatus = status;
}

Pick::ReviewStatus Pick::getReviewStatus() const noexcept
{
    return pImpl->mReviewStatus;
}

/// Algorithms
void Pick::setProcessingAlgorithms(
    const std::vector<std::string> &algorithms) noexcept
{
    pImpl->mProcessingAlgorithms = algorithms;
}

std::vector<std::string> Pick::getProcessingAlgorithms() const noexcept
{
    return pImpl->mProcessingAlgorithms;
}

/// Phase hint
void Pick::setPhaseHint(const std::string &phaseHint) noexcept
{
    pImpl->mPhaseHint = phaseHint;
}

std::string Pick::getPhaseHint() const noexcept
{
    return pImpl->mPhaseHint;
}

void Pick::setLowerAndUpperUncertaintyBound(
    const std::pair<UncertaintyBound, UncertaintyBound> &lowerAndUpperBound)
{
    auto lowerBound = lowerAndUpperBound.first;
    auto upperBound = lowerAndUpperBound.second;
    if (lowerBound.getPercentile() > upperBound.getPercentile())
    {
        throw std::invalid_argument(
           "Lower percentile greater than upper percentile");
    }
    if (lowerBound.getPerturbation() > upperBound.getPerturbation())
    {
        throw std::invalid_argument(
           "Lower perturbation greater than upper perturbation");
    }
    pImpl->mUncertaintyBounds = lowerAndUpperBound;
    pImpl->mHaveUncertaintyBounds = true;
}

std::pair<UncertaintyBound, UncertaintyBound>
Pick::getLowerAndUpperUncertaintyBound() const
{
    if (!haveLowerAndUpperUncertaintyBound())
    {
        throw std::runtime_error("Uncertainty bounds not set");
    }
    return pImpl->mUncertaintyBounds;
}

bool Pick::haveLowerAndUpperUncertaintyBound() const noexcept
{
    return pImpl->mHaveUncertaintyBounds;
}

/// Original packets
void Pick::setOriginalChannels(
    const std::vector<std::string> &originalChannels) noexcept
{
    pImpl->mOriginalChannels = originalChannels;
}

std::vector<std::string> Pick::getOriginalChannels() const noexcept
{
    return pImpl->mOriginalChannels;
}

///  Convert message
std::string Pick::toMessage() const
{
    auto obj = ::toJSONObject(*this);
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result; 
}

void Pick::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());   
}

void Pick::fromMessage(const char *messageIn, const size_t length)
{
    if (length == 0){throw std::invalid_argument("No data");}
    if (messageIn == nullptr)
    {
        throw std::invalid_argument("Message is NULL");
    }
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage> Pick::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<Pick> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    Pick::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<Pick> ();
    return result;
}

/// Message type
std::string Pick::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string Pick::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

std::ostream&
URTS::Broadcasts::Internal::Pick::operator<<(std::ostream &os, const Pick &pick)
{
    try
    {
        auto object = ::toJSONObject(pick);
        return os << object.dump(4); 
    }
    catch (const std::exception &e)
    {
        return os << e.what();
    }
    return os;
}
