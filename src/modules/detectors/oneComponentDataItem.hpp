#ifndef PRIVATE_DETECTORS_ONE_COMPONENT_DATA_ITEM_HPP
#define PRIVATE_DETECTORS_ONE_COMPONENT_DATA_ITEM_HPP
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
#include "urts/services/scalable/packetCache/singleComponentWaveform.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/processingRequest.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/processingResponse.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/requestor.hpp"
#include "urts/database/aqms/channelData.hpp"
#include "threadSafeState.hpp"
#include "getNow.hpp"
namespace
{

size_t toHash(const URTS::Database::AQMS::ChannelData &channel)
{
   auto hashName = channel.getNetwork() + "." 
                 + channel.getStation() + "." 
                 + channel.getChannel() + "." 
                 + channel.getLocationCode();
   return std::hash<std::string>{} (hashName);
}

class OneComponentDataItem
{
public:
    OneComponentDataItem() = delete;
    OneComponentDataItem(const URTS::Database::AQMS::ChannelData &channelData,
                         const std::chrono::microseconds &detectorWindowDuration = std::chrono::microseconds {10080000},
                         const std::chrono::seconds maximumSignalLatency = std::chrono::seconds {180},
                         const int gapToleranceInSamples = 5,
                         const double waitPercentage = 30,
                         const int centerWindowStart = 254,
                         const int centerWindowEnd = 754,
                         const double detectorSamplingRate = 100,
                         std::shared_ptr<UMPS::Logging::ILog> logger = nullptr) :
        mState(::toHash(channelData), ::ThreadSafeState::State::Initializing),
        mChannelData(channelData),
        mLogger(logger),
        mDetectorWindowDuration(detectorWindowDuration),
        mMaximumSignalLatency(maximumSignalLatency),
        mDetectorProbabilitySignalSamplingRate(detectorSamplingRate),
        mCenterWindowStart(centerWindowStart),
        mCenterWindowEnd(centerWindowEnd)
    {
        // Initialize the interpolator
        auto samplingRate = mChannelData.getSamplingRate();
        mGapTolerance = std::chrono::microseconds
        {
            static_cast<int64_t> (std::round(std::max(0, gapToleranceInSamples)
                                            /samplingRate*1.e6))
        };
        mInterpolator.setNominalSamplingRate(
            mChannelData.getSamplingRate());
        mInterpolator.setGapTolerance(mGapTolerance);

        // Will the output probability signals have a different sampling rate?
        mChangesSamplingRate = false;
        if (std::abs(detectorSamplingRate - samplingRate) > 1.e-4)
        {
            mChangesSamplingRate = true;
        }

        // Don't spam the packet cache's with redundant requests
        mQueryWaitInterval = std::chrono::microseconds
        {
            static_cast<int64_t> (std::round(mDetectorWindowDuration.count()
                                            *(waitPercentage/100.)))
        };
 
        // Extract some station naming information
        auto network = mChannelData.getNetwork();
        auto station = mChannelData.getStation();
        auto verticalChannel = mChannelData.getChannel();
        auto locationCode = mChannelData.getLocationCode();
        mName = network + "." + station + "."
              + verticalChannel + "." + locationCode;

        // Predefine a vertical/north/east request
        mVerticalRequest.setNetwork(network);
        mVerticalRequest.setStation(station);
        mVerticalRequest.setChannel(verticalChannel);
        mVerticalRequest.setLocationCode(locationCode);
        mVerticalRequest.setIdentifier(0);

        // Predefine inference requests
        mPInferenceRequest.setSamplingRate(samplingRate);
        mPInferenceRequest.setIdentifier(1);

        // Predefine probability packets
        std::string pChannel{verticalChannel, 0, 2};
        pChannel.push_back('P');
        mPProbabilityPacket.setNetwork(network);
        mPProbabilityPacket.setStation(station); 
        mPProbabilityPacket.setChannel(pChannel);
        mPProbabilityPacket.setLocationCode(locationCode);
        mPProbabilityPacket.setSamplingRate(detectorSamplingRate);

        // Initialize timing
        updateLastProbabilityTimeToNow();
        setState(::ThreadSafeState::State::ReadyToQueryData);
    }
    /// @result The hash for this item
    [[nodiscard]] size_t getHash() const
    {
        return mState.getHash();
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
        if (getState() != ::ThreadSafeState::State::QueryData){return;}
        // Don't spam the packet caches
        auto timeNow = ::getNow();
//std::cout << std::setprecision(16) << mLastQueryTime.count() << " " << timeNow.count() << " " << mQueryWaitInterval.count() << std::endl;
        if (timeNow - mQueryWaitInterval < mLastQueryTime)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            return;
        }
#ifndef NDEBUG
        mPipelineDuration = -1;
        mPipelineStartTime = timeNow;
#endif
        // Deal with highly latent data
        if (timeNow - mMaximumSignalLatency > mLastProbabilityTime)
        {
            updateLastProbabilityTimeToNow();
        }
        // Update my request identifier (and avoid overflow in a billion years)
        if (mRequestIdentifier > std::numeric_limits<int64_t>::max() - 10) 
        {
            mRequestIdentifier = 0;
        }
        // Setup the query intervals.  Since we don't retain the first samples
        // because of training details, we need to accomodate for that delay
        // in the query.  Additionally, we add a safety margin as well.
        auto t0QueryMuSec = mLastProbabilityTime
                          - mStartWindowTime
                          - mPrepadQuery;
        auto t0Query = static_cast<double> (t0QueryMuSec.count()*1.e-6);
        auto t1Query = static_cast<double> (timeNow.count()*1.e-6);
        URTS::Services::Scalable::PacketCache::DataRequest dataRequest;
        try
        {
            // Create the individual requests from start time to infinity
            std::pair<double, double> queryTimes{t0Query, t1Query};
            mVerticalRequest.setQueryTimes(queryTimes);
            mVerticalRequest.setIdentifier(mRequestIdentifier);
        }
        catch (const std::exception &e)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            throw std::runtime_error("Failed to create request: "
                                   + std::string{e.what()});
        }
        std::unique_ptr<URTS::Services::Scalable::PacketCache::DataResponse>
            reply{nullptr};
        try
        {
            reply = requestor.request(mVerticalRequest);
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
        if (reply == nullptr)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            throw std::runtime_error("Request for " + mName
                                   + " may have timed out");
        }
        if (reply->getReturnCode() !=
            URTS::Services::Scalable::PacketCache::DataResponse
                                                 ::ReturnCode::Success)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            throw std::runtime_error("Request for " + mName
                                   + " failed with error code: " 
                                   + std::to_string(reply->getReturnCode()));
        }

        // Unpack the responses (need to divine vertical/north/east)
        auto nPackets = reply->getNumberOfPackets();
        if (nPackets < 1)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            return;
            //throw std::runtime_error("Request for " + mName
            //                       + " expected 3 responses but received "
            //                       + std::to_string(nResponses));
        }
        // Is there data?
        if (reply->getNumberOfPackets() < 1)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            return;
            //throw std::runtime_error("At least one channel for " + mName
            //                       + " does not have any packets");
        }
        // Set the signle-channel waveform (and interpolate).  This
        // also truncates the signal.
        try
        {
            mInterpolator.set(*reply); //dataResponsesPtr[0]);
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
        // Give up early
        if (signalDuration < mDetectorWindowDuration)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            return;
        }

        // Okay, let's extract some signals.
#ifndef NDEBUG
        const auto &vReference = mInterpolator.getSignalReference();
        const auto &gReference = mInterpolator.getGapIndicatorReference();
        assert(vReference.size() == gReference.size());
#endif 
        // Do we have a minimum amount of data to perform inference?
        if (signalDuration < mDetectorWindowDuration)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            return;
        }
        // Do we have a minimum amount of data to do any updates?
        auto endWindowDuration = mDetectorWindowDuration - mEndWindowTime;
        if (t1Interpolated < mLastProbabilityTime + endWindowDuration)
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
    [[nodiscard]] const URTS::Database::AQMS::ChannelData &getChannelDataReference() const
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
    /// @brief Performs a P inference.
    void performP(
        URTS::Services::Scalable::Detectors::UNetOneComponentP::Requestor
            &pRequestor)
    {
        mInferencedP = false;
        mBroadcastP = false;
        namespace UPDetectors = URTS::Services::Scalable::Detectors::UNetOneComponentP;
        if (getState() != ::ThreadSafeState::State::InferenceP)
        {
            return;
        }
        // Make a request
        auto vertical     = mInterpolator.getSignalReference();
        auto gapIndicator = mInterpolator.getGapIndicatorReference();
        auto haveGaps     = mInterpolator.haveGaps();
        // Figure out where to start query
        auto t0Signal = mInterpolator.getStartTime();
        auto t1Signal = mInterpolator.getEndTime();
#ifndef NDEBUG
        auto endWindowDuration = mDetectorWindowDuration - mEndWindowTime;
        assert(t1Signal - t0Signal >= mDetectorWindowDuration);
        assert(t1Signal >= mLastProbabilityTime + endWindowDuration);
#endif
        try
        {
            auto pSlidingWindow
                 = UPDetectors::ProcessingRequest
                              ::InferenceStrategy::SlidingWindow;
            mPInferenceRequest.setSignal(vertical, pSlidingWindow);
        }
        catch (const std::exception &e)
        {
            setState(::ThreadSafeState::State::ReadyToQueryData);
            throw std::runtime_error(e.what());
        } 
        // Do it
        std::unique_ptr<UPDetectors::ProcessingResponse> pResponse{nullptr};
        int nPSamplesOut = 0;
        try
        {
            pResponse = pRequestor.request(mPInferenceRequest); 
            if (pResponse == nullptr)
            {
                throw std::runtime_error("P request timed out");
            }
            if (pResponse->haveProbabilitySignal())
            {
                nPSamplesOut
                    = static_cast<int>
                      (pResponse->getProbabilitySignalReference().size());
                mInferencedP = true;
            }
        }
        catch (const std::exception &e)
        {
            if (mLogger)
            {
                mLogger->warn("P inference request failed: " + std::string{e.what()});
            }
        } 
        auto dtMuSec = std::round(1e6/mDetectorProbabilitySignalSamplingRate);
        auto idtMuSec
            = std::chrono::microseconds {static_cast<int64_t> (dtMuSec)};
        // Figure out where to stop extracting signals.  Basically, we have to
        // leave-off the tail.  We are also gauranteed that the signal
        // duration is at least mDetectorWindowDuration (e.g., 10.08) seconds
        // otherwise we would have never submitted the request.
        int i1
             = static_cast<int> (
                  std::round(
                     ((t1Signal.count() - endWindowDuration.count())
                     - t0Signal.count() )/dtMuSec
                  )
               );
#ifndef NDEBUG
        assert(i1 >= 0);
#endif
        i1 = std::min(nPSamplesOut, i1);
        int i0 = 0;
        // In this case, our detector has had a proper build up time
        if (t0Signal + mStartWindowTime <= mLastProbabilityTime + idtMuSec/2)
        {
            // Begin extracting this signal at the last probability time
            i0 = static_cast<int> (
                   std::round(
                       (mLastProbabilityTime.count() - t0Signal.count())/dtMuSec
                   )
                 );
        }
        else
        {
            // Instead, we deal with a gap.  In this case, the first valid 
            // probability time typically 2.54 seconds into the probability
            // signal.
            i0 = static_cast<int> (
                 std::round(mStartWindowTime.count()/dtMuSec));
        }
        i0 = std::max(0, std::min(i1, i0));
//std::cout << i0 << " " << " " << i1 << std::endl;
        // Unpack the signal 
        if (mInferencedP)
        {
            const auto &pRef = pResponse->getProbabilitySignalReference();
            std::vector<double> pSignal(std::max(0, i1 - i0), 0);
            if (!haveGaps)
            {
#ifndef NDEBUG
                assert(i0 >= 0);
                assert(i1 <= static_cast<int> (pRef.size()));
#endif
                std::copy(pRef.begin() + i0, pRef.begin() + i1,
                          pSignal.data());
            }
            else
            {
                if (mLogger){mLogger->warn("Dealing with P gaps");} //std::cout << "dealing with gaps p" << std::endl;
                if (!mChangesSamplingRate)
                {
                    for (int i = i0; i < i1; ++i)
                    {
                        pSignal[i - i0] = pRef[i]*gapIndicator[i];
                    }
                }
                else
                {
                    // TODO
                    for (int i = i0; i < i1; ++i)
                    {   
                        pSignal[i - i0] = pRef[i];
                    }
                }
            }
            if (!pSignal.empty())
            {
                mPProbabilityPacket.setStartTime(t0Signal + i0*idtMuSec);
                mPProbabilityPacket.setData(std::move(pSignal));
                mBroadcastP = true;
            }
        }
        // At this point we are `successful.'  Sure, the broadcast coudl fail,
        // or now data was extracted, but the machine must move forward and
        // iterate at our last valid probability time.
        mLastProbabilityTime = t0Signal
                             + i1*std::chrono::microseconds
                                  {static_cast<int64_t> (dtMuSec)};
        setState(::ThreadSafeState::State::ReadyForBroadcasting);
    }
    /// @brief Broadcasts packets
    void broadcast(URTS::Broadcasts::Internal::DataPacket::Publisher &publisher)
    {
        if (getState() != ::ThreadSafeState::State::BroadcastP)
        {
            return;
        }
        //std::cout << std::setprecision(16) << mName << " " << getNow().count()*1.e-6 << " " << mPProbabilityPacket.getStartTime().count()*1.e-6 << " " << mPProbabilityPacket.getEndTime().count()*1.e-6 << " " << mLastProbabilityTime.count()*1.e-6 << std::endl; 
        std::string error;
        if (mBroadcastP)
        {
            try
            {
                publisher.send(mPProbabilityPacket);
            }
            catch (const std::exception &e)
            {
                if (mLogger){mLogger->warn("Problems broadcasting P");}
                error = "Problems broadcasting P: " + std::string {e.what()};
            }
        }
#ifndef NDEBUG
        auto endTime = ::getNow();
        auto pipelineDuration
            = std::chrono::duration_cast<std::chrono::microseconds>
              (endTime - mPipelineStartTime);
        mPipelineDuration
            = static_cast<double> (pipelineDuration.count())*1.e-6;
/*
        std::cout << mName << " " 
                  << mPipelineDuration << " "
                  << endTime.count() << " "
                  << mPipelineStartTime.count() << std::endl;
*/
#endif
        setState(::ThreadSafeState::State::ReadyToQueryData);
        if (!error.empty()){throw std::runtime_error(error);}
    }
//private:
    /// Defines the state
    ::ThreadSafeState mState;
    /// Channel data
    URTS::Database::AQMS::ChannelData mChannelData;
    /// Logger
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    /// Name of single-channel data
    std::string mName;
    /// Utility for interpolating 1C waveforms
    URTS::Services::Scalable::PacketCache::SingleComponentWaveform mInterpolator;
    /// Basically, these are fully defined data requests which will be updated
    /// with the query times into a bulk data request
    URTS::Services::Scalable::PacketCache::DataRequest mVerticalRequest;
    // Partially populated requests for the inference services
    URTS::Services::Scalable::Detectors::UNetOneComponentP::ProcessingRequest mPInferenceRequest;
    // Partially defined data packet
    URTS::Broadcasts::Internal::DataPacket::DataPacket mPProbabilityPacket;
    // When the next query should begin.  It will query up to now.
    std::chrono::microseconds mNextQueryStartTime{0};
    // Last time packet cache was queried
    std::chrono::microseconds mLastQueryTime{0};
    // The detector window's duration
    std::chrono::microseconds mDetectorWindowDuration{10080000};
    // The maximum signal latency to consider.
    std::chrono::seconds mMaximumSignalLatency{180};
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
#ifndef NDEBUG
    // Utilities for timing
    double mPipelineDuration{-1};
    std::chrono::microseconds mPipelineStartTime{0};
#endif
    // Helps us unpack bulk data requests.
    int64_t mRequestIdentifier{0};
    // Detector sampling rate output signal
    double mDetectorProbabilitySignalSamplingRate{100};
    // Detector center window start index
    int mCenterWindowStart{254};
    // Detector center window end index
    int mCenterWindowEnd{754};
    // Defines what type of processing to perform. 
    bool mInferencedP{false};
    bool mInferencedS{false};
    bool mBroadcastP{false};
    bool mBroadcastS{false};
    bool mChangesSamplingRate{false};
};
[[nodiscard]] [[maybe_unused]]
bool operator<(const OneComponentDataItem &lhs,
               const OneComponentDataItem &rhs)
{
   return lhs.getHash() < rhs.getHash();
}
}
#endif
