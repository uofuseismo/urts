#include <nlohmann/json.hpp>
#include "urts/services/scalable/locators/uLocator/locationResponse.hpp"
#include "urts/services/scalable/locators/uLocator/origin.hpp"
#include "urts/services/scalable/locators/uLocator/arrival.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Locators::ULocator::LocationResponse"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Locators::ULocator;

namespace
{

std::string toCBORObject(const LocationResponse &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    obj["ReturnCode"] = static_cast<int> (message.getReturnCode());
    auto origin = message.getOrigin();
    if (origin)
    {
        nlohmann::json originObject;
        originObject["Latitude"] = origin->getLatitude();
        originObject["Longitude"] = origin->getLongitude();
        originObject["Depth"] = origin->getDepth();
        originObject["Time"]
            = static_cast<int64_t> (origin->getTime().count());
        auto originIdentifier = origin->getIdentifier();
        if (originIdentifier)
        {
            originObject["Identifier"] = *originIdentifier;
        }
        originObject["DepthFixedToFreeSurface"]
            = origin->depthFixedToFreeSurface();
        const auto &arrivals = origin->getArrivalsReference();
        if (!arrivals.empty())
        {
            nlohmann::json arrivalObjects;
            for (const auto &arrival : arrivals)
            {
                nlohmann::json arrivalObject;
                arrivalObject["Network"] = arrival.getNetwork();
                arrivalObject["Station"] = arrival.getStation();
                arrivalObject["Time"]
                    = static_cast<int64_t> (arrival.getTime().count());
                arrivalObject["Phase"]
                    = static_cast<int> (arrival.getPhase());
                auto standardError = arrival.getStandardError();
                if (standardError)
                {
                    arrivalObject["StandardError"] = *standardError;
                }
                auto arrivalIdentifier = arrival.getIdentifier();
                if (arrivalIdentifier)
                {
                    arrivalObject["Identifier"] = *arrivalIdentifier;
                }
                auto travelTime = arrival.getTravelTime();
                if (travelTime)
                {
                    arrivalObject["TravelTime"] = *travelTime;
                }
                arrivalObjects.push_back(std::move(arrivalObject));
            }
            originObject["Arrivals"] = arrivalObjects;
        }
        obj["Origin"] = originObject;
    }
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

LocationResponse
    fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    LocationResponse result;
    if (obj["MessageType"] != result.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    result.setIdentifier(obj["Identifier"].template get<int64_t> ());
    result.setReturnCode(
        static_cast<LocationResponse::ReturnCode>
        (obj["ReturnCode"].template get<int> ()));
    if (obj.contains("Origin"))
    {
        auto originObject = obj["Origin"];
        Origin origin;
        origin.setLatitude(
            originObject["Latitude"].template get<double> ());
        origin.setLongitude(
            originObject["Longitude"].template get<double> ());
        origin.setDepth(originObject["Depth"].template get<double> ());
            origin.setTime(
                std::chrono::microseconds
                {
                     originObject["Time"].template get<int64_t> ()
                });
        if (originObject.contains("Identifier"))
        {
            origin.setIdentifier(
                originObject["Identifier"].template get<int64_t> ());
        }
        auto depthFixed
            = originObject["DepthFixedToFreeSurface"].template get<bool> ();
        origin.toggleDepthFixedToFreeSurface(depthFixed);
        if (originObject.contains("Arrivals"))
        {
            std::vector<Arrival> arrivals;
            for (const auto &arrivalObject : originObject["Arrivals"])
            {
                Arrival arrival;
                arrival.setNetwork(arrivalObject["Network"].template get<std::string> ());
                arrival.setStation(arrivalObject["Station"].template get<std::string> ());
                auto iTime = arrivalObject["Time"].template get<int64_t> (); 
                arrival.setTime(std::chrono::microseconds {iTime});
                auto phase
                    = static_cast<Arrival::Phase>
                      (arrivalObject["Phase"].template get<int> ());
                arrival.setPhase(phase);
                if (arrivalObject.contains("StandardError"))
                {
                    arrival.setStandardError(
                        arrivalObject["StandardError"].template get<double> ());
                }
                if (arrivalObject.contains("Identifier"))
                {
                    arrival.setIdentifier(
                        arrivalObject["Identifier"].template get<int64_t> ());
                }
                if (arrivalObject.contains("TravelTime"))
                {
                    arrival.setTravelTime(
                        arrivalObject["TravelTime"].template get<double> ());
                }
                arrivals.push_back(std::move(arrival));
            }
            if (!arrivals.empty()){origin.setArrivals(arrivals);}
        }
        result.setOrigin(origin);
    }
    return result;
}

}

class LocationResponse::LocationResponseImpl
{
public:
    Origin mOrigin;
    int64_t mIdentifier{0};
    ReturnCode mReturnCode{ReturnCode::AlgorithmicFailure};
    bool mHaveReturnCode{false};
    bool mHaveOrigin{false};
};

/// Constructor
LocationResponse::LocationResponse() :
    pImpl(std::make_unique<LocationResponseImpl> ())
{
}

/// Copy constructor
LocationResponse::LocationResponse(
    const LocationResponse &response)
{
    *this = response;
}

/// Move constructor
LocationResponse::LocationResponse(
    LocationResponse &&response) noexcept
{
    *this = std::move(response);
}

/// Copy assignment
LocationResponse& 
LocationResponse::operator=(const LocationResponse &response)
{
    if (&response == this){return *this;}
    pImpl = std::make_unique<LocationResponseImpl> (*response.pImpl);
    return *this;
}

/// Move assignment
LocationResponse&
LocationResponse::operator=(LocationResponse &&response) noexcept
{
    if (&response == this){return *this;}
    pImpl = std::move(response.pImpl);
    return *this;
}

/// Destructor
LocationResponse::~LocationResponse() = default;

/// Reset class
void LocationResponse::clear() noexcept
{
    pImpl = std::make_unique<LocationResponseImpl> ();
}

/// Origin
void LocationResponse::setOrigin(const Origin &origin)
{
    if (!origin.haveLatitude())
    {
        throw std::invalid_argument("Latitude not set");
    }
    if (!origin.haveLongitude())
    {
        throw std::invalid_argument("Longitude not set");
    }
    if (!origin.haveDepth())
    {
        throw std::invalid_argument("Depth not set");
    }
    if (!origin.haveTime())
    {
        throw std::invalid_argument("Origin time not set");
    }
    pImpl->mOrigin = origin;
    pImpl->mHaveOrigin = true;
}

std::optional<Origin> LocationResponse::getOrigin() const noexcept
{
    return pImpl->mHaveOrigin ?
           std::optional<Origin> (pImpl->mOrigin) : std::nullopt;
}

/// Identifier
void LocationResponse::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

int64_t LocationResponse::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Return code
void LocationResponse::setReturnCode(ReturnCode returnCode) noexcept
{
    pImpl->mReturnCode = returnCode;
    pImpl->mHaveReturnCode = true;
}

LocationResponse::ReturnCode LocationResponse::getReturnCode() const
{
    if (!haveReturnCode()){throw std::runtime_error("Return code not set");}
    return pImpl->mReturnCode;
}

bool LocationResponse::haveReturnCode() const noexcept
{
    return pImpl->mHaveReturnCode;
}

/// Message type
std::string LocationResponse::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string LocationResponse::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///  Convert message
std::string LocationResponse::toMessage() const
{
    return ::toCBORObject(*this);
}

void LocationResponse::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());   
}

void LocationResponse::fromMessage(
    const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage>
    LocationResponse::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<LocationResponse> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    LocationResponse::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<LocationResponse> ();
    return result;
}
