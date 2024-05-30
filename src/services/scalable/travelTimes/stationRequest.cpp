#include <string>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "urts/services/scalable/travelTimes/stationRequest.hpp"
#include "database/aqms/utilities.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::TravelTimes::StationRequest"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::TravelTimes;

namespace
{

std::string toCBORObject(const StationRequest &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Network"] = message.getNetwork(); // Throws
    obj["Station"] = message.getStation(); // Throws
    obj["EventLatitude"]  = message.getLatitude();  // Throws
    obj["EventLongitude"] = message.getLongitude(); // Throws
    obj["EventDepth"]     = message.getDepth();     // Throws 
    obj["Identifier"] = message.getIdentifier();
    obj["Phase"] = static_cast<int> (message.getPhase()); // Throws
    obj["Region"] = static_cast<int> (message.getRegion()); 
    obj["UseCorrections"] = message.useCorrections();
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

StationRequest
    fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    StationRequest result;
    if (obj["MessageType"] != result.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    result.setNetwork(obj["Network"].template get<std::string> ());
    result.setStation(obj["Station"].template get<std::string> ());
    result.setLatitude(obj["EventLatitude"].template get<double> ());
    result.setLongitude(obj["EventLongitude"].template get<double> ());
    result.setDepth(obj["EventDepth"].template get<double> ());
    result.setIdentifier(obj["Identifier"].template get<int64_t> ());
    result.setPhase(
        static_cast<StationRequest::Phase> (obj["Phase"].template get<int> ()));
    result.setRegion(
        static_cast<StationRequest::Region>
            (obj["Region"].template get<int> ()));
    if (obj["UseCorrections"].template get<bool> ())
    {
        result.enableCorrections();
    }
    else
    {
        result.disableCorrections();
    }
    return result;
}

}


class StationRequest::StationRequestImpl
{
public:
    std::string mNetwork;
    std::string mStation;
    double mDepth{0};
    double mLatitude{0};
    double mLongitude{0};
    int64_t mIdentifier{0};
    StationRequest::Phase mPhase{StationRequest::Phase::P};
    StationRequest::Region mRegion{StationRequest::Region::EventBased};
    bool mHaveDepth{false};
    bool mHaveLatitude{false};
    bool mHaveLongitude{false};
    bool mHavePhase{false};
    bool mUseCorrections{true};
};

/// Constructor
StationRequest::StationRequest() :
    pImpl(std::make_unique<StationRequestImpl> ())
{
}

/// Copy constructor
StationRequest::StationRequest(const StationRequest &request)
{
    *this = request;
}

/// Move constructor
StationRequest::StationRequest(StationRequest &&request) noexcept
{
    *this = std::move(request);
}

/// Copy assignment
StationRequest& StationRequest::operator=(const StationRequest &request)
{
    if (&request == this){return *this;}
    pImpl = std::make_unique<StationRequestImpl> (*request.pImpl);
    return *this;
}

/// Move assignment
StationRequest& StationRequest::operator=(StationRequest &&request) noexcept
{
    if (&request == this){return *this;}
    pImpl = std::move (request.pImpl);
    return *this;
}

/// Destructor
StationRequest::~StationRequest() = default;

/// Reset class
void StationRequest::clear() noexcept
{
    pImpl = std::make_unique<StationRequestImpl> ();
}

/// Network
void StationRequest::setNetwork(const std::string &networkIn)
{
    auto network = networkIn;
    std::remove_if(network.begin(), network.end(), ::isspace);
    std::transform(network.begin(), network.end(), network.begin(), ::toupper);
    if (network.empty()){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string StationRequest::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set");}
    return pImpl->mNetwork;
}

bool StationRequest::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void StationRequest::setStation(const std::string &stationIn)
{
    auto station = stationIn;
    std::remove_if(station.begin(), station.end(), ::isspace);
    std::transform(station.begin(), station.end(), station.begin(), ::toupper);
    if (station.empty()){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = station;
}

std::string StationRequest::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set");}
    return pImpl->mStation;
}

bool StationRequest::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Latitude
void StationRequest::setLatitude(const double latitude)
{
    if (latitude < -90 || latitude > 90)
    {
        throw std::invalid_argument("Latitude must be in range [-90,90]");
    }
    pImpl->mLatitude = latitude;
    pImpl->mHaveLatitude = true;
}

double StationRequest::getLatitude() const
{
    if (!haveLatitude()){throw std::runtime_error("Latitude not set");}
    return pImpl->mLatitude;
}

bool StationRequest::haveLatitude() const noexcept
{
    return pImpl->mHaveLatitude;
}

/// Longitude
void StationRequest::setLongitude(const double longitude) noexcept
{
    pImpl->mLongitude = ::lonTo180(longitude);
    pImpl->mHaveLongitude = true;
}

double StationRequest::getLongitude() const
{
    if (!haveLongitude()){throw std::runtime_error("Longitude not set");}
    return pImpl->mLongitude;
}

bool StationRequest::haveLongitude() const noexcept
{
    return pImpl->mHaveLongitude;
}


/// Depth
void StationRequest::setDepth(const double depth)
{
    if (depth < -8800 || depth > 800000)
    {
        throw std::invalid_argument("Depth must be in range [-8800, 800000]");
    }
    pImpl->mDepth = depth;
    pImpl->mHaveDepth = true;
}

double StationRequest::getDepth() const
{
    if (!haveDepth()){throw std::runtime_error("Depth not set");}
    return pImpl->mDepth;
}

bool StationRequest::haveDepth() const noexcept
{
    return pImpl->mHaveDepth;
}

/// Phase
void StationRequest::setPhase(const Phase phase) noexcept
{
    pImpl->mPhase = phase;
    pImpl->mHavePhase = true;
}

StationRequest::Phase StationRequest::getPhase() const
{
    if (!havePhase()){throw std::runtime_error("Phase not set");}
    return pImpl->mPhase;
}

bool StationRequest::havePhase() const noexcept
{
    return pImpl->mHavePhase;
}

/// Region
void StationRequest::setRegion(const Region region) noexcept
{
    pImpl->mRegion = region;
}

StationRequest::Region StationRequest::getRegion() const noexcept
{
    return pImpl->mRegion;
}

/// Corrections
void StationRequest::enableCorrections() noexcept
{
    pImpl->mUseCorrections = true;
}

void StationRequest::disableCorrections() noexcept
{
    pImpl->mUseCorrections = false;
}

bool StationRequest::useCorrections() const noexcept
{
    return pImpl->mUseCorrections;
}

/// Identifier
void StationRequest::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

int64_t StationRequest::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Message type
std::string StationRequest::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string
StationRequest::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///  Convert message
std::string StationRequest::toMessage() const
{
    return ::toCBORObject(*this);
}

void StationRequest::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());
}

void StationRequest::fromMessage(
    const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage>
    StationRequest::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<StationRequest> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    StationRequest::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<StationRequest> (); 
    return result;
}
