#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "urts/services/scalable/detectors/uNetThreeComponentP/inferenceRequest.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Detectors::UNetThreeComponentP::InferenceRequest"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Detectors::UNetThreeComponentP;

namespace
{

std::string toCBORObject(const InferenceRequest &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    if (!message.haveSignals()){throw std::runtime_error("Signals not set");}
    obj["VerticalSignal"] = message.getVerticalSignal();
    obj["NorthSignal"] = message.getNorthSignal();
    obj["EastSignal"] = message.getEastSignal();
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
    std::vector<double> north = obj["NorthSignal"];
    std::vector<double> east = obj["EastSignal"];
    result.setVerticalNorthEastSignal(std::move(vertical),
                                      std::move(north),
                                      std::move(east),
                                      strategy);
    return result;
}


}

class InferenceRequest::RequestImpl
{
public:
    std::vector<double> mVerticalSignal;
    std::vector<double> mNorthSignal;
    std::vector<double> mEastSignal;
    int64_t mIdentifier{0};
    InferenceRequest::InferenceStrategy mInferenceStrategy{
        InferenceRequest::InferenceStrategy::SlidingWindow};
    bool mHaveSignals{false};
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

/// Valid sampling rate?
bool InferenceRequest::isValidSignalLength(const int nSamples) noexcept
{
    if (nSamples < InferenceRequest::getMinimumSignalLength()){return false;}
    if (nSamples%16 != 0){return false;}
    return true;
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
void InferenceRequest::setVerticalNorthEastSignal(
    std::vector<double> &&vertical,
    std::vector<double> &&north,
    std::vector<double> &&east,
    const InferenceStrategy strategy)
{
    if (vertical.size() != north.size() || vertical.size() != east.size())
    {
        throw std::invalid_argument("Signal sizes are inconsistent");
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
        if (!InferenceRequest::isValidSignalLength(vertical.size()))
        {
            throw std::invalid_argument("Invalid signal length");
        }
    }
    pImpl->mVerticalSignal = std::move(vertical);
    pImpl->mNorthSignal = std::move(north);
    pImpl->mEastSignal = std::move(east);
    pImpl->mInferenceStrategy = strategy;
    pImpl->mHaveSignals = true;
}

void InferenceRequest::setVerticalNorthEastSignal(
    const std::vector<double> &vertical,
    const std::vector<double> &north,
    const std::vector<double> &east, 
    const InferenceStrategy strategy)
{
    if (vertical.size() != north.size() || vertical.size() != east.size())
    {
        throw std::invalid_argument("Signal sizes are inconsistent");
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
        if (!InferenceRequest::isValidSignalLength(vertical.size()))
        {
            throw std::invalid_argument("Invalid signal length");
        }
    }
    pImpl->mVerticalSignal = vertical;
    pImpl->mNorthSignal = north;
    pImpl->mEastSignal = east;
    pImpl->mInferenceStrategy = strategy;
    pImpl->mHaveSignals = true;
}

std::vector<double> InferenceRequest::getVerticalSignal() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mVerticalSignal;
}

std::vector<double> InferenceRequest::getNorthSignal() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mNorthSignal;
}

std::vector<double> InferenceRequest::getEastSignal() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mEastSignal;
}
 
const std::vector<double>
&InferenceRequest::getVerticalSignalReference() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mVerticalSignal;
}

const std::vector<double> 
&InferenceRequest::getNorthSignalReference() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mNorthSignal;
}
 
const std::vector<double>
&InferenceRequest::getEastSignalReference() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mEastSignal;
}

bool InferenceRequest::haveSignals() const noexcept
{   
    return pImpl->mHaveSignals;
}

InferenceRequest::InferenceStrategy
InferenceRequest::getInferenceStrategy() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
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
