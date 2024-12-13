#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "urts/broadcasts/internal/origin/arrival.hpp"
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"
#include "private/isEmpty.hpp"


using namespace URTS::Broadcasts::Internal::Origin;
namespace UPick = URTS::Broadcasts::Internal::Pick;

/*
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
    // Phase
    obj["Phase"] = pick.getPhase();
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
*/

class Arrival::ArrivalImpl
{
public:
    std::vector<std::string> mProcessingAlgorithms;
    std::vector<std::string> mOriginalChannels;
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    //std::string mAlgorithm{"unspecified"};
    std::pair<UPick::UncertaintyBound, UPick::UncertaintyBound>
        mUncertaintyBounds;
    std::chrono::microseconds mTime{0};
    double mResidual{0};
    double mSignalToNoiseRatio{0};
    uint64_t mIdentifier{0};
    uint64_t mOriginIdentifier{0};
    Arrival::Phase mPhase;
    Arrival::FirstMotion mFirstMotion{Arrival::FirstMotion::Unknown};
    Arrival::ReviewStatus mReviewStatus{Arrival::ReviewStatus::Automatic};
    bool mHaveUncertaintyBounds{false};
    bool mHaveTime{false};
    bool mHavePhase{false};
    bool mHaveIdentifier{false};
    bool mHaveResidual{false};
    bool mHaveOriginIdentifier{false};
    bool mHaveSignalToNoiseRatio{false};
};

/// Constructor
Arrival::Arrival() :
    pImpl(std::make_unique<ArrivalImpl> ())
{
}

/// Copy construtcor
Arrival::Arrival(const Arrival &arrival)
{
    *this = arrival;
}

/// Move construtcor
Arrival::Arrival(Arrival &&arrival) noexcept
{
    *this = std::move(arrival);
}

/// Copy assignment
Arrival& Arrival::operator=(const Arrival &arrival)
{
    if (&arrival == this){return *this;}
    pImpl = std::make_unique<ArrivalImpl> (*arrival.pImpl);
    return *this;
}

/// Move assignment
Arrival& Arrival::operator=(Arrival &&arrival) noexcept
{
    if (&arrival == this){return *this;}
    pImpl = std::move(arrival.pImpl);
    return *this;
}

/// Destructor
Arrival::~Arrival() = default;

/// Reset the class 
void Arrival::clear() noexcept
{
    pImpl = std::make_unique<ArrivalImpl> ();
}

/// Network
void Arrival::setNetwork(const std::string &network)
{
    if (::isEmpty(network)){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string Arrival::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set yet");}
    return pImpl->mNetwork;
}

bool Arrival::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void Arrival::setStation(const std::string &station)
{
    if (::isEmpty(station)){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = station;
}

std::string Arrival::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set yet");}
    return pImpl->mStation;
}

bool Arrival::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void Arrival::setChannel(const std::string &channel)
{
    if (::isEmpty(channel)){throw std::invalid_argument("Channel is empty");}
    pImpl->mChannel = channel;
}

std::string Arrival::getChannel() const
{
    if (!haveChannel()){throw std::runtime_error("Channel not set yet");}
    return pImpl->mChannel;
}

bool Arrival::haveChannel() const noexcept
{
    return !pImpl->mChannel.empty();
}

/// Location code
void Arrival::setLocationCode(const std::string &location)
{
    if (::isEmpty(location))
    {
        throw std::invalid_argument("Location code is empty");
    }
    pImpl->mLocationCode = location;
}

std::string Arrival::getLocationCode() const
{
    if (!haveLocationCode())
    {
        throw std::runtime_error("Location code not set yet");
    }
    return pImpl->mLocationCode;
}

bool Arrival::haveLocationCode() const noexcept
{
    return !pImpl->mLocationCode.empty();
}

/// Arrival time
void Arrival::setTime(const double time) noexcept
{
    auto pickTime
       = std::chrono::microseconds{static_cast<int64_t> (std::round(time*1.e6))};
    setTime(pickTime);
}

void Arrival::setTime(const std::chrono::microseconds &time) noexcept
{
    pImpl->mTime = time;
    pImpl->mHaveTime = true;
}

std::chrono::microseconds Arrival::getTime() const
{
    if (!haveTime()){throw std::runtime_error("Time not yet set");}
    return pImpl->mTime;
}

bool Arrival::haveTime() const noexcept
{
    return pImpl->mHaveTime;
}

/// Arrival identifier
void Arrival::setIdentifier(const uint64_t id) noexcept
{
    pImpl->mIdentifier = id;
    pImpl->mHaveIdentifier = true;
}

uint64_t Arrival::getIdentifier() const
{
    if (!haveIdentifier()){throw std::runtime_error("Identifier not yet set");}
    return pImpl->mIdentifier;
}

bool Arrival::haveIdentifier() const noexcept
{
    return pImpl->mHaveIdentifier;
}

/// First motion
void Arrival::setFirstMotion(const Arrival::FirstMotion firstMotion) noexcept
{
    pImpl->mFirstMotion = firstMotion;
}

Arrival::FirstMotion Arrival::getFirstMotion() const noexcept
{
    return pImpl->mFirstMotion;
}

/// Review status
void Arrival::setReviewStatus(const Arrival::ReviewStatus status) noexcept
{
    pImpl->mReviewStatus = status;
}

Arrival::ReviewStatus Arrival::getReviewStatus() const noexcept
{
    return pImpl->mReviewStatus;
}

/// Algorithms
void Arrival::setProcessingAlgorithms(
    const std::vector<std::string> &algorithms) noexcept
{
    pImpl->mProcessingAlgorithms = algorithms;
}

std::vector<std::string> Arrival::getProcessingAlgorithms() const noexcept
{
    return pImpl->mProcessingAlgorithms;
}

/// Phase hint
void Arrival::setPhase(const Phase phase) noexcept
{
    pImpl->mPhase = phase;
    pImpl->mHavePhase = true;
}

Arrival::Phase Arrival::getPhase() const
{
    if (!havePhase()){throw std::runtime_error("Phase not set");}
    return pImpl->mPhase;
}

bool Arrival::havePhase() const noexcept
{
    return pImpl->mHavePhase;
}

void Arrival::setLowerAndUpperUncertaintyBound(
    const std::pair<UPick::UncertaintyBound,
                    UPick::UncertaintyBound> &lowerAndUpperBound)
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

std::optional<std::pair<UPick::UncertaintyBound, UPick::UncertaintyBound>>
Arrival::getLowerAndUpperUncertaintyBound() const
{
    if (pImpl->mHaveUncertaintyBounds)
    {
        return pImpl->mUncertaintyBounds;
    }
    return std::nullopt;
}

/// Residual
void Arrival::setResidual(double residual) noexcept
{
    pImpl->mResidual = residual;
    pImpl->mHaveResidual = true;
}

std::optional<double> Arrival::getResidual() const noexcept
{
    if (pImpl->mHaveResidual){return pImpl->mResidual;}
    return std::nullopt;
}

/// SNR
void Arrival::setSignalToNoiseRatio(const double snr) noexcept
{
    pImpl->mSignalToNoiseRatio = snr;
    pImpl->mHaveSignalToNoiseRatio = true;
}

std::optional<double> Arrival::getSignalToNoiseRatio() const noexcept
{
    if (pImpl->mHaveSignalToNoiseRatio){return pImpl->mSignalToNoiseRatio;}
    return std::nullopt;
}

/// Origin identifier
void Arrival::setOriginIdentifier(const uint64_t id) noexcept
{
    pImpl->mOriginIdentifier = id; 
    pImpl->mHaveOriginIdentifier = true;
}

std::optional<uint64_t> Arrival::getOriginIdentifier() const noexcept
{
    if (pImpl->mHaveOriginIdentifier)
    {
        return pImpl->mIdentifier;
    }
    return std::nullopt;
}


/// Original channels
void Arrival::setOriginalChannels(
    const std::vector<std::string> &originalChannels) noexcept
{
    pImpl->mOriginalChannels = originalChannels;
}

std::vector<std::string> Arrival::getOriginalChannels() const noexcept
{
    return pImpl->mOriginalChannels;
}
