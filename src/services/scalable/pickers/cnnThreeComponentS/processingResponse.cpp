#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <uussmlmodels/pickers/cnnThreeComponentS/inference.hpp>
#include "urts/services/scalable/pickers/cnnThreeComponentS/processingResponse.hpp"

#define MESSAGE_TYPE "URTS::Services::Scalable::Pickers::CNNThreeComponentS::ProcessingResponse"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Services::Scalable::Pickers::CNNThreeComponentS;
namespace MLModels = UUSSMLModels::Pickers::CNNThreeComponentS;

namespace
{

std::string toCBORObject(const ProcessingResponse &message)
{
    nlohmann::json obj;
    obj["MessageType"] = message.getMessageType();
    obj["MessageVersion"] = message.getMessageVersion();
    obj["Identifier"] = message.getIdentifier();
    obj["ReturnCode"] = static_cast<int> (message.getReturnCode());
    if (!message.haveCorrection())
    {   
        throw std::runtime_error("Pick correction not set");
    }   
    if (message.getReturnCode() == ProcessingResponse::ReturnCode::Success)
    {
        obj["Correction"] = message.getCorrection();
    }
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result;
}

ProcessingResponse
    fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    ProcessingResponse result;
    if (obj["MessageType"] != result.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    result.setIdentifier(obj["Identifier"].get<int64_t> ());
    result.setReturnCode(
       static_cast<ProcessingResponse::ReturnCode> (
         obj["ReturnCode"].get<int> ()
    ));
    if (result.getReturnCode() == ProcessingResponse::ReturnCode::Success)
    {
        result.setCorrection(obj["Correction"].get<double> ());
    }
    return result;
}

}

class ProcessingResponse::ResponseImpl
{
public:
    double mCorrection{0};
    int64_t mIdentifier{0};
    ProcessingResponse::ReturnCode mReturnCode;
    bool mHaveCorrection{false};
    bool mHaveReturnCode{false};
};

/// Constructor
ProcessingResponse::ProcessingResponse() :
    pImpl(std::make_unique<ResponseImpl> ())
{
}

/// Copy constructor
ProcessingResponse::ProcessingResponse(
    const ProcessingResponse &response)
{
    *this = response;
}

/// Move constructor
ProcessingResponse::ProcessingResponse(
    ProcessingResponse &&response) noexcept
{
    *this = std::move(response);
}

/// Copy assignment
ProcessingResponse& ProcessingResponse::operator=(
    const ProcessingResponse &response)
{
    if (&response == this){return *this;}
    pImpl = std::make_unique<ResponseImpl> (*response.pImpl);
    return *this;
}

/// Move assignment
ProcessingResponse& ProcessingResponse::operator=(
    ProcessingResponse &&response) noexcept
{
    if (&response == this){return *this;}
    pImpl = std::move(response.pImpl);
    return *this;
}

/// Release memory and reset class
void ProcessingResponse::clear() noexcept
{
    pImpl = std::make_unique<ResponseImpl> ();
}

/// Destructor
ProcessingResponse::~ProcessingResponse()
    = default;

/// Identifier
void ProcessingResponse::setIdentifier(
    const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
}

int64_t ProcessingResponse::getIdentifier() const noexcept
{
    return pImpl->mIdentifier;
}

/// Return code
void ProcessingResponse::setReturnCode(
    const ProcessingResponse::ReturnCode returnCode) noexcept
{
    pImpl->mReturnCode = returnCode;
    pImpl->mHaveReturnCode = true;
}

ProcessingResponse::ReturnCode ProcessingResponse::getReturnCode() const
{
    if (!haveReturnCode()){throw std::runtime_error("Return code not set");}
    return pImpl->mReturnCode;
}

bool ProcessingResponse::haveReturnCode() const noexcept
{
    return pImpl->mHaveReturnCode;
}

/// Correction
void ProcessingResponse::setCorrection(const double correction) noexcept
{
    pImpl->mCorrection = correction;
    pImpl->mHaveCorrection = true;
}

double ProcessingResponse::getCorrection() const
{
    if (!haveCorrection())
    {
        throw std::runtime_error("Pick correction not set");
    }
    return pImpl->mCorrection;
}

bool ProcessingResponse::haveCorrection() const noexcept
{
    return pImpl->mHaveCorrection;
}

/// Message type
std::string ProcessingResponse::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string
ProcessingResponse::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

///  Convert message
std::string ProcessingResponse::toMessage() const
{
    return ::toCBORObject(*this);
}

void ProcessingResponse::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());
}

void ProcessingResponse::fromMessage(
    const char *messageIn, const size_t length)
{
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage>
    ProcessingResponse::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<ProcessingResponse> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    ProcessingResponse::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<ProcessingResponse> (); 
    return result;
}
