#ifndef PRIVATE_DETECTORS_THREE_COMPONENT_DATA_REQUEST_ITEM_HPP
#define PRIVATE_DETECTORS_THREE_COMPONENT_DATA_REQUEST_ITEM_HPP
#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#ifndef NDEBUG
#include <cassert>
#endif
#include <limits>
#include <cmath>
#include "urts/services/scalable/packetCache/requestor.hpp"
#include "urts/services/scalable/packetCache/dataRequest.hpp"
#include "urts/services/scalable/packetCache/dataResponse.hpp"
#include "urts/services/scalable/packetCache/bulkDataRequest.hpp"
#include "urts/services/scalable/packetCache/bulkDataResponse.hpp"
#include "threeComponentChannelData.hpp"
#include "getNow.hpp"
namespace
{

/// @brief Since we have a lot of threads running in this it doesn't make
///        much sense to make a container of processing items that is
///        thread-safe.  Instead, we make a thread-safe processing item
///        that can store intermediate results.
class ThreeComponentDataRequestItem
{
public:
    enum class ProcessingMode
    {
        Initialize,
        Query,
        Inference
    };
public:
    ThreeComponentDataRequestItem() = delete;
    ThreeComponentDataRequestItem(const ::ThreeComponentChannelData &channelData,
                                  const std::chrono::microseconds &detectorWindowDuration = std::chrono::microseconds {10080000},
                                  const int gapToleranceInSamples = 5,
                                  const double waitPercentage = 30,
                                  const int centerWindowStart = 254,
                                  const int centerWindowEnd = 754,
                                  const double detectorSamplingRate = 100) :
        mChannelData(channelData),
        mDetectorWindowDuration(detectorWindowDuration),
        mDetectorProbabilitySignalSamplingRate(detectorSamplingRate),
        mCenterWindowStart(centerWindowStart),
        mCenterWindowEnd(centerWindowEnd)
    {
        mProcessingMode = ProcessingMode::Initialize;
        // Initialize the interpolator
        auto samplingRate = mChannelData.getNominalSamplingRate();
        mGapTolerance = std::chrono::microseconds
        {
            static_cast<int64_t> (std::round(std::max(0, gapToleranceInSamples)
                                            /samplingRate*1.e6))
        };
        mInterpolator.setNominalSamplingRate(
            mChannelData.getNominalSamplingRate());
        mInterpolator.setGapTolerance(mGapTolerance);

        // Figure out where to prepad next query to accomdate sliding window
        std::chrono::microseconds centerWindowStartDuration{
            static_cast<int64_t> (std::round(mCenterWindowStart
                                /mDetectorProbabilitySignalSamplingRate*1.e6))};
        std::chrono::microseconds centerWindowEndDuration{
            static_cast<int64_t> (std::round(mCenterWindowEnd
                                /mDetectorProbabilitySignalSamplingRate*1.e6))};
        mWindowCorrection = (detectorWindowDuration - centerWindowEndDuration)
                          + centerWindowStartDuration;

        // Don't spam the packet cache's with redundant requests
        mQueryWaitInterval = std::chrono::microseconds
        {
            static_cast<int64_t> (std::round(detectorWindowDuration.count()
                                            *(waitPercentage/100.)))
        };
 
        // Extract some station naming information
        auto network = mChannelData.getNetwork();
        auto station = mChannelData.getStation();
        auto verticalChannel = mChannelData.getVerticalChannel();
        auto northChannel    = mChannelData.getNorthChannel();
        auto eastChannel     = mChannelData.getEastChannel();
        auto locationCode = mChannelData.getLocationCode();
        mName = network + "." + station + "." + locationCode;

        // Predefine a vertical/north/east request
        mVerticalRequest.setNetwork(network);
        mVerticalRequest.setStation(station);
        mVerticalRequest.setChannel(verticalChannel);
        mVerticalRequest.setLocationCode(locationCode);
        mVerticalRequest.setIdentifier(0);

        mNorthRequest.setNetwork(network);
        mNorthRequest.setStation(station);
        mNorthRequest.setChannel(northChannel);
        mNorthRequest.setLocationCode(locationCode);
        mNorthRequest.setIdentifier(1);

        mEastRequest.setNetwork(network);
        mEastRequest.setStation(station);
        mEastRequest.setChannel(eastChannel);
        mEastRequest.setLocationCode(locationCode);
        mEastRequest.setIdentifier(2);

        // Initialize timing
        mNextQueryStartTime = ::getNow();
        mProcessingMode = ProcessingMode::Query; 
    }
    /// @result The hash for this item
    [[nodiscard]] size_t getHash() const
    {
        return mChannelData.getHash();
    }
    /// @result Updates the last packet cache query time to now
    void updateLastQueryTimeToNow()
    {
        mLastQueryTime = ::getNow();
    }
    /// @brief Queries the packet cache
    void queryPacketCache(
        URTS::Services::Scalable::PacketCache::Requestor &requestor)
    {
        // Don't spam the packet caches
        auto timeNow = ::getNow();
        if (timeNow - mQueryWaitInterval < mLastQueryTime){return;}

        // Setup the query intervals
        auto t0Query = static_cast<double> (mNextQueryStartTime.count()*1.e-6);
t0Query = t0Query - 10;
        constexpr auto t1Query
            = static_cast<double> (std::numeric_limits<uint32_t>::max());

        // Create the individual requests
        std::pair<double, double> queryTimes{t0Query, t1Query};
        mVerticalRequest.setQueryTimes(queryTimes);
        mNorthRequest.setQueryTimes(queryTimes);
        mEastRequest.setQueryTimes(queryTimes);

        namespace UPC = URTS::Services::Scalable::PacketCache;
        UPC::BulkDataRequest bulkRequest;
        bulkRequest.setIdentifier(mRequestIdentifier);
        bulkRequest.addDataRequest(mVerticalRequest);
        bulkRequest.addDataRequest(mNorthRequest);
        bulkRequest.addDataRequest(mEastRequest);

        // Send the request
        updateLastQueryTimeToNow(); // I've now done a query 
        auto reply = requestor.request(bulkRequest);
        mRequestIdentifier = mRequestIdentifier + 3;
        if (mRequestIdentifier > std::numeric_limits<int64_t>::max() - 10)
        {
            mRequestIdentifier = 0;
        }
        if (reply == nullptr)
        {
            throw std::runtime_error("Request for " + mName
                                   + " may have timed out");
        }
        if (reply->getReturnCode() !=
            UPC::BulkDataResponse::ReturnCode::Success)
        {
            throw std::runtime_error("Request for " + mName
                                   + " failed with error code: " 
                                   + std::to_string(reply->getReturnCode()));
        }

        // Unpack the responses (need to divine vertical/north/east)
        auto nResponses = reply->getNumberOfDataResponses();
        if (nResponses != 3)
        {
            throw std::runtime_error("Request for " + mName
                                   + " expected 3 responses but received "
                                   + std::to_string(nResponses));
        }
        const auto dataResponsesPtr = reply->getDataResponsesPointer();
        std::array<int, 3> indices{-1, -1, -1};
        for (int i = 0; i < 3; ++i)
        {
            auto id = dataResponsesPtr[i].getIdentifier();
            if (id == mVerticalRequest.getIdentifier())
            {
                indices[0] = i;
            }
            else if (id == mNorthRequest.getIdentifier())
            {
                indices[1] = i;
            }
            else if (id == mEastRequest.getIdentifier())
            {
                indices[2] = i;
            }
#ifndef NDEBUG
            else
            {
                assert(false);
            }
#endif
        }
#ifndef NDEBUG
        assert(indices[0] != -1 && indices[1] != -1 && indices[2] != -2);
#endif
        // Is there data?
        if (dataResponsesPtr[indices[0]].getNumberOfPackets() < 1 ||
            dataResponsesPtr[indices[1]].getNumberOfPackets() < 1 ||
            dataResponsesPtr[indices[2]].getNumberOfPackets() < 1)
        {
            throw std::runtime_error("At least one channel for " + mName
                                   + " does not have any packets");
        }
        // Set the three-component waveform (and interpolate).  This
        // also truncates the signal.
        mInterpolator.set(dataResponsesPtr[indices[0]],
                          dataResponsesPtr[indices[1]],
                          dataResponsesPtr[indices[2]]);
        // Get the valid interpolated times
        auto t0Interpolated = mInterpolator.getStartTime();
        auto t1Interpolated = mInterpolator.getEndTime();
        auto signalDuration = t1Interpolated - t0Interpolated;
        if (signalDuration < mDetectorWindowDuration){return;} // Give up early

        // Okay, let's extract some signals.
        auto vReference = mInterpolator.getVerticalSignalReference();
        auto nReference = mInterpolator.getNorthSignalReference();
        auto eReference = mInterpolator.getEastSignalReference();
        auto gReference = mInterpolator.getGapIndicatorReference();
#ifndef NDEBUG
        assert(vReference.size() == nReference.size());
        assert(vReference.size() == eReference.size());
        assert(vReference.size() == gReference.size());
#endif
        // Does the duration exceed a processing window?  If yes, then
        // update the next query time.  Additionally, we have to let the
        // detector know it is time to work.
        if (signalDuration >= mDetectorWindowDuration)
        {
            mNextQueryStartTime = t1Interpolated - mWindowCorrection;
        }
        // Update the processing mode
        mProcessingMode = ProcessingMode::Inference;
    }
    /// @result The center window start index.
    [[nodiscard]] int getCenterWindowStart() const noexcept
    {
        return mCenterWindowStart;
    }
    /// @result The center window end index.
    [[nodiscard]] int getCenterWindowEnd() const noexcept
    {
        return mCenterWindowEnd;
    }
    /// @result The channel data.
    [[nodiscard]] const ::ThreeComponentChannelData &getChannelDataReference() const
    {
        return *&mChannelData;
    }
//private:
    ::ThreeComponentChannelData mChannelData;
    std::string mName;
    URTS::Services::Scalable::PacketCache::ThreeComponentWaveform mInterpolator;
    URTS::Services::Scalable::PacketCache::DataRequest mVerticalRequest;
    URTS::Services::Scalable::PacketCache::DataRequest mNorthRequest;
    URTS::Services::Scalable::PacketCache::DataRequest mEastRequest;
    // When the next query should begin.  It will query up to now.
    std::chrono::microseconds mNextQueryStartTime{0};
    // Last time packet cache was queried
    std::chrono::microseconds mLastQueryTime{0};
    // The detector window's duration
    std::chrono::microseconds mDetectorWindowDuration{10080000};
    // To prevent spamming the PC we wait at least this long between
    // successive PC queries
    std::chrono::microseconds mQueryWaitInterval{3024000};
    std::chrono::microseconds mGapTolerance{50000}; // 5 samples at 100 Hz
    // This is a safety/gamemanship with the PC query
    std::chrono::microseconds mPrepadQuery{500000}; // Default: prepad 0.5 s
    // To account for the sliding window and the region of validity we have
    // to prepad the next query by this much so we can generate a `continuous'
    // stream of probabilities.  This default amount is:
    //    (10.08s - 7.54s) + 2.54s
    // i.e., if we start the next query at 10.08s - 5.08s = 5s then the
    // prewindow nulling extracts the next signal at 5s + 2.54 = 7.54s
    std::chrono::microseconds mWindowCorrection{5000000};
    int64_t mRequestIdentifier{0};
    // Detector sampling rate output signal
    double mDetectorProbabilitySignalSamplingRate{100};
    // Detector center window start index
    int mCenterWindowStart{254};
    // Detector center window end index
    int mCenterWindowEnd{754};
    // Processing mode
    ProcessingMode mProcessingMode{ProcessingMode::Initialize};
};
[[nodiscard]] [[maybe_unused]]
bool operator<(const ThreeComponentDataRequestItem &lhs,
               const ThreeComponentDataRequestItem &rhs)
{
   return lhs.getHash() < rhs.getHash();
}
}
#endif
