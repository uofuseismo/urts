#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "urts/services/scalable/detectors/uNetOneComponentP/inferenceRequest.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Detectors::UNetOneComponentP::InferenceRequest"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Detectors::UNetOneComponentP;

namespace
{

std::string toCBORObject(const InferenceRequest &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    if (!message.haveSignal()){throw std::runtime_error("Signal not set");}
    obj["VerticalSignal"] = message.getSignal();
    obj["InferenceStrategy"]
        = static_cast<int> (message.getInferenceStrategy());
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

InferenceRequest
    fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    InferenceRequest result;
    if (obj["MessageType"] != result.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    result.setIdentifier(obj["Identifier"].get<int64_t> ());
    auto strategy
        = static_cast<InferenceRequest::InferenceStrategy>
          (obj["InferenceStrategy"].get<int> ());
    std::vector<double> vertical = obj["VerticalSignal"];
    result.setSignal(std::move(vertical), strategy);
    return result;
}


}

class InferenceRequest::RequestImpl
{
public:
    std::vector<double> mVerticalSignal;
    int64_t mIdentifier{0};
    InferenceRequest::InferenceStrategy mInferenceStrategy{
        InferenceRequest::InferenceStrategy::SlidingWindow};
    bool mHaveSignal{false};
};

/// Constructor
InferenceRequest::InferenceRequest() :
    pImpl(std::make_unique<RequestImpl> ())
{
}

/// Copy constructor
InferenceRequest::InferenceRequest(const InferenceRequest &request)
{
    *this = request;
}

/// Move constructor
InferenceRequest::InferenceRequest(InferenceRequest &&request) noexcept
{
    *this = std::move(request);
}

/// Copy assignment
InferenceRequest& InferenceRequest::operator=(const InferenceRequest &request)
{
    if (&request == this){return *this;}
    pImpl = std::make_unique<RequestImpl> (*request.pImpl);
    return *this;
}

/// Move assignment
InferenceRequest&
InferenceRequest::operator=(InferenceRequest &&request) noexcept
{
    if (&request == this){return *this;}
    pImpl = std::move(request.pImpl);
    return *this;
}

/// Release memory and reset class
void InferenceRequest::clear() noexcept
{
    pImpl = std::make_unique<RequestImpl> (); 
}

/// Destructor
InferenceRequest::~InferenceRequest() = default;

/// Minimum signal length
int InferenceRequest::getMinimumSignalLength() noexcept
{
    return 1008;
}

/// Sampling rate
double InferenceRequest::getSamplingRate() noexcept
{
    return 100;
}

/// Identifier
void InferenceRequest::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

int64_t InferenceRequest::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Set signals
void InferenceRequest::setSignal(
    std::vector<double> &&vertical,
    const InferenceStrategy strategy)
{
    if (vertical.empty())
    {
        throw std::invalid_argument("Vertical signal is empty");
    }
    if (strategy == InferenceRequest::InferenceStrategy::SlidingWindow)
    {
        if (static_cast<int> (vertical.size()) < getMinimumSignalLength())
        {
            throw std::invalid_argument("Signals must have length at least: "
                                    + std::to_string(getMinimumSignalLength()));
        }
    }
    else
    {
        if (!isValidSignalLength(vertical.size()))
        {
            throw std::invalid_argument("Invalid signal length");
        }
    }
    pImpl->mVerticalSignal = std::move(vertical);
    pImpl->mInferenceStrategy = strategy;
    pImpl->mHaveSignal = true;
}

/// Valid?
bool InferenceRequest::isValidSignalLength(const int nSamples) noexcept
{
    if (nSamples < InferenceRequest::getMinimumSignalLength()){return false;}
    if (nSamples%16 != 0){return false;}
    return true;
}


void InferenceRequest::setSignal(
    const std::vector<double> &vertical,
    const InferenceStrategy strategy)
{
    if (vertical.empty())
    {
        throw std::invalid_argument("Vertical signal is empty");
    }
    if (strategy == InferenceRequest::InferenceStrategy::SlidingWindow)
    {
        if (static_cast<int> (vertical.size()) < getMinimumSignalLength())
        {
            throw std::invalid_argument("Signals must have length at least: "
                                    + std::to_string(getMinimumSignalLength()));
        }
    }
    else
    {
        if (!isValidSignalLength(vertical.size()))
        {
            throw std::invalid_argument("Invalid signal length");
        }
    }
    pImpl->mVerticalSignal = vertical;
    pImpl->mInferenceStrategy = strategy;
    pImpl->mHaveSignal = true;
}

std::vector<double> InferenceRequest::getSignal() const
{
    if (!haveSignal()){throw std::runtime_error("Signal not set");}
    return pImpl->mVerticalSignal;
}

const std::vector<double>
&InferenceRequest::getSignalReference() const
{
    if (!haveSignal()){throw std::runtime_error("Signal not set");}
    return pImpl->mVerticalSignal;
}

bool InferenceRequest::haveSignal() const noexcept
{   
    return pImpl->mHaveSignal;
}

InferenceRequest::InferenceStrategy
InferenceRequest::getInferenceStrategy() const
{
    if (!haveSignal()){throw std::runtime_error("Signal not set");}
    return pImpl->mInferenceStrategy;
}

/// Message type
std::string InferenceRequest::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string
InferenceRequest::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///  Convert message
std::string InferenceRequest::toMessage() const
{
    return ::toCBORObject(*this);
}

void InferenceRequest::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());
}

void InferenceRequest::fromMessage(
    const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage>
    InferenceRequest::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<InferenceRequest> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    InferenceRequest::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<InferenceRequest> ();
    return result;
}
