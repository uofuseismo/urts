#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <uussmlmodels/detectors/uNetThreeComponentP/inference.hpp>
#include "urts/services/scalable/detectors/uNetThreeComponentP/preprocessingRequest.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Detectors::UNetThreeComponentP::PreprocessingRequest"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Detectors::UNetThreeComponentP;
namespace MLModels = UUSSMLModels::Detectors::UNetThreeComponentP;

namespace
{

std::string toCBORObject(const PreprocessingRequest &message)
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
    std::vector<double> north = obj["NorthSignal"];
    std::vector<double> east = obj["EastSignal"];
    result.setVerticalNorthEastSignal(std::move(vertical),
                                      std::move(north),
                                      std::move(east));
    return result;
}

}

class PreprocessingRequest::RequestImpl
{
public:
    std::vector<double> mVerticalSignal;
    std::vector<double> mNorthSignal;
    std::vector<double> mEastSignal;
    double mSamplingRate{
        MLModels::Inference::getSamplingRate()
    };
    int64_t mIdentifier{0};
    bool mHaveSignals{false};
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
void PreprocessingRequest::setVerticalNorthEastSignal(
    const std::vector<double> &vertical,
    const std::vector<double> &north,
    const std::vector<double> &east)
{
    if (vertical.size() != north.size() || vertical.size() != east.size())
    {
        throw std::invalid_argument("Signals sizes are inconsistent");
    }
    if (vertical.empty())
    {
        throw std::invalid_argument("Signals are empty");
    }
    pImpl->mVerticalSignal = vertical;
    pImpl->mNorthSignal = north;
    pImpl->mEastSignal = east;
    pImpl->mHaveSignals = true;
}

void PreprocessingRequest::setVerticalNorthEastSignal(
    std::vector<double> &&vertical,
    std::vector<double> &&north,
    std::vector<double> &&east)
{
    if (vertical.size() != north.size() || vertical.size() != east.size())
    {
        throw std::invalid_argument("Signals sizes are inconsistent");
    }
    if (vertical.empty())
    {
        throw std::invalid_argument("Signals are empty");
    }
    pImpl->mVerticalSignal = std::move(vertical);
    pImpl->mNorthSignal = std::move(north);
    pImpl->mEastSignal = std::move(east);
    pImpl->mHaveSignals = true;
}

std::vector<double> PreprocessingRequest::getVerticalSignal() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mVerticalSignal;
}

std::vector<double> PreprocessingRequest::getNorthSignal() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mNorthSignal;
}

std::vector<double> PreprocessingRequest::getEastSignal() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mEastSignal;
}

const std::vector<double>
&PreprocessingRequest::getVerticalSignalReference() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mVerticalSignal;
}

const std::vector<double>
&PreprocessingRequest::getNorthSignalReference() const
{   
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mNorthSignal;
}

const std::vector<double>
&PreprocessingRequest::getEastSignalReference() const
{   
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mEastSignal;
}

bool PreprocessingRequest::haveSignals() const noexcept
{
    return pImpl->mHaveSignals;
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
