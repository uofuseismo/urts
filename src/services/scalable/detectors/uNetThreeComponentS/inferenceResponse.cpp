#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "urts/services/scalable/detectors/uNetThreeComponentS/inferenceResponse.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentS/inferenceRequest.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Detectors::UNetThreeComponentS::InferenceResponse"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Detectors::UNetThreeComponentS;

namespace
{

std::string toCBORObject(const InferenceResponse &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    obj["SamplingRate"] = message.getSamplingRate();
    obj["ReturnCode"] = static_cast<int> (message.getReturnCode());
    if (!message.haveProbabilitySignal())
    {
        throw std::runtime_error("Probability signal not set");
    }
    obj["ProbabilitySignal"] = message.getProbabilitySignal();
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
    result.setSamplingRate(obj["SamplingRate"].get<double> ());
    result.setIdentifier(obj["Identifier"].get<int64_t> ());
    result.setReturnCode(
       static_cast<InferenceResponse::ReturnCode> (
         obj["ReturnCode"].get<int> ()
    ));
    std::vector<double> probabilitySignal = obj["ProbabilitySignal"];
    result.setProbabilitySignal(std::move(probabilitySignal));
    return result;
}

}

class InferenceResponse::ResponseImpl
{
public:
    std::vector<double> mProbabilitySignal;
    double mSamplingRate{InferenceRequest::getSamplingRate()};
    int64_t mIdentifier{0};
    InferenceResponse::ReturnCode mReturnCode;
    bool mHaveProbabilitySignal{false};
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

/// Sampling rate
void InferenceResponse::setSamplingRate(
    const double samplingRate)
{
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("Sampling rate must be positive");
    }
    pImpl->mSamplingRate = samplingRate;
}

double InferenceResponse::getSamplingRate() const noexcept
{
    return pImpl->mSamplingRate;
}

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

/// Set signals
void InferenceResponse::setProbabilitySignal(const std::vector<double> &signal)
{
    if (signal.empty())
    {
        throw std::invalid_argument("Probability signal is empty");
    }
    pImpl->mProbabilitySignal = signal;
    pImpl->mHaveProbabilitySignal = true;
}

void InferenceResponse::setProbabilitySignal(std::vector<double> &&signal)
{
    if (signal.empty())
    {
        throw std::invalid_argument("Probability signal is empty");
    }
    pImpl->mProbabilitySignal = signal;
    pImpl->mHaveProbabilitySignal = true;
}

std::vector<double> InferenceResponse::getProbabilitySignal() const
{
    if (!haveProbabilitySignal())
    {
        throw std::runtime_error("Probability signals not set");
    }
    return pImpl->mProbabilitySignal;
}

bool InferenceResponse::haveProbabilitySignal() const noexcept
{
    return pImpl->mHaveProbabilitySignal;
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
