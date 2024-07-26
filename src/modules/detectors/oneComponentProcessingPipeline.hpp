#ifndef URTS_MODULES_ONE_COMPONENT_PROCESSING_PIPELINE_HPP
#define URTS_MODULES_ONE_COMPONENT_PROCESSING_PIPELINE_HPP
#include <iostream>
#include <iomanip>
#include <mutex>
#include <array>
#ifndef NDEBUG
#include <cassert>
#endif
#include "urts/database/aqms/channelData.hpp"
#include "urts/broadcasts/internal/dataPacket.hpp"
#include "urts/broadcasts/internal/probabilityPacket.hpp"
#include "urts/services/scalable/packetCache.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP.hpp"
#include "programOptions.hpp"
#include "getNow.hpp"
#include "threeComponentProcessingPipeline.hpp"
//#include "oneComponentChannelData.hpp"
namespace
{

std::unique_ptr<URTS::Services::Scalable::Detectors::
                UNetOneComponentP::ProcessingResponse>
inference1C(const URTS::Services::Scalable::PacketCache::
                        SingleComponentWaveform &interpolator,
            URTS::Services::Scalable::Detectors::UNetOneComponentP::
                  Requestor &requestor,
            URTS::Services::Scalable::Detectors::UNetOneComponentP::
                  ProcessingRequest &request)
{
    const auto &vertical = interpolator.getSignalReference();
    auto slidingWindow
        = URTS::Services::Scalable::Detectors::UNetOneComponentP
              ::ProcessingRequest::InferenceStrategy::SlidingWindow;
    request.setSignal(vertical, slidingWindow);
    auto response = requestor.request(request);
    if (response == nullptr)
    {
        throw std::runtime_error("P request timed out");
    }
    auto success
        = URTS::Services::Scalable::Detectors::UNetOneComponentP
              ::ProcessingResponse::ReturnCode::Success;
    if (response->getReturnCode() != success)
    {
        throw std::runtime_error("P inference request failed with "
                               + std::to_string(response->getReturnCode()));
    }
    if (!response->haveProbabilitySignal())
    {
        throw std::runtime_error("P probability signal not computed");
    }
    return response;
}

std::vector<double>
extractSignal(const bool changesSamplingRate,
              const int i0, const int i1,
              const std::vector<double> &pRef,
              const URTS::Services::Scalable::
                          PacketCache::SingleComponentWaveform &interpolator,
              std::shared_ptr<UMPS::Logging::ILog> &logger)
{
    std::vector<double> pSignal(std::max(0, i1 - i0), 0);
    if (!interpolator.haveGaps())
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
        const auto &gapIndicator = interpolator.getGapIndicatorReference();
        if (!changesSamplingRate)
        {
            for (int i = i0; i < i1; ++i)
            {
#ifndef NDEBUG
                pSignal.at(i - i0) = pRef.at(i)*gapIndicator.at(i);
#else
                pSignal[i - i0] = pRef[i]*gapIndicator[i];
#endif
            }
        }
        else
        {
            logger->warn("ruh roh - gaps and changes sampling rate");
            // TODO
            for (int i = i0; i < i1; ++i)
            {
#ifndef NDEBUG
                pSignal.at(i - i0) = pRef.at(i);
#else
                pSignal[i - i0] = pRef[i];
#endif
            }
        }
    }
    return pSignal;
} 

class OneComponentProcessingItem
{
public:
    enum class State
    {
        Unknown,
        Query,
        Inference,
        Publish
    };
public:
    OneComponentProcessingItem(
        const URTS::Database::AQMS::ChannelData &channelData,
        const std::chrono::microseconds &detectorWindowDuration = std::chrono::microseconds {10080000},
        const std::chrono::seconds maximumSignalLatency = std::chrono::seconds {180},
        const int gapToleranceInSamples = 5,
        const double waitPercentage = 30, 
        const int centerWindowStart = 254,
        const int centerWindowEnd = 754,
        const double detectorSamplingRate = 100) :
        mChannelData(channelData),
        mDetectorWindowDuration(detectorWindowDuration),
        mMaximumSignalLatency(maximumSignalLatency),
        mDetectorProbabilitySignalSamplingRate(detectorSamplingRate),
        mCenterWindowStart(centerWindowStart),
        mCenterWindowEnd(centerWindowEnd),
        mState(OneComponentProcessingItem::State::Unknown)
    {
        // Initialize the interpolator
        auto samplingRate = mChannelData.getSamplingRate();
        mGapTolerance = std::chrono::microseconds
        {
            static_cast<int64_t> (std::round(std::max(0, gapToleranceInSamples)
                                            /samplingRate*1.e6))
        };
        mInterpolator.setNominalSamplingRate(samplingRate);
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
        if (locationCode.empty() || locationCode == "  ")
        {
            locationCode = "--";
        }
        std::vector<std::string> originalChannels{verticalChannel};
        mName = network + "." + station + "."
              + verticalChannel + "." + locationCode;
        mHash = std::hash<std::string>{} (mName); 

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
        mPProbabilityPacket.setOriginalChannels(originalChannels);
        mPProbabilityPacket.setPositiveClassName("P");
        mPProbabilityPacket.setNegativeClassName("Noise");
        mPProbabilityPacket.setAlgorithm("UNetOneComponentP");

        // Initialize timing
        updateLastProbabilityTimeToNow();
        mState = State::Query;
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
        URTS::Services::Scalable::PacketCache::Requestor &requestor,
        std::shared_ptr<UMPS::Logging::ILog> &logger)
    {
        // Are we even in the correct processing mode?
        if (mState != State::Query){return;}
        // Don't spam the packet caches
        auto timeNow = ::getNow();
//std::cout << std::setprecision(16) << mLastQueryTime.count() << " " << timeNow.count() << " " << mQueryWaitInterval.count() << std::endl;
        if (timeNow - mQueryWaitInterval < mLastQueryTime){return;}
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
        mRequestIdentifier = mRequestIdentifier + 1;
        // Setup the query intervals.  Since we don't retain the first samples
        // because of training details, we need to accomodate for that delay
        // in the query.  Additionally, we add a safety margin.
        auto t0QueryMuSec = mLastProbabilityTime
                          - mStartWindowTime
                          - mPrepadQuery;
        auto t1QueryMuSec = timeNow;
        auto t0Query = static_cast<double> (t0QueryMuSec.count()*1.e-6);
        auto t1Query = static_cast<double> (t1QueryMuSec.count()*1.e-6);
        try
        {
            // Create the individual requests from start time to infinity
            std::pair<double, double> queryTimes{t0Query, t1Query};
            mVerticalRequest.setQueryTimes(queryTimes);
            mVerticalRequest.setIdentifier(mRequestIdentifier);
        }
        catch (const std::exception &e)
        {
            logger->error("Instance " + std::to_string(mInstance) 
                        + " failed to create bulk request: "
                        + std::string{e.what()});
            return;
        }
        std::unique_ptr<URTS::Services::Scalable::PacketCache::DataResponse>
            reply{nullptr};
        try
        {
            reply = requestor.request(mVerticalRequest);
        }
        catch (const std::exception &e)
        {
            updateLastQueryTimeToNow();
            logger->warn("Instance " + std::to_string(mInstance)
                       + " encountered packet cache communication error");
            return;
        }
        // Note that I have performed the query
        updateLastQueryTimeToNow();
        if (reply == nullptr)
        {
            logger->warn("Instance " + std::to_string(mInstance)
                        + "'s packet cache request for " + mName
                        + " may have timed out");
            return;
        }
        if (reply->getReturnCode() !=
            URTS::Services::Scalable::PacketCache::DataResponse
                                                 ::ReturnCode::Success)
        {
            logger->warn("Instance " + std::to_string(mInstance)
                       + "'s request for " + mName
                       + " failed with error code: "
                       + std::to_string(reply->getReturnCode()));
            return;
        }
        // Unpack the responses (need to divine vertical/north/east)
        if (reply->getNumberOfPackets() < 1){return;} // Leave in query mode
        // Set the single-channel waveform (and interpolate).  This
        // also truncates the signal.
        try
        {
            mInterpolator.set(*reply,
                              t0QueryMuSec,
                              t1QueryMuSec);
        }
        catch (const std::exception &e)
        {
            logger->error("Instance " + std::to_string(mInstance)
                        +" Failed to set data responses for "
                        + mName + ".  Failed with: "
                        + std::string{e.what()});
            return;
        }
        // Get the valid interpolated times
        auto t0Interpolated = mInterpolator.getStartTime();
        auto t1Interpolated = mInterpolator.getEndTime();
        auto signalDuration = t1Interpolated - t0Interpolated;
        // Give up early b/c there isn't enough new data
        if (signalDuration < mDetectorWindowDuration){return;}
        // Do we have a minimum amount of data to perform inference?
        if (signalDuration < mDetectorWindowDuration)
        {
            return;
        }
        // Do we have a minimum amount of data to do any updates?
        auto endWindowDuration = mDetectorWindowDuration - mEndWindowTime;
        if (t1Interpolated < mLastProbabilityTime + endWindowDuration)
        {
            return;
        }
        // We now have enough signal to perform inference and will get new
        // probability samples after performing inference.
        mState = State::Inference;
    }
    std::pair<int, int> getStartEndProbabilitySignalIndices(const int nSamples)
    {
        // Get the start/time of the probability signal
        auto t0Signal = mInterpolator.getStartTime();
        auto t1Signal = mInterpolator.getEndTime();
        auto endWindowDuration = mDetectorWindowDuration - mEndWindowTime;
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
        i1 = std::min(nSamples, i1);
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
        return std::pair {i0, i1};
    }
    /// @brief Perform inference.
    void performPInference(
        URTS::Services::Scalable::Detectors::UNetOneComponentP::Requestor &pRequestor,
        std::shared_ptr<UMPS::Logging::ILog> &logger)
    {
        mInferencedP = false;
        mBroadcastP = false;
        if (mState != State::Inference){return;}
        // Ensure the times will work out
        auto t0Signal = mInterpolator.getStartTime();
        auto t1Signal = mInterpolator.getEndTime();
#ifndef NDEBUG
        assert(t1Signal - t0Signal >= mDetectorWindowDuration);
        auto endWindowDuration = mDetectorWindowDuration - mEndWindowTime;
        assert(t1Signal >= mLastProbabilityTime + endWindowDuration);
#endif
        // Perform the P and S inference
        std::unique_ptr<URTS::Services::Scalable::Detectors::
                        UNetOneComponentP::ProcessingResponse>
            pResponse{nullptr};
        int nSamplesOut = 0;
        try
        {
            pResponse = ::inference1C(mInterpolator, 
                                      pRequestor,
                                      mPInferenceRequest);
            mInferencedP = true;
            nSamplesOut = static_cast<int>
                          (pResponse->getProbabilitySignalReference().size());
        }
        catch (const std::exception &e)
        {
            logger->error("P inference request failed on instance "
                        + std::to_string(mInstance)
                        + ".  Failed with " + std::string {e.what()}); 
            pResponse = nullptr;
        } 

        if (nSamplesOut < 1)
        {
            mState = State::Query;
            throw std::runtime_error("Failed to get nSamplesOut on instance "
                                   + std::to_string(mInstance));
        }
        // Get start/end indices for P probability signal extraction
        auto [i0, i1] = getStartEndProbabilitySignalIndices(nSamplesOut);
        auto dtMuSec = std::round(1e6/mDetectorProbabilitySignalSamplingRate);
        auto idtMuSec
            = std::chrono::microseconds {static_cast<int64_t> (dtMuSec)};
        if (mInferencedP)
        {
            const auto &pRef = pResponse->getProbabilitySignalReference();
            auto pSignal = ::extractSignal(mChangesSamplingRate,
                                           i0, i1, pRef,
                                           mInterpolator,
                                           logger);
            if (!pSignal.empty())
            {
                mPProbabilityPacket.setStartTime(t0Signal + i0*idtMuSec);
                mPProbabilityPacket.setData(std::move(pSignal));
                mBroadcastP = true;
            }
        }
        // At this point we are `successful.'  Sure, the broadcast could fail,
        // or no data was extracted, but the machine must move forward and
        // iterate at our last valid probability time.
        mLastProbabilityTime = t0Signal
                             + i1*std::chrono::microseconds
                                  {static_cast<int64_t> (dtMuSec)};
        mState = State::Publish;
    }
    /// @brief Broadcasts packets.
    void broadcastP(
        URTS::Broadcasts::Internal::ProbabilityPacket::Publisher &publisher,
        std::shared_ptr<UMPS::Logging::ILog> &logger)
    {
        if (mState != State::Publish){return;}
        //std::cout << std::setprecision(16) << mName << " " << getNow().count()*1.e-6 << " " << mPProbabilityPacket.getStartTime().count()*1.e-6 << " " << mPProbabilityPacket.getEndTime().count()*1.e-6 << " " << mLastProbabilityTime.count()*1.e-6 << std::endl; 
        if (mBroadcastP)
        {
            ::broadcast(mInstance, mName, mPProbabilityPacket,
                        publisher, logger);
        }
#ifndef NDEBUG
        auto endTime = ::getNow();
        auto pipelineDuration
            = std::chrono::duration_cast<std::chrono::microseconds>
              (endTime - mPipelineStartTime);
        mPipelineDuration
            = static_cast<double> (pipelineDuration.count())*1.e-6;
        logger->debug("Pipeline duration for " + mName + " was "
                    + std::to_string(mPipelineDuration) + " (s)");
#endif
        mState = State::Query;
    }
    [[nodiscard]] size_t getHash() const noexcept
    {
        return mHash;
    }
//private:
    /// Channel data
    URTS::Database::AQMS::ChannelData mChannelData;
    /// Name of single-channel data
    std::string mName;
    /// Utility for interpolating 1C waveforms
    URTS::Services::Scalable::PacketCache::SingleComponentWaveform mInterpolator;
    /// Data requests for this SNCL which will be updated with query times
    URTS::Services::Scalable::PacketCache::DataRequest mVerticalRequest;
    // Partially populated requests for the inference services
    URTS::Services::Scalable::Detectors::UNetOneComponentP::ProcessingRequest
        mPInferenceRequest;
    // Partially defined data packet
    URTS::Broadcasts::Internal::ProbabilityPacket::ProbabilityPacket
        mPProbabilityPacket;
    URTS::Broadcasts::Internal::ProbabilityPacket::ProbabilityPacket
        mSProbabilityPacket;
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
    // Hash to identify item
    size_t mHash{0};
    // Detector sampling rate output signal
    double mDetectorProbabilitySignalSamplingRate{100};
    // Detector center window start index
    int mCenterWindowStart{254};
    // Detector center window end index
    int mCenterWindowEnd{754};
    // Instance
    int mInstance{0};
    // Defines the processing state of the packet.
    State mState{State::Unknown};
    bool mInferencedP{false};
    bool mInferencedS{false};
    bool mBroadcastP{false};
    bool mBroadcastS{false};
    bool mChangesSamplingRate{false};
};

class OneComponentProcessingPipeline
{
public:
    OneComponentProcessingPipeline(
        const int instance,
        const ::ProgramOptions &programOptions,
        const std::vector<URTS::Database::AQMS::ChannelData> &oneComponentSensors,
        std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mProgramOptions(programOptions),
        mContext(std::make_shared<UMPS::Messaging::Context> (1)),
        mLogger(logger),
        mInstance(instance)
    {
        /// Create the communication utilities
        mLogger->debug("Instance " + std::to_string(mInstance)
                     + " creating packet cache requestor...");
        mPacketCacheRequestor
            = std::make_unique<URTS::Services::Scalable::
                               PacketCache::Requestor> (mContext, mLogger);
        mPacketCacheRequestor->initialize(
            mProgramOptions.mPacketCacheRequestorOptions);

        mLogger->debug("Instance " + std::to_string(mInstance)
                     + " creating probability publisher...");
        mProbabilityPublisher
            = std::make_unique<URTS::Broadcasts::Internal::ProbabilityPacket::
                               Publisher> (mContext, mLogger);
        mProbabilityPublisher->initialize(
            mProgramOptions.mProbabilityPacketPublisherOptions);

        if (mProgramOptions.mRunP1CDetector)
        {
            mLogger->debug("Instance " + std::to_string(mInstance)
                         + " creating 1C P detector service requestor...");
            m1CPInferenceRequestor
                = std::make_unique<URTS::Services::Scalable::Detectors::UNetOneComponentP::Requestor>
                  (mContext, mLogger);
        }

        P1CDetectorProperties p1CProperties;

        for (const auto &oneComponentSensor : oneComponentSensors)
        {
            ::OneComponentProcessingItem
                item(oneComponentSensor,
                     p1CProperties.mDetectorWindowDuration,
                     mProgramOptions.mMaximumSignalLatency,
                     mProgramOptions.mGapTolerance,
                     mProgramOptions.mDataQueryWaitPercentage,
                     p1CProperties.mWindowStart,
                     p1CProperties.mWindowEnd,
                     p1CProperties.mSamplingRate);
            mPItems.insert(std::pair{item.getHash(), item});
        }
        mInitialized = true;
    }
 
    /// @brief Toggles this as running or not running.
    void setRunning(const bool running)
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        mKeepRunning = running;
    }
    /// @result True indicates the class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept
    {
        std::scoped_lock lock(mMutex);
        return mInitialized;
    }
    /// @result True indicates this should keep running
    [[nodiscard]] bool keepRunning() const
    {
        std::scoped_lock lock(mMutex);
        return mKeepRunning;
    }
    /// @result True indicates this is running.
    [[nodiscard]] bool isRunning() const noexcept
    {
        return keepRunning();
    }
    /// @brief Driver routine that processes data.
    void run()
    {
        while (keepRunning())
        {
            // Loop through the P processing items
            for (auto &item : mPItems)
            {
                // Query data
                try
                {
                    item.second.queryPacketCache(*mPacketCacheRequestor,
                                                 mLogger);
                }
                catch (const std::exception &e)
                {
                    mLogger->error(e.what());
                    item.second.mState
                       = ::OneComponentProcessingItem::State::Query;
                    continue;
                }
                // Perform inference
                try
                {
                     item.second.performPInference(*m1CPInferenceRequestor,
                                                    mLogger);
                }
                catch (const std::exception &e)
                {
                    mLogger->error(e.what());
                    item.second.mState
                       = ::OneComponentProcessingItem::State::Query;
                    continue;
                }
                // Broadcast
                try
                {
                    item.second.broadcastP(*mProbabilityPublisher,
                                           mLogger);
                }
                catch (const std::exception &e)
                {
                    mLogger->error(e.what());
                    item.second.mState
                       = ::OneComponentProcessingItem::State::Query;
                    continue;
                }
            }
        }
    }
//private:
    mutable std::mutex mMutex;
    // Program options
    ::ProgramOptions mProgramOptions;
    // Communication utilities
    std::shared_ptr<UMPS::Messaging::Context> mContext{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<URTS::Broadcasts::Internal::ProbabilityPacket::Publisher>
         mProbabilityPublisher{nullptr};
    std::unique_ptr<URTS::Services::Scalable::PacketCache::Requestor>
         mPacketCacheRequestor{nullptr};
    std::unique_ptr<URTS::Services::Scalable::Detectors::
                          UNetOneComponentP::Requestor>
         m1CPInferenceRequestor{nullptr};
    std::map<size_t, ::OneComponentProcessingItem> mPItems;
    int mInstance{0};
    bool mInitialized{false};
    bool mKeepRunning{true}; 
};
}
#endif
