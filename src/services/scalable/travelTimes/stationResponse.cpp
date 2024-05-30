#include <string>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "urts/services/scalable/travelTimes/stationResponse.hpp"
#include "database/aqms/utilities.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::TravelTimes::StationResponse"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::TravelTimes;

namespace
{

std::string toCBORObject(const StationResponse &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["TravelTime"] = message.getTravelTime(); // Throws
    obj["ReturnCode"] = static_cast<int> (message.getReturnCode()); 
    obj["Identifier"] = message.getIdentifier();
    obj["Region"] = static_cast<int> (message.getRegion()); 
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

StationResponse
    fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    StationResponse result;
    if (obj["MessageType"] != result.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    result.setTravelTime(obj["TravelTime"].template get<double> ());
    result.setReturnCode(
        static_cast<StationResponse::ReturnCode>
            (obj["ReturnCode"].template get<int> ()));
    result.setIdentifier(obj["Identifier"].template get<int64_t> ());
    result.setRegion(
        static_cast<StationRequest::Region>
            (obj["Region"].template get<int> ()));
    return result;
}

}


class StationResponse::StationResponseImpl
{
public:
    double mTravelTime{0};
    int64_t mIdentifier{0};
    StationResponse::ReturnCode
        mReturnCode{StationResponse::ReturnCode::AlgorithmicFailure};
    StationRequest::Region mRegion{StationRequest::Region::EventBased};
    bool mHaveTravelTime{false};
    bool mHaveReturnCode{false};
};

/// Constructor
StationResponse::StationResponse() :
    pImpl(std::make_unique<StationResponseImpl> ())
{
}

/// Copy constructor
StationResponse::StationResponse(const StationResponse &response)
{
    *this = response;
}

/// Move constructor
StationResponse::StationResponse(StationResponse &&response) noexcept
{
    *this = std::move(response);
}

/// Copy assignment
StationResponse& StationResponse::operator=(const StationResponse &response)
{
    if (&response == this){return *this;}
    pImpl = std::make_unique<StationResponseImpl> (*response.pImpl);
    return *this;
}

/// Move assignment
StationResponse& StationResponse::operator=(StationResponse &&response) noexcept
{
    if (&response == this){return *this;}
    pImpl = std::move (response.pImpl);
    return *this;
}

/// Destructor
StationResponse::~StationResponse() = default;

/// Reset class
void StationResponse::clear() noexcept
{
    pImpl = std::make_unique<StationResponseImpl> ();
}

/// Travel time
void StationResponse::setTravelTime(const double travelTime)
{
    if (travelTime < 0)
    {
        throw std::invalid_argument("Travel time must be non-negative");
    }
    pImpl->mTravelTime = travelTime;
    pImpl->mHaveTravelTime = true;
}

double StationResponse::getTravelTime() const
{
    if (!haveTravelTime()){throw std::runtime_error("Travel time not set");}
    return pImpl->mTravelTime;
}

bool StationResponse::haveTravelTime() const noexcept
{
    return pImpl->mHaveTravelTime;
}

/// Return code
void StationResponse::setReturnCode(const ReturnCode returnCode) noexcept
{
    pImpl->mReturnCode = returnCode;
    pImpl->mHaveReturnCode = true;
}

StationResponse::ReturnCode StationResponse::getReturnCode() const
{
    if (!haveReturnCode()){throw std::runtime_error("Return code not set");}
    return pImpl->mReturnCode;
}

bool StationResponse::haveReturnCode() const noexcept
{
    return pImpl->mHaveReturnCode;
}

/// Region
void StationResponse::setRegion(const StationRequest::Region region) noexcept
{
    pImpl->mRegion = region;
}

StationRequest::Region StationResponse::getRegion() const noexcept
{
    return pImpl->mRegion;
}

/// Identifier
void StationResponse::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

int64_t StationResponse::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Message type
std::string StationResponse::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string
StationResponse::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///  Convert message
std::string StationResponse::toMessage() const
{
    return ::toCBORObject(*this);
}

void StationResponse::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());
}

void StationResponse::fromMessage(
    const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage>
    StationResponse::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<StationResponse> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    StationResponse::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<StationResponse> (); 
    return result;
}
