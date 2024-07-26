#include <iostream>
#include <set>
#include <mutex>
#include <thread>
#include <chrono>
#include <filesystem>
#include <vector>
#include <string>
#ifndef NDEBUG
#include <cassert>
#endif
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/dailyFile.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/modules/process.hpp>
#include <umps/modules/processManager.hpp>
#include <umps/proxyBroadcasts/heartbeat/publisherProcess.hpp>
#include <umps/proxyBroadcasts/heartbeat/publisherProcessOptions.hpp>
#include <umps/proxyServices/command/moduleDetails.hpp>
#include <umps/proxyServices/command/replier.hpp>
#include <umps/proxyServices/command/replierOptions.hpp>
#include <umps/proxyServices/command/replierProcess.hpp>
#include <umps/services/connectionInformation/requestorOptions.hpp>
#include <umps/services/connectionInformation/requestor.hpp>
#include <umps/services/connectionInformation/details.hpp>
#include <umps/services/connectionInformation/socketDetails/proxy.hpp>
#include <umps/services/connectionInformation/socketDetails/router.hpp>
#include <umps/services/connectionInformation/socketDetails/xSubscriber.hpp>
#include <umps/services/command/availableCommandsRequest.hpp>
#include <umps/services/command/availableCommandsResponse.hpp>
#include <umps/services/command/commandRequest.hpp>
#include <umps/services/command/commandResponse.hpp>
#include <umps/services/command/service.hpp>
#include <umps/services/command/serviceOptions.hpp>
#include <umps/services/command/terminateRequest.hpp>
#include <umps/services/command/terminateResponse.hpp>
#include <uussmlmodels/detectors/uNetThreeComponentP/inference.hpp>
#include <uussmlmodels/detectors/uNetThreeComponentS/inference.hpp>
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "urts/broadcasts/internal/dataPacket/publisher.hpp"
#include "urts/broadcasts/internal/dataPacket/publisherOptions.hpp"
#include "urts/database/aqms/channelDataTablePoller.hpp"
#include "urts/database/aqms/channelDataTable.hpp"
#include "urts/database/aqms/channelData.hpp"
#include "urts/database/connection/postgresql.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/processingRequest.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/processingResponse.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/requestor.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/requestorOptions.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentS/processingRequest.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentS/processingResponse.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentS/requestor.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentS/requestorOptions.hpp"
#include "urts/services/scalable/packetCache/bulkDataRequest.hpp"
#include "urts/services/scalable/packetCache/bulkDataResponse.hpp"
#include "urts/services/scalable/packetCache/dataRequest.hpp"
#include "urts/services/scalable/packetCache/dataResponse.hpp"
#include "urts/services/scalable/packetCache/requestor.hpp"
#include "urts/services/scalable/packetCache/requestorOptions.hpp"
#include "urts/services/scalable/packetCache/threeComponentWaveform.hpp"
#include "urts/services/scalable/packetCache/sensorRequest.hpp"
#include "urts/services/scalable/packetCache/sensorResponse.hpp"
#include "detectorProperties.hpp"
#include "programOptions.hpp"
#include "splitWork.hpp"
#include "threeComponentChannelData.hpp"
//#include "threeComponentDataItem.hpp"
//#include "oneComponentDataItem.hpp"
#include "getNow.hpp"
#include "threadSafeState.hpp"
#include "threeComponentProcessingPipeline.hpp"
#include "oneComponentProcessingPipeline.hpp"
#include "private/threadSafeQueue.hpp"
#include "private/threadSafeBoundedQueue.hpp"
#include "private/isEmpty.hpp"

#define MODULE_NAME "MLDetector"
namespace UDatabase = URTS::Database;
namespace UDetectors = URTS::Services::Scalable::Detectors;

/// @result Gets the command line input options as a string.
[[nodiscard]] std::string getInputOptions() noexcept
{
    std::string commands{
R"""(
Commands: 
   inferenceQueueSize  Displays the size of the inference queues.
   help                Displays this message.
)"""};
    return commands;
}

/// @result The logger for this application.
std::shared_ptr<UMPS::Logging::ILog>
createLogger(const std::string &moduleName = MODULE_NAME,
             const std::filesystem::path logFileDirectory = "/var/log/urts",
             const UMPS::Logging::Level verbosity = UMPS::Logging::Level::Info,
             const int hour = 0,
             const int minute = 0)
{
    auto logFileName = moduleName + ".log";
    auto fullLogFileName = logFileDirectory / logFileName;
    auto logger = std::make_shared<UMPS::Logging::DailyFile> (); 
    logger->initialize(moduleName,
                       fullLogFileName,
                       verbosity,
                       hour, minute);
    logger->info("Starting logging for " + moduleName);
    return logger;
}

/// Make the one and three-component channel list from database.
/// @param[out] threeComponentSensors  The list of three-component sensors as
///                                    gleaned from the database.
/// @param[out] oneComponentSensors    The list of one-component sensors as
///                                    gleaned from the database.
/// @param[in] channels                The channels in the database. 
/// @param[in] activeNetworks          Only retain channels in active networks.
void makeOneAndThreeComponentStation(
    std::vector<::ThreeComponentChannelData> *threeComponentSensors,
    std::vector<UDatabase::AQMS::ChannelData> *oneComponentSensors,
    const std::vector<UDatabase::AQMS::ChannelData> &channels,
    std::shared_ptr<UMPS::Logging::ILog> logger,
    const std::set<std::string> activeNetworks = std::set<std::string> {"UU", "WY", "US", "PB"},
    const std::set<double> validSamplingRates = std::set<double> {100})
{
    // Custom structs
    struct ThreeComponentLink
    {
        int mVertical{-1};
        int mNorth{-1};
        int mEast{-1};
    };
    struct OneComponentLink
    {
        int mVertical{-1};
    };
    // Reset results
    threeComponentSensors->clear();
    oneComponentSensors->clear();
    // Get channels that are on when program starts
    auto now = ::getNow(); // Time in microseconds since epoch
    auto nChannels = static_cast<int> (channels.size());
    std::vector<ThreeComponentLink> threeComponentLinks;
    std::vector<OneComponentLink> oneComponentLinks;
    std::vector<bool> isClassified(nChannels, false);
    for (int i = 0; i < nChannels; ++i)
    {
        if (isClassified[i]){continue;} // Already did this one
        // Is channel active?
        if (channels.at(i).getOnDate() >= now &&
            channels.at(i).getOffDate() < now)
        {
            continue;
        }
        auto network = channels.at(i).getNetwork();
        auto station = channels.at(i).getStation();
        auto channel = channels.at(i).getChannel();
        if (channel.size() != 3)
        {
            logger->warn("Unhandled channel code: " + channel);
        }
        auto samplingRateGainCode = channel.substr(0, 2);
        auto component = channel.substr(2, 1); 
        auto locationCode = channels.at(i).getLocationCode();
        if (component != "Z"){continue;}
        ThreeComponentLink threeComponentLink{i, -1, -1};
        OneComponentLink oneComponentLink{i}; 
        for (int j = 0; j < nChannels; ++j)
        {
            if (i == j){continue;} // Don't want to deal with this
            if (isClassified[j]){continue;} // Already did this one
            // Is channel active?
            if (channels.at(j).getOnDate() >= now &&
                channels.at(j).getOffDate() < now)
            {
                continue;
            }
            if (network == channels.at(j).getNetwork() &&
                station == channels.at(j).getStation() &&
                locationCode == channels.at(j).getLocationCode())
            {
                auto otherChannel = channels.at(j).getChannel();
                auto otherComponent = otherChannel.substr(2, 1);
                if (channel == otherChannel){continue;}
                if (otherChannel.find(samplingRateGainCode) !=
                    std::string::npos)
                {
                    if (otherComponent == "N" || otherComponent == "1")
                    {
                        threeComponentLink.mNorth = j;
                    }
                    else if (otherComponent == "E" || otherComponent == "2")
                    {
                        threeComponentLink.mEast = j;
                    }
                    else
                    {
                        logger->warn("Unhandled channel: " + otherChannel);
                    }
                }
            }
        }
        // Station is classified -> is it 1c vertical or 3c?
        if (threeComponentLink.mVertical >= 0 &&
            threeComponentLink.mNorth >= 0 &&
            threeComponentLink.mEast >= 0)
        {
            isClassified.at(threeComponentLink.mVertical) = true;
            isClassified.at(threeComponentLink.mNorth) = true;
            isClassified.at(threeComponentLink.mEast) = true;
            threeComponentLinks.push_back(threeComponentLink);
        }
        else
        {
            if (oneComponentLink.mVertical >= 0)
            {
                isClassified.at(oneComponentLink.mVertical) = true;
                oneComponentLinks.push_back(oneComponentLink);         
            }
            else
            {
                logger->warn("Missing channel for: "
                            + network + "." + station + "."
                            + channel + "." + locationCode);
            }
        }
    }
    // Clean it up
    auto nThreeComponentChannelDatas = static_cast<int> (threeComponentLinks.size());
    auto nOneComponentSensors = static_cast<int> (oneComponentLinks.size());
    logger->debug("Number of three-component sensors: "
                +  std::to_string(nThreeComponentChannelDatas));
    logger->debug("Number of one-component sensors: "
                 + std::to_string(nOneComponentSensors));
    logger->debug("Number of unclassified channels: "
                + std::to_string(nChannels
                               - (3*nThreeComponentChannelDatas + nOneComponentSensors)));
    std::vector<bool> isAssigned(nChannels, false);
    for (const auto &threeComponentLink : threeComponentLinks)
    {
        auto iv = threeComponentLink.mVertical;
        auto in = threeComponentLink.mNorth;
        auto ie = threeComponentLink.mEast;
        if (std::abs(channels[iv].getSamplingRate()
                   - channels[in].getSamplingRate()) > 1.e-4 ||
            std::abs(channels[iv].getSamplingRate()
                   - channels[ie].getSamplingRate()) > 1.e-4)
        {
            logger->warn("Inconsistent sampling rates.  Skipping...");
            continue;
        }
        isAssigned[iv] = true;
        isAssigned[in] = true;
        isAssigned[ie] = true;
        ::ThreeComponentChannelData channelData(channels[iv],
                                                channels[in],
                                                channels[ie]);
        threeComponentSensors->push_back(std::move(channelData));
    }
    for (const auto &oneComponentLink : oneComponentLinks)
    {
         auto iv = oneComponentLink.mVertical;
#ifndef NDEBUG
        assert(!isAssigned[iv]);
#endif
        isAssigned[oneComponentLink.mVertical] = true;
        oneComponentSensors->push_back(channels[iv]);
    }
    // Apply mask
    auto temp3C = *threeComponentSensors;
    threeComponentSensors->clear();
    for (int i = 0; i < static_cast<int> (temp3C.size()); ++i)
    {
         auto network = temp3C.at(i).getNetwork();
         auto samplingRate = temp3C.at(i).getNominalSamplingRate();
         if (!activeNetworks.empty() &&
             activeNetworks.find(network) == activeNetworks.end())
         {
             continue;
         }
         bool isValidSamplingRate{false};
         for (const auto &validSamplingRate : validSamplingRates)
         { 
             if (std::abs(samplingRate - validSamplingRate) < 1.e-5)
             {
                 isValidSamplingRate = true;
                 break;
             } 
         }
         if (!isValidSamplingRate){continue;}
         threeComponentSensors->push_back(temp3C[i]);
    }
    logger->debug("Number of three-component sensors after sampling rate and network mask: " 
                + std::to_string(threeComponentSensors->size()));
 
    auto temp1C = *oneComponentSensors;
    oneComponentSensors->clear();
    for (int i = 0; i < static_cast<int> (temp1C.size()); ++i)
    {
         auto network = temp1C.at(i).getNetwork();
         auto samplingRate = temp1C.at(i).getSamplingRate();
         if (!activeNetworks.empty() &&
             activeNetworks.find(network) == activeNetworks.end())
         {
             continue;
         }
         bool isValidSamplingRate{false};
         for (const auto &validSamplingRate : validSamplingRates)
         {
             if (std::abs(samplingRate - validSamplingRate) < 1.e-5)
             {
                 isValidSamplingRate = true;
                 break;
             }
         }
         if (!isValidSamplingRate){continue;}
         oneComponentSensors->push_back(temp1C[i]);
    }
    logger->debug("Number of one-component sensors after network mask: " 
                + std::to_string(oneComponentSensors->size()));
}
 
/// @brief Parses the command line options.
[[nodiscard]] std::string parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(
R"""(
The mlDetector drives the UNet detectors and output P and S probability
signals to a probability broadcast.  Example usage:
    mlDetector --ini=mlDetector.ini
Allowed options)""");
    desc.add_options()
        ("help", "Produces this help message")
        ("ini",  boost::program_options::value<std::string> (),
                 "Defines the initialization file for this executable");
    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);
    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return iniFile;
    }
    if (vm.count("ini"))
    {
        iniFile = vm["ini"].as<std::string>();
        if (!std::filesystem::exists(iniFile))
        {
            throw std::runtime_error("Initialization file: " + iniFile
                                   + " does not exist");
        }
    }
    else
    {
        throw std::runtime_error("Initialization file was not set");
    }
    return iniFile;
}

///--------------------------------------------------------------------------///
/// @brief This does the heavy lifting and runs all the detectors.
class Detector : public UMPS::Modules::IProcess
{
public:
    Detector(const ::ProgramOptions &programOptions,
             std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mProgramOptions(programOptions),
        mLogger(logger)
    {
        auto nThreads = programOptions.mThreads;

        // Make a database connection
        mLogger->debug("Connecting to AQMS database...");
        auto databaseConnection
            = std::make_shared<UDatabase::Connection::PostgreSQL> ();
        databaseConnection->setAddress(mProgramOptions.mDatabaseAddress);
        databaseConnection->setDatabaseName(mProgramOptions.mDatabaseName);
        databaseConnection->setUser(mProgramOptions.mDatabaseReadOnlyUser);
        databaseConnection->setPassword(
            mProgramOptions.mDatabaseReadOnlyPassword);
        databaseConnection->setPort(mProgramOptions.mDatabasePort);
        databaseConnection->connect();

        auto connectionHandle
            = std::static_pointer_cast<UDatabase::Connection::IConnection> (
                databaseConnection);

        mChannelDataPoller.initialize(
            connectionHandle,
            UDatabase::AQMS::ChannelDataTablePoller::QueryMode::Current,
            programOptions.mDatabasePollerInterval);
        mChannelDataPoller.start();
        std::this_thread::sleep_for(std::chrono::milliseconds {250});

        // Figure out the 1c and 3c channel maps
        auto channels = mChannelDataPoller.getChannelData();
        std::vector<::ThreeComponentChannelData> threeComponentSensors;
        std::vector<UDatabase::AQMS::ChannelData> oneComponentSensors;
        ::makeOneAndThreeComponentStation(&threeComponentSensors,
                                          &oneComponentSensors,
                                          channels,
                                          mLogger,
                                          mProgramOptions.mActiveNetworks,
                                          mProgramOptions.mValidSamplingRates);
        if (threeComponentSensors.empty() && oneComponentSensors.empty())
        {
            mLogger->warn("No channels read from database");
        }

        // Send work out to processes
        if (mProgramOptions.mRunP3CDetector || mProgramOptions.mRunS3CDetector)
        {
            mLogger->info("Partitioning "
                        + std::to_string(threeComponentSensors.size())
                        + " 3C pipeplines among "
                        + std::to_string(nThreads) + " threads");
            auto jobsToThread = ::splitWork(threeComponentSensors.size(), nThreads);
            for (int i = 0; i < nThreads; ++i)
            {
                std::vector<::ThreeComponentChannelData> sub3CSensorList;
                for (int job = jobsToThread.at(i);
                         job < jobsToThread.at(i + 1);
                         ++job)
                {
                    sub3CSensorList.push_back(threeComponentSensors.at(job));
                }
                m3CPipelines.push_back(std::make_unique<::ThreeComponentProcessingPipeline>
                    (i,
                     mProgramOptions,
                     sub3CSensorList,
                     mLogger));
                std::this_thread::sleep_for (std::chrono::microseconds {250});
            }
        } 
        if (mProgramOptions.mRunP1CDetector)
        {
            mLogger->info("Partitioning "
                        + std::to_string(oneComponentSensors.size())
                        + " 1C pipeplines among "
                        + std::to_string(nThreads) + " threads");
            auto jobsToThread = ::splitWork(threeComponentSensors.size(), nThreads);
            for (int i = 0; i < nThreads; ++i) 
            {
                std::vector<URTS::Database::AQMS::ChannelData> sub1CSensorList;
                for (int job = jobsToThread.at(i);
                         job < jobsToThread.at(i + 1);
                         ++job)
                {
                    sub1CSensorList.push_back(oneComponentSensors.at(job));
                }
                m1CPipelines.push_back(std::make_unique<::OneComponentProcessingPipeline>
                    (i,
                     mProgramOptions,
                     sub1CSensorList,
                     mLogger));
                std::this_thread::sleep_for (std::chrono::microseconds {250});
           }
        }

        // Instantiate the local command replier
        mLocalCommand
            = std::make_unique<UMPS::Services::Command::Service> (mLogger);
        UMPS::Services::Command::ServiceOptions localServiceOptions;
        localServiceOptions.setModuleName(getName());
        localServiceOptions.setCallback(
            std::bind(&Detector::commandCallback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
        mLocalCommand->initialize(localServiceOptions);
        mInitialized = true;
    }
/*
    Detector(
        const ProgramOptions programOptions,
        std::unique_ptr<URTS::Broadcasts::Internal::DataPacket::Publisher>
            &&probabilityPublisher,
        std::unique_ptr<URTS::Services::Scalable::PacketCache::Requestor>
            &&packetCacheRequestor,
        std::unique_ptr<UDetectors::UNetThreeComponentP::Requestor>
            &&inference3CP,
        std::unique_ptr<UDetectors::UNetThreeComponentS::Requestor>
            &&inference3CS,
        std::unique_ptr<UDetectors::UNetOneComponentP::Requestor>
            &&inference1CP,
        std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mProgramOptions(programOptions),
        mProbabilityPublisher(std::move(probabilityPublisher)),
        mPacketCacheRequestor(std::move(packetCacheRequestor)),
        m3CPInferenceRequestor(std::move(inference3CP)),
        m3CSInferenceRequestor(std::move(inference3CS)),
        mLogger(logger)
    {
        mModuleName = mProgramOptions.mModuleName;
        if (!mProbabilityPublisher->isInitialized())
        {
            throw std::invalid_argument(
               "Probability publisher not initialized");
        }
        if (!mPacketCacheRequestor->isInitialized())
        {
            throw std::invalid_argument(
               "Packet cache requestor not initialized");
        }
        if (m3CPInferenceRequestor != nullptr)
        {
            if (!m3CPInferenceRequestor->isInitialized())
            {
                throw std::invalid_argument("3C P requestor not initialized");
            }
            mRun3CPDetector = true;
        }
        if (m3CSInferenceRequestor != nullptr)
        {
            if (!m3CSInferenceRequestor->isInitialized())
            {
                throw std::invalid_argument("3C S requestor not initialized");
            }
            mRun3CSDetector = true;
        }
        if (!mRun3CPDetector && !mRun3CSDetector)
        {
            throw std::invalid_argument("No detectors to run!");
        }
        if (mLogger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> ();
        }
        // Make a database connection
        mLogger->debug("Connecting to AQMS database...");
        auto databaseConnection
            = std::make_shared<UDatabase::Connection::PostgreSQL> ();
        databaseConnection->setAddress(mProgramOptions.mDatabaseAddress);
        databaseConnection->setDatabaseName(mProgramOptions.mDatabaseName);
        databaseConnection->setUser(mProgramOptions.mDatabaseReadOnlyUser);
        databaseConnection->setPassword(
            mProgramOptions.mDatabaseReadOnlyPassword);
        databaseConnection->setPort(mProgramOptions.mDatabasePort);
        databaseConnection->connect();

        auto connectionHandle
            = std::static_pointer_cast<UDatabase::Connection::IConnection> (
                databaseConnection);

        mChannelDataPoller.initialize(
            connectionHandle,
            UDatabase::AQMS::ChannelDataTablePoller::QueryMode::Current,
            programOptions.mDatabasePollerInterval);
        mChannelDataPoller.start();
        std::this_thread::sleep_for(std::chrono::milliseconds {250});

        // Determine if the P and S 3C inputs are the same.
        P3CDetectorProperties p3CProperties;
        S3CDetectorProperties s3CProperties;
        mPS3CInputsAreEqual = (p3CProperties == s3CProperties);
#ifndef NDEBUG
        assert(mPS3CInputsAreEqual);
#endif 

        // Instantiate the local command replier
        mLocalCommand
            = std::make_unique<UMPS::Services::Command::Service> (mLogger);
        UMPS::Services::Command::ServiceOptions localServiceOptions;
        localServiceOptions.setModuleName(getName());
        localServiceOptions.setCallback(
            std::bind(&Detector::commandCallback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
        mLocalCommand->initialize(localServiceOptions);
        mInitialized = true;
    }
*/
    /// @brief Destructor
    ~Detector() override
    {
        stop();
    }
    /// Initialized?
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
    [[nodiscard]] bool isRunning() const noexcept override
    {
        return keepRunning();
    }
    /// @result The module name.
    [[nodiscard]] std::string getName() const noexcept override
    {
        return mModuleName;
    }
    /// @brief Toggles this as running or not running
    void setRunning(const bool running)
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        mKeepRunning = running;
    }
    /// @brief Starts the processes.
    void start() override
    {
        stop();
        if (!isInitialized())
        {
            throw std::runtime_error("Class not initialized");
        }
        setRunning(true);
        for (auto &p : m3CPipelines)
        {
            p->setRunning(true);
        }
        for (auto &p : m1CPipelines)
        {
            p->setRunning(true);
        }
        std::this_thread::sleep_for (std::chrono::milliseconds {250});
        for (int it = 0; it < static_cast<int> (m3CPipelines.size()); ++it)
        {
            auto thread = std::thread(&::ThreeComponentProcessingPipeline::run,
                                      &*m3CPipelines[it]);
            mPipelineThreads.push_back(std::move(thread));
        }
/*
        mLogger->debug("Starting the probability broadcast thread...");
        mPublisherThread = std::thread(&Detector::publishProbabilityPackets,
                                       this);
        if (mPS3CInputsAreEqual)
        {
            mInference3CPSThread = std::thread(&Detector::performInference3CPS,
                                               this);
        }
        else
        {
            if (mRun3CPDetector)
            {
                mLogger->debug("Starting the 3CP inference thread...");
                mInference3CPThread = std::thread(&Detector::performInference3CP,
                                                  this);
            }
            if (mRun3CSDetector)
            {
                mLogger->debug("Starting the 3CS inference thread...");
                mInference3CSThread = std::thread(&Detector::performInference3CS,
                                                  this);
            }
        }
        mLogger->debug("Starting the packet cache query thread...");
        mPacketCacheThread = std::thread(&Detector::queryWaveforms, this);
        mLogger->debug("Starting the task manager thread...");
        mTaskManagerThread = std::thread(&Detector::createTasks, this); 
*/
        mLogger->debug("Starting the local command proxy..."); 
        mLocalCommand->start();
    }
    /// @brief Stops the process
    void stop() override
    {
        setRunning(false);
        for (auto &p : m3CPipelines){p->setRunning(false);}
        for (auto &p : m1CPipelines){p->setRunning(false);}
        for (auto &t : mPipelineThreads)
        {
            if (t.joinable()){t.join();}
        }
        if (mPublisherThread.joinable()){mPublisherThread.join();}
        if (mInference3CPSThread.joinable()){mInference3CPSThread.join();}
        if (mInference3CPThread.joinable()){mInference3CPThread.join();}
        if (mInference3CSThread.joinable()){mInference3CSThread.join();}
        if (mInference1CPThread.joinable()){mInference1CPThread.join();}
        if (mPacketCacheThread.joinable()){mPacketCacheThread.join();}
        if (mTaskManagerThread.joinable()){mTaskManagerThread.join();}
        if (mLocalCommand != nullptr)
        {
            if (mLocalCommand->isRunning()){mLocalCommand->stop();}
        }
    }
    /// @brief Callback for interacting with user 
    std::unique_ptr<UMPS::MessageFormats::IMessage>
        commandCallback(const std::string &messageType,
                        const void *data,
                        size_t length)
    {   
        namespace USC = UMPS::Services::Command;
        mLogger->debug("Command request received");
        USC::AvailableCommandsRequest availableCommandsRequest;
        USC::CommandRequest commandRequest;
        USC::TerminateRequest terminateRequest;
        if (messageType == availableCommandsRequest.getMessageType())
        {
            USC::AvailableCommandsResponse response;
            response.setCommands(getInputOptions());
            try
            {
                availableCommandsRequest.fromMessage(
                    static_cast<const char *> (data), length);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to unpack commands request");
            }
            return response.clone();
        }
        else if (messageType == commandRequest.getMessageType())
        {
            USC::CommandResponse response;
            try
            {
                commandRequest.fromMessage(
                    static_cast<const char *> (data), length);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to unpack commands request: "
                             + std::string {e.what()});
                response.setReturnCode(
                    USC::CommandResponse::ReturnCode::ApplicationError);
                return response.clone();
            }
            auto command = commandRequest.getCommand();
            if (command == "inferenceQueueSize")
            {
                auto psQueueSize = mPS3CInferenceQueue.size();
                auto message = "P and S 3C inference queue size is "
                             + std::to_string(psQueueSize);
                response.setResponse(message);
                response.setReturnCode(
                    USC::CommandResponse::ReturnCode::Success);
            }
            else if (command != "help")
            {
                mLogger->debug("Invalid command: " + command);
                response.setResponse("Invalid command: " + command);
                response.setReturnCode(
                    USC::CommandResponse::ReturnCode::InvalidCommand);
            }
            else
            {
                response.setResponse(getInputOptions());
                response.setReturnCode(
                    USC::CommandResponse::ReturnCode::Success);
            }
            return response.clone();
        }
        else if (messageType == terminateRequest.getMessageType())
        {
            mLogger->info("Received terminate request...");
            USC::TerminateResponse response;
            try
            {
                terminateRequest.fromMessage(
                    static_cast<const char *> (data), length);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to unpack terminate request.");
                response.setReturnCode(
                    USC::TerminateResponse::ReturnCode::InvalidCommand);
                return response.clone();
            }
            issueStopCommand();
            response.setReturnCode(USC::TerminateResponse::ReturnCode::Success);
            return response.clone();
        }
        // Return
        mLogger->error("Unhandled message: " + messageType);
        USC::AvailableCommandsResponse commandsResponse;
        commandsResponse.setCommands(::getInputOptions());
        return commandsResponse.clone();
    }
    /// @brief This thread manages the channel lists.
    /// TODO: Dead
/*
    void updateChannelLists()
    {
        // Figure out the 1c and 3c channel maps
        auto channels = mChannelDataPoller.getChannelData();
        std::vector<::ThreeComponentChannelData> threeComponentSensors;
        std::vector<UDatabase::AQMS::ChannelData> oneComponentSensors;
        ::makeOneAndThreeComponentStation(&threeComponentSensors,
                                          &oneComponentSensors,
                                          channels,
                                          mLogger,
                                          mProgramOptions.mActiveNetworks,
                                          mProgramOptions.mValidSamplingRates);
        if (threeComponentSensors.empty() && oneComponentSensors.empty())
        {
            mLogger->warn("No channels read from database");
        }
        // We can recycle data queries 
        P3CDetectorProperties p3CProperties;
        if (mPS3CInputsAreEqual)
        {
            if (mRun3CPDetector || mRun3CSDetector)
            {
                for (const auto &threeComponentSensor : threeComponentSensors)
                {
                    ::ThreeComponentDataItem
                        item(threeComponentSensor,
                             p3CProperties.mDetectorWindowDuration,
                             mProgramOptions.mMaximumSignalLatency,
                             mProgramOptions.mGapTolerance,
                             mProgramOptions.mDataQueryWaitPercentage,
                             p3CProperties.mWindowStart,
                             p3CProperties.mWindowEnd,
                             p3CProperties.mSamplingRate,
                             mLogger);
                    m3CPSDataItems.insert(std::pair{item.getHash(), item});
                }
            }
        }
        else
        {
#ifndef NDEBUG
            assert(false);
#else
            throw std::runtime_error("Inputs are unequal");
#endif
        }
//S3CDetectorProperties s3CProperties;
//            if (mRun3CPDetector)
//            {
//                for (const auto &threeComponentSensor : threeComponentSensors)
//                {
//                    ::ThreeComponentDataItem
//                        item(threeComponentSensor,
//                             detectorWindowDuration,
//                             mProgramOptions.mGapTolerance,
//                             waitPct,
//                             pCenterWindowStart,
//                             pCenterWindowEnd,
//                             pDetectorSamplingRate);
//                    m3CPDataRequestItems.push_back(std::move(item));
//                }
//            }
//            if (mRun3CSDetector)
//            {
//                for (const auto &threeComponentSensor : threeComponentSensors)
//                {
//                    ::ThreeComponentDatatem
//                        item(threeComponentSensor,
//                             detectorWindowDuration,
//                             mProgramOptions.mGapTolerance,
//                             waitPct,
//                             sCenterWindowStart,
//                             sCenterWindowEnd,
//                             sDetectorSamplingRate);
//                    m3CSDataRequestItems.push_back(std::move(item));
//                }
//            }
//        }
        if (mRun1CPDetector)
        {
//            mLogger->warn("1c p detector is still in devleopement");
//            P3CDetectorProperties p1CProperties;
//            for (const auto &oneComponentSensor : oneComponentSensors)
//            {
//                ::OneComponentDataItem
//                    item(oneComponentSensor,
//                         p1CProperties.mDetectorWindowDuration,
//                         mProgramOptions.mMaximumSignalLatency,
//                         mProgramOptions.mGapTolerance,
//                         mProgramOptions.mDataQueryWaitPercentage,
//                         p1CProperties.mWindowStart,
//                         p1CProperties.mWindowEnd,
//                         p1CProperties.mSamplingRate,
//                         mLogger);
//                m1CPDataItems.insert( std::pair{item.getHash(), item} );
//            }
        }
    }
*/
    /// @brief This is the manager thread.  It basically monitors the pipeline.
    /// TODO: Dead
/*
    void createTasks()
    {
        updateChannelLists();
        while (keepRunning())
        {
            for (auto &dataItem : m3CPSDataItems)
            {
//if (dataItem.second.getName() != "WY.YPP.HH[ZNE].01"){continue;}
                if (dataItem.second.getState() ==
                    ::ThreadSafeState::State::ReadyToQueryData)
                {
                    dataItem.second.setState(
                        ::ThreadSafeState::State::QueryData);
                    mPSDataQueryBoundedQueue.push(dataItem.first);
                }
                else if (dataItem.second.getState() ==
                         ::ThreadSafeState::State::ReadyForInferencing)
                {
                    mLogger->debug("Sending: " + dataItem.second.getName()
                                 + " for PS inferencing");
                    dataItem.second.setState(
                        ::ThreadSafeState::State::InferencePS);
                    // Safely add to the queue
                    while (mPS3CInferenceQueue.size() > mMaxInferenceItems)
                    {
                        auto purgeHash = mPS3CInferenceQueue.try_pop();
                        if (purgeHash != nullptr)
                        {
                            auto jt = m3CPSDataItems.find(*purgeHash);
                            if (jt == m3CPSDataItems.end())
                            {
                                mLogger->warn("PS inference job for "
                                            + jt->second.getName()
                                            + " timed out");
                                jt->second.updateLastProbabilityTimeToNow();
                                jt->second.setState(
                                    ::ThreadSafeState::State::ReadyToQueryData);
                            }
                        }
                    }
                    mPS3CInferenceQueue.push(dataItem.first);
                }
                else if (dataItem.second.getState() ==
                         ::ThreadSafeState::State::ReadyForBroadcasting)
                {
                    dataItem.second.setState(
                        ::ThreadSafeState::State::BroadcastPS);
                    mPSBroadcastQueue.push(dataItem.first);
                }
            }
            for (auto it = m3CPSDataItems.begin(); it != m3CPSDataItems.end(); ++it)
            {
                if (it->second.getState() == ::ThreadSafeState::State::Destroy)
                {
                    m3CPSDataItems.erase(it);
                }
            }
            std::this_thread::sleep_for (std::chrono::milliseconds {10});
        } // Loop
    }
    /// @brief This queries signals from the packet cache.
    /// TODO dead
    void queryWaveforms()
    {
        while (keepRunning())
        {
            auto hash = mPSDataQueryBoundedQueue.try_pop();
            if (hash == nullptr){continue;}  
            auto dataItemIterator = m3CPSDataItems.find(*hash);
            if (dataItemIterator == m3CPSDataItems.end())
            {
                mLogger->warn("Data item no longer exists in map");
                continue;
            }
            try
            {
                dataItemIterator->second.queryPacketCache(*mPacketCacheRequestor);
            }
            catch (const std::exception &e)
            {
                mLogger->debug("Query failed with: " + std::string {e.what()});
                dataItemIterator->second.setState(
                    ::ThreadSafeState::State::ReadyToQueryData);
                continue;
            }
            std::this_thread::sleep_for (std::chrono::milliseconds {10});
        }
    }
    /// @brief Performs the P and S inferencing
    /// TODO dead
    void performInference3CPS()
    {
        while (keepRunning())
        { 
            auto hash = mPS3CInferenceQueue.try_pop();
            if (hash == nullptr){continue;}
            auto dataItemIterator = m3CPSDataItems.find(*hash);
            if (dataItemIterator == m3CPSDataItems.end())
            {
                mLogger->warn("Data item no longer exists in map");
                continue;
            }
            try
            {
                mLogger->debug("Inference request started for "
                             + dataItemIterator->second.getName()
                             + ".  Queue size is now "
                             + std::to_string(mPS3CInferenceQueue.size()));
                dataItemIterator->second.performPAndSInference(
                    *m3CPInferenceRequestor,
                    *m3CSInferenceRequestor);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Inference failed with: "
                              + std::string {e.what()});
                dataItemIterator->second.setState(
                    ::ThreadSafeState::State::ReadyToQueryData);
                continue;
            }
            std::this_thread::sleep_for (std::chrono::milliseconds {10});
        }
    } 
    /// @brief Stitch the waveforms together.
    /// @brief Inference engine for P 3C waveforms.
    /// TODO dead
    void performInference3CP()
    {
        while (keepRunning())
        {
            auto hash = mP3CInferenceQueue.try_pop();
            if (hash == nullptr){continue;}
            auto dataItemIterator = m3CPDataItems.find(*hash);
            if (dataItemIterator == m3CPDataItems.end())
            {
                mLogger->warn("Data item no longer exists in map");
                continue;
            }
            try
            {
                //dataItemIterator->second.pInference(*m3CPInferenceRequestor);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Inference failed with: "
                              + std::string {e.what()});
                dataItemIterator->second.setState(
                    ::ThreadSafeState::State::ReadyToQueryData);
                continue;
            }
        }
    }
    /// @brief Inference engine for S 3C waveforms.
    /// TODO dead
    void performInference3CS()
    {
        while (keepRunning())
        {
            auto hash = mS3CInferenceQueue.try_pop();
            if (hash == nullptr){continue;}
            auto dataItemIterator = m3CSDataItems.find(*hash);
            if (dataItemIterator == m3CSDataItems.end())
            {
                mLogger->warn("Data item no longer exists in map");
                continue;
            }
            try
            {
                //dataItemIterator->second.pInference(*m3CPInferenceRequestor);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Inference failed with: "
                              + std::string {e.what()});
                dataItemIterator->second.setState(
                    ::ThreadSafeState::State::ReadyToQueryData);
                continue;
            }
        }
    }
    /// @brief Publishes probability packets.
    /// TODO dead
    void publishProbabilityPackets()
    {
        while (keepRunning())
        {
            auto hash = mPSBroadcastQueue.try_pop();
            if (hash == nullptr){continue;}
            auto dataItemIterator = m3CPSDataItems.find(*hash);
            if (dataItemIterator == m3CPSDataItems.end())
            {
                mLogger->warn("Data item no longer exists in map");
                continue;
            }
            try
            {
                dataItemIterator->second.broadcast(*mProbabilityPublisher);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Broadcast failed with: "
                              + std::string {e.what()});
                dataItemIterator->second.setState(
                    ::ThreadSafeState::State::ReadyToQueryData);
                continue;
            }
        }
        mLogger->debug("Probability publisher thread exiting...");
    }
*/
///public: 
    mutable std::mutex mMutex;
    std::vector<std::thread> mPipelineThreads;
    std::vector<std::unique_ptr<::ThreeComponentProcessingPipeline>> m3CPipelines;
    std::vector<std::unique_ptr<::OneComponentProcessingPipeline>> m1CPipelines;
    std::thread mPublisherThread;
    std::thread mInference3CPSThread;
    std::thread mInference3CPThread;
    std::thread mInference3CSThread;
    std::thread mInference1CPThread;
    std::thread mPacketCacheThread;
    std::thread mTaskManagerThread;
    std::string mModuleName{MODULE_NAME};
    ::ProgramOptions mProgramOptions;
    UDatabase::AQMS::ChannelDataTablePoller mChannelDataPoller;
    std::unique_ptr<URTS::Broadcasts::Internal::DataPacket::Publisher>
         mProbabilityPublisher{nullptr};
    std::unique_ptr<URTS::Services::Scalable::PacketCache::Requestor>
         mPacketCacheRequestor{nullptr};
    std::unique_ptr<UDetectors::UNetThreeComponentP::Requestor>
         m3CPInferenceRequestor{nullptr};
    std::unique_ptr<UDetectors::UNetThreeComponentS::Requestor>
         m3CSInferenceRequestor{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<UMPS::Services::Command::Service> mLocalCommand{nullptr};
    ::ThreadSafeBoundedQueue<size_t> mPSDataQueryBoundedQueue{2000};
    ::ThreadSafeQueue<size_t> mPS3CInferenceQueue;
    ::ThreadSafeQueue<size_t> mP3CInferenceQueue;
    ::ThreadSafeQueue<size_t> mS3CInferenceQueue;
    ::ThreadSafeQueue<size_t> mP1CInferenceQueue;
    ::ThreadSafeQueue<size_t> mPSBroadcastQueue;
    ::ThreadSafeQueue<URTS::Broadcasts::Internal::DataPacket::DataPacket>
        mProbabilityPacketToPublisherQueue;
    //std::map<size_t, ::ThreeComponentDataItem> m3CPSDataItems;
    //std::map<size_t, ::ThreeComponentDataItem> m3CPDataItems;
    //std::map<size_t, ::ThreeComponentDataItem> m3CSDataItems;
    //std::map<size_t, ::OneComponentDataItem>   m1CPDataItems;
    size_t mMaxInferenceItems{100};
    bool mPS3CInputsAreEqual{true};
    bool mRun3CPDetector{false};
    bool mRun3CSDetector{false};
    bool mRun1CPDetector{false};
    bool mKeepRunning{true};
    bool mInitialized{false};
};

int main(int argc, char *argv[])
{
    // Get the ini file from the command line
    std::string iniFile;
    try 
    {
        iniFile = ::parseCommandLineOptions(argc, argv);
        if (iniFile.empty()){return EXIT_SUCCESS;}
    }
    catch (const std::exception &e) 
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    // Parse the initialization file
    ProgramOptions programOptions;
    try 
    {
        programOptions.parseInitializationFile(iniFile);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    // Create the logger
    constexpr int hour = 0;
    constexpr int minute = 0;
    auto logger = createLogger(programOptions.mModuleName,
                               programOptions.mLogFileDirectory,
                               programOptions.mVerbosity,
                               hour, minute);
    // Create the contexts
    auto generalContext = std::make_shared<UMPS::Messaging::Context> (1);
    auto packetCacheContext = std::make_shared<UMPS::Messaging::Context> (1);
    auto clientContext = std::make_shared<UMPS::Messaging::Context> (1);
    auto publisherContext = std::make_shared<UMPS::Messaging::Context> (1);
    // Initialize the various processes
    logger->info("Initializing processes...");
    UMPS::Modules::ProcessManager processManager(logger);
    try
    {
        namespace UCI = UMPS::Services::ConnectionInformation;
        // Connect to the operator
        logger->debug("Connecting to uOperator...");
        const std::string operatorSection{"uOperator"};
        auto uOperator = UCI::createRequestor(iniFile, operatorSection,
                                              generalContext, logger);
        auto zapOptions = uOperator->getZAPOptions();

        // Create a heartbeat
        logger->debug("Creating heartbeat process...");
        namespace UHeartbeat = UMPS::ProxyBroadcasts::Heartbeat;
        auto heartbeat = UHeartbeat::createHeartbeatProcess(*uOperator, iniFile,
                                                            "Heartbeat",
                                                            generalContext,
                                                            logger);
        processManager.insert(std::move(heartbeat));
        // Create a publisher 
        logger->debug("Creating probability packet publisher...");
        namespace UDP = URTS::Broadcasts::Internal::ProbabilityPacket;
        UDP::PublisherOptions publisherOptions;
        if (!programOptions.mProbabilityPacketBroadcastAddress.empty())
        {
            publisherOptions.setAddress(
                programOptions.mProbabilityPacketBroadcastAddress);
        }
        else
        {
            publisherOptions.setAddress(
                uOperator->getProxyBroadcastFrontendDetails(
                  programOptions.mProbabilityPacketBroadcastName).getAddress());
        }
        publisherOptions.setZAPOptions(zapOptions);
        publisherOptions.setHighWaterMark(
            programOptions.mProbabilityPacketHighWaterMark);
        auto probabilityPublisher
            = std::make_unique<UDP::Publisher> (publisherContext, logger);
//        probabilityPublisher->initialize(publisherOptions);
        programOptions.mProbabilityPacketPublisherOptions = publisherOptions;

        // Create the packet requestor
        logger->debug("Initializing packetCache client...");
        URTS::Services::Scalable::PacketCache::RequestorOptions upcOptions;
        if (!programOptions.mPacketCacheServiceAddress.empty())
        {
            upcOptions.setAddress(programOptions.mPacketCacheServiceAddress);
        }
        else
        {
            upcOptions.setAddress(
               uOperator->getProxyServiceFrontendDetails(
                    programOptions.mPacketCacheServiceName).getAddress()
            );
        }
        upcOptions.setZAPOptions(zapOptions);
        upcOptions.setReceiveTimeOut(programOptions.mDataRequestReceiveTimeOut);
        auto packetCacheRequestor
            = std::make_unique<URTS::Services::Scalable::PacketCache::Requestor>
              (packetCacheContext, logger);
//        packetCacheRequestor->initialize(upcOptions);
        programOptions.mPacketCacheRequestorOptions = upcOptions;

        // Create a 3C P client
        namespace UNetP = UDetectors::UNetThreeComponentP;
        std::unique_ptr<UNetP::Requestor> inference3CPRequestor{nullptr};
        if (programOptions.mRunP3CDetector)
        {
            logger->debug("Initializing P 3C detector client...");
            UNetP::RequestorOptions requestOptions;
            if (!programOptions.mP3CDetectorServiceAddress.empty())
            {
                requestOptions.setAddress(
                    programOptions.mP3CDetectorServiceAddress);
            }
            else
            {
                requestOptions.setAddress(
                    uOperator->getProxyServiceFrontendDetails(
                        programOptions.mP3CDetectorServiceName).getAddress());
            } 
            requestOptions.setZAPOptions(zapOptions);
            requestOptions.setReceiveTimeOut(
                programOptions.mInferenceRequestReceiveTimeOut);
            inference3CPRequestor
                = std::make_unique<UNetP::Requestor> (clientContext, logger);
//            inference3CPRequestor->initialize(requestOptions);
            programOptions.mP3CDetectorRequestorOptions = requestOptions;
        }

        // Create a 3C S client
        namespace UNetS = UDetectors::UNetThreeComponentS;
        std::unique_ptr<UNetS::Requestor> inference3CSRequestor{nullptr};
        if (programOptions.mRunS3CDetector)
        {
            logger->debug("Initializing S 3C detector client...");
            UNetS::RequestorOptions requestOptions;
            if (!programOptions.mS3CDetectorServiceAddress.empty())
            {
                requestOptions.setAddress(
                    programOptions.mS3CDetectorServiceAddress);
            }
            else
            {
                requestOptions.setAddress(
                    uOperator->getProxyServiceFrontendDetails(
                        programOptions.mS3CDetectorServiceName).getAddress());
            }
            requestOptions.setZAPOptions(zapOptions);
            requestOptions.setReceiveTimeOut(
                programOptions.mInferenceRequestReceiveTimeOut);
            inference3CSRequestor
                = std::make_unique<UNetS::Requestor> (clientContext, logger);
//            inference3CSRequestor->initialize(requestOptions);
            programOptions.mS3CDetectorRequestorOptions = requestOptions;
        }

        namespace UNetP1C = UDetectors::UNetOneComponentP;
        if (programOptions.mRunP1CDetector)
        {
            logger->error("1c p in dev");
/*
            UNet1CP::RequestorOptions requestOptions;
            if (!programOptions.mP3CDetectorServiceAddress.empty())
            {
                requestOptions.setAddress(
                    programOptions.mP3CDetectorServiceAddress);
            }
            else
            {
                requestOptions.setAddress(
                    uOperator->getProxyServiceFrontendDetails(
                        programOptions.mP3CDetectorServiceName).getAddress());
            }
            requestOptions.setZAPOptions(zapOptions);
            requestOptions.setReceiveTimeOut(
                programOptions.mInferenceRequestReceiveTimeOut);
            inference1CPRequestor
                = std::make_unique<UNetP1C::Requestor> (clientContext, logger);
            programOptions.mP3CDetectorRequestorOptions = requestOptions;
*/
        }

        // Create the detector
/*
        auto detectorProcess
            = std::make_unique<::Detector> (programOptions,
                                            std::move(probabilityPublisher),
                                            std::move(packetCacheRequestor),
                                            std::move(inference3CPRequestor),
                                            std::move(inference3CSRequestor),
                                            logger);
*/
        auto detectorProcess = std::make_unique<::Detector> (programOptions, logger);
        // Create the remote replier
        logger->debug("Creating module registry replier process...");
        namespace URemoteCommand = UMPS::ProxyServices::Command;
        URemoteCommand::ModuleDetails moduleDetails;
        moduleDetails.setName(programOptions.mModuleName);

        auto callbackFunction = std::bind(&Detector::commandCallback,
                                          &*detectorProcess,
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          std::placeholders::_3);
        auto remoteReplierProcess
            = URemoteCommand::createReplierProcess(*uOperator,
                                                   moduleDetails,
                                                   callbackFunction,
                                                   iniFile,
                                                   "ModuleRegistry",
                                                   nullptr, // Make new context
                                                   logger);
        // Add the remote replier and inference engine
        processManager.insert(std::move(remoteReplierProcess));
        processManager.insert(std::move(detectorProcess));
    }
    catch (const std::exception &e)
    {
        //std::cerr << e.what() << std::endl;
        logger->error(e.what());
        return EXIT_FAILURE;
    }
    // Start the processes
    logger->info("Starting processes...");
    try
    {
        processManager.start();
    }
    catch (const std::exception &e)
    {
        //std::cerr << e.what() << std::endl;
        logger->error(e.what());
        return EXIT_FAILURE;
    }
    // The main thread waits and, when requested, sends a stop to all processes
    logger->info("Starting main thread...");
    processManager.handleMainThread();
    return EXIT_SUCCESS;
}
