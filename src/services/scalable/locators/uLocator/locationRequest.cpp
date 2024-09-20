#include <nlohmann/json.hpp>
#include "urts/services/scalable/locators/uLocator/locationRequest.hpp"
#include "urts/services/scalable/locators/uLocator/arrival.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Locators::ULocator::LocationRequest"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Locators::ULocator;

namespace
{

std::string toCBORObject(const LocationRequest &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    obj["LocationStrategy"] = static_cast<int> (message.getLocationStrategy());
    const auto &arrivals = message.getArrivalsReference();
    if (!arrivals.empty())
    {
        nlohmann::json jsonArrivals;
        for (const auto &arrival : arrivals)
        {
            nlohmann::json arrivalObject;
            arrivalObject["Network"] = arrival.getNetwork();
            arrivalObject["Station"] = arrival.getStation();
            arrivalObject["Time"]
                 = static_cast<int64_t> (arrival.getTime().count());
            arrivalObject["Phase"] = static_cast<int> (arrival.getPhase());
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
            jsonArrivals.push_back(std::move(arrivalObject)); 
        }
        obj["Arrivals"] = jsonArrivals;
    }
    else
    {
        throw std::runtime_error("No arrivals!");
    }
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

LocationRequest
    fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    LocationRequest result;
    if (obj["MessageType"] != result.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    result.setIdentifier(obj["Identifier"].template get<int64_t> ());
    result.setLocationStrategy(
        static_cast<LocationRequest::LocationStrategy> (
            obj["LocationStrategy"].template get<int> ()
        )
    );
    if (!obj["Arrivals"].is_null())
    {
        std::vector<Arrival> arrivals;
        for (const auto &arrivalObject : obj["Arrivals"])
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
            arrivals.push_back(std::move(arrival));
        }
        result.setArrivals(arrivals);
    }
    return result;
}

}

class LocationRequest::LocationRequestImpl
{
public:
    std::vector<Arrival> mArrivals;
    int64_t mIdentifier{0};
    LocationStrategy mLocationStrategy{LocationStrategy::General};
};


/// Constructor
LocationRequest::LocationRequest() :
    pImpl(std::make_unique<LocationRequestImpl> ())
{
}

/// Copy constructor
LocationRequest::LocationRequest(
    const LocationRequest &request)
{
    *this = request;
}

/// Move constructor
LocationRequest::LocationRequest(LocationRequest &&request) noexcept
{
    *this = std::move(request);
}

/// Copy assignment
LocationRequest& 
LocationRequest::operator=(const LocationRequest &request)
{
    if (&request == this){return *this;}
    pImpl = std::make_unique<LocationRequestImpl> (*request.pImpl);
    return *this;
}

/// Move assignment
LocationRequest&
LocationRequest::operator=(LocationRequest &&request) noexcept
{
    if (&request == this){return *this;}
    pImpl = std::move(request.pImpl);
    return *this;
}

/// Destructor
LocationRequest::~LocationRequest() = default;

/// Reset class and release memory
void LocationRequest::clear() noexcept
{
    pImpl = std::make_unique<LocationRequestImpl> ();
}

/// Message type
std::string LocationRequest::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string LocationRequest::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

/// Arrivals
void LocationRequest::setArrivals(const std::vector<Arrival> &arrivals)
{
    if (arrivals.empty()){throw std::invalid_argument("No arrivals!");}
    for (const auto &arrival : arrivals)
    {
        if (!arrival.haveNetwork())
        {
            throw std::invalid_argument("Network not set");
        }
        if (!arrival.haveStation())
        {
            throw std::invalid_argument("Station not set");
        }
        if (!arrival.haveTime())
        {
            throw std::invalid_argument("Arrival time not set");
        }
        if (!arrival.havePhase())
        {
            throw std::invalid_argument("Phase not set");
        }
    }
    // Check the arrivals make sense
    auto nArrivals = static_cast<int> (arrivals.size());
    for (int i = 0; i < nArrivals; ++i)
    { 
        for (int j = i + 1; j < nArrivals; ++j)
        {
            if (arrivals[i].getNetwork() == arrivals[j].getNetwork() &&
                arrivals[i].getStation() == arrivals[j].getStation())
            {
                // Duplicate phase
                if (arrivals[i].getPhase() == arrivals[j].getPhase())
                {
                    throw std::invalid_argument("Duplicate phase arrival");
                }
                if (arrivals[i].getPhase() == Arrival::Phase::P)
                {
                    if (arrivals[j].getTime() <= arrivals[i].getTime())
                    {
                        throw std::invalid_argument("S precedes P arrival");
                    }
                }
                else if (arrivals[i].getPhase() == Arrival::Phase::S)
                {
                    if (arrivals[i].getTime() <= arrivals[j].getTime())
                    {
                        throw std::invalid_argument("S precedes P arrival");
                    }
                }
                else
                { 
                    throw std::invalid_argument("Invalid phase");
                }
            }
        }
    }
    pImpl->mArrivals = arrivals;
}

std::vector<Arrival> LocationRequest::getArrivals() const
{
    if (!haveArrivals()){throw std::runtime_error("No arrivals set");}
    return pImpl->mArrivals;
}

const std::vector<Arrival>
    &LocationRequest::getArrivalsReference() const
{
    if (!haveArrivals()){throw std::runtime_error("No arrivals set");}
    return *&pImpl->mArrivals;
}

bool LocationRequest::haveArrivals() const noexcept
{
    return !pImpl->mArrivals.empty();
}

/// Identifier
void LocationRequest::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

int64_t LocationRequest::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Location strategy
void LocationRequest::setLocationStrategy(
    const LocationStrategy strategy) noexcept
{
    pImpl->mLocationStrategy = strategy;
}

LocationRequest::LocationStrategy
LocationRequest::getLocationStrategy() const noexcept
{
    return pImpl->mLocationStrategy;
}

/// Convert message
std::string LocationRequest::toMessage() const
{
    return ::toCBORObject(*this);
}

void LocationRequest::fromMessage(
    const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

void LocationRequest::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());   
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage>
    LocationRequest::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<LocationRequest> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    LocationRequest::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<LocationRequest> ();
    return result;
}
