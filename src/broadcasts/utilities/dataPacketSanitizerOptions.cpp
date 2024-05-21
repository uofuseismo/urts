#include <string>
#include <chrono>
#include "urts/broadcasts/utilities/dataPacketSanitizerOptions.hpp"

using namespace URTS::Broadcasts::Utilities;

class DataPacketSanitizerOptions::DataPacketSanitizerOptionsImpl
{
public:
    std::chrono::seconds mMaxFutureTime{0};
    std::chrono::seconds mMaxLatency{500};
    std::chrono::seconds mLogBadDataInterval{3600};
    bool mLogBadData{true};
};

/// Constructor
DataPacketSanitizerOptions::DataPacketSanitizerOptions() :
    pImpl(std::make_unique<DataPacketSanitizerOptionsImpl> ())
{
}

/// Copy constructor
DataPacketSanitizerOptions::DataPacketSanitizerOptions(
    const DataPacketSanitizerOptions &options)
{
    *this = options;
}

/// Move constructor
DataPacketSanitizerOptions::DataPacketSanitizerOptions(
    DataPacketSanitizerOptions &&options) noexcept
{
    *this = std::move(options);
}

/// The max future time
void DataPacketSanitizerOptions::setMaximumFutureTime(
    const std::chrono::seconds &maxFutureTime)
{
    if (maxFutureTime.count() < 0)
    {
        throw std::invalid_argument("Maximum future time must be positive");
    }
    pImpl->mMaxFutureTime = maxFutureTime; 
}

std::chrono::seconds 
DataPacketSanitizerOptions::getMaximumFutureTime() const noexcept
{
    return pImpl->mMaxFutureTime;
}

/// The max past time
void DataPacketSanitizerOptions::setMaximumLatency(
    const std::chrono::seconds &maxLatency)
{
    if (maxLatency.count() < 0)
    {
        throw std::invalid_argument("Maximum latency time must be positive");
    }
    pImpl->mMaxLatency = maxLatency;
}

std::chrono::seconds 
DataPacketSanitizerOptions::getMaximumLatency() const noexcept
{
    return pImpl->mMaxLatency;
}


/// Logging interval
void DataPacketSanitizerOptions::setBadDataLoggingInterval(
    const std::chrono::seconds &interval) noexcept
{
    pImpl->mLogBadDataInterval = interval;
}

std::chrono::seconds 
DataPacketSanitizerOptions::getBadDataLoggingInterval() const noexcept
{
    return pImpl->mLogBadDataInterval;
}

bool DataPacketSanitizerOptions::logBadData() const noexcept
{
    return (pImpl->mLogBadDataInterval.count() > 0);
}

/// Copy assignment
DataPacketSanitizerOptions& 
DataPacketSanitizerOptions::operator=(
    const DataPacketSanitizerOptions &options)
{ 
    if (&options == this){return *this;}
    pImpl = std::make_unique<DataPacketSanitizerOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
DataPacketSanitizerOptions& 
DataPacketSanitizerOptions::operator=(
    DataPacketSanitizerOptions &&options) noexcept
{ 
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Reset class
void DataPacketSanitizerOptions::clear() noexcept
{
    pImpl = std::make_unique<DataPacketSanitizerOptionsImpl> ();
}

/// Destructor
DataPacketSanitizerOptions::~DataPacketSanitizerOptions() = default;
