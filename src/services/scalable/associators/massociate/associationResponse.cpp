#include <vector>
#include <nlohmann/json.hpp>
#include "urts/services/scalable/associators/massociate/associationResponse.hpp"
#include "urts/services/scalable/associators/massociate/arrival.hpp"
#include "urts/services/scalable/associators/massociate/origin.hpp"
#include "urts/services/scalable/associators/massociate/pick.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Associators::MAssociate::AssocationResponse"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Associators::MAssociate;

namespace
{

std::string toCBORObject(const AssociationResponse &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    obj["ReturnCode"] = static_cast<int> (message.getReturnCode());
    const auto &origins = message.getOriginsReference();
    if (!origins.empty())
    {
        nlohmann::json jsonOrigins;
        for (const auto &origin : origins)
        {
            nlohmann::json originObject;
            originObject["Latitude"] = origin.getLatitude();
            originObject["Longitude"] = origin.getLongitude();
            originObject["Depth"] = origin.getDepth();
            originObject["Time"]
                = static_cast<int64_t> (origin.getTime().count());
            const auto &arrivals = origin.getArrivalsReference(); 
            if (!arrivals.empty())
            {
                nlohmann::json arrivalObjects;
                for (const auto &arrival : arrivals)
                {
                    nlohmann::json arrivalObject;
                    arrivalObject["Network"] = arrival.getNetwork();
                    arrivalObject["Station"] = arrival.getStation();
                    arrivalObject["Channel"] = arrival.getChannel();
                    arrivalObject["LocationCode"] = arrival.getLocationCode();
                    arrivalObject["Time"]
                        = static_cast<int64_t> (arrival.getTime().count());
                    arrivalObject["Phase"]
                        = static_cast<int> (arrival.getPhase());
                    arrivalObject["Identifier"] = arrival.getIdentifier();
                    if (auto travelTime = arrival.getTravelTime())
                    {
                        arrivalObject["TravelTime"] = *travelTime;
                    }
                    else
                    {
                        arrivalObject["TravelTime"] = nullptr;
                    }
                    arrivalObjects.push_back(std::move(arrivalObject));
                }
                originObject["Arrivals"] = arrivalObjects;
            }
            else
            {
                originObject["Arrivals"] = nullptr;
            }
            jsonOrigins.push_back(std::move(originObject));
        }
        obj["Origins"] = jsonOrigins;
    }
    else
    {
        obj["Origins"] = nullptr;
    }
    const auto &picks = message.getUnassociatedPicksReference();
    if (!picks.empty())
    {
        nlohmann::json jsonPicks;
        for (const auto &pick : picks)
        {
            nlohmann::json pickObject;
            pickObject["Network"] = pick.getNetwork();
            pickObject["Station"] = pick.getStation();
            pickObject["Channel"] = pick.getChannel();
            pickObject["LocationCode"] = pick.getLocationCode();
            pickObject["Time"] = static_cast<int64_t> (pick.getTime().count());
            pickObject["PhaseHint"] = static_cast<int> (pick.getPhaseHint());
            pickObject["StandardError"] = pick.getStandardError();
            pickObject["Identifier"] = pick.getIdentifier();
            jsonPicks.push_back(std::move(pickObject)); 
        }
        obj["UnassociatedPicks"] = jsonPicks;
    }
    else
    {
        obj["UnasociatedPicks"] = nullptr;
    }
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

AssociationResponse
    fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    AssociationResponse result;
    if (obj["MessageType"] != result.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    result.setIdentifier(obj["Identifier"].template get<int64_t> ());
    result.setReturnCode(
        static_cast<AssociationResponse::ReturnCode>
        (obj["ReturnCode"].template get<int> ()));
    if (!obj["Origins"].is_null())
    {
        std::vector<Origin> origins;
        for (const auto &originObject : obj["Origins"])
        {
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
            if (!originObject["Arrivals"].is_null())  
            {
                std::vector<Arrival> arrivals;
                for (const auto &arrivalObject : originObject["Arrivals"])
                {
                    Arrival arrival;
                    arrival.setNetwork(
                        arrivalObject["Network"].template get<std::string> ());
                    arrival.setStation(
                        arrivalObject["Station"].template get<std::string> ());
                    arrival.setChannel(
                        arrivalObject["Channel"].template get<std::string> ());
                    arrival.setLocationCode(
                        arrivalObject["LocationCode"].template
                        get<std::string> ());
                    auto iTime = arrivalObject["Time"].template get<int64_t> ();
                    arrival.setTime(std::chrono::microseconds {iTime});
                    arrival.setPhase(
                        static_cast<Arrival::Phase>
                        (arrivalObject["Phase"].template get<int> ())
                    );
                    arrival.setIdentifier(
                        arrivalObject["Identifier"].template get<int64_t> ());
                    if (!arrivalObject["TravelTime"].is_null())
                    {
                        arrival.setTravelTime(
                           arrivalObject["TravelTime"].template get<double> ());
                    }
                    arrivals.push_back(std::move(arrival));
                }
                origin.setArrivals(arrivals);
            }
            origins.push_back(std::move(origin)); 
        }
        result.setOrigins(origins);
    }
    if (!obj["UnassociatedPicks"].is_null())
    {
        std::vector<Pick> picks;
        for (const auto &pickObject : obj["UnassociatedPicks"])
        {
            Pick pick;
            pick.setNetwork(pickObject["Network"].template get<std::string> ());
            pick.setStation(pickObject["Station"].template get<std::string> ());
            pick.setChannel(pickObject["Channel"].template get<std::string> ());
            pick.setLocationCode(
                pickObject["LocationCode"].template get<std::string> ());
            auto iTime = pickObject["Time"].template get<int64_t> ();
            pick.setTime(std::chrono::microseconds {iTime});
            auto phaseHint
                = static_cast<Pick::PhaseHint>
                  (pickObject["PhaseHint"].template get<int> ());
            pick.setPhaseHint(phaseHint);
            pick.setStandardError(
                pickObject["StandardError"].template get<double> ());
            pick.setIdentifier(
                pickObject["Identifier"].template get<int64_t> ());
            picks.push_back(std::move(pick));
        }
        result.setUnassociatedPicks(picks);
    }
    return result;
}

}

class AssociationResponse::AssociationResponseImpl
{
public:
    std::vector<Origin> mOrigins;
    std::vector<Pick> mPicks;
    int64_t mIdentifier{0};
    ReturnCode mReturnCode{ReturnCode::AlgorithmicFailure};
    bool mHaveReturnCode{false};
};

/// Constructor
AssociationResponse::AssociationResponse() :
    pImpl(std::make_unique<AssociationResponseImpl> ())
{
}

/// Copy constructor
AssociationResponse::AssociationResponse(
    const AssociationResponse &response)
{
    *this = response;
}

/// Move constructor
AssociationResponse::AssociationResponse(
    AssociationResponse &&response) noexcept
{
    *this = std::move(response);
}

/// Copy assignment
AssociationResponse& 
AssociationResponse::operator=(const AssociationResponse &response)
{
    if (&response == this){return *this;}
    pImpl = std::make_unique<AssociationResponseImpl> (*response.pImpl);
    return *this;
}

/// Move assignment
AssociationResponse&
AssociationResponse::operator=(AssociationResponse &&response) noexcept
{
    if (&response == this){return *this;}
    pImpl = std::move(response.pImpl);
    return *this;
}

/// Destructor
AssociationResponse::~AssociationResponse() = default;

/// Set the picks
void AssociationResponse::setUnassociatedPicks(const std::vector<Pick> &picks)
{
    for (const auto &pick : picks)
    {
        if (!pick.haveNetwork())
        {
            throw std::invalid_argument("Network not set");
        }
        if (!pick.haveStation())
        {
            throw std::invalid_argument("Station not set");
        }
        if (!pick.haveChannel())
        {
            throw std::invalid_argument("Channel not set");
        }
        if (!pick.haveLocationCode())
        {
            throw std::invalid_argument("Location code not set");
        }
        if (!pick.haveTime())
        {
            throw std::invalid_argument("Pick time not set");
        }
        if (!pick.havePhaseHint())
        {
            throw std::invalid_argument("Phase hint not set");
        }
    }
    pImpl->mPicks = picks;
}
 
std::vector<Pick> AssociationResponse::getUnassociatedPicks() const noexcept
{
    return pImpl->mPicks;
}

const std::vector<Pick> 
&AssociationResponse::getUnassociatedPicksReference() const noexcept
{
    return *&pImpl->mPicks;
}

/// Origins
void AssociationResponse::setOrigins(const std::vector<Origin> &origins)
{
    for (const auto &origin : origins)
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
    }
    pImpl->mOrigins = origins;
}

std::vector<Origin> AssociationResponse::getOrigins() const noexcept
{
    return pImpl->mOrigins;
}

const std::vector<Origin>
&AssociationResponse::getOriginsReference() const noexcept
{
    return *&pImpl->mOrigins;
}

/// Identifier
void AssociationResponse::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

int64_t AssociationResponse::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Return code
void AssociationResponse::setReturnCode(ReturnCode returnCode) noexcept
{
    pImpl->mReturnCode = returnCode;
}

AssociationResponse::ReturnCode AssociationResponse::getReturnCode() const
{
    if (!haveReturnCode()){throw std::runtime_error("Return code not set");}
    return pImpl->mReturnCode;
}

bool AssociationResponse::haveReturnCode() const noexcept
{
    return pImpl->mHaveReturnCode;
}

/// Message type
std::string AssociationResponse::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string AssociationResponse::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///  Convert message
std::string AssociationResponse::toMessage() const
{
    return ::toCBORObject(*this);
}

void AssociationResponse::fromMessage(
    const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage>
    AssociationResponse::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<AssociationResponse> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    AssociationResponse::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<AssociationResponse> ();
    return result;
}

