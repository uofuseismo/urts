#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/inferenceResponse.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::FirstMotionClassifiers::CNNOneComponentP::InferenceResponse"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::FirstMotionClassifiers::CNNOneComponentP;

namespace
{

std::string toCBORObject(const InferenceResponse &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    obj["ReturnCode"] = static_cast<int> (message.getReturnCode());
    if (!message.haveFirstMotion())
    {
        throw std::runtime_error("First motion not set");
    }
    if (!message.haveProbabilities())
    {
        throw std::runtime_error("Posterior probabilities not set");
    }
    if (message.getReturnCode() == InferenceResponse::ReturnCode::Success)
    {
        obj["FirstMotion"] = static_cast<int> (message.getFirstMotion());
        auto p = message.getProbabilities();
        obj["ProbabilityUp"] = std::get<0> (p);
        obj["ProbabilityDown"] = std::get<1> (p);
        obj["ProbabilityUnknown"] = std::get<2> (p);
    }
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

InferenceResponse
    fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    InferenceResponse result;
    if (obj["MessageType"] != result.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    result.setIdentifier(obj["Identifier"].get<int64_t> ());
    result.setReturnCode(
        static_cast<InferenceResponse::ReturnCode> (
            obj["ReturnCode"].get<int> ()
    ));
    if (result.getReturnCode() == InferenceResponse::ReturnCode::Success)
    {
        auto pUp = obj["ProbabilityUp"].get<double> ();
        auto pDown = obj["ProbabilityDown"].get<double> ();
        auto pUnknown = obj["ProbabilityUnknown"].get<double> ();
        result.setProbabilities(std::tuple {pUp, pDown, pUnknown});
        result.setFirstMotion(
           static_cast<InferenceResponse::FirstMotion>
              (obj["FirstMotion"].get<int> ())
        );
    }
    return result;
}

}

class InferenceResponse::ResponseImpl
{
public:
    std::tuple<double, double, double> mPosteriorProbabilities{0, 0, 1};
    int64_t mIdentifier{0};
    InferenceResponse::FirstMotion
        mFirstMotion{InferenceResponse::FirstMotion::Unknown};
    InferenceResponse::ReturnCode mReturnCode;
    bool mHavePosteriorProbabilities{false};
    bool mHaveFirstMotion{false};
    bool mHaveReturnCode{false};
};

/// Constructor
InferenceResponse::InferenceResponse() :
    pImpl(std::make_unique<ResponseImpl> ())
{
}

/// Copy constructor
InferenceResponse::InferenceResponse(
    const InferenceResponse &response)
{
    *this = response;
}

/// Move constructor
InferenceResponse::InferenceResponse(
    InferenceResponse &&response) noexcept
{
    *this = std::move(response);
}

/// Copy assignment
InferenceResponse& InferenceResponse::operator=(
    const InferenceResponse &response)
{
    if (&response == this){return *this;}
    pImpl = std::make_unique<ResponseImpl> (*response.pImpl);
    return *this;
}

/// Move assignment
InferenceResponse& InferenceResponse::operator=(
    InferenceResponse &&response) noexcept
{
    if (&response == this){return *this;}
    pImpl = std::move(response.pImpl);
    return *this;
}

/// Release memory and reset class
void InferenceResponse::clear() noexcept
{
    pImpl = std::make_unique<ResponseImpl> ();
}

/// Destructor
InferenceResponse::~InferenceResponse()
    = default;

/// Identifier
void InferenceResponse::setIdentifier(
    const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

int64_t InferenceResponse::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Return code
void InferenceResponse::setReturnCode(
    const InferenceResponse::ReturnCode returnCode) noexcept
{
    pImpl->mReturnCode = returnCode;
    pImpl->mHaveReturnCode = true;
}

InferenceResponse::ReturnCode InferenceResponse::getReturnCode() const
{
    if (!haveReturnCode()){throw std::runtime_error("Return code not set");}
    return pImpl->mReturnCode;
}

bool InferenceResponse::haveReturnCode() const noexcept
{
    return pImpl->mHaveReturnCode;
}

/// Posterior probability
void InferenceResponse::setProbabilities(
    const std::tuple<double, double, double> &probability)
{
    auto pUp = std::get<0> (probability);
    auto pDown = std::get<1> (probability);
    auto pUnknown = std::get<2> (probability);
    if (std::abs(pUp + pDown + pUnknown) - 1 > 1.e-5)
    {
        throw std::invalid_argument("Probabilities do not sum to unity: "
                                  + std::to_string(pUp) + "," 
                                  + std::to_string(pDown) + "," 
                                  + std::to_string(pUnknown));
    }
    if (pUp < 0 || pUp > 1)
    {
        throw std::invalid_argument("pUp out of bounds [0,1]");
    }
    if (pDown < 0 || pDown > 1)
    {
        throw std::invalid_argument("pDown out of bounds [0,1]");
    }
    if (pUnknown < 0 || pUnknown > 1)
    {
        throw std::invalid_argument("pUnknown out of bounds [0,1]");
    }
    pImpl->mPosteriorProbabilities = probability;
    pImpl->mHavePosteriorProbabilities = true;
}

std::tuple<double, double, double> InferenceResponse::getProbabilities() const
{
    if (!haveProbabilities())
    {
        throw std::runtime_error("Probabilities not set");
    }
    return pImpl->mPosteriorProbabilities;
}

bool InferenceResponse::haveProbabilities() const noexcept
{
    return pImpl->mHavePosteriorProbabilities;
}

/// First motion
void InferenceResponse::setFirstMotion(
    const InferenceResponse::FirstMotion firstMotion) noexcept
{
    pImpl->mFirstMotion = firstMotion;
    pImpl->mHaveFirstMotion = true;
}

InferenceResponse::FirstMotion InferenceResponse::getFirstMotion() const
{
    if (!haveFirstMotion()){throw std::runtime_error("First motion not set");}
    return pImpl->mFirstMotion;
}

bool InferenceResponse::haveFirstMotion() const noexcept
{
    return pImpl->mHaveFirstMotion;
}

/// Message type
std::string InferenceResponse::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string
InferenceResponse::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///  Convert message
std::string InferenceResponse::toMessage() const
{
    return ::toCBORObject(*this);
}

void InferenceResponse::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());
}

void InferenceResponse::fromMessage(
    const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage>
    InferenceResponse::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<InferenceResponse> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    InferenceResponse::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<InferenceResponse> (); 
    return result;
}
