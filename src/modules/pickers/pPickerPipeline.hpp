#ifndef URTS_MODULES_PICKERS_P_PICKER_PIPELINE_HPP
#define URTS_MODULES_PICKERS_P_PICKER_PIPELINE_HPP
#include <vector>
#include <atomic>
#include <cmath>
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
#include "private/threadSafeQueue.hpp"
#include "pickToRefine.hpp"

namespace
{

[[nodiscard]] [[maybe_unused]]
std::string toName(const std::string &network,
                   const std::string &station,
                   const std::string &channel,
                   const std::string &locationCode)
{
    auto name = network + "." + station + "."
              + channel + "." + locationCode;
    return name;
}

[[nodiscard]] [[maybe_unused]]
std::vector<double>
centerAndCut(
    const URTS::Services::Scalable::PacketCache::SingleComponentWaveform
       &waveform,
    const std::chrono::microseconds &centerTime,
    const std::chrono::microseconds &windowPreTime,
    const std::chrono::microseconds &windowPostTime)
{
    auto nSamples = waveform.getNumberOfSamples();
    auto startTime = waveform.getStartTime();
    auto endTime = waveform.getEndTime();
    auto iSamplingRateMuS = waveform.getNominalSamplingPeriod();
    auto samplingRateMuS = static_cast<double> (iSamplingRateMuS.count());
    auto iSamplingRateMuS2 = iSamplingRateMuS/2;
    if (centerTime < startTime || centerTime > endTime)
    {
        throw std::runtime_error("Center time not in start/end time");
    }
    auto centerTimeOffset = centerTime - startTime;
    if (centerTime + windowPreTime < startTime)
    {
        throw std::runtime_error("Not enough lead time in signal");
    }
    if (centerTime + windowPostTime > endTime)
    {
        throw std::runtime_error("Not enough post time in signal");
    }
    auto centerIndex
        = static_cast<int> (std::round(centerTimeOffset.count()
                                      /samplingRateMuS));
    auto nSamplesBefore
        = static_cast<int> (std::round(windowPreTime.count()/samplingRateMuS));
    std::chrono::microseconds windowDuration = windowPostTime - windowPreTime;
    auto nSamplesInWindow
        = static_cast<int> (std::round(windowDuration.count()/samplingRateMuS))
        + 1;
    auto i0 = centerIndex + nSamplesBefore;
    auto i1 = i0 + nSamplesInWindow;
    if (i0 < 0)
    {
        throw std::runtime_error("Cannot start window cut before 0");
    }
    auto anticipatedDuration = (i1 - i0)*iSamplingRateMuS;
    if (anticipatedDuration <= windowDuration + iSamplingRateMuS2)
    {
        for (int i = 0; i < nSamples; ++i)
        {
            i1 = i1 + 1;
            anticipatedDuration = (i1 - i0)*iSamplingRateMuS;
            if (anticipatedDuration > windowDuration + iSamplingRateMuS2)
            {
                break;
            }
        }
    }
    anticipatedDuration = (i1 - i0)*iSamplingRateMuS;
    if (anticipatedDuration <= windowDuration + iSamplingRateMuS2)
    {
        throw std::runtime_error("Could not extend window");
    } 
    if (i1 > nSamples)
    {
        throw std::runtime_error("Cannot end window cut after signal length");
    }
    // Finally cut the signal
    auto signalReference = waveform.getSignalReference();  
    std::vector<double> result(i1 - i0, 0);
    std::copy(signalReference.data() + i0, signalReference.data() + i1,
              result.data());
    //std::cout << "nsamples p: " << i1 - i0 << " " << result.size() << std::endl;
    return result;
}

struct P1CPickerProperties
{
/*
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
*/
#ifndef NDEBUG
    P1CPickerProperties()
    {
        assert(mPostWindow - mPreWindow == mWindowDuration);
    }
#endif
    const std::string mAlgorithm{"CNNOneComponentPPicker"};
    const std::chrono::microseconds mPreWindow{-2000000};
    const std::chrono::microseconds mPostWindow{1990000};
    const std::chrono::microseconds mWindowDuration{3990000};
    const std::chrono::microseconds mMinimumPerturbation{-850000};
    const std::chrono::microseconds mMaximumPerturbation{+850000};
    double mSamplingRate{UUSSMLModels::Pickers::CNNOneComponentP::
                         Inference::getSamplingRate()};
    int mExpectedSignalLength{UUSSMLModels::Pickers::CNNOneComponentP::
                              Inference::getExpectedSignalLength()};
};

struct P1CFirstMotionProperties
{
#ifndef NDEBUG
    P1CFirstMotionProperties()
    {
        assert(mPostWindow - mPreWindow == mWindowDuration);
    }
#endif
    const std::string mAlgorithm{"CNNOnecomponentPFirstMotionClassifier"};
    const std::chrono::microseconds mPreWindow{-2000000};
    const std::chrono::microseconds mPostWindow{1990000};
    const std::chrono::microseconds mWindowDuration{3990000};
    const std::chrono::microseconds mMinimumPerturbation{-500000}; 
    const std::chrono::microseconds mMaximumPerturbation{+500000}; 
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
    OneComponentProcessingItem(
        const URTS::Database::AQMS::ChannelData &verticalChannelData,
        const ::ProgramOptions &programOptions,
        const int instance) :
        mVerticalChannelData(verticalChannelData),
        mInstance(instance)
    {
        // Figure out which utilities we are running
        mRunPPicker = programOptions.mRunPPicker;
        mRunFirstMotionClassifier = programOptions.mRunFirstMotionClassifier;
 
        P1CPickerProperties pickerProperties;
        P1CFirstMotionProperties fmProperties;
        mFirstMotionClassifierHalfWindowDuration
            = fmProperties.mWindowDuration/2;
 
        // Figure out +/- shift tolerance
        auto maxShift
            = std::max(std::abs(pickerProperties.mMinimumPerturbation.count()),
                       std::abs(pickerProperties.mMaximumPerturbation.count()));
        if (!mRunPPicker && mRunFirstMotionClassifier)
        {
            maxShift
                = std::max(std::abs(fmProperties.mMinimumPerturbation.count()),
                           std::abs(fmProperties.mMaximumPerturbation.count()));
        }
        mTolerance = std::chrono::microseconds {maxShift};

        auto network = mVerticalChannelData.getNetwork();
        auto station = mVerticalChannelData.getStation();
        auto channel = mVerticalChannelData.getChannel();
        auto locationCode = mVerticalChannelData.getLocationCode();

        mName = network + "." + station + "." + channel + "." + locationCode;

        // Target sampling rate
        auto samplingRate = verticalChannelData.getSamplingRate();
        mInterpolator.setNominalSamplingRate(samplingRate);
        auto gapTolerance = std::chrono::microseconds
        {
            static_cast<int64_t> (std::round(
                std::max(0, programOptions.mGapTolerance)/samplingRate*1.e6))
        };
        mInterpolator.setGapTolerance(gapTolerance);

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
        mPick.setChannel(channel); // Picking happens on untransformed channel

        mPick.setLocationCode(locationCode);
        mPick.setPhaseHint("P");
        mPick.setFirstMotion(URTS::Broadcasts::Internal::Pick::Pick::
                             FirstMotion::Unknown);
        mPick.setReviewStatus(URTS::Broadcasts::Internal::Pick::Pick::
                              ReviewStatus::Automatic);
        URTS::Broadcasts::Internal::Pick::UncertaintyBound lowerBound;
        URTS::Broadcasts::Internal::Pick::UncertaintyBound upperBound;
        lowerBound.setPercentile(15.9);
        upperBound.setPercentile(84.1);
        lowerBound.setPerturbation(std::chrono::microseconds{-25000});
        upperBound.setPerturbation(std::chrono::microseconds{ 25000});
        mPick.setLowerAndUpperUncertaintyBound(
           std::pair{lowerBound, upperBound});

        // Flip first motion b/c of sensor?
        mFirstMotionMultiplier = 1;
        if (verticalChannelData.haveDip())
        {
            // SEED convention -90 is + up and 90 is + down.
            // So if the dip is is +90 then there's a reversal.
            if (std::abs(verticalChannelData.getDip() - 90) < 1.e-4)
            {
                mFirstMotionMultiplier =-1;
            }
        }
    }
    /// @brief Convenience funtion to process a pick.
    [[nodiscard]]
    URTS::Broadcasts::Internal::Pick::Pick refinePick(
        const URTS::Broadcasts::Internal::Pick::Pick &initialPick,
        URTS::Services::Scalable::PacketCache::Requestor
              &packetCacheRequestor,
        URTS::Services::Scalable::Pickers::CNNOneComponentP::
              Requestor &pickRequestor,
        URTS::Services::Scalable::FirstMotionClassifiers::
              CNNOneComponentP::Requestor &fmRequestor,
        std::shared_ptr<UMPS::Logging::ILog> &logger)
    {
        // Do things match up?
        if (mPick.getNetwork() != initialPick.getNetwork())
        {
            throw std::runtime_error("Inconsistent networks");
        }
        if (mPick.getStation() != initialPick.getStation())
        {
            throw std::runtime_error("Inconsistent stations");
        }
        if (mPick.getChannel().at(0) != initialPick.getChannel().at(0) ||
            mPick.getChannel().at(1) != initialPick.getChannel().at(1))
        {
            throw std::runtime_error("Inconsistent channels");
        }
        if (mPick.getLocationCode() != initialPick.getLocationCode())
        {
            throw std::runtime_error("Inconsistent location code");
        }
        // Initialize pick
        auto result = mPick;
        result.setTime(initialPick.getTime());
        result.setIdentifier(initialPick.getIdentifier()); 
        result.setPhaseHint(initialPick.getPhaseHint());
        result.setReviewStatus(initialPick.getReviewStatus());
        result.setProcessingAlgorithms(initialPick.getProcessingAlgorithms());
        if (initialPick.haveLowerAndUpperUncertaintyBound())
        {
            result.setLowerAndUpperUncertaintyBound(
                initialPick.getLowerAndUpperUncertaintyBound());
        }
        // Query data
/*
        try
        {
*/
            queryPacketCache(result.getTime(), packetCacheRequestor);
/*
        }
        catch (const std::exception &e)
        {
            logger->error(e.what());
            result.setChannel(initialPick.getChannel());
            result.setOriginalChannels(initialPick.getOriginalChannels());
            return result; // Really just giving back what was sent
        }
*/
        // Perform inference and update pick
        try
        {
            performInference(&result, pickRequestor, fmRequestor, logger);
        }
        catch (const std::exception &e)
        {
            logger->error(e.what());
            result.setChannel(initialPick.getChannel());
            result.setOriginalChannels(initialPick.getOriginalChannels());
            return result;
        }
        //std::cout << "updated pick p : " << initialPick << " " << result << std::endl;
        return result;
    }
    /// @brief Query the packet cache.
    void queryPacketCache(
        const std::chrono::microseconds &pickTime,
        URTS::Services::Scalable::PacketCache::Requestor &packetCacheRequestor)
    {
        P1CPickerProperties pickerProperties;
        P1CFirstMotionProperties fmProperties;
        auto timeBefore =-pickerProperties.mPreWindow;
        if (fmProperties.mPreWindow < pickerProperties.mPreWindow)
        {
            timeBefore =-fmProperties.mPreWindow;
        }
        auto timeAfter = pickerProperties.mPostWindow;
        if (fmProperties.mPostWindow > pickerProperties.mPostWindow)
        {
            timeAfter = fmProperties.mPostWindow;
        }

        // Update my request identifier (and avoid overflow in a billion years)
        if (mRequestIdentifier > std::numeric_limits<int64_t>::max() - 10)
        {
            mRequestIdentifier = 0;
        }
        auto t0QueryMuSec = pickTime - timeBefore - (mPadQuery + mTolerance);
        auto t1QueryMuSec = pickTime + timeAfter  + (mPadQuery + mTolerance);
        auto t0Query = static_cast<double> (t0QueryMuSec.count())*1.e-6;
        auto t1Query = static_cast<double> (t1QueryMuSec.count())*1.e-6;
        try
        {
            std::pair<double, double> queryTimes{t0Query, t1Query};
            mVerticalRequest.setQueryTimes(queryTimes); 
            mVerticalRequest.setIdentifier(mRequestIdentifier);
        }
        catch (const std::exception &e)
        {
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + " failed to create data request: " 
                              + std::string{e.what()};
            throw std::runtime_error(errorMessage);
        }

        auto reply = packetCacheRequestor.request(mVerticalRequest);
        if (reply == nullptr)
        {
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + "'s packet cache request for " + mName
                              + " may have timed out";
            throw std::runtime_error(errorMessage);
        }
        try
        {
            // This will center the pick around t0Query, t1Query
            mInterpolator.set(*reply, t0QueryMuSec, t1QueryMuSec); 
        }
        catch (const std::exception &e)
        {
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + " failed to set data responses for "
                              + mName + ".  Failed with: "
                              + std::string{e.what()};
            throw std::runtime_error(errorMessage);
        }
        // Really not set up to deal with gaps
        if (mInterpolator.haveGaps())
        {
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + ": " + mName + " has gaps.  Skipping...";
            throw std::runtime_error(errorMessage);
        }
        // Get the valid interpolated times
        auto t0Interpolated = mInterpolator.getStartTime();
        auto t1Interpolated = mInterpolator.getEndTime();
        // The initial pick needs to be within some tolerance of the window
        // center.  The tolerance is defined by the perturbations in the 
        // training.
        if ((pickTime - mTolerance) - t0Interpolated < timeBefore)
        {
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + ": P pick too close to start of window";
            throw std::runtime_error(errorMessage);
        }
        if (t1Interpolated - (pickTime + mTolerance) < timeAfter)
        {
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + ": P pick too close to end of window";
            throw std::runtime_error(errorMessage);
        }
    }
    /// @brief Perform the pick regression and first motion classification
    ///        inference.
    void performInference(
        URTS::Broadcasts::Internal::Pick::Pick *pick,
        URTS::Services::Scalable::Pickers::CNNOneComponentP::
              Requestor &pickRequestor,
        URTS::Services::Scalable::FirstMotionClassifiers::
              CNNOneComponentP::Requestor &fmRequestor,
        std::shared_ptr<UMPS::Logging::ILog> &logger)
    {
        auto algorithms = pick->getProcessingAlgorithms();
        // Refine the P pick
        if (mRunPPicker)
        {
            bool computePick = false;
            try
            {
                auto verticalSignal
                     = ::centerAndCut(mInterpolator,
                                      pick->getTime(),
                                      std::chrono::microseconds {-2000000},
                                      std::chrono::microseconds {+1990000});
                mPickRequest.setVerticalSignal(
                    std::move(verticalSignal));
                mPickRequest.setIdentifier(mRequestIdentifier);
                computePick = true;
            }
            catch (const std::exception &e)
            {
                logger->error("Instance " + std::to_string(mInstance)
                            + " P picker signal cut failed with: " + e.what());
            }
            if (computePick)
            {
                auto pickResponse = pickRequestor.request(mPickRequest);
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
                        auto pickCorrection
                            = std::chrono::microseconds {
                                static_cast<int64_t> (
                                std::round(pickResponse->getCorrection()*1.e6))
                              };
                        pick->setTime(pick->getTime() + pickCorrection);
                        algorithms.push_back("CNNOneComponentPPicker");
                        // TODO uncertainties
                        pick->setLowerAndUpperUncertaintyBound(
                             mPick.getLowerAndUpperUncertaintyBound());
                    }
                    else
                    {
                        logger->warn("P pick request failed for " + mName
                                   + " on instance " + std::to_string(mInstance)
                                   + ".  Failed with error code "
                                   + std::to_string(static_cast<int>
                                                       (returnCode)));
                    }
                }
            }
        }
        // Set the the pick time
        int firstMotion{0};
        if (mRunFirstMotionClassifier)
        {
            bool computeFirstMotion = false;
            try
            {
                auto verticalSignal
                     = ::centerAndCut(mInterpolator,
                                      pick->getTime(),
                                      std::chrono::microseconds {-2000000},
                                      std::chrono::microseconds {+1990000});
                mFirstMotionRequest.setVerticalSignal(
                    std::move(verticalSignal));
                mFirstMotionRequest.setIdentifier(mRequestIdentifier);
                computeFirstMotion = true;
            }
            catch (const std::exception &e) 
            {
                logger->error("Instance " + std::to_string(mInstance)
                            + " first motion signal cut failed with: "
                            + e.what());
            }
            if (computeFirstMotion)
            {
                auto fmResponse = fmRequestor.request(mFirstMotionRequest);
                if (fmResponse == nullptr)
                {
                    logger->error("First motion request for  " + mName
                               + " timed out");
                }
                else
                {
                    auto returnCode = fmResponse->getReturnCode();
                    if (returnCode ==
                        URTS::Services::Scalable::FirstMotionClassifiers::
                        CNNOneComponentP::ProcessingResponse::
                        ReturnCode::Success)
                    {
                        firstMotion = 
                           static_cast<int> (fmResponse->getFirstMotion());
                        // Handle flipped sensors
                        firstMotion = mFirstMotionMultiplier*firstMotion;
                        pick->setFirstMotion(
                           static_cast<URTS::Broadcasts::Internal::Pick
                                          ::Pick::FirstMotion> (firstMotion));
                        algorithms.push_back(
                           "CNNOneComponentPFirstMotionClassifier");
                    }
                    else
                    {
                        logger->warn("First motion request failed for " + mName
                                   + " on instance " + std::to_string(mInstance)
                                   + ".  Failed with error code "
                                   + std::to_string(static_cast<int>
                                                       (returnCode)));
                    }
                }
            }
        }
        pick->setProcessingAlgorithms(algorithms);
    }
    URTS::Services::Scalable::PacketCache::SingleComponentWaveform
        mInterpolator;
    URTS::Database::AQMS::ChannelData mVerticalChannelData;
    URTS::Services::Scalable::PacketCache::DataRequest mVerticalRequest;
    URTS::Services::Scalable::Pickers::CNNOneComponentP::ProcessingRequest
        mPickRequest;
    URTS::Services::Scalable::FirstMotionClassifiers::CNNOneComponentP::
          ProcessingRequest mFirstMotionRequest;
    URTS::Broadcasts::Internal::Pick::Pick mPick;
    std::string mName;
    std::chrono::microseconds mFirstMotionClassifierHalfWindowDuration;
    std::chrono::microseconds mTolerance{850000};   // +/- 0.85 s
    std::chrono::microseconds mPadQuery{500000}; // Pre/post pad query 0.5s
    int64_t mRequestIdentifier{0};
    int mInstance{0};
    int mFirstMotionMultiplier{1}; // Handle polarity flips
    bool mRunPPicker{true};
    bool mRunFirstMotionClassifier{true};
};

///--------------------------------------------------------------------------///
///                                 Refines Picks                            ///
///--------------------------------------------------------------------------///
class PPickerPipeline
{
public:
    PPickerPipeline(const int instance,
                    const ::ProgramOptions &programOptions,
                    std::shared_ptr<URTS::Database::AQMS::
                                    ChannelDataTablePoller> &databasePoller,
                    std::shared_ptr<::ThreadSafeQueue<::PickToRefine> >
                           &pickProcessorQueue,
                    std::shared_ptr<
                         ::ThreadSafeQueue<URTS::Broadcasts::
                                           Internal::Pick::Pick>>
                           &pickPublisherQueue,
                    std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mProgramOptions(programOptions),
        mContext(std::make_shared<UMPS::Messaging::Context> (1)),
        mDatabasePoller(databasePoller),
        mPickProcessorQueue(pickProcessorQueue),
        mPickPublisherQueue(pickPublisherQueue),
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

        if (mProgramOptions.mRunPPicker)
        {
            mLogger->debug("Instance " + std::to_string(mInstance)
                         + " creating P pick regressor requestor...");
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
    /// @brief Destructor
    ~PPickerPipeline()
    {
        stop();
    }
    /// @brief Creates a processing item if it does not exists
    void createProcessingItem(const std::string &network,
                              const std::string &station,
                              const std::string &channel,
                              const std::string &locationCode)
    {
        // Set name and check if it already exists
        auto name = ::toName(network, station, channel, locationCode);
        if (mProcessingItems.contains(name)){return;}
        // Okay, let's add it
        mLogger->info("Instance " + std::to_string(mInstance)
                    + " adding " + name);
        auto channelDataVector = mDatabasePoller->getChannelData(network,
                                                                 station,
                                                                 channel,
                                                                 locationCode);
        if (channelDataVector.empty())
        {
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + " cannot find channel data for " + name;
            throw std::runtime_error(errorMessage);
        }
        // In case we somehow get multiple matches for the channel data
        int channelIndex = 0;
        if (channelDataVector.size() > 1)
        {
            // Take most recent channel
            mLogger->warn("Instance "
                        + std::to_string(mInstance)
                        + " using most recent channel information for "
                        + name);
            auto maxOnDate = channelDataVector[0].getOnDate() ;
            for (int i = 1; i < static_cast<int> (channelDataVector.size());
                 ++i)
            {
                if (channelDataVector[i].getOnDate() > maxOnDate)
                {   
                    maxOnDate = channelDataVector[i].getOnDate();
                    channelIndex = i;
                }
            }
        }
        // Is this channel still active?
        auto endTime = channelDataVector.at(channelIndex).getOffDate();
        auto now
            = std::chrono::duration_cast<std::chrono::microseconds> (
                std::chrono::system_clock::now().time_since_epoch());
        if (endTime < now)
        {  
            throw std::runtime_error("Instance " + std::to_string(mInstance)
                                  + name + " is closed in database");
        }
        mProcessingItems.insert(
           std::pair {name,
                      ::OneComponentProcessingItem(
                          channelDataVector[channelIndex],
                          mProgramOptions,
                          mInstance)});
    }
    /// @result Sets the class as running or not running.
    void setRunning(const bool running) noexcept
    {
        mKeepRunning = running;
    }
    /// @result True indicates the process should keep working.
    [[nodiscard]] bool keepRunning() const noexcept
    {
        return mKeepRunning;
    }
    /// @brief Reads picks from the queue, refines them, then pushes them
    ///        onto the publisher queue.
    void run()
    {
        std::chrono::milliseconds timeOut{10};
        ::PickToRefine initialPick;
        while (keepRunning())
        {
            bool isRetry{false};
            auto gotPick = mPickProcessorQueue->wait_until_and_pop(
                                &initialPick, timeOut);
            // If I didn't get a pick try to go through my internal queue
            if (!gotPick && !mReprocessingQueue.empty())
            {
                // This is sorted in descneding order so the last element
                // should be closest to now.
                if (mReprocessingQueue.back().isReadyToProcess())
                {
                    mLogger->info("Will reprocess old P pick");
                    initialPick = mReprocessingQueue.back();
                    mReprocessingQueue.pop_back();
                    gotPick = true;
                    isRetry = true;
                }
            }
            if (gotPick)
            {
                try
                {
                    // Figure out a name  
                    auto originalChannels = initialPick.pick.getOriginalChannels();
                    std::string channel;
                    for (const auto &originalChannel : originalChannels)
                    {
                        if (originalChannel.back() == 'Z')
                        {
                            channel = originalChannel;
                            break;
                        }
                    }
                    if (channel.empty())
                    {
                        channel = initialPick.pick.getChannel();
                        if (channel.back() == 'P')
                        {
                            if (channel.size() != 3)
                            {
                                mLogger->error("Unhandled channel: " + channel);
                                continue;
                            }
                            channel[2] = 'Z';
                        }
                        else
                        {
                            mLogger->error("Unhandled channel: " + channel);
                            continue;
                        }
                    }
                    if (channel.empty())
                    {
                        mLogger->error("Could not divine channel");
                        continue;
                    }
                    auto network = initialPick.pick.getNetwork();
                    auto station = initialPick.pick.getStation();
                    auto locationCode = initialPick.pick.getLocationCode();
                    // If the processing item doesn't exist then make it
                    createProcessingItem(network,
                                         station,
                                         channel,
                                         locationCode);
                    // Process this pick
                    auto name = ::toName(network, station,
                                         channel, locationCode);
                    auto it = mProcessingItems.find(name);
#ifndef NDEBUG
                    assert(it != mProcessingItems.end());
#endif
                    auto refinedPick
                        = it->second.refinePick(initialPick.pick,
                                                *mPacketCacheRequestor,
                                                *mPickerRequestor,
                                                *mFirstMotionRequestor,
                                                mLogger);
                    mPickPublisherQueue->push(refinedPick); 
                    if (isRetry)
                    {   
                        mLogger->info("Successfully reprocessed P pick");
                    }
                }
                catch (const std::exception &e)
                {
                    if (initialPick.mTries == 0)
                    {
                        mLogger->warn(std::string {e.what()}
                          + "; will add P pick to reprocessing list");
                    }
                    else
                    {
                        mLogger->warn(std::string {e.what()}
                          + "; re-inserting into P reprocessing list");
                    }
                    try
                    {
                        initialPick.setNextTry();
                        mReprocessingQueue.push_back(std::move(initialPick));
                        if (mReprocessingQueue.size() >
                            mMaximumReprocessingQueueSize)
                        {
                            mLogger->warn(
                                "Reprocessing P queue is full; sending last one"); 
                            try
                            {
                                auto pickToPublish = mReprocessingQueue.back().pick;
                                mReprocessingQueue.pop_back();
                                mPickPublisherQueue->push(std::move(pickToPublish));
                            }
                            catch (const std::exception &e)
                            {
                                mLogger->error("P reprocessing pop queue failed with: "
                                             + std::string {e.what()});
                            }
                        }
                        // Sort is descending order so we can use vector
                        // intrinsics to remove last element
                        std::sort(mReprocessingQueue.begin(),
                                  mReprocessingQueue.end(),
                                  [](const auto &lhs, const auto &rhs)
                                  {
                                     return lhs.nextTry > rhs.nextTry;
                                  });
                    }
                    catch (const std::exception &e)
                    {
                        mLogger->error(e.what());
                        mPickPublisherQueue->push(initialPick.pick);
                    }
                }
            } // End check on getting a pick
        }
    }
    void start()
    {
        stop();
        setRunning(true);
        mThread = std::thread(&::PPickerPipeline::run, this);
    } 
    void stop()
    {
        setRunning(false);
        if (mThread.joinable()){mThread.join();}
    }
///private:
    std::thread mThread;
    ::ProgramOptions mProgramOptions; 
    std::shared_ptr<UMPS::Messaging::Context> mContext{nullptr};
    std::shared_ptr<URTS::Database::AQMS::ChannelDataTablePoller>
        mDatabasePoller{nullptr};
    std::vector<::PickToRefine> mReprocessingQueue;
    std::shared_ptr<::ThreadSafeQueue<::PickToRefine>>
        mPickProcessorQueue{nullptr};
    std::shared_ptr<::ThreadSafeQueue<URTS::Broadcasts::Internal::Pick::Pick>>
        mPickPublisherQueue{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<URTS::Services::Scalable::PacketCache::Requestor>
        mPacketCacheRequestor{nullptr};
    std::unique_ptr<URTS::Services::Scalable::
                    Pickers::CNNOneComponentP::Requestor>
        mPickerRequestor{nullptr};
    std::unique_ptr<URTS::Services::Scalable::
                    FirstMotionClassifiers::CNNOneComponentP::Requestor>
        mFirstMotionRequestor{nullptr};
    std::map<std::string, ::OneComponentProcessingItem> mProcessingItems;
    size_t mMaximumReprocessingQueueSize{128};
    int mInstance{0};
    bool mInputsEqual{true};
    std::atomic<bool> mKeepRunning{true};
};
}
#endif
