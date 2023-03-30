#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <thread>
#include <mutex>
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
#include <umps/services/connectionInformation/socketDetails/xPublisher.hpp>
#include <umps/services/connectionInformation/socketDetails/xSubscriber.hpp>
#include <umps/services/command/availableCommandsRequest.hpp>
#include <umps/services/command/availableCommandsResponse.hpp>
#include <umps/services/command/commandRequest.hpp>
#include <umps/services/command/commandResponse.hpp>
#include <umps/services/command/service.hpp>
#include <umps/services/command/serviceOptions.hpp>
#include <umps/services/command/terminateRequest.hpp>
#include <umps/services/command/terminateResponse.hpp>
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "urts/broadcasts/internal/dataPacket/subscriber.hpp"
#include "urts/broadcasts/internal/dataPacket/subscriberOptions.hpp"
#include "urts/broadcasts/internal/pick/pick.hpp"
#include "urts/broadcasts/internal/pick/publisher.hpp"
#include "urts/broadcasts/internal/pick/publisherOptions.hpp"
#include "urts/database/aqms/channelDataTablePoller.hpp"
#include "urts/database/aqms/channelDataTable.hpp"
#include "urts/database/aqms/channelData.hpp"
#include "urts/database/connection/postgresql.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/processingRequest.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/processingResponse.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/requestor.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/requestorOptions.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS/processingRequest.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS/processingResponse.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS/requestor.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS/requestorOptions.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/processingRequest.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/processingResponse.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/requestor.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/requestorOptions.hpp"
#include "urts/services/scalable/packetCache/bulkDataRequest.hpp"
#include "urts/services/scalable/packetCache/bulkDataResponse.hpp"
#include "urts/services/scalable/packetCache/dataRequest.hpp"
#include "urts/services/scalable/packetCache/dataResponse.hpp"
#include "urts/services/scalable/packetCache/requestor.hpp"
#include "urts/services/scalable/packetCache/requestorOptions.hpp"
#include "urts/services/scalable/packetCache/threeComponentWaveform.hpp"
#include "programOptions.hpp"
#include "pPickerPipeline.hpp"
#include "thresholdDetectorOptions.hpp"
#include "private/threadSafeQueue.hpp"
#include "private/isEmpty.hpp"

#define MODULE_NAME "MLPicker"
namespace UDatabase = URTS::Database;
namespace UPickers = URTS::Services::Scalable::Pickers;
namespace UFM = URTS::Services::Scalable::FirstMotionClassifiers;

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

///--------------------------------------------------------------------------///
///                                 Picker                                   ///
///--------------------------------------------------------------------------///
/// @brief This does the heavy lifting and runs all the ML models.
class Picker : public UMPS::Modules::IProcess
{
public:
    Picker(const ::ProgramOptions &programOptions,
           std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mProgramOptions(programOptions),
        mLogger(logger)
    {
        // Instantiate the local command replier
        mLocalCommand
            = std::make_unique<UMPS::Services::Command::Service> (mLogger);
        UMPS::Services::Command::ServiceOptions localServiceOptions;
        localServiceOptions.setModuleName(getName());
        localServiceOptions.setCallback(
            std::bind(&Picker::commandCallback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
        mLocalCommand->initialize(localServiceOptions);
        mInitialized = true;
    }
    /// Destructor
    ~Picker() override
    {
        stop();
    } 
    /// Stop
    void stop() override
    {
        setRunning(false);
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
            if (command != "help")
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
//private:
    mutable std::mutex mMutex;
    ::ProgramOptions mProgramOptions;
    std::string mModuleName{MODULE_NAME};
    std::unique_ptr<URTS::Broadcasts::Internal::DataPacket::Subscriber>
        mProbabilitySubscriber{nullptr};
    std::unique_ptr<URTS::Services::Scalable::PacketCache::Requestor>
        mPacketCacheRequestor{nullptr};
    std::unique_ptr<UPickers::CNNOneComponentP::Requestor>
        mPPickerInferenceRequestor{nullptr};
    std::unique_ptr<UPickers::CNNThreeComponentS::Requestor>
        mSPickerInferenceRequestor{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<UMPS::Services::Command::Service> mLocalCommand{nullptr};
    ::ThreadSafeQueue<size_t> mInferenceQueue;
    bool mKeepRunning{true};
    bool mInitialized{false};
};
/// @brief Parses the command line options.
[[nodiscard]] std::string parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(
R"""(
The mlPicker refines preliminary P and S picks generated by the mlDetector.
Example usage:
    mlPickerDetector --ini=mlPicker.ini
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
    ::ProgramOptions programOptions;
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
    auto pubSubContext = std::make_shared<UMPS::Messaging::Context> (1);
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

        // Probability packet subscriber
        URTS::Broadcasts::Internal::DataPacket::SubscriberOptions
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
        subscriberOptions.setHighWaterMark(0); // Infinite 
        programOptions.mDataPacketSubscriberOptions = subscriberOptions;

        // Packet cache requestor
        URTS::Services::Scalable::PacketCache::RequestorOptions upcOptions;
        if (!programOptions.mPacketCacheServiceAddress.empty())
        {
            upcOptions.setAddress(programOptions.mPacketCacheServiceAddress);
        }
        else
        {
            logger->debug("Fetching packetCache address...");
            upcOptions.setAddress(
               uOperator->getProxyServiceFrontendDetails(
                    programOptions.mPacketCacheServiceName).getAddress()
            );
        }
        upcOptions.setZAPOptions(zapOptions);
        upcOptions.setReceiveTimeOut(programOptions.mDataRequestReceiveTimeOut);
        programOptions.mPacketCacheRequestorOptions = upcOptions;

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
        irOptions.setZAPOptions(zapOptions);
        programOptions.mIncrementerRequestorOptions = irOptions;

        // P picker client
        URTS::Services::Scalable::Pickers::CNNOneComponentP::
              RequestorOptions pOptions;
        if (programOptions.mRunPPicker)
        {
            URTS::Services::Scalable::Pickers::CNNOneComponentP
                ::RequestorOptions pOptions;
            if (!programOptions.mPPickerServiceAddress.empty())
            {
                pOptions.setAddress(programOptions.mPPickerServiceAddress);
            }
            else
            {
                logger->debug("Fetching P picker address...");
                pOptions.setAddress(
                    uOperator->getProxyServiceFrontendDetails(
                       programOptions.mPPickerServiceName).getAddress()
                );
            }
            pOptions.setZAPOptions(zapOptions);
            pOptions.setReceiveTimeOut(
                programOptions.mInferenceRequestReceiveTimeOut);
            programOptions.mPPickerRequestorOptions = pOptions;
        }
        // S picker client
        URTS::Services::Scalable::Pickers::CNNThreeComponentS::
              RequestorOptions sOptions;
        if (programOptions.mRunSPicker)
        {
            URTS::Services::Scalable::Pickers::CNNThreeComponentS
                ::RequestorOptions sOptions;
            if (!programOptions.mSPickerServiceAddress.empty())
            {
                sOptions.setAddress(programOptions.mSPickerServiceAddress);
            }
            else
            {
                logger->debug("Fetching S picker address...");
                sOptions.setAddress(
                    uOperator->getProxyServiceFrontendDetails(
                       programOptions.mSPickerServiceName).getAddress()

                );
            }
            sOptions.setZAPOptions(zapOptions);
            sOptions.setReceiveTimeOut(
                programOptions.mInferenceRequestReceiveTimeOut);
            programOptions.mSPickerRequestorOptions = sOptions;
        }

        // First motion classifier client
        URTS::Services::Scalable::FirstMotionClassifiers::
              CNNOneComponentP::RequestorOptions fmOptions;
        if (programOptions.mRunFirstMotionClassifier)
        {
            URTS::Services::Scalable::FirstMotionClassifiers::CNNOneComponentP
                ::RequestorOptions fmOptions;
            if (!programOptions.mFirstMotionClassifierServiceAddress.empty())
            {
                fmOptions.setAddress(
                    programOptions.mFirstMotionClassifierServiceAddress);
            }
            else
            {
                logger->debug("Fetching first motion classifier address...");
                fmOptions.setAddress(
                    uOperator->getProxyServiceFrontendDetails(
                       programOptions.mFirstMotionClassifierServiceName)
                                     .getAddress()

                );
            }
            fmOptions.setZAPOptions(zapOptions);
            fmOptions.setReceiveTimeOut(
                programOptions.mInferenceRequestReceiveTimeOut);
            programOptions.mFirstMotionClassifierRequestorOptions = fmOptions;
        }

        // Create the remote replier
/*
        logger->debug("Creating module registry replier process...");
        namespace URemoteCommand = UMPS::ProxyServices::Command;
        URemoteCommand::ModuleDetails moduleDetails;
        moduleDetails.setName(programOptions.mModuleName);
        auto callbackFunction = std::bind(&Picker::commandCallback,
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
*/
        // Add the remote replier and inference engine
        //processManager.insert(std::move(remoteReplierProcess));
        //processManager.insert(std::move(detectorProcess));
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


