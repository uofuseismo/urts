#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <filesystem>
#include <mutex>
#ifndef NDEBUG
#include <cassert>
#endif
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <umps/modules/process.hpp>
#include <umps/modules/processManager.hpp>
#include <umps/logging/dailyFile.hpp>
#include <umps/logging/standardOut.hpp>
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
#include <umps/services/connectionInformation/socketDetails/xSubscriber.hpp>
#include <umps/services/command/availableCommandsRequest.hpp>
#include <umps/services/command/availableCommandsResponse.hpp>
#include <umps/services/command/commandRequest.hpp>
#include <umps/services/command/commandResponse.hpp>
#include <umps/services/command/service.hpp>
#include <umps/services/command/serviceOptions.hpp>
#include <umps/services/command/terminateRequest.hpp>
#include <umps/services/command/terminateResponse.hpp>
#include <umps/modules/operator/readZAPOptions.hpp>
#include <umps/messaging/context.hpp>
#include <umps/authentication/zapOptions.hpp>
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "urts/broadcasts/internal/dataPacket/publisher.hpp"
#include "urts/broadcasts/internal/dataPacket/publisherOptions.hpp"
#include "urts/broadcasts/external/seedlink/client.hpp"
#include "urts/broadcasts/external/seedlink/clientOptions.hpp"
#include "private/isEmpty.hpp"

 
#define MODULE_NAME "broadcastSEEDLink"

namespace UAuth = UMPS::Authentication;
namespace UServices = UMPS::Services;
namespace UCI = UMPS::Services::ConnectionInformation;
namespace UModules = UMPS::Modules;
namespace UHeartbeat = UMPS::ProxyBroadcasts::Heartbeat;
namespace UDP = URTS::Broadcasts::Internal::DataPacket;

std::string parseCommandLineOptions(int argc, char *argv[]);

namespace
{

/// @result Gets the command line input options as a string.
[[nodiscard]] std::string getInputOptions() noexcept
{
    std::string commands;
    commands = "Commands:\n";
    commands = commands + "   quit   Exits the program.\n";
    commands = commands + "   packetsSent  Number of packets sent in last minute.\n";
    commands = commands + "   help   Displays this message.\n";
    return commands;
}

}

/// @brief Defines the module options.
struct ProgramOptions
{
    /// @brief Load the module options from an initialization file.
    void parseInitializationFile(const std::string &iniFile)
    {
        boost::property_tree::ptree propertyTree;
        boost::property_tree::ini_parser::read_ini(iniFile, propertyTree);
        //------------------------------ General -----------------------------//
        // Module name
        mModuleName
            = propertyTree.get<std::string> ("General.moduleName", mModuleName);
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
        // Oldest packet time
        auto expirationTime = static_cast<int> (mExpirationTime.count());
        expirationTime
            =  propertyTree.get<int> ("General.expirationTime", expirationTime);
        if (expirationTime < 0)
        {
            throw std::invalid_argument(
                "General.expirationTime must be non-negative"); 
        }
        mExpirationTime = std::chrono::seconds {expirationTime};
        // Future time
        auto futureTime = static_cast<int> (mFutureTime.count());
        if (futureTime < 0)
        {
            throw std::invalid_argument(
                "General.futureTime must be non-negative");
        }
        mFutureTime = std::chrono::seconds {futureTime};
        //----------------------------Publisher Options-----------------------//
        mDataPacketBroadcastName
            = propertyTree.get<std::string>
                ("PublisherOptions.dataPacketBroadcast",
                 mDataPacketBroadcastName);
        if (mDataPacketBroadcastName.empty())
        {
            throw std::runtime_error(
               "PublisherOptions.dataPacketBroadcast not set");
        }

        mBroadcastAddress
            = propertyTree.get<std::string> ("PublisherOptions.address", "");
        //----------------------------- SEEDLink ----------------------------//
        auto seedlinkAddress = mSEEDLinkClientOptions.getAddress();
        mSEEDLinkClientOptions.setAddress(
            propertyTree.get<std::string> ("SEEDLink.address", seedlinkAddress)
        );
        if (seedlinkAddress.empty())
        {
            throw std::runtime_error("SEEDLink address not set");
        }
        auto seedlinkPort = mSEEDLinkClientOptions.getPort();
        mSEEDLinkClientOptions.setPort(
            propertyTree.get<int> ("SEEDLink.port", seedlinkPort)
        );
    }
    UAuth::ZAPOptions mZAPOptions;
    URTS::Broadcasts::External::SEEDLink::ClientOptions mSEEDLinkClientOptions;
    std::string mModuleName{MODULE_NAME};
    std::string mDataPacketBroadcastName{"DataPacket"};
    std::string mBroadcastAddress{""};
    std::string mHeartbeatBroadcastName{"Heartbeat"};
    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    std::chrono::seconds mHeartBeatInterval{30};
    std::chrono::seconds mExpirationTime{std::chrono::minutes {10}}; // 10 Minutes
    std::chrono::seconds mFutureTime{0}; // Do not allow data from future
    int mEarthwormWait{0};
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
};

/// @result The logger for this application.
std::shared_ptr<UMPS::Logging::ILog>
    createLogger(const std::string &moduleName = MODULE_NAME,
                 const std::filesystem::path logFileDirectory = "/var/log/urts",
                 const UMPS::Logging::Level verbosity = UMPS::Logging::Level::Info,
                 const int hour = 0, const int minute = 0)
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

//----------------------------------------------------------------------------//
/// @brief Reads packets from the wave ring then brodcasts them to UMPS.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class BroadcastPackets : public UMPS::Modules::IProcess
{
public:
    BroadcastPackets() = default;
    BroadcastPackets(
        const std::string &moduleName, 
        std::unique_ptr<UDP::Publisher> &&packetPublisher,
        std::unique_ptr<URTS::Broadcasts::External::SEEDLink::Client>
             &&seedLink,
        std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mPacketPublisher(std::move(packetPublisher)),
        mSEEDLinkClient(std::move(seedLink)),
        mLogger(logger)
    {
        if (mSEEDLinkClient == nullptr)
        {
            throw std::invalid_argument("SEEDLink client is NULL");
        }
        if (mPacketPublisher == nullptr)
        {
            throw std::invalid_argument("Packet publisher is NULL");
        }
        if (!mSEEDLinkClient->isInitialized())
        {
            throw std::invalid_argument("SEEDLink client not initialized");
        }
        if (!mPacketPublisher->isInitialized())
        {
            throw std::invalid_argument("Packet publisher not initialized");
        }
        if (mLogger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> ();
        }
        mLocalCommand
            = std::make_unique<UMPS::Services::Command::Service> (mLogger);
        UMPS::Services::Command::ServiceOptions localServiceOptions;
        localServiceOptions.setModuleName(moduleName);
        localServiceOptions.setCallback(
            std::bind(&BroadcastPackets::commandCallback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
        mLocalCommand->initialize(localServiceOptions);
        mInitialized = true;
    }
    /// Initialized?
    [[nodiscard]] bool isInitialized() const noexcept
    {
        std::scoped_lock lock(mMutex);
        return mInitialized;
    }
    /// Destructor
    ~BroadcastPackets() override
    {
        stop();
    }
    /// @result True indicates this should keep running
    [[nodiscard]] bool keepRunning() const
    {
        std::scoped_lock lock(mMutex);
        return mKeepRunning; 
    }
    /// @brief Toggles this as running or not running
    void setRunning(const bool running)
    {   
        std::lock_guard<std::mutex> lockGuard(mMutex);
        mKeepRunning = running;
    }
    /// @brief Gets the process name.
    [[nodiscard]] std::string getName() const noexcept override
    {
        return "BroadcastSEEDLinkPackets";
    }
    /// @brief Stops the process.
    void stop() override
    {
        setRunning(false);
        mSEEDLinkClient->stop();
        if (mBroadcastThread.joinable()){mBroadcastThread.join();}
        if (mLocalCommand != nullptr)
        {
            if (mLocalCommand->isRunning()){mLocalCommand->stop();}
        }
    }
    /// @brief Starts the process.
    void start() override
    {
        stop();
        if (!isInitialized())
        {
            throw std::runtime_error("Class not initialized");
        }
        setRunning(true);
        mLogger->debug("Starting the SEEDLink client thread...");
        mSEEDLinkClient->start();
        mLogger->debug("Starting the broadcast thread...");
        mBroadcastThread = std::thread(&BroadcastPackets::run,
                                       this);
        mLogger->debug("Starting the local command proxy...");
        mLocalCommand->start();
    }
    /// @result True indicates this is running.
    [[nodiscard]] bool isRunning() const noexcept override
    {
        return keepRunning();
    }
    /// @brief Reads EW messages and publishes them to an URTS broadcast
    void run()
    {
        if (!mSEEDLinkClient->isInitialized())
        {
            throw std::runtime_error("SEEDLink client not yet initialized");
        }
        if (!mPacketPublisher->isInitialized())
        { 
            throw std::runtime_error("Publisher not yet initialized");
        }
        mLogger->debug("Earthworm broadcast thread is starting");
        int numberOfPacketsSent = 0;
        mNumberOfPacketsSent = 0;
        auto packetMonitorStart = std::chrono::high_resolution_clock::now();
        while (keepRunning())
        {
            auto startClock = std::chrono::high_resolution_clock::now();
            auto startClockEpoch
                = std::chrono::duration_cast<std::chrono::microseconds>
                  (startClock.time_since_epoch());
            auto broadcastTimeStart
                = startClockEpoch
                - std::chrono::microseconds {mExpirationTime};
            auto broadcastTimeEnd
                = startClockEpoch
                + std::chrono::microseconds {mFutureTime};
            // Read from the earthworm ring
            std::vector<UDP::DataPacket> packets;
            // Send the packets off
            bool getNextPacket = true;
            while (getNextPacket)
            {
                auto packet = mSEEDLinkClient->try_pop();
                if (packet == nullptr)
                {
                    getNextPacket = false;
                }
                else
                {
                    // Don't forward empty packets
                    if (packet->getNumberOfSamples() < 1){continue;}
                    try
                    {
                        // Make sure time makes sense
                        auto packetStartTime = packet->getStartTime();
                        auto packetEndTime = packet->getEndTime();
                        if (packetEndTime   >= broadcastTimeStart &&
                            packetStartTime <= broadcastTimeEnd)
                        {
                            mPacketPublisher->send(*packet);
                            numberOfPacketsSent = numberOfPacketsSent + 1;
                        }
                        else
                        {
                            if (packetEndTime < broadcastTimeStart)
                            {
                                mLogger->debug("Expired packet skipped for "
                                              + packet->getNetwork() + "."
                                              + packet->getStation() + "."
                                              + packet->getChannel() + "."
                                              + packet->getLocationCode() 
                                              + "; not forwarding");
                            }
                            if (packetStartTime > broadcastTimeEnd)
                            {
                                mLogger->debug("Future packet detector for "
                                             + packet->getNetwork() + "." 
                                             + packet->getStation() + "." 
                                             + packet->getChannel() + "." 
                                             + packet->getLocationCode() 
                                             + "; not forwarding");
                            }
                        }
                    }
                    catch (const std::exception &e)
                    {
                        mLogger->error(e.what());
                    }
                }
            }
            // Update
            auto endClock = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>
                            (endClock - startClock);
            if (duration < mBroadcastInterval)
            {
                auto wait = mBroadcastInterval - duration;
                std::this_thread::sleep_for(wait);
            }
            else
            {
                startClock = endClock;
            }
            // Update my packets sent counter
            duration = std::chrono::duration_cast<std::chrono::seconds>
                       (endClock - packetMonitorStart);
            if (duration > std::chrono::seconds {60})
            {
                mNumberOfPacketsSent = numberOfPacketsSent;
                numberOfPacketsSent = 0;
                packetMonitorStart = endClock;
            }
        }
        mLogger->debug("Earthworm broadcast thread is terminating");
    }
    // Callback for local interaction
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
        else if (messageType == terminateRequest.getMessageType())
        {
            USC::TerminateResponse response;
            try
            {
                terminateRequest.fromMessage(
                    static_cast<const char *> (data), length);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to unpack terminate request");
                response.setReturnCode(
                    USC::TerminateResponse::ReturnCode::InvalidCommand);
                return response.clone();
            }
            issueStopCommand(); 
            response.setReturnCode(USC::TerminateResponse::ReturnCode::Success);
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
                mLogger->error("Failed to unpack text request");
                response.setReturnCode(
                    USC::CommandResponse::ReturnCode::ApplicationError);
            }
            auto command = commandRequest.getCommand(); 
            if (command == "quit")
            {
                mLogger->debug("Issuing quit command...");
                issueStopCommand(); 
                response.setResponse("Bye!  But next time use the terminate command.");
                response.setReturnCode(
                    USC::CommandResponse::ReturnCode::Success);
            }
            else if (command == "packetsSent")
            {
                mLogger->debug("Issuing packetsSent command...");
                response.setResponse("Number of packets sent in last minute: "
                                   + std::to_string(mNumberOfPacketsSent));
                response.setReturnCode(
                    USC::CommandResponse::ReturnCode::InvalidCommand);
            }
            else
            {
                response.setResponse(getInputOptions());
                if (command != "help")
                {
                    mLogger->debug("Invalid command: " + command);
                    response.setResponse("Invalid command: " + command);
                    response.setReturnCode(
                        USC::CommandResponse::ReturnCode::InvalidCommand);
                }
                else
                {
                    response.setReturnCode(
                        USC::CommandResponse::ReturnCode::Success);
                }
            }
            return response.clone();
        }
        else
        {
            mLogger->error("Unhandled message type: " + messageType);
        }
        // Return
        USC::AvailableCommandsResponse commandsResponse; 
        commandsResponse.setCommands(getInputOptions());
        return commandsResponse.clone();
    }
    mutable std::mutex mMutex;
    std::thread mBroadcastThread;
    std::unique_ptr<UDP::Publisher> mPacketPublisher{nullptr};
    std::unique_ptr<URTS::Broadcasts::External::SEEDLink::Client>
         mSEEDLinkClient{nullptr};
    std::unique_ptr<UMPS::Services::Command::Service> mLocalCommand{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::chrono::seconds mBroadcastInterval{1};
    std::chrono::seconds mExpirationTime{std::chrono::minutes {10}}; // 10 Minutes
    std::chrono::seconds mFutureTime{0}; // Do not allow data from future
    int mNumberOfPacketsSent{0};
    bool mKeepRunning{true};
    bool mInitialized{false};
};

///--------------------------------------------------------------------------///
///                                 Main Function                            ///
///--------------------------------------------------------------------------///

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
    // This module only needs one context.  There's not much data to move.
    auto context = std::make_shared<UMPS::Messaging::Context> (1);
    // Initialize the various processes
    logger->info("Initializing processes...");
    UMPS::Modules::ProcessManager processManager(logger);
    try
    {
        logger->debug("Connecting to uOperator...");
        const std::string operatorSection{"uOperator"};
        auto uOperator = UCI::createRequestor(iniFile, operatorSection,
                                              context, logger);
        programOptions.mZAPOptions = uOperator->getZAPOptions();

        logger->debug("Creating heartbeat process...");
        auto heartbeat = UHeartbeat::createHeartbeatProcess(*uOperator, iniFile,
                                                            "Heartbeat",
                                                            context, logger);
        processManager.insert(std::move(heartbeat));

        logger->debug("Creating packet broadcast process...");
        auto packetAddress = programOptions.mBroadcastAddress;
        if (::isEmpty(packetAddress))
        {
            packetAddress
                = uOperator->getProxyBroadcastFrontendDetails(
                     programOptions.mDataPacketBroadcastName).getAddress();
            programOptions.mBroadcastAddress = packetAddress;
        }
        // Create the SEEDLink client
        auto seedLinkClient
            = std::make_unique<URTS::Broadcasts::External::SEEDLink::Client>
              (logger);
        seedLinkClient->initialize(programOptions.mSEEDLinkClientOptions);
#ifndef NDEBUG
        assert(seedLinkClient->isInitialized());
#endif
        // Create the publisher
        UDP::PublisherOptions packetPublisherOptions;
        auto packetPublisher
            = std::make_unique<UDP::Publisher> (context, logger);
        packetPublisherOptions.setAddress(programOptions.mBroadcastAddress);
        packetPublisherOptions.setZAPOptions(programOptions.mZAPOptions);
        packetPublisher->initialize(packetPublisherOptions);

        auto broadcastProcess 
            = std::make_unique<BroadcastPackets> (programOptions.mModuleName,
                                                  std::move(packetPublisher), 
                                                  std::move(seedLinkClient),
                                                  logger);
        broadcastProcess->mExpirationTime = programOptions.mExpirationTime;
        broadcastProcess->mFutureTime = programOptions.mFutureTime;
        // Create the remote registry
        auto callbackFunction = std::bind(&BroadcastPackets::commandCallback,
                                          &*broadcastProcess,
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          std::placeholders::_3);
        namespace URemoteCommand = UMPS::ProxyServices::Command;
        URemoteCommand::ModuleDetails moduleDetails;
        moduleDetails.setName(programOptions.mModuleName);
        auto remoteReplier
            = URemoteCommand::createReplierProcess(*uOperator,
                                                   moduleDetails,
                                                   callbackFunction,
                                                   iniFile,
                                                   "ModuleRegistry",
                                                   nullptr, // Make new context
                                                   logger);


        processManager.insert(std::move(remoteReplier));
        processManager.insert(std::move(broadcastProcess)); 
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        logger->error(e.what());
        return EXIT_FAILURE;
    }
    // Start the modules
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
    // Done
    logger->info("Exiting...");
    return EXIT_SUCCESS;
}

///--------------------------------------------------------------------------///
///                            Utility Functions                             ///
///--------------------------------------------------------------------------///
/// Read the program options from the command line
std::string parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Produce help message")
        ("ini",  boost::program_options::value<std::string> (), 
                 "Defines the initialization file for this module");
    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);
    if (vm.count("help"))
    {
        std::cout << MODULE_NAME
                  << " is for broadcasting traceBuf2 packets "
                  << "on an Earthworm wave ring to UMPS."
                  << std::endl << std::endl;
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
