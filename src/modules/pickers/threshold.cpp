#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>
#include <map>
#include <chrono>
#ifndef NDEBUG
#include <cassert>
#endif
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/services/command/availableCommandsRequest.hpp>
#include <umps/services/command/availableCommandsResponse.hpp>
#include <umps/services/command/commandRequest.hpp>
#include <umps/services/command/commandResponse.hpp>
#include <umps/services/command/service.hpp>
#include <umps/services/command/serviceOptions.hpp>
#include <umps/services/command/terminateRequest.hpp>
#include <umps/services/command/terminateResponse.hpp>
#include <umps/services/connectionInformation/requestorOptions.hpp>
#include <umps/services/connectionInformation/requestor.hpp>
#include <umps/services/connectionInformation/details.hpp>
#include <umps/services/connectionInformation/socketDetails/proxy.hpp>
#include <umps/services/connectionInformation/socketDetails/router.hpp>
#include <umps/services/connectionInformation/socketDetails/xSubscriber.hpp>
#include <umps/services/connectionInformation/socketDetails/xPublisher.hpp>
#include <umps/proxyServices/command/moduleDetails.hpp>
#include <umps/proxyServices/command/replier.hpp>
#include <umps/proxyServices/command/replierOptions.hpp>
#include <umps/proxyServices/command/replierProcess.hpp>
#include <umps/logging/dailyFile.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/modules/process.hpp>
#include <umps/modules/processManager.hpp>
#include <umps/proxyBroadcasts/heartbeat/publisherProcess.hpp>
#include <umps/proxyBroadcasts/heartbeat/publisherProcessOptions.hpp>
#include "urts/broadcasts/internal/pick.hpp"
#include "urts/broadcasts/internal/probabilityPacket.hpp"
#include "urts/services/standalone/incrementer.hpp"
#include "thresholdDetectorOptions.hpp"
#include "thresholdDetector.hpp"
#include "triggerWindow.hpp"
#include "private/isEmpty.hpp"
#include "private/threadSafeQueue.hpp"

#define MODULE_NAME "ThresholdPicker"

namespace
{

/// @result Gets the command line input options as a string.
[[nodiscard]] std::string getInputOptions() noexcept
{
    std::string commands{
R"""(
Commands: 
   help    Displays this message.
)"""};
    return commands;
}

enum class PhaseType : int
{
    Unknown = 0,
    P = 1,
    S = 2
};

/// @result Converts a trigger window and probability packet to a pick.
template<typename T>
[[nodiscard]]
URTS::Broadcasts::Internal::Pick::Pick
triggerWindowToPick(
    const URTS::Modules::Pickers::TriggerWindow<T> &triggerWindow,
    const URTS::Broadcasts::Internal::ProbabilityPacket::
                                      ProbabilityPacket &packet,
    const PhaseType phaseType,
    const int64_t pickIdentifier)
{
    URTS::Broadcasts::Internal::Pick::Pick result;
    URTS::Broadcasts::Internal::Pick::UncertaintyBound lowerBound;
    URTS::Broadcasts::Internal::Pick::UncertaintyBound upperBound;
    lowerBound.setPercentile(15.9);
    upperBound.setPercentile(84.1);
    result.setReviewStatus(URTS::Broadcasts::Internal::Pick::
                                 Pick::ReviewStatus::Automatic);
    result.setFirstMotion(URTS::Broadcasts::Internal::Pick::
                                Pick::FirstMotion::Unknown);
    result.setIdentifier(pickIdentifier);
    result.setNetwork(packet.getNetwork());
    result.setStation(packet.getStation());
    result.setChannel(packet.getChannel());
    result.setOriginalChannels(packet.getOriginalChannels());
    result.setLocationCode(packet.getLocationCode());
    result.setTime(triggerWindow.getMaximum().first);
    std::vector<std::string> algorithms;
    auto algorithm = packet.getAlgorithm();
    if (!algorithm.empty()){algorithms.push_back(algorithm);}
    algorithms.push_back("ThresholdPicker");
    result.setProcessingAlgorithms(algorithms);
    if (phaseType == ::PhaseType::P)
    {
        lowerBound.setPerturbation(std::chrono::microseconds{-50000});
        upperBound.setPerturbation(std::chrono::microseconds{ 50000});
        result.setLowerAndUpperUncertaintyBound(
            std::pair{lowerBound, upperBound});
        result.setPhaseHint("P");
    }
    else if (phaseType == ::PhaseType::S)
    {
        lowerBound.setPerturbation(std::chrono::microseconds{-100000});
        upperBound.setPerturbation(std::chrono::microseconds{ 100000});
        result.setLowerAndUpperUncertaintyBound(
            std::pair{lowerBound, upperBound}); 
        result.setPhaseHint("S");
    }
    return result;
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
    //auto logger = std::make_shared<UMPS::Logging::StandardOut> (verbosity);
    auto logger = std::make_shared<UMPS::Logging::DailyFile> ();  
    logger->initialize(moduleName,
                       fullLogFileName,
                       verbosity,
                       hour, minute);
    logger->info("Starting logging for " + moduleName);
    return logger;
}

struct ProgramOptions
{
public:
    ProgramOptions()
    {
        mPThresholdDetectorOptions.setOnThreshold(0.9);
        mPThresholdDetectorOptions.setOffThreshold(0.72);
        mPThresholdDetectorOptions.setMinimumGapSize(5);

        mSThresholdDetectorOptions.setOnThreshold(0.92);
        mSThresholdDetectorOptions.setOffThreshold(0.72);
        mSThresholdDetectorOptions.setMinimumGapSize(5);
    }
    /// @brief Load the module options from an initialization file.
    void parseInitializationFile(const std::string &iniFile)
    {
        boost::property_tree::ptree propertyTree;
        boost::property_tree::ini_parser::read_ini(iniFile, propertyTree);
        //----------------------------- General ------------------------------//
        // Module name
        mModuleName
            = propertyTree.get<std::string> ("General.moduleName",
                                             mModuleName);
        if (mModuleName.empty())
        {
            throw std::runtime_error("Module name not defined");
        }
        // Verbosity
        mVerbosity = static_cast<UMPS::Logging::Level>
                     (propertyTree.get<int> ("General.verbose",
                                             static_cast<int> (mVerbosity)));
        // Log file directory
        mLogFileDirectory
            = propertyTree.get<std::string> ("General.logFileDirectory",
                                             mLogFileDirectory.string());
        if (!mLogFileDirectory.empty() &&
            !std::filesystem::exists(mLogFileDirectory))
        {
            std::cout << "Creating log file directory: "
                      << mLogFileDirectory << std::endl;
            if (!std::filesystem::create_directories(mLogFileDirectory))
            {
                throw std::runtime_error("Failed to make log directory");
            }
        }
        //-------------------------- Detector Options ------------------------//
        mProbabilityPacketBroadcastName
            = propertyTree.get<std::string> (
                 "ThresholdPicker.probabilityPacketBroadcastName",
                 mProbabilityPacketBroadcastName);
        mProbabilityPacketBroadcastAddress
            = propertyTree.get<std::string> (
                 "ThresholdPicker.probabilityPacketBroadcastAddress",
                 mProbabilityPacketBroadcastAddress);
        if (::isEmpty(mProbabilityPacketBroadcastName) &&
            ::isEmpty(mProbabilityPacketBroadcastAddress))
        {
            throw std::runtime_error("Probability broadcast indeterminable");
        }

        mPickBroadcastName
            = propertyTree.get<std::string> (
                "ThresholdPicker.pickBroadcastName",
                mPickBroadcastName);
        mPickBroadcastAddress
            = propertyTree.get<std::string> (
                "ThresholdPicker.pickBroadcastAddress",
                mPickBroadcastAddress);
        if (::isEmpty(mPickBroadcastName) &&
            ::isEmpty(mPickBroadcastAddress))
        {
            throw std::runtime_error("Pick broadcast indeterminable");
        }

        mIncrementerServiceName
            = propertyTree.get<std::string> (
                "ThresholdPicker.incrementerServiceName",
                mIncrementerServiceName);
        mIncrementerServiceAddress
            = propertyTree.get<std::string> (
                "ThresholdPicker.incrementerServiceAddress",
                mIncrementerServiceAddress);
        if (::isEmpty(mIncrementerServiceName) &&
            ::isEmpty(mIncrementerServiceAddress))
        {
            throw std::runtime_error("Incrementer service indeterminable");
        }

        // P on/off thresholds
        auto onThreshold
            = propertyTree.get<double> ("ThresholdDetector.pOnThreshold",
                                   mPThresholdDetectorOptions.getOnThreshold());
        mPThresholdDetectorOptions.setOnThreshold(onThreshold);
        auto offThreshold
            = propertyTree.get<double> ("ThresholdDetector.pOffThreshold",
                                   mPThresholdDetectorOptions.getOnThreshold());
        mPThresholdDetectorOptions.setOffThreshold(offThreshold);
        auto gapSize
            = propertyTree.get<int> ("ThresholdDetector.pMinimumGapSize",
                               mPThresholdDetectorOptions.getMinimumGapSize());
        gapSize = std::max(0, gapSize);
        mPThresholdDetectorOptions.setMinimumGapSize(gapSize);

        // S on/off thresholds
        onThreshold
            = propertyTree.get<double> ("ThresholdDetector.sOnThreshold",
                                   mSThresholdDetectorOptions.getOnThreshold());
        mSThresholdDetectorOptions.setOnThreshold(onThreshold);
        offThreshold
            = propertyTree.get<double> ("ThresholdDetector.sOffThreshold",
                                   mSThresholdDetectorOptions.getOnThreshold());
        mSThresholdDetectorOptions.setOffThreshold(offThreshold);
        gapSize
            = propertyTree.get<int> ("ThresholdDetector.sMinimumGapSize",
                               mSThresholdDetectorOptions.getMinimumGapSize());
        gapSize = std::max(0, gapSize);
        mSThresholdDetectorOptions.setMinimumGapSize(gapSize);

        // Increment time out
        auto requestTimeOut
            = propertyTree.get<int>
                ("ThresholdDetector.incrementRequestTimeOut",
                 mIncrementRequestReceiveTimeOut.count());
        if (requestTimeOut < 0)
        {
            throw std::invalid_argument("Requests cannot indefinitely block");
        }
        mIncrementRequestReceiveTimeOut
            = std::chrono::milliseconds {requestTimeOut};

        auto packetReceiveTimeOut
            = propertyTree.get<int>
                ("ThresholdDetector.receiveTimeOut",
                 mProbabilityPacketReceiveTimeOut.count());
        packetReceiveTimeOut = std::max(0, packetReceiveTimeOut);
        mProbabilityPacketReceiveTimeOut 
            = std::chrono::milliseconds {packetReceiveTimeOut};
    }
//public:
    URTS::Broadcasts::Internal::ProbabilityPacket::SubscriberOptions
        mProbabilityPacketSubscriberOptions;
    URTS::Broadcasts::Internal::Pick::PublisherOptions mPickPublisherOptions;
    URTS::Services::Standalone::Incrementer::RequestorOptions
        mIncrementerRequestorOptions;
    URTS::Modules::Pickers::ThresholdDetectorOptions mPThresholdDetectorOptions;
    URTS::Modules::Pickers::ThresholdDetectorOptions mSThresholdDetectorOptions;
    std::string mModuleName{MODULE_NAME};
    std::string mProbabilityPacketBroadcastName{"ProbabilityPacket"};
    std::string mProbabilityPacketBroadcastAddress;
    std::string mPickBroadcastName{"Pick"};
    std::string mPickBroadcastAddress;
    std::string mIncrementerServiceName{"Incrementer"};
    std::string mIncrementerServiceAddress;
    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    std::chrono::milliseconds mIncrementRequestReceiveTimeOut{1000}; // 1 s
    std::chrono::milliseconds mProbabilityPacketReceiveTimeOut{10};
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
};

[[nodiscard]]
std::pair<std::string, PhaseType>
getNameAndPhaseType(const URTS::Broadcasts::Internal::ProbabilityPacket::
                                ProbabilityPacket &packet)
{
    ::PhaseType phaseType{::PhaseType::Unknown};
    std::string name;
    auto positiveClassName = packet.getPositiveClassName();
    if (positiveClassName == "P")
    {
        phaseType = ::PhaseType::P;
    }
    else if (positiveClassName == "S")
    {
        phaseType = ::PhaseType::S;
    }
    else
    {
        throw std::invalid_argument("Unhandled positive class name: "
                                  + positiveClassName);
    }
    if (!packet.haveNetwork()){throw std::invalid_argument("Network not set");}
    if (!packet.haveStation()){throw std::invalid_argument("Station not set");}
    if (!packet.haveChannel()){throw std::invalid_argument("Channel not set");}
    std::string locationCode{"--"};
    if (packet.haveLocationCode()){locationCode = packet.getLocationCode();} 
    name = packet.getNetwork() + "." + packet.getStation() + "."
         + packet.getChannel() + "." + locationCode;
    return std::pair {name, phaseType};
}

class ThresholdPicker : public UMPS::Modules::IProcess
{
public:
    ThresholdPicker(const ::ProgramOptions &options,
                    std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mOptions(options),
        mLogger(logger),
        mContext(std::make_shared<UMPS::Messaging::Context> (1))
    {
        // Create the probability packet subscriber
        mProbabilityPacketSubscriber
            = std::make_unique<URTS::Broadcasts::Internal::ProbabilityPacket::
                               Subscriber> (mContext, mLogger);
        mProbabilityPacketSubscriber->initialize(
            mOptions.mProbabilityPacketSubscriberOptions);

        // Create the pick publisher
        mPickPublisher
            = std::make_unique<URTS::Broadcasts::Internal::
                               Pick::Publisher> (mContext, mLogger);
        mPickPublisher->initialize(mOptions.mPickPublisherOptions);
        // Create the incrementer
        mIncrementerRequestor
            = std::make_unique<URTS::Services::Standalone::
                               Incrementer::Requestor> (mContext, mLogger);
        mIncrementerRequestor->initialize(
            mOptions.mIncrementerRequestorOptions);
        std::this_thread::sleep_for(std::chrono::milliseconds {100});
        // Check everything started
        if (!mProbabilityPacketSubscriber->isInitialized())
        {
            throw std::runtime_error("Probality subscriber not initialized");
        }
        if (!mPickPublisher->isInitialized())
        {
            throw std::runtime_error("Pick publisher not initialized");
        }
        if (!mIncrementerRequestor->isInitialized())
        {
            throw std::runtime_error("Incrementer requestor not initialized");
        }

        // Instantiate the local command replier
        mLocalCommand
            = std::make_unique<UMPS::Services::Command::Service> (mLogger);
        UMPS::Services::Command::ServiceOptions localServiceOptions;
        localServiceOptions.setModuleName(getName());
        localServiceOptions.setCallback(
            std::bind(&ThresholdPicker::commandCallback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
        mLocalCommand->initialize(localServiceOptions);
        mInitialized = true;
    }
    /// @brief Destructor
    ~ThresholdPicker() override
    {
        stop();
    } 
    /// @result True indicates this should keep running
    [[nodiscard]] bool keepRunning() const
    {
        return mKeepRunning;
    }
    /// @brief Toggles this as running or not running
    void setRunning(const bool running)
    {
        mKeepRunning = running;
    }
    /// @result True indicates this is running.
    [[nodiscard]] bool isRunning() const noexcept override
    {
        return keepRunning();
    }
    /// @result Initialized?
    [[nodiscard]] bool isInitialized() const noexcept
    {
        return mInitialized;
    }
    /// @brief Stops the modules
    void stop() override
    {
        setRunning(false);
        if (mPickPublisherThread.joinable()){mPickPublisherThread.join();}
        if (mPickerThread.joinable()){mPickerThread.join();}
        if (mPacketSubscriberThread.joinable()){mPacketSubscriberThread.join();}
        if (mLocalCommand != nullptr)
        {
            if (mLocalCommand->isRunning()){mLocalCommand->stop();}
        }
    }
    /// @brief Starts the modules.
    void start() override
    {
        stop();
        if (!isInitialized())
        {
            throw std::runtime_error("Class not initialized");
        }
        setRunning(true);
        std::this_thread::sleep_for(std::chrono::milliseconds {10});
        // Make thread that fetches probability packets
        mLogger->debug("Starting the pick publisher thread...");
        mPickPublisherThread
             = std::thread(&::ThresholdPicker::publishPicks, this);
        mLogger->debug("Starting the packet processing thread...");
        mPickerThread
             = std::thread(&::ThresholdPicker::processProbabilityPackets, this);
        mLogger->debug("Starting the packet subscriber thread...");
        mPacketSubscriberThread
            = std::thread(&::ThresholdPicker::getPackets, this);
        mLogger->debug("Starting the local command proxy..."); 
        mLocalCommand->start();
    }
    /// @brief Reads packets
    void getPackets()
    {
        while (keepRunning())
        {
            auto packet = mProbabilityPacketSubscriber->receive();
            if (packet != nullptr)
            {
                mProbabilityPacketQueue.push(std::move(*packet));
            }
        }
    }
    /// @brief Processes probability packets
    void processProbabilityPackets()
    {
        URTS::Services::Standalone::Incrementer::IncrementRequest
            incrementRequest;
        incrementRequest.setItem(URTS::Services::Standalone::
                                Incrementer::IncrementRequest::Item::PhasePick);
        int64_t incrementRequestIdentifier{0};
        std::vector<URTS::Modules::Pickers::TriggerWindow<double>>
            triggerWindows;
        std::pair<std::string, ::PhaseType> nameAndPhaseType;
        std::chrono::milliseconds timeOut{10};
        while (keepRunning())
        {
            URTS::Broadcasts::Internal::ProbabilityPacket::
                ProbabilityPacket packet;
            auto gotPacket
                = mProbabilityPacketQueue.wait_until_and_pop(&packet, timeOut);
            if (gotPacket)
            {
                try
                {
                    nameAndPhaseType = ::getNameAndPhaseType(packet);
                }
                catch (const std::exception &e)
                {
                    mLogger->error(e.what());
                    continue;
                }
                const auto &name = nameAndPhaseType.first;
                const auto phaseType = nameAndPhaseType.second;
                if (phaseType == ::PhaseType::P)
                {
                    auto it = mPDetectors.find(name);    
                    if (it == mPDetectors.end())
                    {
                        URTS::Modules::Pickers::ThresholdDetector detector{mLogger};
                        try
                        {
                            mLogger->info("Adding P threshold detector: "
                                        + name);
                            detector.initialize(
                                mOptions.mPThresholdDetectorOptions);
                            mPDetectors.insert( {name, detector} );
                        }
                        catch (const std::exception &e)
                        {
                            std::string errorMessage
                                = "Failed to initialize P detector for " + name
                                + ".  Failed with: " + std::string{e.what()};
                            mLogger->error(errorMessage);
                            continue;
                        }
#ifndef NDEBUG
                        assert(mPDetectors.find(name) != mPDetectors.end());
#endif
                    }
                    it = mPDetectors.find(name);
                    if (it != mPDetectors.end())
                    {
                        triggerWindows.clear();
                        try
                        {
                            it->second.apply(packet, &triggerWindows);
                        }
                        catch (const std::exception &e)
                        {
                            std::string errorMessage
                                = "Failed to update P detector for " + name
                                + ".  Failed with: " + std::string{e.what()};
                            mLogger->error(errorMessage);
                            continue;
                        }
                        for (const auto &triggerWindow : triggerWindows)
                        {
                            int64_t pickIdentifier{0};
                            incrementRequestIdentifier
                                 = incrementRequestIdentifier + 1;
                            if (incrementRequestIdentifier >
                                std::numeric_limits<int64_t>::max() - 10)
                            {
                                incrementRequestIdentifier = 0;
                            } 
                            incrementRequest.setIdentifier(
                                  incrementRequestIdentifier);
                            try
                            {
                                auto incrementResponse 
                                    = mIncrementerRequestor->request(
                                        incrementRequest);
                                pickIdentifier = incrementResponse->getValue();
                            }
                            catch (const std::exception &e)
                            {
                                std::string errorMessage
                                     = "P increment request failed for " + name
                                     + ".  Failed with: "
                                     + std::string{e.what()};
                                mLogger->error(errorMessage);
                            }
                            try
                            {
                                auto pick
                                    = ::triggerWindowToPick(triggerWindow,
                                                            packet,
                                                            ::PhaseType::P,
                                                            pickIdentifier);
                                mPickPublisherQueue.push(std::move(pick));
                            }
                            catch (const std::exception &e)
                            {
                                std::string errorMessage
                                    = "Failed to make P pick for " + name
                                    + ".  Failed with: "
                                    + std::string{e.what()};
                                mLogger->error(errorMessage);
                            }
                        } // Loop on trigger windows
                    } // End check on detector found
                }
                else if (phaseType == ::PhaseType::S)
                {
                    auto it = mSDetectors.find(name);
                    if (it == mSDetectors.end())
                    {
                        URTS::Modules::Pickers::ThresholdDetector detector{mLogger};
                        try
                        {
                            mLogger->info("Adding S threshold detector: "
                                        + name);
                            detector.initialize(
                                mOptions.mSThresholdDetectorOptions);
                            mSDetectors.insert( {name, detector} );
                        }
                        catch (const std::exception &e) 
                        {
                            std::string errorMessage
                                = "Failed to initialize S detector for " + name
                                + ".  Failed with: " + std::string{e.what()};
                            mLogger->error(errorMessage);
                            continue;
                        }
#ifndef NDEBUG
                        assert(mSDetectors.find(name) != mSDetectors.end());
#endif
                    }
                    it = mSDetectors.find(name);
                    if (it != mSDetectors.end())
                    {
                        triggerWindows.clear();
                        try
                        {   
                            it->second.apply(packet, &triggerWindows);
                        }
                        catch (const std::exception &e)
                        {
                            std::string errorMessage
                                = "Failed to update S detector for " + name
                                + ".  Failed with: " + std::string{e.what()};
                            mLogger->error(errorMessage);
                            continue;
                        }
                        for (const auto &triggerWindow : triggerWindows)
                        {
                            int64_t pickIdentifier{0};
                            incrementRequestIdentifier
                                 = incrementRequestIdentifier + 1;
                            if (incrementRequestIdentifier >
                                std::numeric_limits<int64_t>::max() - 10)
                            {
                                incrementRequestIdentifier = 0;
                            }
                            incrementRequest.setIdentifier(
                                  incrementRequestIdentifier);
                            try
                            {
                                auto incrementResponse 
                                    = mIncrementerRequestor->request(
                                        incrementRequest);
                                pickIdentifier = incrementResponse->getValue();
                            }
                            catch (const std::exception &e)
                            {
                                std::string errorMessage
                                     = "S increment request failed for " + name
                                     + ".  Failed with: "
                                     + std::string{e.what()};
                                mLogger->error(errorMessage);
                            }
                            try
                            {
                                auto pick
                                    = ::triggerWindowToPick(triggerWindow,
                                                            packet,
                                                            ::PhaseType::S,
                                                            pickIdentifier);
                                mPickPublisherQueue.push(std::move(pick));
                            }
                            catch (const std::exception &e)
                            {
                                std::string errorMessage
                                    = "Failed to make S pick for " + name
                                    + ".  Failed with: "
                                    + std::string{e.what()};
                                mLogger->error(errorMessage);
                            }
                        } // Loop on trigger windows
                    }
                }
                else
                {
                    mLogger->error("Unhandled phase type");
                    continue;
                }
            }
        }
    }
    /// @brief Publishes picks
    void publishPicks()
    {
        std::chrono::milliseconds timeOut{10};
        while (keepRunning())
        {
            URTS::Broadcasts::Internal::Pick::Pick pick;
            auto gotPick
                = mPickPublisherQueue.wait_until_and_pop(&pick, timeOut);
            if (gotPick)
            {
                try
                {
                    mPickPublisher->send(pick);
                }
                catch (const std::exception &e)
                {
                    mLogger->error("Failed to send pick.  Failed with "
                                 + std::string{e.what()});
                }
            }
        }
    }
    /// @brief Callback function
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
            response.setCommands(::getInputOptions());
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
            if (command != "help")
            {
                mLogger->debug("Invalid command: " + command);
                response.setResponse("Invalid command: " + command);
                response.setReturnCode(
                    USC::CommandResponse::ReturnCode::InvalidCommand);
            }
            else
            {
                response.setResponse(::getInputOptions());
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
//private:
    ::ProgramOptions mOptions;
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::shared_ptr<UMPS::Messaging::Context> mContext{nullptr};
    std::unique_ptr<URTS::Broadcasts::Internal::Pick::Publisher>
        mPickPublisher{nullptr};
    std::unique_ptr<URTS::Broadcasts::Internal::ProbabilityPacket::Subscriber>
        mProbabilityPacketSubscriber{nullptr};
    std::unique_ptr<URTS::Services::Standalone::Incrementer::Requestor>
        mIncrementerRequestor{nullptr};
    std::unique_ptr<UMPS::Services::Command::Service> mLocalCommand{nullptr};
    ::ThreadSafeQueue<URTS::Broadcasts::Internal::ProbabilityPacket
                          ::ProbabilityPacket> mProbabilityPacketQueue;
    ::ThreadSafeQueue<URTS::Broadcasts::Internal::Pick::Pick>
        mPickPublisherQueue; 
    std::thread mPacketSubscriberThread;
    std::thread mPickerThread;
    std::thread mPickPublisherThread;
    std::map<std::string, URTS::Modules::Pickers::ThresholdDetector>
        mPDetectors; 
    std::map<std::string, URTS::Modules::Pickers::ThresholdDetector>
        mSDetectors;
    std::atomic<bool> mKeepRunning{true};
    std::atomic<bool> mInitialized{false};
};

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

}

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
    auto logger = ::createLogger(programOptions.mModuleName,
                                 programOptions.mLogFileDirectory,
                                 programOptions.mVerbosity,
                                 hour, minute);

    // Create the contexts
    auto context = std::make_shared<UMPS::Messaging::Context> (1);
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
                                              context, logger);
        auto zapOptions = uOperator->getZAPOptions();

        // Create a heartbeat
        logger->debug("Creating heartbeat process...");
        namespace UHeartbeat = UMPS::ProxyBroadcasts::Heartbeat;
        auto heartbeat = UHeartbeat::createHeartbeatProcess(*uOperator, iniFile,
                                                            "Heartbeat",
                                                            context,
                                                            logger);
        processManager.insert(std::move(heartbeat));

        // Probability packet subscriber
        URTS::Broadcasts::Internal::ProbabilityPacket::SubscriberOptions
            subscriberOptions;
        if (!programOptions.mProbabilityPacketBroadcastAddress.empty())
        {
            subscriberOptions.setAddress(
                programOptions.mProbabilityPacketBroadcastAddress);
        }
        else
        {
            logger->debug("Fetching probability subscriber address..."); 
            subscriberOptions.setAddress(
                uOperator->getProxyBroadcastBackendDetails(
                  programOptions.mProbabilityPacketBroadcastName).getAddress());
        }
        subscriberOptions.setZAPOptions(zapOptions);
        subscriberOptions.setTimeOut(
            programOptions.mProbabilityPacketReceiveTimeOut);
        subscriberOptions.setHighWaterMark(0); // Infinite
        programOptions.mProbabilityPacketSubscriberOptions = subscriberOptions;

        // Pick publisher
        logger->debug("Creating pick publisher...");
        namespace UPick = URTS::Broadcasts::Internal::Pick;
        URTS::Broadcasts::Internal::Pick::PublisherOptions publisherOptions;
        if (!programOptions.mProbabilityPacketBroadcastAddress.empty())
        {
            publisherOptions.setAddress(
                programOptions.mPickBroadcastAddress);
        }
        else
        {
            publisherOptions.setAddress(
                uOperator->getProxyBroadcastFrontendDetails(
                  programOptions.mPickBroadcastName).getAddress());
        }
        publisherOptions.setZAPOptions(zapOptions);
        publisherOptions.setHighWaterMark(0); // Infinite 
        programOptions.mPickPublisherOptions = publisherOptions;

        // Incrementer requestor 
        URTS::Services::Standalone::Incrementer::RequestorOptions irOptions;
        if (!programOptions.mIncrementerServiceAddress.empty())
        {
            irOptions.setAddress(programOptions.mIncrementerServiceAddress);
        }
        else
        {
            logger->debug("Fetching incrementer address...");
            irOptions.setAddress(
               uOperator->getProxyServiceFrontendDetails(
                   programOptions.mIncrementerServiceName).getAddress()
            );
        }
        irOptions.setReceiveTimeOut(
            programOptions.mIncrementRequestReceiveTimeOut);
        irOptions.setZAPOptions(zapOptions);
        programOptions.mIncrementerRequestorOptions = irOptions;

        // Create the picker process
        auto pickerProcess
            = std::make_unique<::ThresholdPicker> (programOptions, logger);

        // Create the remote replier
        logger->debug("Creating module registry replier process...");
        namespace URemoteCommand = UMPS::ProxyServices::Command;
        URemoteCommand::ModuleDetails moduleDetails;
        moduleDetails.setName(programOptions.mModuleName);
        auto callbackFunction = std::bind(&ThresholdPicker::commandCallback,
                                          &*pickerProcess,
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
        processManager.insert(std::move(pickerProcess));
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
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
        std::cerr << e.what() << std::endl;
        logger->error(e.what());
        return EXIT_FAILURE;
    }
    // The main thread waits and, when requested, sends a stop to all processes
    logger->info("Starting main thread...");
    processManager.handleMainThread();
    return EXIT_SUCCESS;
}
