#include <chrono>
#include <string>
#include <filesystem>
#include <umps/authentication/zapOptions.hpp>
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/serviceOptions.hpp"
#include "private/isEmpty.hpp"

using namespace URTS::Services::Scalable::FirstMotionClassifiers::CNNOneComponentP;
namespace UAuth = UMPS::Authentication;

class ServiceOptions::ServiceOptionsImpl
{
public:
    UAuth::ZAPOptions mZAPOptions;
    std::string mModelWeightsFile;
    std::string mAddress;
    std::chrono::milliseconds mPollingTimeOut{10};
    Device mDevice{Device::CPU};
    int mSendHighWaterMark{8192};
    int mReceiveHighWaterMark{4096};
};

/// Constructor
ServiceOptions::ServiceOptions() :
    pImpl(std::make_unique<ServiceOptionsImpl> ())
{
}

/// Copy constructor
ServiceOptions::ServiceOptions(const ServiceOptions &options)
{
    *this = options;
}

/// Move constructor
ServiceOptions::ServiceOptions(ServiceOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment
ServiceOptions& ServiceOptions::operator=(const ServiceOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<ServiceOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
ServiceOptions& ServiceOptions::operator=(ServiceOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Reset class
void ServiceOptions::clear() noexcept
{
    pImpl = std::make_unique<ServiceOptionsImpl> ();
}

/// Destructor
ServiceOptions::~ServiceOptions() = default;

/// Model weight file
void ServiceOptions::setModelWeightsFile(const std::string &weightFile)
{
    if (!std::filesystem::exists(weightFile))
    {
        throw std::invalid_argument("Weight file does not exist");
    }
    pImpl->mModelWeightsFile = weightFile;
}

std::string ServiceOptions::getModelWeightsFile() const
{
    if (!haveModelWeightsFile())
    {
        throw std::runtime_error("Weight file not set");
    }
    return pImpl->mModelWeightsFile;
}

bool ServiceOptions::haveModelWeightsFile() const noexcept
{
    return !pImpl->mModelWeightsFile.empty();
}

/// Sets the backend address
void ServiceOptions::setAddress(const std::string &address)
{
    if (::isEmpty(address)){throw std::invalid_argument("Address is empty");}
    pImpl->mAddress = address;
}

std::string ServiceOptions::getAddress() const
{
    if (!haveAddress())
    {
        throw std::runtime_error("Replier address not set");
    }
    return pImpl->mAddress; 
}

bool ServiceOptions::haveAddress() const noexcept
{
    return !pImpl->mAddress.empty();
}

/// Device for inference
void ServiceOptions::setDevice(const Device device) noexcept
{
    pImpl->mDevice = device;
}

ServiceOptions::Device ServiceOptions::getDevice() const noexcept
{
    return pImpl->mDevice;
}

/// ZAP Options
void ServiceOptions::setZAPOptions(
    const UAuth::ZAPOptions &zapOptions) noexcept
{
    pImpl->mZAPOptions = zapOptions;
}

UAuth::ZAPOptions ServiceOptions::getZAPOptions() const noexcept
{
    return pImpl->mZAPOptions;
}

/// Time out
void ServiceOptions::setPollingTimeOut(
    const std::chrono::milliseconds &timeOut)
{
    if (timeOut.count() < 0)
    {
        throw std::invalid_argument("Time out cannot be negative");
    }
    pImpl->mPollingTimeOut = timeOut;
}

std::chrono::milliseconds 
ServiceOptions::getPollingTimeOut() const noexcept
{
    return pImpl->mPollingTimeOut;
}

/// High water mark
void ServiceOptions::setSendHighWaterMark(const int highWaterMark)
{
    pImpl->mSendHighWaterMark = highWaterMark;
}

int ServiceOptions::getSendHighWaterMark() const noexcept
{
    return pImpl->mSendHighWaterMark;
}

void ServiceOptions::setReceiveHighWaterMark(const int highWaterMark)
{
    pImpl->mReceiveHighWaterMark = highWaterMark;
}

int ServiceOptions::getReceiveHighWaterMark() const noexcept
{
    return pImpl->mReceiveHighWaterMark;
}
