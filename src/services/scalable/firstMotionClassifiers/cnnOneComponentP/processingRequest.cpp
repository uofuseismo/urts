#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <uussmlmodels/firstMotionClassifiers/cnnOneComponentP/inference.hpp>
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/processingRequest.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::FirstMotionClassifiers::CNNOneComponentP::ProcessingRequest"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::FirstMotionClassifiers::CNNOneComponentP;
namespace MLModels = UUSSMLModels::FirstMotionClassifiers::CNNOneComponentP;

namespace
{

std::string toCBORObject(const ProcessingRequest &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    obj["SamplingRate"] = message.getSamplingRate();
    obj["Threshold"] = message.getThreshold();
    if (!message.haveSignal()){throw std::runtime_error("Signal not set");}
    obj["VerticalSignal"] = message.getVerticalSignal();
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
    result.setThreshold(obj["Threshold"].get<double> ());
    std::vector<double> vertical = obj["VerticalSignal"];
    result.setVerticalSignal(std::move(vertical));
    return result;
}


}

class ProcessingRequest::RequestImpl
{
public:
    std::vector<double> mVerticalSignal;
    int64_t mIdentifier{0};
    double mThreshold{1.0/3.0};
    double mSamplingRate{MLModels::Inference::getSamplingRate()};
    bool mHaveSignal{false};
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

/// Expected signal length
int ProcessingRequest::getExpectedSignalLength() noexcept
{
    return MLModels::Inference::getExpectedSignalLength();
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
void ProcessingRequest::setVerticalSignal(std::vector<double> &&vertical)
{
    /// Try to head off a problem.  Can still be tricked by setting the signal
    /// then the sampling rate.
    if (std::abs(getSamplingRate() - MLModels::Inference::getSamplingRate()) 
        < 1.e-5)
    {
        if (static_cast<int> (vertical.size()) !=
            ProcessingRequest::getExpectedSignalLength())
        {
            throw std::invalid_argument("Signal length should be: "
                + std::to_string(ProcessingRequest::getExpectedSignalLength()));
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
    pImpl->mHaveSignal = true;
}

void ProcessingRequest::setVerticalSignal(const std::vector<double> &vertical)
{
    /// Try to head off a problem.  Can still be tricked by setting the signal
    /// then the sampling rate.
    if (std::abs(getSamplingRate() - MLModels::Inference::getSamplingRate()) 
        < 1.e-5)
    {
        if (static_cast<int> (vertical.size()) !=
            ProcessingRequest::getExpectedSignalLength())
        {
            throw std::invalid_argument("Signal length should be: "
                + std::to_string(ProcessingRequest::getExpectedSignalLength()));
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
    pImpl->mHaveSignal = true;
}

std::vector<double> ProcessingRequest::getVerticalSignal() const
{
    if (!haveSignal()){throw std::runtime_error("Signal not set");}
    return pImpl->mVerticalSignal;
}

const std::vector<double>
&ProcessingRequest::getVerticalSignalReference() const
{
    if (!haveSignal()){throw std::runtime_error("Signal not set");}
    return pImpl->mVerticalSignal;
}

bool ProcessingRequest::haveSignal() const noexcept
{   
    return pImpl->mHaveSignal;
}

/// Probability threshold
void ProcessingRequest::setThreshold(const double threshold)
{
    if (threshold < 0 || threshold > 1)
    {
        throw std::invalid_argument("Threshold must be in range [0,1]");
    }
    pImpl->mThreshold = threshold;
}

double ProcessingRequest::getThreshold() const noexcept
{
    return pImpl->mThreshold;
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
