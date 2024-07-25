#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "urts/services/scalable/detectors/uNetThreeComponentP/processingRequest.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/inferenceRequest.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Detectors::UNetThreeComponentP::ProcessingRequest"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Detectors::UNetThreeComponentP;

namespace
{

std::string toCBORObject(const ProcessingRequest &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    obj["SamplingRate"] = message.getSamplingRate();
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

ProcessingRequest
    fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    ProcessingRequest result;
    if (obj["MessageType"] != result.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    result.setIdentifier(obj["Identifier"].get<int64_t> ());
    result.setSamplingRate(obj["SamplingRate"].get<double> ());
    auto strategy
        = static_cast<ProcessingRequest::InferenceStrategy>
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

class ProcessingRequest::RequestImpl
{
public:
    std::vector<double> mVerticalSignal;
    std::vector<double> mNorthSignal;
    std::vector<double> mEastSignal;
    int64_t mIdentifier{0};
    double mSamplingRate{InferenceRequest::getSamplingRate()};
    ProcessingRequest::InferenceStrategy mInferenceStrategy{
        ProcessingRequest::InferenceStrategy::SlidingWindow};
    bool mHaveSignals{false};
};

/// Constructor
ProcessingRequest::ProcessingRequest() :
    pImpl(std::make_unique<RequestImpl> ())
{
}

/// Copy constructor
ProcessingRequest::ProcessingRequest(const ProcessingRequest &request)
{
    *this = request;
}

/// Move constructor
ProcessingRequest::ProcessingRequest(ProcessingRequest &&request) noexcept
{
    *this = std::move(request);
}

/// Copy assignment
ProcessingRequest&
ProcessingRequest::operator=(const ProcessingRequest &request)
{
    if (&request == this){return *this;}
    pImpl = std::make_unique<RequestImpl> (*request.pImpl);
    return *this;
}

/// Move assignment
ProcessingRequest&
ProcessingRequest::operator=(ProcessingRequest &&request) noexcept
{
    if (&request == this){return *this;}
    pImpl = std::move(request.pImpl);
    return *this;
}

/// Release memory and reset class
void ProcessingRequest::clear() noexcept
{
    pImpl = std::make_unique<RequestImpl> (); 
}

/// Destructor
ProcessingRequest::~ProcessingRequest() = default;

/// Minimum signal length
int ProcessingRequest::getMinimumSignalLength() noexcept
{
    return InferenceRequest::getMinimumSignalLength();
}

/// Identifier
void ProcessingRequest::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

int64_t ProcessingRequest::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Sampling rate
void ProcessingRequest::setSamplingRate(const double samplingRate)
{
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("Sampling rate must be positive");
    }
    pImpl->mSamplingRate = samplingRate;
}

double ProcessingRequest::getSamplingRate() const noexcept
{
    return pImpl->mSamplingRate;
}

/// Set signals
void ProcessingRequest::setVerticalNorthEastSignal(
    std::vector<double> &&vertical,
    std::vector<double> &&north,
    std::vector<double> &&east,
    const InferenceStrategy strategy)
{
    if (vertical.size() != north.size() || vertical.size() != east.size())
    {
        throw std::invalid_argument("Signal sizes are inconsistent");
    }
    /// Try to head off a problem.  Can still be tricked by setting the signal
    /// then the sampling rate.
    if (std::abs(getSamplingRate() - InferenceRequest::getSamplingRate())
        < 1.e-5)
    {
        if (strategy == ProcessingRequest::InferenceStrategy::SlidingWindow)
        {
            if (static_cast<int> (vertical.size()) < getMinimumSignalLength())
            {
                throw std::invalid_argument(
                    "Signals must have length at least: "
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
    }
    else
    {
        if (vertical.empty())
        {
            throw std::invalid_argument("Signals cannot be empty");
        }
    }
    pImpl->mVerticalSignal = std::move(vertical);
    pImpl->mNorthSignal = std::move(north);
    pImpl->mEastSignal = std::move(east);
    pImpl->mInferenceStrategy = strategy;
    pImpl->mHaveSignals = true;
}

void ProcessingRequest::setVerticalNorthEastSignal(
    const std::vector<double> &vertical,
    const std::vector<double> &north,
    const std::vector<double> &east, 
    const InferenceStrategy strategy)
{
    if (vertical.size() != north.size() || vertical.size() != east.size())
    {
        throw std::invalid_argument("Signal sizes are inconsistent");
    }
    /// Try to head off a problem.  Can still be tricked by setting the signal
    /// then the sampling rate.
    if (std::abs(getSamplingRate() - InferenceRequest::getSamplingRate()) 
        < 1.e-5)
    {
        if (strategy == ProcessingRequest::InferenceStrategy::SlidingWindow)
        {
            if (static_cast<int> (vertical.size()) < getMinimumSignalLength())
            {
                throw std::invalid_argument(
                    "Signals must have length at least: "
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
    }
    else
    {
        if (vertical.empty())
        {
            throw std::invalid_argument("Signals cannot be empty");
        }
    }
    pImpl->mVerticalSignal = vertical;
    pImpl->mNorthSignal = north;
    pImpl->mEastSignal = east;
    pImpl->mInferenceStrategy = strategy;
    pImpl->mHaveSignals = true;
}

std::vector<double> ProcessingRequest::getVerticalSignal() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mVerticalSignal;
}

std::vector<double> ProcessingRequest::getNorthSignal() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mNorthSignal;
}

std::vector<double> ProcessingRequest::getEastSignal() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mEastSignal;
}
 
const std::vector<double>
&ProcessingRequest::getVerticalSignalReference() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mVerticalSignal;
}

const std::vector<double> 
&ProcessingRequest::getNorthSignalReference() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mNorthSignal;
}
 
const std::vector<double>
&ProcessingRequest::getEastSignalReference() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mEastSignal;
}

bool ProcessingRequest::haveSignals() const noexcept
{   
    return pImpl->mHaveSignals;
}

ProcessingRequest::InferenceStrategy
ProcessingRequest::getInferenceStrategy() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mInferenceStrategy;
}

/// Message type
std::string ProcessingRequest::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string
ProcessingRequest::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///  Convert message
std::string ProcessingRequest::toMessage() const
{
    return ::toCBORObject(*this);
}

void ProcessingRequest::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());
}

void ProcessingRequest::fromMessage(
    const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage>
    ProcessingRequest::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<ProcessingRequest> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    ProcessingRequest::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<ProcessingRequest> ();
    return result;
}
