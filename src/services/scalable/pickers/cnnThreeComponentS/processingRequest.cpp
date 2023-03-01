#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <uussmlmodels/pickers/cnnThreeComponentS/inference.hpp>
#include "urts/services/scalable/pickers/cnnThreeComponentS/processingRequest.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Pickers::CNNThreeComponentS::ProcessingRequest"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Pickers::CNNThreeComponentS;
namespace MLModels = UUSSMLModels::Pickers::CNNThreeComponentS;

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
    std::vector<double> vertical = obj["VerticalSignal"];
    std::vector<double> north = obj["NorthSignal"];
    std::vector<double> east = obj["EastSignal"];
    result.setVerticalNorthEastSignal(std::move(vertical),
                                      std::move(north),
                                      std::move(east));
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
    double mSamplingRate{MLModels::Inference::getSamplingRate()};
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
void ProcessingRequest::setVerticalNorthEastSignal(
    std::vector<double> &&vertical,
    std::vector<double> &&north,
    std::vector<double> &&east)
{
    if (vertical.size() != north.size() || vertical.size() != east.size())
    {   
        throw std::invalid_argument("Inconsistent signal sizes");
    }
    /// Try to head off a problem.  Can still be tricked by setting the signal
    /// then the sampling rate.
    if (vertical.empty())
    {
        throw std::invalid_argument("Signals cannot be empty");
    }
    auto expectedSamplingPeriod = 1.0/MLModels::Inference::getSamplingRate();
    double expectedDuration
        = (MLModels::Inference::getExpectedSignalLength() - 1)
         *expectedSamplingPeriod;
    double signalDuration = (vertical.size() - 1)/getSamplingRate();
    if (signalDuration < expectedDuration - expectedSamplingPeriod/2)
    {
        throw std::invalid_argument("Signal time is too short");
    }
    pImpl->mVerticalSignal = std::move(vertical);
    pImpl->mNorthSignal = std::move(north);
    pImpl->mEastSignal = std::move(east);
    pImpl->mHaveSignals = true;
}

void ProcessingRequest::setVerticalNorthEastSignal(
    const std::vector<double> &vertical,
    const std::vector<double> &north,
    const std::vector<double> &east)
{
    if (vertical.size() != north.size() || vertical.size() != east.size())
    {
        throw std::invalid_argument("Inconsistent signal sizes");
    }
    /// Try to head off a problem.  Can still be tricked by setting the signal
    /// then the sampling rate.
    if (vertical.empty())
    {
        throw std::invalid_argument("Signals cannot be empty");
    }
    auto expectedSamplingPeriod = 1.0/MLModels::Inference::getSamplingRate();
    double expectedDuration
        = (MLModels::Inference::getExpectedSignalLength() - 1)
         *expectedSamplingPeriod;
    double signalDuration = (vertical.size() - 1)/getSamplingRate();
    if (signalDuration < expectedDuration - expectedSamplingPeriod/2)
    {
        throw std::invalid_argument("Signal time is too short");
    }
    pImpl->mVerticalSignal = vertical;
    pImpl->mNorthSignal = north;
    pImpl->mEastSignal = east;
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
