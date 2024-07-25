#include <string>
#include <chrono>
#include <umps/messaging/xPublisherXSubscriber/publisherOptions.hpp>
#include <umps/authentication/zapOptions.hpp>
#include "urts/broadcasts/internal/probabilityPacket/publisherOptions.hpp"

using namespace URTS::Broadcasts::Internal::ProbabilityPacket;
namespace UAuth = UMPS::Authentication;

class PublisherOptions::PublisherOptionsImpl
{
public:
    PublisherOptionsImpl()
    {
        mOptions.setHighWaterMark(8192); // No point making this infinite
        mOptions.setTimeOut(std::chrono::milliseconds{1000});
    }
    UMPS::Messaging::XPublisherXSubscriber::PublisherOptions mOptions;
};

/// C'tor
PublisherOptions::PublisherOptions() :
    pImpl(std::make_unique<PublisherOptionsImpl> ())
{
}

/// Copy c'tor
PublisherOptions::PublisherOptions(const PublisherOptions &options)
{
    *this = options;
}

/// Move c'tor
PublisherOptions::PublisherOptions(PublisherOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Destructor
PublisherOptions::~PublisherOptions() = default;

/// Reset class
void PublisherOptions::clear() noexcept
{
    pImpl = std::make_unique<PublisherOptionsImpl> ();
}

/// Copy assignment
PublisherOptions&
    PublisherOptions::operator=(const PublisherOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<PublisherOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
PublisherOptions&
    PublisherOptions::operator=(PublisherOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// High water mark
void PublisherOptions::setHighWaterMark(const int hwm)
{
    pImpl->mOptions.setHighWaterMark(hwm);
}

int PublisherOptions::getHighWaterMark() const noexcept
{
    return pImpl->mOptions.getHighWaterMark();
}

/// Address
void PublisherOptions::setAddress(const std::string &address)
{
    pImpl->mOptions.setAddress(address);
}

std::string PublisherOptions::getAddress() const
{
    return pImpl->mOptions.getAddress();
}

bool PublisherOptions::haveAddress() const noexcept
{
    return pImpl->mOptions.haveAddress();
}

/// ZAP options
void PublisherOptions::setZAPOptions(const UAuth::ZAPOptions &options)
{
    pImpl->mOptions.setZAPOptions(options);
} 

UAuth::ZAPOptions PublisherOptions::getZAPOptions() const noexcept
{
    return pImpl->mOptions.getZAPOptions();
}

/// Timeout
void PublisherOptions::setTimeOut(
    const std::chrono::milliseconds &timeOut) noexcept
{
    pImpl->mOptions.setTimeOut(timeOut);
}

std::chrono::milliseconds PublisherOptions::getTimeOut() const noexcept
{
    return pImpl->mOptions.getTimeOut();
}