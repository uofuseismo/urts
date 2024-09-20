#include <string>
#include <chrono>
#include <umps/authentication/zapOptions.hpp>
#include "urts/services/scalable/locators/uLocator/requestorOptions.hpp"
#include "private/isEmpty.hpp"

using namespace URTS::Services::Scalable::Locators::ULocator;
namespace UAuth = UMPS::Authentication;

class RequestorOptions::RequestorOptionsImpl
{
public:
    std::string mAddress;
    UAuth::ZAPOptions mZAPOptions;
    int mSendHighWaterMark{2048};
    int mReceiveHighWaterMark{0}; // Infinite
    std::chrono::milliseconds mSendTimeOut{0};
    std::chrono::milliseconds mReceiveTimeOut{60000};
};

/// Constructor
RequestorOptions::RequestorOptions() :
    pImpl(std::make_unique<RequestorOptionsImpl> ())
{
}

/// Copy constructor
RequestorOptions::RequestorOptions(const RequestorOptions &options)
{
    *this = options;
}

/// Move constructor
RequestorOptions::RequestorOptions(RequestorOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment
RequestorOptions& RequestorOptions::operator=(const RequestorOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<RequestorOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
RequestorOptions& 
    RequestorOptions::operator=(RequestorOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Reset class
void RequestorOptions::clear() noexcept
{
    pImpl = std::make_unique<RequestorOptionsImpl> ();
}

/// Destructor
RequestorOptions::~RequestorOptions() = default;

/// End point to bind to
void RequestorOptions::setAddress(const std::string &address)
{
    if (::isEmpty(address)){throw std::invalid_argument("Address is empty");}
    pImpl->mAddress = address;
}

std::string RequestorOptions::getAddress() const
{
    if (!haveAddress()){throw std::runtime_error("Address not set");}
    return pImpl->mAddress;
}

bool RequestorOptions::haveAddress() const noexcept
{
    return !pImpl->mAddress.empty();
}

/// ZAP Options
void RequestorOptions::setZAPOptions(const UAuth::ZAPOptions &options)
{
    pImpl->mZAPOptions = options;
}

UAuth::ZAPOptions RequestorOptions::getZAPOptions() const noexcept
{
    return pImpl->mZAPOptions;
}

/// High water mark
void RequestorOptions::setSendHighWaterMark(const int highWaterMark)
{
    pImpl->mSendHighWaterMark = highWaterMark;
}

int RequestorOptions::getSendHighWaterMark() const noexcept
{
    return pImpl->mSendHighWaterMark;
}

void RequestorOptions::setReceiveHighWaterMark(const int highWaterMark)
{
    pImpl->mReceiveHighWaterMark = highWaterMark;
}

int RequestorOptions::getReceiveHighWaterMark() const noexcept
{
    return pImpl->mReceiveHighWaterMark;
}

/// Time out
void RequestorOptions::setReceiveTimeOut(
    const std::chrono::milliseconds &timeOut) noexcept
{
    constexpr std::chrono::milliseconds zero{0};
    if (timeOut < zero)
    {
        pImpl->mReceiveTimeOut = std::chrono::milliseconds {-1};
    }
    else
    {
        pImpl->mReceiveTimeOut = timeOut;
    }
}

std::chrono::milliseconds RequestorOptions::getReceiveTimeOut() const noexcept
{
    return pImpl->mReceiveTimeOut;
}

void RequestorOptions::setSendTimeOut(
    const std::chrono::milliseconds &timeOut) noexcept
{
    constexpr std::chrono::milliseconds zero{0};
    if (timeOut < zero)
    {
        pImpl->mSendTimeOut = std::chrono::milliseconds {-1};
    }
    else
    {
        pImpl->mSendTimeOut = timeOut;
    }
}

std::chrono::milliseconds RequestorOptions::getSendTimeOut() const noexcept
{
    return pImpl->mSendTimeOut;
}
