#ifndef PRIVATE_DETECTORS_THREE_COMPONENT_DATA_ITEM_HPP
#define PRIVATE_DETECTORS_THREE_COMPONENT_DATA_ITEM_HPP
#include <iostream>
#include <iomanip>
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
#include "threadSafeState.hpp"
#include "getNow.hpp"
namespace
{

/// @brief This is like a row in a database that holds the state of a 
///        three-component waveform.
class ThreeComponentDataItem
{
public:
    enum class ProcessingMode
    {
        P,    /*!< P only processing and inference. */
        S,    /*!< S only processing and inference. */
        PAndS /*!< P and S processing and inference. */
    };
public:
    ThreeComponentDataItem() = delete;
    ThreeComponentDataItem(const ::ThreeComponentChannelData &channelData,
                           const std::chrono::microseconds &detectorWindowDuration = std::chrono::microseconds {10080000},
                           const int gapToleranceInSamples = 5,
                           const double waitPercentage = 30,
                           const int centerWindowStart = 254,
                           const int centerWindowEnd = 754,
                           const double detectorSamplingRate = 100) :
        mState(channelData.getHash(), ::ThreadSafeState::State::Initializing),
        mChannelData(channelData),
        mDetectorWindowDuration(detectorWindowDuration),
        mDetectorProbabilitySignalSamplingRate(detectorSamplingRate),
        mCenterWindowStart(centerWindowStart),
        mCenterWindowEnd(centerWindowEnd)
    {
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
/*
        std::chrono::microseconds centerWindowStartDuration{
            static_cast<int64_t> (std::round(mCenterWindowStart
                                /mDetectorProbabilitySignalSamplingRate*1.e6))};
        std::chrono::microseconds centerWindowEndDuration{
            static_cast<int64_t> (std::round(mCenterWindowEnd
                                /mDetectorProbabilitySignalSamplingRate*1.e6))};
        mWindowCorrection = (detectorWindowDuration - centerWindowEndDuration)
                          + centerWindowStartDuration;
*/

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
        std::string channelCode3C{verticalChannel, 0, 2}; 
        channelCode3C.push_back('[');
        channelCode3C.push_back(verticalChannel.back());
        channelCode3C.push_back(northChannel.back());
        channelCode3C.push_back(eastChannel.back());
        channelCode3C.push_back(']');
        mName = network + "." + station + "."
              + channelCode3C + "." + locationCode;

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
        updateLastProbabilityTimeToNow();
        setState(::ThreadSafeState::State::ReadyToQueryData);
    }
    /// @result The hash for this item
    [[nodiscard]] size_t getHash() const
    {
        return mChannelData.getHash();
    }
    /// @result The name of this data itme.
    [[nodiscard]] std::string getName() const
    {
        return mName;
    }
    /// @result Updates the last packet cache query time to now
    void updateLastQueryTimeToNow()
    {
        mLastQueryTime = ::getNow();
    }
    /// @result Updates the probability packet valid start time to now
    void updateLastProbabilityTimeToNow()
    {
        mLastProbabilityTime = ::getNow();
    }
    /// @brief Queries the packet cache
    void queryPacketCache(
        URTS::Services::Scalable::PacketCache::Requestor &requestor)
    {
        // Are we even in the correct processing mode?
        auto myState = getState();
        if (myState != ::ThreadSafeState::State::QueryData){return;}
        // Don't spam the packet caches
        auto timeNow = ::getNow();
        if (timeNow - mQueryWaitInterval < mLastQueryTime)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            return;
        }

        // Setup the query intervals.  Since we don't retain the first samples
        // because of training details, we need to accomodate for that delay
        // in the query.  Additionally, we add a safety margin as well.
        auto t0QueryMuSec = mLastProbabilityTime
                          - mStartWindowTime
                          - mPrepadQuery;
        auto t0Query = static_cast<double> (t0QueryMuSec.count()*1.e-6);
t0Query = t0Query - 10;
        constexpr auto t1Query
            = static_cast<double> (std::numeric_limits<uint32_t>::max());

        namespace UPC = URTS::Services::Scalable::PacketCache;
        UPC::BulkDataRequest bulkRequest;
        try
        {
            // Create the individual requests from start time to infinity
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
        }
        catch (const std::exception &e)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            throw std::runtime_error("Failed to create request: "
                                   + std::string{e.what()});
        }
        std::unique_ptr<URTS::Services::Scalable::PacketCache::BulkDataResponse>
            reply{nullptr};
        try
        {
            reply = requestor.request(bulkRequest);
        }
        catch (const std::exception &e)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            updateLastQueryTimeToNow();
            throw std::runtime_error("Packet cache communication error: "
                                   + std::string {e.what()});
        }
        // Note that I have performed the query
        updateLastQueryTimeToNow();
        // Update my request identifier (and avoid overflow in a billion years)
        mRequestIdentifier = mRequestIdentifier + 3;
        if (mRequestIdentifier > std::numeric_limits<int64_t>::max() - 10)
        {
            mRequestIdentifier = 0;
        }
        if (reply == nullptr)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            throw std::runtime_error("Request for " + mName
                                   + " may have timed out");
        }
        if (reply->getReturnCode() !=
            UPC::BulkDataResponse::ReturnCode::Success)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            throw std::runtime_error("Request for " + mName
                                   + " failed with error code: " 
                                   + std::to_string(reply->getReturnCode()));
        }

        // Unpack the responses (need to divine vertical/north/east)
        auto nResponses = reply->getNumberOfDataResponses();
std::cout << std::setprecision(16) << t0Query << " " << nResponses << " " << mName << std::endl;
        if (nResponses != 3)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
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
            setState(::ThreadSafeState::State::ReadyToQueryData);
            throw std::runtime_error("At least one channel for " + mName
                                   + " does not have any packets");
        }
        // Set the three-component waveform (and interpolate).  This
        // also truncates the signal.
        try
        {
            mInterpolator.set(dataResponsesPtr[indices[0]],
                              dataResponsesPtr[indices[1]],
                              dataResponsesPtr[indices[2]]);
        }
        catch (const std::exception &e)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            throw std::runtime_error("Failed to set data responses for "
                                   + mName + ".  Failed with: "
                                   + std::string{e.what()});
        }
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
        // Do we have a minimum amount of data to perform inference?
        if (signalDuration < mDetectorWindowDuration)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            return;
        }
        // Do we have a minimum amount of data to do any updates?
        if (t1Interpolated - (mDetectorWindowDuration - mEndWindowTime) <
            mLastProbabilityTime)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            return;
        }
        // We now have enough signal to perform inference and will get new
        // probability samples after performing inference.
        setState(::ThreadSafeState::State::ReadyForInferencing);
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
    /// @brief Updates the state
    void setState(const ::ThreadSafeState::State state) noexcept
    {
        mState.setState(state);
    }
    [[nodiscard]] ::ThreadSafeState::State getState() const noexcept
    {
        return mState.getState();
    }
//private:
    /// Defines the state
    ::ThreadSafeState mState;
    /// Channel data
    ::ThreeComponentChannelData mChannelData;
    /// Name of three-component data
    std::string mName;
    /// Utility for interpolating 3C waveforms
    URTS::Services::Scalable::PacketCache::ThreeComponentWaveform mInterpolator;
    /// Basically, these are fully defined data requests which will be updated
    /// with the query times into a bulk data request
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
    std::chrono::microseconds mStartWindowTime{2540000};
    std::chrono::microseconds mEndWindowTime{7540000};
    // This is the time stamp immediately after the last probability
    // sample.  From this we can define appropriate data queries and
    // processing results.
    std::chrono::microseconds mLastProbabilityTime{0};
    // Helps us unpack bulk data requests.
    int64_t mRequestIdentifier{0};
    // Detector sampling rate output signal
    double mDetectorProbabilitySignalSamplingRate{100};
    // Detector center window start index
    int mCenterWindowStart{254};
    // Detector center window end index
    int mCenterWindowEnd{754};
    // Defines what type of processing to perform. 
//    ProcessingMode mProcessingMode(ProcessingMode::PAndS);
};
[[nodiscard]] [[maybe_unused]]
bool operator<(const ThreeComponentDataItem &lhs,
               const ThreeComponentDataItem &rhs)
{
   return lhs.getHash() < rhs.getHash();
}
}
#endif
