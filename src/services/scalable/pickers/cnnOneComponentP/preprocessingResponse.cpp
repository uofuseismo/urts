#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "urts/services/scalable/pickers/cnnOneComponentP/preprocessingResponse.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/inferenceRequest.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Pickers::CNNOneComponentP::PreprocessingResponse"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Pickers::CNNOneComponentP;

namespace
{

std::string toCBORObject(const PreprocessingResponse &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    obj["SamplingRate"] = message.getSamplingRate();
    obj["ReturnCode"] = static_cast<int> (message.getReturnCode());
    if (!message.haveSignal()){throw std::runtime_error("Signal not set");}
    obj["VerticalSignal"] = message.getVerticalSignal();
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

PreprocessingResponse
    fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    PreprocessingResponse result;
    if (obj["MessageType"] != result.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    result.setSamplingRate(obj["SamplingRate"].get<double> ());
    result.setIdentifier(obj["Identifier"].get<int64_t> ());
    result.setReturnCode(
       static_cast<PreprocessingResponse::ReturnCode> (
         obj["ReturnCode"].get<int> ()
    ));
    std::vector<double> vertical = obj["VerticalSignal"];
    result.setVerticalSignal(std::move(vertical));
    return result;
}

}

class PreprocessingResponse::ResponseImpl
{
public:
    std::vector<double> mVerticalSignal;
    double mSamplingRate{InferenceRequest::getSamplingRate()};
    int64_t mIdentifier{0};
    PreprocessingResponse::ReturnCode mReturnCode;
    bool mHaveSignal{false};
    bool mHaveReturnCode{false};
};

/// Constructor
PreprocessingResponse::PreprocessingResponse() :
    pImpl(std::make_unique<ResponseImpl> ())
{
}

/// Copy constructor
PreprocessingResponse::PreprocessingResponse(
    const PreprocessingResponse &response)
{
    *this = response;
}

/// Move constructor
PreprocessingResponse::PreprocessingResponse(
    PreprocessingResponse &&response) noexcept
{
    *this = std::move(response);
}

/// Copy assignment
PreprocessingResponse& PreprocessingResponse::operator=(
    const PreprocessingResponse &response)
{
    if (&response == this){return *this;}
    pImpl = std::make_unique<ResponseImpl> (*response.pImpl);
    return *this;
}

/// Move assignment
PreprocessingResponse& PreprocessingResponse::operator=(
    PreprocessingResponse &&response) noexcept
{
    if (&response == this){return *this;}
    pImpl = std::move(response.pImpl);
    return *this;
}

/// Release memory and reset class
void PreprocessingResponse::clear() noexcept
{
    pImpl = std::make_unique<ResponseImpl> ();
}

/// Destructor
PreprocessingResponse::~PreprocessingResponse()
    = default;

/// Sampling rate
void PreprocessingResponse::setSamplingRate(
    const double samplingRate)
{
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("Sampling rate must be positive");
    }
    pImpl->mSamplingRate = samplingRate;
}

double PreprocessingResponse::getSamplingRate() const noexcept
{
    return pImpl->mSamplingRate;
}

/// Identifier
void PreprocessingResponse::setIdentifier(
    const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

int64_t PreprocessingResponse::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Return code
void PreprocessingResponse::setReturnCode(
    const PreprocessingResponse::ReturnCode returnCode) noexcept
{
    pImpl->mReturnCode = returnCode;
    pImpl->mHaveReturnCode = true;
}

PreprocessingResponse::ReturnCode PreprocessingResponse::getReturnCode() const
{
    if (!haveReturnCode()){throw std::runtime_error("Return code not set");}
    return pImpl->mReturnCode;
}

bool PreprocessingResponse::haveReturnCode() const noexcept
{
    return pImpl->mHaveReturnCode;
}

/// Set signals
void PreprocessingResponse::setVerticalSignal(
    const std::vector<double> &vertical)
{
    if (vertical.empty())
    {
        throw std::invalid_argument("Signal is empty");
    }
    pImpl->mVerticalSignal = vertical;
    pImpl->mHaveSignal = true;
}

void PreprocessingResponse::setVerticalSignal(
    std::vector<double> &&vertical)
{
    if (vertical.empty())
    {
        throw std::invalid_argument("Signal is empty");
    }
    pImpl->mVerticalSignal = std::move(vertical);
    pImpl->mHaveSignal = true;
}

std::vector<double> PreprocessingResponse::getVerticalSignal() const
{
    if (!haveSignal()){throw std::runtime_error("Signal not set");}
    return pImpl->mVerticalSignal;
}

bool PreprocessingResponse::haveSignal() const noexcept
{
    return pImpl->mHaveSignal;
}

/// Message type
std::string PreprocessingResponse::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string
PreprocessingResponse::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///  Convert message
std::string PreprocessingResponse::toMessage() const
{
    return ::toCBORObject(*this);
}

void PreprocessingResponse::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());
}

void PreprocessingResponse::fromMessage(
    const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage>
    PreprocessingResponse::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<PreprocessingResponse> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    PreprocessingResponse::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<PreprocessingResponse> (); 
    return result;
}
