#include <cmath>
#include <algorithm>
#include <chrono>
#include <map>
#include <set>
#include <string>
#include <boost/circular_buffer.hpp>
#include <umps/logging/standardOut.hpp>
#include "urts/broadcasts/utilities/dataPacketSanitizer.hpp"
#include "urts/broadcasts/utilities/dataPacketSanitizerOptions.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#ifdef WITH_EARTHWORM
#include "urts/broadcasts/external/earthworm/traceBuf2.hpp"
#endif

using namespace URTS::Broadcasts::Utilities;

namespace
{

struct DataPacketHeader
{
public:
    DataPacketHeader() = default;
    explicit DataPacketHeader(const URTS::Broadcasts::Internal::DataPacket::DataPacket &packet)
    {
        auto network = packet.getNetwork();
        auto station = packet.getStation(); 
        auto channel = packet.getChannel();
        std::string locationCode;
        try
        {
            locationCode = packet.getLocationCode();
        }
        catch (...)
        {
        }
        // Trace name
        name = network + "."  + station + "." + channel;
        if (!locationCode.empty())
        {
            name = name + "." + locationCode;
        }
        // Start and end time
        startTime = packet.getStartTime();
        endTime = packet.getEndTime();
        // Sampling rate (approximate)
        samplingRate
            = static_cast<int> (std::round(packet.getSamplingRate()));
        // Number of samples
        nSamples = packet.getNumberOfSamples();
    } 
#ifdef WITH_EARTHWORM

#endif
    bool operator<(const ::DataPacketHeader &rhs) const
    {
        return startTime < rhs.startTime;
    } 
    bool operator>(const ::DataPacketHeader &rhs) const
    {
        return startTime > rhs.startTime;
    }
    bool operator==(const ::DataPacketHeader &rhs) const
    {
        if (rhs.name != name){return false;}
        if (rhs.samplingRate - samplingRate != 0)
        {
            throw std::runtime_error("Inconsistent sampling rates for: " + name);
            return false;
        }
        auto dStartTime = (rhs.startTime.count() - startTime.count());
        if (samplingRate < 105)
        {
            return (dStartTime < std::chrono::microseconds {15000}.count());
        }
        else if (samplingRate < 255)
        {
            return (dStartTime < std::chrono::microseconds {4500}.count());
        }
        else if (samplingRate < 505)
        {
            return (dStartTime < std::chrono::microseconds {2500}.count());
        }
        else if (samplingRate < 1005)
        {
            return (dStartTime < std::chrono::microseconds {1500}.count());
        }
        throw std::runtime_error(
            "Could not classify sampling rate: " + std::to_string(samplingRate)
          + " for " + name);
        return false;
    } 

    std::string name; // Packet name NETWORK.STATION.CHANNEL.LOCATION
    std::chrono::microseconds startTime{0}; // UTC time of first sample
    std::chrono::microseconds endTime{0}; // UTC time of last sample
    // Typically `observed' sampling rates wobble around a nominal sampling rate
    int samplingRate{100};
    int nSamples{0}; // Number of samples in packet
};

[[nodiscard]] int estimateCapacity(const ::DataPacketHeader &header,
                                   const std::chrono::seconds &memory)
{
    auto duration
        = std::max(0.0,
                   std::round( (header.nSamples - 1.)
                               /std::max(1, header.samplingRate)));
    std::chrono::seconds packetDuration{static_cast<int> (duration)};
    return std::max(1000, static_cast<int> (memory.count()/duration)) + 1;
}

}

class DataPacketSanitizer::DataPacketSanitizerImpl
{
public:
    DataPacketSanitizerImpl(std::shared_ptr<UMPS::Logging::ILog> logger = nullptr) :
        mLogger(logger)
    {
        if (logger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> ();
        }         
    }
    void logBadData(const std::chrono::microseconds &nowMuSec)
    {
        if (!mLogBadData){return;}
        auto nowSeconds
            = std::chrono::duration_cast<std::chrono::seconds> (nowMuSec);
        if (nowSeconds > mLastLogTime + mLogBadDataInterval)
        {
            if (!mFutureChannels.empty())
            {
                std::string message{"Future data detected for:"};
                for (const auto &channel : mFutureChannels)
                {
                    message = message + " " + channel;
                }
                mLogger->info(message);
                mFutureChannels.clear();
            }
            if (!mDuplicateChannels.empty())
            {
                std::string message{"Duplicate data detected for:"};
                for (const auto &channel : mDuplicateChannels)
                {
                    message = message + " " + channel;
                }
                mLogger->info(message);
                mDuplicateChannels.clear();
            }
            if (!mBadTimingChannels.empty())
            {
                std::string message{"Bad timing data detected for:"};
                for (const auto &channel : mBadTimingChannels)
                {
                    message = message + " " + channel;
                }
                mLogger->info(message);
                mBadTimingChannels.clear();
            }
            if (!mExpiredChannels.empty())
            {
                std::string message{"Expired data detected for:"};
                for (const auto &channel : mExpiredChannels)
                {
                    message = message + " " + channel;
                }
                mLogger->info(message);
                mExpiredChannels.clear();
            }
            if (!mEmptyChannels.empty())
            {
                std::string message{"Empty packets detected for:"};
                for (const auto &channel : mEmptyChannels)
                {
                    message = message + " " + channel;
                }
                mLogger->info(message);
                mEmptyChannels.clear();
            }
            mLastLogTime = nowSeconds;
        }
    }
    [[nodiscard]] bool allow(const ::DataPacketHeader &header)
    {
        if (header.nSamples <= 0)
        {
            if (mLogBadData)
            {
                mLogger->debug("Empty packet detected");
                mEmptyChannels.insert(header.name);
            }
            return false;
        }
        // Computing the current time after the scraping the ring is
        // conservative.  Basically, this allows for a zero-latency,
        // 1 sample packet, to be successfully passed through.
        auto now = std::chrono::high_resolution_clock::now();
        auto nowMuSeconds
            = std::chrono::time_point_cast<std::chrono::microseconds>
              (now).time_since_epoch();
        logBadData(nowMuSeconds);
        auto earliestTime = nowMuSeconds - mMaxLatency;
        // Too old?
        if (header.endTime < earliestTime)
        {
            if (mLogBadData)
            {
                mLogger->debug(header.name
                           + "'s data has expired; skipping...");
                if (!mExpiredChannels.contains(header.name))
                {
                    mExpiredChannels.insert(header.name);
                }
            }
            return false;
        }
        // From the future?
        auto latestTime  = nowMuSeconds + mMaxFutureTime;
        if (header.endTime > latestTime)
        {
            if (mLogBadData)
            {
                mLogger->debug(header.name
                           + "'s data is in future data; skipping...");
                if (!mFutureChannels.contains(header.name))
                {
                    mFutureChannels.insert(header.name);
                }
            }
            return false;
        }
        // Does this channel exist?
        auto circularBufferIndex = mCircularBuffers.find(header.name);
        bool firstExample{false};
        if (circularBufferIndex == mCircularBuffers.end())
        {
             auto capacity
                 = ::estimateCapacity(header,
                                      mCircularBufferDuration);
             mLogger->info("Creating new circular buffer for: "
                         + header.name + " with capacity: "
                         + std::to_string(capacity));
             boost::circular_buffer<::DataPacketHeader>
                 newCircularBuffer(capacity);
             newCircularBuffer.push_back(header);
             mCircularBuffers.insert(std::pair{header.name,
                                               std::move(newCircularBuffer)});
             firstExample = true;
        }
        // Now we should definitely be able to find the appropriate circular
        // buffer for this stream 
        circularBufferIndex = mCircularBuffers.find(header.name);
        if (circularBufferIndex == mCircularBuffers.end())
        {
            mLogger->error(
                "Algorithm error - circular buffer doesn't exist for: "
               + header.name);
            return false;
        }
        // See if this header exists (exactly)
        auto headerIndex
            = std::find(circularBufferIndex->second.begin(),
                        circularBufferIndex->second.end(),
                        header);
        if (headerIndex != circularBufferIndex->second.end())
        {
            if (!firstExample)
            {
                if (mLogBadData)
                {
                    mLogger->debug("Detected duplicate for: "
                                 + header.name);
                    if (!mDuplicateChannels.contains(header.name))
                    {
                        mDuplicateChannels.insert(header.name);
                    }
                }
                return false;
            }
            else
            {
                mLogger->debug("Initial duplicate found for: "
                             + header.name + "; everything is fine!");
            }
        }
        // Insert it (typically new stuff shows up)
        if (header > circularBufferIndex->second.back())
        {
            mLogger->debug("Inserting " + header.name
                         + " at end of circular buffer");
            circularBufferIndex->second.push_back(header);
            return true;
        }
        // The packet is old.  We have to check for a GPS slip.
        for (const auto &streamHeader : circularBufferIndex->second)
        {
            if ((header.startTime >= streamHeader.startTime &&
                 header.startTime <= streamHeader.endTime) ||
                (header.endTime >= streamHeader.startTime &&
                 header.endTime <= streamHeader.endTime))
            {
                if (mLogBadData)
                {
                    mLogger->debug("Detected possible timing slip for: "
                                 + header.name);
                    if (!mBadTimingChannels.contains(header.name))
                    {   
                        mBadTimingChannels.insert(header.name);
                    }
                }
                return false;
            }
        }
        // This appears to be a valid (out-of-order) back-fill
        mLogger->debug("Inserting " + header.name
                     + " in circular buffer then sorting...");
        circularBufferIndex->second.push_back(header);
        std::sort(circularBufferIndex->second.begin(),
                  circularBufferIndex->second.end(),
                  [](const ::DataPacketHeader &lhs, const ::DataPacketHeader &rhs)
                  {
                      return lhs.startTime < rhs.startTime;
                  });
        return true;
    }
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    mutable std::map<std::string, boost::circular_buffer<::DataPacketHeader>>
        mCircularBuffers;
    DataPacketSanitizerOptions mOptions;
    std::set<std::string> mFutureChannels;
    std::set<std::string> mDuplicateChannels;
    std::set<std::string> mBadTimingChannels;
    std::set<std::string> mExpiredChannels;
    std::set<std::string> mEmptyChannels;
    std::chrono::microseconds mMaxFutureTime{0};
    std::chrono::microseconds mMaxLatency{500000000};
    std::chrono::seconds mLogBadDataInterval{3600};
    std::chrono::seconds mCircularBufferDuration{1800};
    std::chrono::seconds mLastLogTime{0};
    bool mLogBadData{true};
};

/// Constructor
DataPacketSanitizer::DataPacketSanitizer() :
    pImpl(std::make_unique<DataPacketSanitizerImpl> (nullptr))
{
}

DataPacketSanitizer::DataPacketSanitizer(
    std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<DataPacketSanitizerImpl> (logger))
{
}

/// Copy constructor
DataPacketSanitizer::DataPacketSanitizer(
    const DataPacketSanitizer &sanitizer)
{
    *this = sanitizer;
}

/// Move constructor
DataPacketSanitizer::DataPacketSanitizer(
    DataPacketSanitizer &&sanitizer) noexcept
{
    *this = std::move(sanitizer);
}

/// Copy assignment
DataPacketSanitizer&
DataPacketSanitizer::operator=(const DataPacketSanitizer &sanitizer)
{
    if (&sanitizer == this){return *this;} 
    pImpl = std::make_unique<DataPacketSanitizerImpl> (*sanitizer.pImpl);
    return *this;
}

/// Move assignment
DataPacketSanitizer&
DataPacketSanitizer::operator=(DataPacketSanitizer &&sanitizer) noexcept
{
    if (&sanitizer == this){return *this;}
    pImpl = std::move(sanitizer.pImpl);
    return *this;
}

/// Initialize
void DataPacketSanitizer::initialize(const DataPacketSanitizerOptions &options)
{
    clear();
    pImpl->mMaxFutureTime = options.getMaximumFutureTime();
    pImpl->mMaxLatency = options.getMaximumLatency();
    pImpl->mLogBadData = options.logBadData();
pImpl->mLogger->info("Max latency: " + std::to_string(pImpl->mMaxLatency.count()));
    if (options.logBadData())
    {
         pImpl->mLogBadDataInterval = options.getBadDataLoggingInterval();
    }
    pImpl->mCircularBufferDuration = std::chrono::duration_cast<std::chrono::seconds> (3*pImpl->mMaxLatency);
    pImpl->mOptions = options;
}

/// Reset the class
void DataPacketSanitizer::clear() noexcept
{
    pImpl = std::make_unique<DataPacketSanitizerImpl> ();
}

/// Destructor
DataPacketSanitizer::~DataPacketSanitizer() = default;

/// Allow this packet?
bool DataPacketSanitizer::allow(
    const URTS::Broadcasts::Internal::DataPacket::DataPacket &packet)
{
    // Construct the trace header for the circular buffer
    ::DataPacketHeader header;
    try
    {
        header = ::DataPacketHeader {packet}; // Copy elision
    }
    catch (const std::exception &e)
    {
        pImpl->mLogger->warn(
            "Failed to unpack dataPacketHeader.  Failed because: "
          + std::string {e.what()} + "; Not allowing...");
        return false;
    }
    return pImpl->allow(header);
}
