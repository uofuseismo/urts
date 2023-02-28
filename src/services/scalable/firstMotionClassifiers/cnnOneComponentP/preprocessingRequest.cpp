#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <uussmlmodels/firstMotionClassifiers/cnnOneComponentP/inference.hpp>
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/preprocessingRequest.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::FirstMotionClassifiers::CNNOneComponentP::PreprocessingRequest"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::FirstMotionClassifiers::CNNOneComponentP;
namespace MLModels = UUSSMLModels::FirstMotionClassifiers::CNNOneComponentP;

namespace
{

std::string toCBORObject(const PreprocessingRequest &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    obj["SamplingRate"] = message.getSamplingRate();
    if (!message.haveSignal()){throw std::runtime_error("Signal not set");}
    obj["VerticalSignal"] = message.getVerticalSignal();
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

PreprocessingRequest
    fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    PreprocessingRequest result;
    if (obj["MessageType"] != result.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    result.setSamplingRate(obj["SamplingRate"].get<double> ());
    result.setIdentifier(obj["Identifier"].get<int64_t> ());
    std::vector<double> vertical = obj["VerticalSignal"];
    result.setVerticalSignal(std::move(vertical));
    return result;
}

}

class PreprocessingRequest::RequestImpl
{
public:
    std::vector<double> mVerticalSignal;
    double mSamplingRate{MLModels::Inference::getSamplingRate()};
    int64_t mIdentifier{0};
    bool mHaveSignal{false};
};

/// Constructor
PreprocessingRequest::PreprocessingRequest() :
    pImpl(std::make_unique<RequestImpl> ())
{
}

/// Copy constructor
PreprocessingRequest::PreprocessingRequest(const PreprocessingRequest &request)
{
    *this = request;
}

/// Move constructor
PreprocessingRequest::PreprocessingRequest(
    PreprocessingRequest &&request) noexcept
{
    *this = std::move(request);
}

/// Copy assignment
PreprocessingRequest& PreprocessingRequest::operator=(
    const PreprocessingRequest &request)
{
    if (&request == this){return *this;}
    pImpl = std::make_unique<RequestImpl> (*request.pImpl);
    return *this;
}

/// Move assignment
PreprocessingRequest& PreprocessingRequest::operator=(
    PreprocessingRequest &&request) noexcept
{
    if (&request == this){return *this;}
    pImpl = std::move(request.pImpl);
    return *this;
}

/// Release memory and reset class
void PreprocessingRequest::clear() noexcept
{
    pImpl = std::make_unique<RequestImpl> ();
}

/// Destructor
PreprocessingRequest::~PreprocessingRequest()
    = default;

/// Sampling rate
void PreprocessingRequest::setSamplingRate(
    const double samplingRate)
{
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("Sampling rate must be positive");
    }
    pImpl->mSamplingRate = samplingRate;
}

double PreprocessingRequest::getSamplingRate() const noexcept
{
    return pImpl->mSamplingRate;
}

/// Identifier
void PreprocessingRequest::setIdentifier(
    const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

int64_t PreprocessingRequest::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Set signals
void PreprocessingRequest::setVerticalSignal(
    const std::vector<double> &vertical)
{
    if (vertical.empty())
    {
        throw std::invalid_argument("Signal is empty");
    }
    pImpl->mVerticalSignal = vertical;
    pImpl->mHaveSignal = true;
}

void PreprocessingRequest::setVerticalSignal(
    std::vector<double> &&vertical)
{
    if (vertical.empty())
    {
        throw std::invalid_argument("Signal is empty");
    }
    pImpl->mVerticalSignal = std::move(vertical);
    pImpl->mHaveSignal = true;
}

std::vector<double> PreprocessingRequest::getVerticalSignal() const
{
    if (!haveSignal()){throw std::runtime_error("Signals not set");}
    return pImpl->mVerticalSignal;
}

const std::vector<double>
&PreprocessingRequest::getVerticalSignalReference() const
{
    if (!haveSignal()){throw std::runtime_error("Signals not set");}
    return pImpl->mVerticalSignal;
}

bool PreprocessingRequest::haveSignal() const noexcept
{
    return pImpl->mHaveSignal;
}

/// Message type
std::string PreprocessingRequest::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string
PreprocessingRequest::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///  Convert message
std::string PreprocessingRequest::toMessage() const
{
    return ::toCBORObject(*this);
}

void PreprocessingRequest::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());
}

void PreprocessingRequest::fromMessage(
    const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage>
    PreprocessingRequest::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<PreprocessingRequest> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    PreprocessingRequest::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<PreprocessingRequest> (); 
    return result;
}
