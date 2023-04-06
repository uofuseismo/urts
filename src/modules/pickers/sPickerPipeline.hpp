#ifndef URTS_MODULES_PICKERS_S_PICKER_PIPELINE_HPP
#define URTS_MODULES_PICKERS_S_PICKER_PIPELINE_HPP
#include <vector>
#include <array>
#include <atomic>
#include <cmath>
#include <string>
#include <uussmlmodels/pickers/cnnThreeComponentS/inference.hpp>
#include <umps/logging/log.hpp>
#include <umps/messaging/context.hpp>
#include "urts/database/aqms/channelData.hpp"
#include "urts/broadcasts/internal/pick.hpp"
#include "urts/services/scalable/packetCache.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS.hpp"
#include "urts/services/standalone/incrementer.hpp"
#include "private/threadSafeQueue.hpp"

namespace
{

[[nodiscard]] [[maybe_unused]]
std::string toName(const std::string &network,
                   const std::string &station,
                   const std::string &verticalChannel,
                   const std::string &northChannel,
                   const std::string &eastChannel,
                   const std::string &locationCode)
{
    auto channel = verticalChannel
                 + "_" + northChannel.back()
                 + "_" + eastChannel.back();
    auto name = network + "." + station + "."
              + channel + "." + locationCode;
    return name;
}

void centerAndCut(
    std::vector<double> *verticalSignal,
    std::vector<double> *northSignal,
    std::vector<double> *eastSignal,
    const URTS::Services::Scalable::PacketCache::ThreeComponentWaveform
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
    // Finally cut the signals
    auto signalReference = waveform.getVerticalSignalReference();  
    verticalSignal->resize(i1 - i0, 0);
    std::copy(signalReference.data() + i0, signalReference.data() + i1,
              verticalSignal->data());

    signalReference = waveform.getNorthSignalReference();
    northSignal->resize(i1 - i0, 0); 
    std::copy(signalReference.data() + i0, signalReference.data() + i1, 
              northSignal->data());

    signalReference = waveform.getEastSignalReference();
    eastSignal->resize(i1 - i0, 0);   
    std::copy(signalReference.data() + i0, signalReference.data() + i1,
              eastSignal->data());
}

struct S3CPickerProperties
{
#ifndef NDEBUG
    S3CPickerProperties()
    {
        assert(mPostWindow - mPreWindow == mWindowDuration);
    }
#endif
    const std::string mAlgorithm{"CNNThreeComponentSPicker"};
    const std::chrono::microseconds mPreWindow{-3000000};
    const std::chrono::microseconds mPostWindow{2990000};
    const std::chrono::microseconds mWindowDuration{5990000};
    const std::chrono::microseconds mMinimumPerturbation{-850000};
    const std::chrono::microseconds mMaximumPerturbation{+850000};
    double mSamplingRate{UUSSMLModels::Pickers::CNNThreeComponentS::
                         Inference::getSamplingRate()};
    int mExpectedSignalLength{UUSSMLModels::Pickers::CNNThreeComponentS::
                              Inference::getExpectedSignalLength()};
};

class ThreeComponentProcessingItem
{
public:
    ThreeComponentProcessingItem(
        const URTS::Database::AQMS::ChannelData &verticalChannelData,
        const URTS::Database::AQMS::ChannelData &northChannelData,
        const URTS::Database::AQMS::ChannelData &eastChannelData,
        const ::ProgramOptions &programOptions,
        const int instance) :
        mVerticalChannelData(verticalChannelData),
        mNorthChannelData(northChannelData),
        mEastChannelData(eastChannelData),
        mInstance(instance)
    {
        // Some checks
        auto network = mVerticalChannelData.getNetwork();
        auto station = mVerticalChannelData.getStation();
        auto verticalChannel = mVerticalChannelData.getChannel();
        auto northChannel    = mNorthChannelData.getChannel();
        auto eastChannel     = mEastChannelData.getChannel();
        auto locationCode = mVerticalChannelData.getLocationCode();
        auto samplingRate = verticalChannelData.getSamplingRate();
        if (network != mNorthChannelData.getNetwork() ||
            network != mEastChannelData.getNetwork())
        {
            throw std::invalid_argument("Inconsistent networks");
        }
        if (station != mNorthChannelData.getStation() ||
            station != mEastChannelData.getStation())
        {
            throw std::invalid_argument("Inconsistent stations");
        }
        if (locationCode != mNorthChannelData.getLocationCode() ||
            locationCode != mEastChannelData.getLocationCode())
        {
            throw std::invalid_argument("Inconsistent location codes");
        }
        if (std::abs(samplingRate
                   - mNorthChannelData.getSamplingRate()) > 1.e-7 ||
            std::abs(samplingRate
                   - mEastChannelData.getSamplingRate()) > 1.e-7)
        {
            throw std::invalid_argument("Inconsistent sampling rates");
        }
     
        // Figure out +/- shift tolerance
        S3CPickerProperties pickerProperties;
 
        auto maxShift
            = std::max(std::abs(pickerProperties.mMinimumPerturbation.count()),
                       std::abs(pickerProperties.mMaximumPerturbation.count()));
        mTolerance = std::chrono::microseconds {maxShift};
 
        auto channel = verticalChannel
                     + "_" + northChannel.back()
                     + "_" + eastChannel.back();
        mName = network + "." + station + "." + channel + "." + locationCode;

        // Target sampling rate
        mInterpolator.setNominalSamplingRate(samplingRate);
        auto gapTolerance = std::chrono::microseconds
        {
            static_cast<int64_t> (std::round(
                std::max(0, programOptions.mGapTolerance)/samplingRate*1.e6))
        };
        mInterpolator.setGapTolerance(gapTolerance);

        // Predefine some vertical data request information
        mVerticalDataRequest.setNetwork(network);
        mVerticalDataRequest.setStation(station);
        mVerticalDataRequest.setChannel(verticalChannel);
        mVerticalDataRequest.setLocationCode(locationCode);
        mVerticalDataRequest.setIdentifier(0);

        mNorthDataRequest.setNetwork(network);
        mNorthDataRequest.setStation(station);
        mNorthDataRequest.setChannel(northChannel);
        mNorthDataRequest.setLocationCode(locationCode);
        mNorthDataRequest.setIdentifier(1);

        mEastDataRequest.setNetwork(network);
        mEastDataRequest.setStation(station);
        mEastDataRequest.setChannel(eastChannel);
        mEastDataRequest.setLocationCode(locationCode);
        mEastDataRequest.setIdentifier(2);

        // Predefine some pick request infromation
        mPickRequest.setSamplingRate(verticalChannelData.getSamplingRate());
        mPickRequest.setIdentifier(0);

        // Predefine some pick information
        mPick.setNetwork(network);
        mPick.setStation(station);
        mPick.setChannel(northChannel); // Arbitrarily choose non-vertical 
        mPick.setOriginalChannels(std::vector<std::string> {verticalChannel,
                                                            northChannel,
                                                            eastChannel});
        mPick.setLocationCode(locationCode);
        mPick.setPhaseHint("S");
        mPick.setFirstMotion(URTS::Broadcasts::Internal::Pick::Pick::
                             FirstMotion::Unknown);
        mPick.setReviewStatus(URTS::Broadcasts::Internal::Pick::Pick::
                              ReviewStatus::Automatic);
        URTS::Broadcasts::Internal::Pick::UncertaintyBound lowerBound;
        URTS::Broadcasts::Internal::Pick::UncertaintyBound upperBound;
        lowerBound.setPercentile(15.9);
        upperBound.setPercentile(84.1);
        lowerBound.setPerturbation(std::chrono::microseconds{-50000});
        upperBound.setPerturbation(std::chrono::microseconds{ 50000});
        mPick.setLowerAndUpperUncertaintyBound(
           std::pair{lowerBound, upperBound});
    }
    /// @brief Convenience funtion to process a pick.
    [[nodiscard]]
    URTS::Broadcasts::Internal::Pick::Pick refinePick(
        const URTS::Broadcasts::Internal::Pick::Pick &initialPick,
        URTS::Services::Scalable::PacketCache::Requestor
              &packetCacheRequestor,
        URTS::Services::Scalable::Pickers::CNNThreeComponentS::
              Requestor &pickRequestor,
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
        try
        {
            queryPacketCache(result.getTime(), packetCacheRequestor);
        }
        catch (const std::exception &e)
        {
            logger->error(e.what());
            result.setChannel(initialPick.getChannel());
            result.setOriginalChannels(initialPick.getOriginalChannels());
            return result; // Really just giving back what was sent
        }
        // Perform inference and update pick
        try
        {
            performInference(&result, pickRequestor, logger);
        }
        catch (const std::exception &e)
        {
            logger->error(e.what());
            result.setChannel(initialPick.getChannel());
            result.setOriginalChannels(initialPick.getOriginalChannels());
            return result;
        }
        //std::cout << "updated pick s: " << initialPick << " " << result << std::endl;
        return result;
    }
    /// @brief Query the packet cache.
    void queryPacketCache(
        const std::chrono::microseconds &pickTime,
        URTS::Services::Scalable::PacketCache::Requestor &packetCacheRequestor)
    {
        S3CPickerProperties pickerProperties;
        auto timeBefore =-pickerProperties.mPreWindow;
        auto timeAfter  = pickerProperties.mPostWindow;

        // Update my request identifier (and avoid overflow in a billion years)
        if (mRequestIdentifier > std::numeric_limits<int64_t>::max() - 10)
        {
            mRequestIdentifier = 0;
        }
        auto t0QueryMuSec = pickTime - timeBefore - (mPadQuery + mTolerance);
        auto t1QueryMuSec = pickTime + timeAfter  + (mPadQuery + mTolerance);
        auto t0Query = static_cast<double> (t0QueryMuSec.count())*1.e-6;
        auto t1Query = static_cast<double> (t1QueryMuSec.count())*1.e-6;
        URTS::Services::Scalable::PacketCache::BulkDataRequest bulkRequest;
        try
        {
            std::pair<double, double> queryTimes{t0Query, t1Query};
            mVerticalDataRequest.setQueryTimes(queryTimes); 
            mNorthDataRequest.setQueryTimes(queryTimes);
            mEastDataRequest.setQueryTimes(queryTimes);

            bulkRequest.setIdentifier(mRequestIdentifier);
            bulkRequest.addDataRequest(mVerticalDataRequest);
            bulkRequest.addDataRequest(mNorthDataRequest);
            bulkRequest.addDataRequest(mEastDataRequest);
        }
        catch (const std::exception &e)
        {
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + " failed to create data request: " 
                              + std::string{e.what()};
            throw std::runtime_error(errorMessage);
        }

        auto reply = packetCacheRequestor.request(bulkRequest);
        if (reply == nullptr)
        {
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + "'s packet cache request for " + mName
                              + " may have timed out";
            throw std::runtime_error(errorMessage);
        }
        // Unpack the responses (need to divine vertical/north/east)
        auto nResponses = reply->getNumberOfDataResponses();
        if (nResponses != 3)
        {
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + " did not get three channels back";
            throw std::runtime_error(errorMessage);
        }
        const auto dataResponsesPtr = reply->getDataResponsesPointer();
        std::array<int, 3> indices{-1, -1, -1};
        for (int i = 0; i < 3; ++i)
        {
            auto id = dataResponsesPtr[i].getIdentifier();
            if (id == mVerticalDataRequest.getIdentifier())
            {
                indices[0] = i;
            }
            else if (id == mNorthDataRequest.getIdentifier())
            {
                indices[1] = i;
            }
            else if (id == mEastDataRequest.getIdentifier())
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
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + " has no data in a bulk response stream";
            throw std::runtime_error(errorMessage);
        }
        try
        {
            // This will center the pick around t0Query, t1Query
            mInterpolator.set(dataResponsesPtr[indices[0]],
                              dataResponsesPtr[indices[1]],
                              dataResponsesPtr[indices[2]],
                              t0QueryMuSec,
                              t1QueryMuSec);
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
                              + ": S pick too close to start of window";
            throw std::runtime_error(errorMessage);
        }
        if (t1Interpolated - (pickTime + mTolerance) < timeAfter)
        {
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + ": S pick too close to end of window";
            throw std::runtime_error(errorMessage);
        }
    }
    /// @brief Perform the pick regression and first motion classification
    ///        inference.
    void performInference(
        URTS::Broadcasts::Internal::Pick::Pick *pick,
        URTS::Services::Scalable::Pickers::CNNThreeComponentS::
              Requestor &pickRequestor,
        std::shared_ptr<UMPS::Logging::ILog> &logger)
    {
        auto algorithms = pick->getProcessingAlgorithms();
        bool computePick = false;
        try
        {
            std::vector<double> verticalSignal;
            std::vector<double> northSignal;
            std::vector<double> eastSignal;
            ::centerAndCut(&verticalSignal,
                           &northSignal,
                           &eastSignal,
                           mInterpolator,
                           pick->getTime(),
                           std::chrono::microseconds {-3000000},
                           std::chrono::microseconds {+2990000});
             mPickRequest.setVerticalNorthEastSignal(
                  std::move(verticalSignal),
                  std::move(northSignal),
                  std::move(eastSignal));
             mPickRequest.setIdentifier(mRequestIdentifier);
             computePick = true;
        }
        catch (const std::exception &e)
        {
            logger->error("Instance " + std::to_string(mInstance)
                        + " S picker signal cut failed with: " + e.what());
        }
        if (computePick)
        {
            auto pickResponse = pickRequestor.request(mPickRequest);
            if (pickResponse == nullptr)
            {
                logger->error("S pick request for  " + mName
                            + " timed out");
            }
            else
            {
                auto returnCode = pickResponse->getReturnCode();
                if (returnCode ==
                    URTS::Services::Scalable::Pickers::CNNThreeComponentS::
                          ProcessingResponse::ReturnCode::Success)
                {
                    auto pickCorrection
                        = std::chrono::microseconds {
                            static_cast<int64_t> (
                            std::round(pickResponse->getCorrection()*1.e6))
                          };
                    pick->setTime(pick->getTime() + pickCorrection);
                    algorithms.push_back("CNNThreeComponentSPicker");
                    // TODO uncertainties
                    pick->setLowerAndUpperUncertaintyBound(
                         mPick.getLowerAndUpperUncertaintyBound());
                    pick->setProcessingAlgorithms(algorithms);
                }
                else
                {
                    logger->warn("S pick request failed for " + mName
                               + " on instance " + std::to_string(mInstance)
                               + ".  Failed with error code "
                               + std::to_string(static_cast<int>
                                                  (returnCode)));
                }
            }
        }
    }
    URTS::Services::Scalable::PacketCache::ThreeComponentWaveform
        mInterpolator;
    URTS::Database::AQMS::ChannelData mVerticalChannelData;
    URTS::Database::AQMS::ChannelData mNorthChannelData;
    URTS::Database::AQMS::ChannelData mEastChannelData;
    URTS::Services::Scalable::PacketCache::DataRequest mVerticalDataRequest;
    URTS::Services::Scalable::PacketCache::DataRequest mNorthDataRequest;
    URTS::Services::Scalable::PacketCache::DataRequest mEastDataRequest;
    URTS::Services::Scalable::Pickers::CNNThreeComponentS::ProcessingRequest
        mPickRequest;
    URTS::Broadcasts::Internal::Pick::Pick mPick;
    std::string mName;
    std::chrono::microseconds mFirstMotionClassifierHalfWindowDuration;
    std::chrono::microseconds mTolerance{850000};   // +/- 0.75 s
    std::chrono::microseconds mPadQuery{500000}; // Pre/post pad query 0.5s
    int64_t mRequestIdentifier{0};
    int mInstance{0};
};

///--------------------------------------------------------------------------///
///                                 Refines Picks                            ///
///--------------------------------------------------------------------------///
class SPickerPipeline
{
public:
    SPickerPipeline(const int instance,
                    const ::ProgramOptions &programOptions,
                    std::shared_ptr<URTS::Database::AQMS::
                                    ChannelDataTablePoller> &databasePoller,
                    std::shared_ptr<
                         ::ThreadSafeQueue<URTS::Broadcasts::
                                           Internal::Pick::Pick>>
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
        if (!mProgramOptions.mRunSPicker)
        {
            throw std::runtime_error("Should not run S pick regressor");
        }
        mLogger->debug("Instance " + std::to_string(mInstance)
                     + " creating packet cache requestor...");
        mPacketCacheRequestor
            = std::make_unique<URTS::Services::Scalable::
                                   PacketCache::Requestor> (mContext, mLogger);
        mPacketCacheRequestor->initialize(
            mProgramOptions.mPacketCacheRequestorOptions);

        mLogger->debug("Instance " + std::to_string(mInstance)
                     + " creating S pick regressor requestor...");
        mPickerRequestor
            = std::make_unique<URTS::Services::Scalable::Pickers::
                               CNNThreeComponentS::Requestor>
                              (mContext, mLogger);
        mPickerRequestor->initialize(
            mProgramOptions.mSPickerRequestorOptions);
    }
    /// @brief Destructor
    ~SPickerPipeline()
    {
        stop();
    }
    /// @brief Fetches the channel data vector
    [[nodiscard]]
    URTS::Database::AQMS::ChannelData
        fetchChannelData(const std::string &network,
                         const std::string &station,
                         const std::string &channel,
                         const std::string &locationCode)
    {
        auto name = network + "." + station + "."
                  + channel + "." + locationCode;
        auto channelDataVector
            = mDatabasePoller->getChannelData(network,
                                              station,
                                              channel,
                                              locationCode);
        if (channelDataVector.empty())
        {
            auto errorMessage = "Instance " + std::to_string(mInstance)
                              + " cannot find channel data for "
                              + name;
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
        auto endTime = channelDataVector.at(channelIndex).getOffDate();
        auto now
            = std::chrono::duration_cast<std::chrono::microseconds> (
                std::chrono::system_clock::now().time_since_epoch());
        if (endTime < now)
        {
            throw std::runtime_error("Instance " + std::to_string(mInstance)
                                  + name + " is closed in database");
        }
        return channelDataVector[channelIndex];
    }
    /// @brief Creates a processing item if it does not exists
    void createProcessingItem(const std::string &network,
                              const std::string &station,
                              const std::string &verticalChannel,
                              const std::string &northChannel,
                              const std::string &eastChannel,
                              const std::string &locationCode)
    {
        // Set name and check if it already exists
        auto name = ::toName(network, station, 
                             verticalChannel, northChannel, eastChannel,
                             locationCode);
        if (mProcessingItems.contains(name)){return;}
        // Okay, let's fetch the channel data and add it
        mLogger->info("Instance " + std::to_string(mInstance)
                    + " adding " + name);
        auto verticalChannelData
            = fetchChannelData(network, station,
                               verticalChannel, locationCode);
        auto northChannelData
            = fetchChannelData(network, station,
                               northChannel, locationCode);
        auto eastChannelData
            = fetchChannelData(network, station,
                               eastChannel, locationCode);
        mProcessingItems.insert(
           std::pair {name,
                      ::ThreeComponentProcessingItem(
                          verticalChannelData,
                          northChannelData,
                          eastChannelData,
                          mProgramOptions,
                          mInstance)});
    }
    void setRunning(const bool running) noexcept
    {
        mKeepRunning = running;
    }
    [[nodiscard]] bool keepRunning() const noexcept
    {
        return mKeepRunning;
    }
    void run()
    {
        std::chrono::milliseconds timeOut{10};
        URTS::Broadcasts::Internal::Pick::Pick initialPick;
        while (keepRunning())
        {
            auto gotPick = mPickProcessorQueue->wait_until_and_pop(
                                &initialPick, timeOut);
            if (gotPick)
            {
                try
                {
                    // Figure out a name  
                    auto originalChannels = initialPick.getOriginalChannels();
                    auto network = initialPick.getNetwork();
                    auto station = initialPick.getStation();
                    auto locationCode = initialPick.getLocationCode();
                    std::string verticalChannel;
                    std::string northChannel;
                    std::string eastChannel;
                    if (originalChannels.size() == 3)
                    {
                        for (const auto &originalChannel : originalChannels)
                        {
                            if (originalChannel.back() == 'Z')
                            {
                                verticalChannel = originalChannel;
                            }
                            else if (originalChannel.back() == 'N' ||
                                     originalChannel.back() == '1')
                            {
                                northChannel = originalChannel;
                            }
                            else if (originalChannel.back() == 'E' ||
                                     originalChannel.back() == '2')
                            {
                                eastChannel = originalChannel;
                            }
                            else
                            {
                                mLogger->error("Unhandled channel "
                                             + originalChannel);
                            }
                        }
                    }
                    else
                    {
                        auto channel = initialPick.getChannel();
                        verticalChannel = channel;
                        northChannel = channel;
                        eastChannel = channel;
                        // Take my best guess
                        if (channel.back() == 'S')
                        {
                            if (channel.size() != 3)
                            {
                                mLogger->error("Unhandled channel: " + channel);
                            }
                            verticalChannel[2] = 'Z';
                            northChannel[2] = 'N';
                            eastChannel[2] = 'E';
                        }
                        else
                        {
                            mLogger->error("Cannot determine channels: "
                                         + channel);
                            continue;
                        } 
                    }
                    // Did I figure this out?
                    if (verticalChannel.empty() ||
                        northChannel.empty() ||
                        eastChannel.empty())
                    {
                        mLogger->error("Could not unwind channel names");
                        continue;
                    }
                    // If the processing item doesn't exist then make it
                    createProcessingItem(network,
                                         station,
                                         verticalChannel,
                                         northChannel,
                                         eastChannel,
                                         locationCode);
                    // Process this pick
                    auto name = ::toName(network,
                                         station,
                                         verticalChannel,
                                         northChannel,
                                         eastChannel,
                                         locationCode);
                    auto it = mProcessingItems.find(name);
#ifndef NDEBUG
                    assert(it != mProcessingItems.end());
#endif
                    auto refinedPick
                        = it->second.refinePick(initialPick,
                                                *mPacketCacheRequestor,
                                                *mPickerRequestor,
                                                mLogger);
                    mPickPublisherQueue->push(refinedPick); 
                }
                catch (const std::exception &e)
                {
                    mLogger->error(e.what());
                    mPickPublisherQueue->push(initialPick);
                }
            } // End check on getting a pick
        }
    }
    void start()
    {
        stop();
        setRunning(true);
        mThread = std::thread(&::SPickerPipeline::run, this);
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
    std::shared_ptr<::ThreadSafeQueue<URTS::Broadcasts::Internal::Pick::Pick>>
         mPickProcessorQueue{nullptr};
    std::shared_ptr<::ThreadSafeQueue<URTS::Broadcasts::Internal::Pick::Pick>>
         mPickPublisherQueue{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<URTS::Services::Scalable::PacketCache::Requestor>
        mPacketCacheRequestor{nullptr};
    std::unique_ptr<URTS::Services::Scalable::
                    Pickers::CNNThreeComponentS::Requestor>
        mPickerRequestor{nullptr};
    std::map<std::string, ::ThreeComponentProcessingItem> mProcessingItems;
    int mInstance{0};
    bool mInputsEqual{true};
    std::atomic<bool> mKeepRunning{true};
};
}
#endif
