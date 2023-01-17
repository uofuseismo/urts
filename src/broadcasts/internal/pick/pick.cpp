#include <chrono>
#include <string>
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
    auto phaseHint = pick.getPhaseHint();
    if (!phaseHint.empty())
    {
        obj["PhaseHint"] = phaseHint;
    }
    else
    {
        obj["PhaseHint"] = nullptr;
    }
    // Polarity
    obj["Polarity"] = static_cast<int> (pick.getPolarity()); 
    // Review
    obj["ReviewStatus"] = static_cast<int> (pick.getReviewStatus());

    // Algorithm
    obj["Algorithm"] = pick.getAlgorithm();
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
    pick.setPolarity(
        static_cast<Pick::Polarity> (obj["Polarity"].get<int> ()));
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
    if (!obj["Algorithm"].is_null())
    {
        pick.setAlgorithm(obj["Algorithm"].get<std::string> ());
    }
    return pick;
}

/*
Pick fromJSONMessage(const std::string &message)
{
    auto obj = nlohmann::json::parse(message);
    return ::objectToPick(obj);
}
*/

Pick fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    return ::objectToPick(obj);
}

}

class Pick::PickImpl
{
public:
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    std::string mPhaseHint;
    std::string mAlgorithm{"unspecified"};
    std::pair<UncertaintyBound, UncertaintyBound> mUncertaintyBounds;
    std::chrono::microseconds mTime{0};
    uint64_t mIdentifier{0};
    Pick::Polarity mPolarity{Pick::Polarity::Unknown};
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
    if (::isEmpty(location)){throw std::invalid_argument("location is empty");}
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

/// Polarity
void Pick::setPolarity(const Pick::Polarity polarity) noexcept
{
    pImpl->mPolarity = polarity;
}

Pick::Polarity Pick::getPolarity() const noexcept
{
    return pImpl->mPolarity;
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

/// Algorithm
void Pick::setAlgorithm(const std::string &algorithm) noexcept
{
    pImpl->mAlgorithm = algorithm;
}

std::string Pick::getAlgorithm() const noexcept
{
    return pImpl->mAlgorithm;
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

/// Create JSON
/*
std::string Pick::toJSON(const int nIndent) const
{
    auto obj = toJSONObject(*this);
    return obj.dump(nIndent);
}

/// Create CBOR
std::string Pick::toCBOR() const
{
    auto obj = toJSONObject(*this);
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result; 
}

/// From JSON
void Pick::fromJSON(const std::string &message)
{
    *this = fromJSONMessage(message);
}

/// From CBOR
void Pick::fromCBOR(const std::string &data)
{
    fromCBOR(reinterpret_cast<const uint8_t *> (data.data()), data.size());
}

void Pick::fromCBOR(const uint8_t *data, const size_t length)
{
    if (length == 0){throw std::invalid_argument("No data");}
    if (data == nullptr)
    {
        throw std::invalid_argument("data is NULL");
    }
    *this = fromCBORMessage(data, length);
}
*/

///  Convert message
std::string Pick::toMessage() const
{
    auto obj = ::toJSONObject(*this);
    //std::cout << obj << std::endl;
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

