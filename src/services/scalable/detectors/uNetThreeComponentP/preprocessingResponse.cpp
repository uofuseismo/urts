#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <uussmlmodels/detectors/uNetThreeComponentP/inference.hpp>
#include "urts/services/scalable/detectors/uNetThreeComponentP/preprocessingResponse.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Detectors::UNetThreeComponentP::PreprocessingResponse"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Detectors::UNetThreeComponentP;
namespace MLModels = UUSSMLModels::Detectors::UNetThreeComponentP;

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
    if (!message.haveSignals()){throw std::runtime_error("Signals not set");}
    obj["VerticalSignal"] = message.getVerticalSignal();
    obj["NorthSignal"] = message.getNorthSignal();
    obj["EastSignal"] = message.getEastSignal();
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
    std::vector<double> north = obj["NorthSignal"];
    std::vector<double> east = obj["EastSignal"];
    result.setVerticalNorthEastSignal(std::move(vertical),
                                      std::move(north),
                                      std::move(east));
    return result;
}

}

class PreprocessingResponse::ResponseImpl
{
public:
    std::vector<double> mVerticalSignal;
    std::vector<double> mNorthSignal;
    std::vector<double> mEastSignal;
    double mSamplingRate{MLModels::Inference::getSamplingRate()};  
    int64_t mIdentifier{0};
    PreprocessingResponse::ReturnCode mReturnCode;
    bool mHaveSignals{false};
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
void PreprocessingResponse::setVerticalNorthEastSignal(
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

void PreprocessingResponse::setVerticalNorthEastSignal(
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

std::vector<double> PreprocessingResponse::getVerticalSignal() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mVerticalSignal;
}

std::vector<double> PreprocessingResponse::getNorthSignal() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mNorthSignal;
}

std::vector<double> PreprocessingResponse::getEastSignal() const
{
    if (!haveSignals()){throw std::runtime_error("Signals not set");}
    return pImpl->mEastSignal;
}

bool PreprocessingResponse::haveSignals() const noexcept
{
    return pImpl->mHaveSignals;
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
