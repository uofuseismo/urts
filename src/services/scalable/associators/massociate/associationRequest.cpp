#include <vector>
#include <nlohmann/json.hpp>
#include "urts/services/scalable/associators/massociate/associationRequest.hpp"
#include "urts/services/scalable/associators/massociate/pick.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Associators::MAssociate::AssocationRequest"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Associators::MAssociate;

namespace
{

std::string toCBORObject(const AssociationRequest &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    const auto &picks = message.getPicksReference();
/*
    if (!picks.empty())
    {
        nlohmann::json jsonPicks;
        for (const auto &pick : picks)
        {
            nlohmann::json pickObject;
            propertiesToJSON(pick, pickObject);
            jsonPicks.push_back(std::move(pickObject)); 
        }
        obj["Picks"] = jsonPicks;
    }
    else
    {
        obj["Picks"] = nullptr;
    }
*/
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

AssociationRequest
    fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    AssociationRequest result;
    if (obj["MessageType"] != result.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    result.setIdentifier(obj["Identifier"].template get<int64_t> ());
    if (!obj["Picks"].is_null())
    {
        for (const auto &pickObject : obj["Picks"])
        {
            
        } 
    }
    return result;
}

}

class AssociationRequest::AssociationRequestImpl
{
public:
    std::vector<Pick> mPicks;
    int64_t mIdentifier{0};
};

/// Constructor
AssociationRequest::AssociationRequest() :
    pImpl(std::make_unique<AssociationRequestImpl> ())
{
}

/// Copy constructor
AssociationRequest::AssociationRequest(
    const AssociationRequest &request)
{
    *this = request;
}

/// Move constructor
AssociationRequest::AssociationRequest(AssociationRequest &&request) noexcept
{
    *this = std::move(request);
}

/// Copy assignment
AssociationRequest& 
AssociationRequest::operator=(const AssociationRequest &request)
{
    if (&request == this){return *this;}
    pImpl = std::make_unique<AssociationRequestImpl> (*request.pImpl);
    return *this;
}

/// Move assignment
AssociationRequest&
AssociationRequest::operator=(AssociationRequest &&request) noexcept
{
    if (&request == this){return *this;}
    pImpl = std::move(request.pImpl);
    return *this;
}

/// Destructor
AssociationRequest::~AssociationRequest() = default;

/// Set the picks
void AssociationRequest::setPicks(const std::vector<Pick> &picks)
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
        // Try to figure the phase hint out even though it's optional in the
        // pick message
/*
        auto phaseHint = pick.getPhaseHint();
        bool guessPhaseHint{false};
        if (phaseHint.empty())
        {
            guessPhaseHint = true;
        }
        else
        {
            if (phaseHint != "P" && phaseHint != "S")
            {
                guessPhaseHint = true;
            }
        }
        if (guessPhaseHint)
        {
            auto channel = pick.getChannel();
            if (channel.size() == 3)
            {
                // Vertical channels `should' make P picks...
                if (channel[2] == 'Z' || channel[2] == 'P')
                {
                    phaseHint = "P"; 
                }
                else
                { 
                    phaseHint = "S";
                }
                pick.setPhaseHint(channel);
            }
            else
            {
                throw std::invalid_argument(
                    "Phase hint not set and couldn't guess it");
            }
        }
*/
    }
    pImpl->mPicks = picks;
}
 
std::vector<Pick> AssociationRequest::getPicks() const noexcept
{
    return pImpl->mPicks;
}

const std::vector<Pick> &AssociationRequest::getPicksReference() const noexcept
{
    return *&pImpl->mPicks;
}

/// Identifier
void AssociationRequest::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

int64_t AssociationRequest::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Message type
std::string AssociationRequest::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string AssociationRequest::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///  Convert message
std::string AssociationRequest::toMessage() const
{
    return ::toCBORObject(*this);
}

void AssociationRequest::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());
}

void AssociationRequest::fromMessage(
    const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage>
    AssociationRequest::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<AssociationRequest> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    AssociationRequest::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<AssociationRequest> ();
    return result;
}
