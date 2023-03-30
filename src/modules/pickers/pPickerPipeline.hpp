#ifndef URTS_MODULES_PICKERS_P_PICKER_PIPELINE_HPP
#define URTS_MODULES_PICKERS_P_PICKER_PIPELINE_HPP
#include <vector>
#include <string>
#include <uussmlmodels/pickers/cnnOneComponentP/inference.hpp>
#include <uussmlmodels/firstMotionClassifiers/cnnOneComponentP/inference.hpp>
#include <umps/logging/log.hpp>
#include <umps/messaging/context.hpp>
#include "urts/database/aqms/channelData.hpp"
#include "urts/broadcasts/internal/pick.hpp"
#include "urts/services/scalable/packetCache.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP.hpp"
#include "urts/services/standalone/incrementer.hpp"

namespace
{

struct P1CPickerProperties
{
    P1CPickerProperties()
    {
        mWindowDuration
            = std::chrono::microseconds {
                static_cast<int64_t> (std::round((mExpectedSignalLength - 1)
                                                 /mSamplingRate*1.e6))
              };
        auto [lag0, lag1]
            = UUSSMLModels::Pickers::CNNOneComponentP::Inference
                          ::getMinimumAndMaximumPerturbation();
        mMinimumPerturbation = std::chrono::microseconds {
                static_cast<int64_t> (std::round(lag0*1.e6))};
        mMaximumPerturbation = std::chrono::microseconds {
                static_cast<int64_t> (std::round(lag1*1.e6))};
#ifndef NDEBUG
        assert(mMaximumPerturbation > mMinimumPerturbation);
#endif
    }
    std::chrono::microseconds mWindowDuration{3990000};
    std::chrono::microseconds mMinimumPerturbation{-850000};
    std::chrono::microseconds mMaximumPerturbation{+850000};
    double mSamplingRate{UUSSMLModels::Pickers::CNNOneComponentP::
                         Inference::getSamplingRate()};
    int mExpectedSignalLength{UUSSMLModels::Pickers::CNNOneComponentP::
                              Inference::getExpectedSignalLength()};
};

struct P1CFirstMotionProperties
{
    P1CFirstMotionProperties()
    {
        mWindowDuration
            = std::chrono::microseconds {
                static_cast<int64_t> (std::round((mExpectedSignalLength - 1)
                                                /mSamplingRate*1.e6))
              }; 
    }
    std::chrono::microseconds mWindowDuration{3990000};
    double mSamplingRate{UUSSMLModels::FirstMotionClassifiers::
                         CNNOneComponentP::Inference::getSamplingRate()};
    int mExpectedSignalLength{UUSSMLModels::FirstMotionClassifiers::
                        CNNOneComponentP::Inference::getExpectedSignalLength()};
};

bool operator==(const P1CPickerProperties &lhs,
                const P1CFirstMotionProperties &rhs)
{
    if (lhs.mExpectedSignalLength != rhs.mExpectedSignalLength){return false;}
    if (lhs.mWindowDuration != rhs.mWindowDuration){return false;}
    if (std::abs(lhs.mSamplingRate - rhs.mSamplingRate) > 1.e-14){return false;}
    return true;
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
        const URTS::Database::AQMS::ChannelData &verticalChannelData) :
        mVerticalChannelData(verticalChannelData),
        mState(OneComponentProcessingItem::State::Unknown)
    {
        P1CFirstMotionProperties fmProperties;
        mFirstMotionClassifierHalfWindowDuration
            = fmProperties.mWindowDuration/2;

        auto network = mVerticalChannelData.getNetwork();
        auto station = mVerticalChannelData.getStation();
        auto channel = mVerticalChannelData.getChannel();
        auto locationCode = mVerticalChannelData.getLocationCode();

        mName = network + "." + station + "." + channel + "." + locationCode;
        mHash = std::hash<std::string>{} (mName);

        // Target sampling rate
        mInterpolator.setNominalSamplingRate(
            verticalChannelData.getSamplingRate());
//        mInterpolator.setGapTolerance();

        // Predefine some vertical data request information
        mVerticalRequest.setNetwork(network);
        mVerticalRequest.setStation(station);
        mVerticalRequest.setChannel(channel);
        mVerticalRequest.setLocationCode(locationCode);
        mVerticalRequest.setIdentifier(0);

        // Predefine some pick request infromation
        mPickRequest.setSamplingRate(verticalChannelData.getSamplingRate());
        mPickRequest.setIdentifier(0);

        // Predefine some pick information
        mPick.setNetwork(network);
        mPick.setStation(station);
        mPick.setChannel(channel);
        mPick.setLocationCode(locationCode);
        mPick.setPhaseHint("P");
        mPick.setFirstMotion(URTS::Broadcasts::Internal::Pick::Pick::
                             FirstMotion::Unknown);
        mPick.setReviewStatus(URTS::Broadcasts::Internal::Pick::Pick::
                              ReviewStatus::Automatic);
        mPick.setAlgorithm("CNNOneComponentP");

        mState = OneComponentProcessingItem::State::Query;
    }
/*
    /// @brief Initializes the pick given this crude pick estimate from the
    ///        detector.
    void initializePick(const std::chrono::microseconds &pickTime)
    {

    }
    /// @brief Query the packet cache.
    void queryPacketCache(const std::chrono::microseconds &pickTime)
    {

        // Update my request identifier (and avoid overflow in a billion years)
        if (mRequestIdentifier > std::numeric_limits<int64_t>::max() - 10)
        {
            mRequestIdentifier = 0;
        }
        auto t0QueryMuSec = pickTime - mTimeBefore - mPadQuery;
        auto t1QueryMuSec = pickTime + mTimeAfter  + mPadQuery;
        auto t0Query = static_cast<double> (t0QueryMuSec.count())*1.e-6;
        auto t1Qiery = static_cast<double> (t1QueryMuSec.count())*1.e-6;
        try
        {
            std::pair<double, double> queryTimes{t0Query, t1Query};
            mVerticalRequest.setQueryTimes(queryTimes); 
            mVerticalRequest.setIdentifier(mRequestIdentifier);
        }
        catch (const std::exception &e)
        {
            logger->error("Instance " + std::to_string(mInstance) 
                        + " failed to create data request: "
                        + std::string{e.what()});
            return;
        }

        std::unique_ptr<URTS::Services::Scalable::PacketCache::DataResponse> reply{nullptr};
        if (reply == nullptr)
        {
            logger->warn("Instance " + std::to_string(mInstance)
                        + "'s packet cache request for " + mName
                        + " may have timed out");
            return;
        }
        try
        {
            // This will center the pick around t0Query, t1Query
            mInterpolator.set(*reply, t0QueryMuSec, t1QueryMuSec); 
        }
        catch (const std::exception &e)
        {
            logger->error("Instance " + std::to_string(mInstance)
                        +" Failed to set data responses for "
                        + mName + ".  Failed with: "
                        + std::string{e.what()});
            return;
        }
        // Really not set up to deal with gaps
        if (mInterpolator.haveGaps())
        {
            logger->debug("Instance " + std::to_string(mInstance)
                        + ": " + mName + " has gaps.  Skipping...");
            return;
        }
        // Get the valid interpolated times
        auto t0Interpolated = mInterpolator.getStartTime();
        auto t1Interpolated = mInterpolator.getEndTime();
        // The initial pick needs to be within some tolerance of the window
        // center.  The tolerance is defined by the perturbations in the 
        // training.
        if ((pickTime - mTolerance) - t0Interpolated < mTimeBefore)
        {
            logger->error("Instance " + std::to_string(mInstance)
                        + ": Pick too close to start of window");
            return;
        }
        if (t1Interpolated - (pickTime + mTolerance) < mTimeAfter)
        {
            logger->error("Instance " + std::to_string(mInstance)
                        + ": Pick too close to end of window");
            return;
        }
    }
*/
    /// @brief Perform the pick regression and first motion classification
    ///        inference.
    void performInference(
        URTS::Services::Scalable::Pickers::CNNOneComponentP::
              Requestor &pickRequestor,
        URTS::Services::Scalable::FirstMotionClassifiers::
              CNNOneComponentP::Requestor &fmRequestor,
        std::shared_ptr<UMPS::Logging::ILog> &logger)
    {
        // Set the requests
        std::unique_ptr<URTS::Services::Scalable::Pickers::CNNOneComponentP::
                        ProcessingResponse> pickResponse{nullptr};
        std::chrono::microseconds pickCorrection{0};
        if (mRunPPicker)
        {
            mPickRequest.setVerticalSignal(mInterpolator.getSignalReference());
            pickResponse = pickRequestor.request(mPickRequest);
            if (pickResponse == nullptr)
            {
                logger->error("P pick request for  " + mName
                            + " timed out");
            }
            else
            {
                auto returnCode = pickResponse->getReturnCode();
                if (returnCode ==
                    URTS::Services::Scalable::Pickers::CNNOneComponentP::
                          ProcessingResponse::ReturnCode::Success)
                {
                    pickCorrection
                        = std::chrono::microseconds {
                            static_cast<int64_t> (
                               std::round(pickResponse->getCorrection()*1.e6))
                          };
                }
                else
                {
                    logger->warn("P pick request failed for " + mName
                               + " failed with error code"
                               + std::to_string(static_cast<int> (returnCode)));
                }
            }
        }
        // Set the the pick time
        // TODO uncertainties
        mPick.setTime(mPick.getTime() + pickCorrection);
        std::unique_ptr<URTS::Services::Scalable::FirstMotionClassifiers::
                        CNNOneComponentP::ProcessingResponse>
                       fmResponse{nullptr};
        int firstMotion{0};
        if (mRunFirstMotionClassifier)
        {
            // Attempt to center the pick for the first motion classifier
/*
            auto pickTime = mPick.getTime();
            const auto &signal = mInterpolator.getSignalReference();
            auto dt = 1./mInterpolator.getNominalSamplingRate();
            auto t0 = mInterpolator.getStartTime();
            auto t1 = mInterpolator.getEndTime();
            auto offset = pickTime - t0;
            auto newStartTime = pickTime
                              - mFirstMotionClassifierHalfWindowDuration; 
            auto newEndTime   = pickTime
                              + mFirstMotionClassifierHalfWindowDuration;
            bool setOriginalSignal{false};
            if (newStartTime > std::chrono::microseconds {0} &&
                newEndTime <= t1)
            {
                auto i0 = std::max(0, static_cast<int>
                                      (std::round(offset.count()*1.e-6)/dt));
                auto windowDuration
                    = 2*mFirstMotionClassifierHalfWindowDuration;
                auto nSamples = static_cast<int> (
                                std::round(windowDuration.count()*1.e-6/dt))
                              + 1;
                auto i1 = std::min(static_cast<int> (signal.size()),
                                   i0 + i1);
                try
                {
                    std::vector<double> subSignal(nCopy);
                    std::copy(signal.begin() + i0, signal.begin() + i1, 
            }
*/
            try 
            {
 //               mFirstMotionRequest.setVerticalSignal(signal);
                auto fmResponse = fmRequestor.request(mFirstMotionRequest);
                if (fmResponse == nullptr)
                {

                }
            }
            catch (const std::exception &e)
            {

            } 
        }
        mPick.setFirstMotion(static_cast<URTS::Broadcasts::Internal::Pick
                                         ::Pick::FirstMotion> (firstMotion));
    }
    /// @result The hash.
    [[nodiscard]] size_t getHash() const noexcept
    {
        return mHash;
    }
    URTS::Services::Scalable::PacketCache::SingleComponentWaveform
        mInterpolator;
/*
    std::chrono::microseconds mTimeBefore{2000000}; //+2 s
    std::chrono::microseconds mTimeAfter{2000000};  //-2 s
    //std::chrono::micrseoconds mMaxShift{
    std::chrono::microseconds mPadQuery{500000}; // Pre/post pad query 0.5s
    int64_t mRequestIdentifier{0};
    int mInstance{0};
    bool mRunPicker{true};
    bool mRunFirstMotion{true};
*/
    URTS::Database::AQMS::ChannelData mVerticalChannelData;
    URTS::Services::Scalable::PacketCache::DataRequest mVerticalRequest;
    URTS::Services::Scalable::Pickers::CNNOneComponentP::ProcessingRequest
        mPickRequest;
    URTS::Services::Scalable::FirstMotionClassifiers::CNNOneComponentP::
          ProcessingRequest mFirstMotionRequest;
    URTS::Broadcasts::Internal::Pick::Pick mPick;
    std::string mName;
    size_t mHash{std::hash<std::string>{} ("")};
    std::chrono::microseconds mFirstMotionClassifierHalfWindowDuration;
    State mState{State::Unknown};
    bool mRunPPicker{true};
    bool mRunFirstMotionClassifier{true};
};

class PPickerPipeline
{
public:
    PPickerPipeline(const int instance,
                    const ::ProgramOptions &programOptions,
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
                     + " creating pick publisher...");
        mPickPublisher
            = std::make_unique<URTS::Broadcasts::Internal::Pick::Publisher>
              (mContext, mLogger);
        mPickPublisher->initialize(mProgramOptions.mPickPublisherOptions);

        if (mProgramOptions.mRunPPicker)
        {
            mLogger->debug("Instance " + std::to_string(mInstance)
                         + " creating P picker requestor...");
            mPickerRequestor
                = std::make_unique<URTS::Services::Scalable::Pickers::
                                   CNNOneComponentP::Requestor>
                                  (mContext, mLogger);
            mPickerRequestor->initialize(
                mProgramOptions.mPPickerRequestorOptions);
        }

        if (mProgramOptions.mRunFirstMotionClassifier)
        {
            mLogger->debug("Instance " + std::to_string(mInstance)
                         + " creating P first motion classifier requestor...");
            mFirstMotionRequestor
                = std::make_unique<URTS::Services::Scalable::
                                   FirstMotionClassifiers::
                                   CNNOneComponentP::Requestor>
                                  (mContext, mLogger);
            mFirstMotionRequestor->initialize(
                mProgramOptions.mFirstMotionClassifierRequestorOptions);
        }

        P1CPickerProperties p1CPickerProperties;
        P1CFirstMotionProperties p1CFirstMotionProperties;
        mInputsEqual = true;
        if (mProgramOptions.mRunPPicker &&
            mProgramOptions.mRunFirstMotionClassifier)
        {
            P1CPickerProperties p1CPickerProperties;
            P1CFirstMotionProperties p1CFirstMotionProperties;
            mInputsEqual = (p1CPickerProperties == p1CFirstMotionProperties);
        }
#ifndef NDEBUG
        assert(mInputsEqual);
#endif

        if (mInputsEqual)
        {

        }
        else
        {
#ifndef NDEBUG
            assert(mInputsEqual);
#else
            throw std::runtime_error("Unequal inputs not programmed");
#endif
        }
    }
///private:
    ::ProgramOptions mProgramOptions; 
    std::shared_ptr<UMPS::Messaging::Context> mContext{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<URTS::Services::Scalable::PacketCache::Requestor>
        mPacketCacheRequestor{nullptr};
    std::unique_ptr<URTS::Services::Scalable::
                    Pickers::CNNOneComponentP::Requestor>
        mPickerRequestor{nullptr};
    std::unique_ptr<URTS::Services::Scalable::
                    FirstMotionClassifiers::CNNOneComponentP::Requestor>
        mFirstMotionRequestor{nullptr};
    std::unique_ptr<URTS::Services::Standalone::Incrementer::Requestor>
        mIncrementRequestor{nullptr};
    std::unique_ptr<URTS::Broadcasts::Internal::Pick::Publisher>
        mPickPublisher{nullptr};
    std::chrono::microseconds tBefore{2500000};
    std::chrono::microseconds tAfter{2500000};
    int mInstance{0};
    bool mInputsEqual{true};
};
}
#endif
