#include <iostream>
#include <filesystem>
#include <chrono>
#include <string>
#include <thread>
#include <mutex>
#ifndef NDEBUG
#include <cassert>
#endif
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/dailyFile.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
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
#include <umps/services/connectionInformation/socketDetails/dealer.hpp>
#include <umps/services/command/availableCommandsRequest.hpp>
#include <umps/services/command/availableCommandsResponse.hpp>
#include <umps/services/command/commandRequest.hpp>
#include <umps/services/command/commandResponse.hpp>
#include <umps/services/command/service.hpp>
#include <umps/services/command/serviceOptions.hpp>
#include <umps/services/command/terminateRequest.hpp>
#include <umps/services/command/terminateResponse.hpp>
#include "urts/services/standalone/incrementer/serviceOptions.hpp"
#include "urts/services/standalone/incrementer/service.hpp"
#include "private/isEmpty.hpp"

namespace UIncrementer = URTS::Services::Standalone::Incrementer;
namespace UAuth = UMPS::Authentication;
namespace UCI = UMPS::Services::ConnectionInformation;

#define MODULE_NAME "incrementer"

[[nodiscard]] std::string parseCommandLineOptions(int argc, char *argv[]);

/// @result Gets the command line input options as a string.
[[nodiscard]] std::string getInputOptions() noexcept
{
    std::string commands;
    commands = "Commands:\n";
    commands = commands + "   quit   Exits the program.\n";
    commands = commands + "   help   Displays this message.\n";
    return commands;
}

/// @resultGets the input line.
[[nodiscard]] std::string getInputLine() noexcept
{
    return std::string{MODULE_NAME} + "$";
}

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

/// @brief Defines the module options.
struct ProgramOptions
{
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
        //-----------------Service Connection Information --------------------//
        mServiceName = propertyTree.get<std::string>
                       ("Incrementer.serviceName", mServiceName);
                              
        auto backendAddress = propertyTree.get<std::string>
                              ("Incrementer.serviceAddress", "");
        if (!::isEmpty(backendAddress))
        {
            mIncrementerServiceOptions.setAddress(backendAddress);
        }
        if (::isEmpty(mServiceName) && ::isEmpty(backendAddress))
        {
            throw std::runtime_error("Service backend address indeterminable");
        }

        auto pollingTimeOut
            = propertyTree.get<int64_t>
              ("Incrementer.servicePollingTimeOut",
               mIncrementerServiceOptions.getPollingTimeOut().count());
        mIncrementerServiceOptions.setPollingTimeOut(
            std::chrono::milliseconds {pollingTimeOut});

        auto receiveHWM
            = propertyTree.get<int> ("Incrementer.serviceReceiveHighWaterMark",
                  mIncrementerServiceOptions.getReceiveHighWaterMark());
        mIncrementerServiceOptions.setReceiveHighWaterMark(receiveHWM);

        auto sendHWM
            = propertyTree.get<int> ("Incrementer.serviceSendHighWaterMark",
                  mIncrementerServiceOptions.getSendHighWaterMark());
        mIncrementerServiceOptions.setSendHighWaterMark(sendHWM); 
        //------------------------ Incrementer Options -----------------------//
        auto sqlite3FileName
            = propertyTree.get<std::string>
                ("Incrementer.sqlite3FileName",
                 mIncrementerServiceOptions.getSqlite3FileName());
        mIncrementerServiceOptions.setSqlite3FileName(sqlite3FileName);

        auto increment
            = propertyTree.get<int> ("Incrementer.increment",
                                     mIncrementerServiceOptions.getIncrement());
        mIncrementerServiceOptions.setIncrement(increment);

        auto initialValue
            = propertyTree.get<int32_t>
                  ("Incrementer.initialValue",
                   mIncrementerServiceOptions.getInitialValue());
        mIncrementerServiceOptions.setInitialValue(initialValue);

    }
///private:
    UIncrementer::ServiceOptions mIncrementerServiceOptions;
    UAuth::ZAPOptions mZAPOptions;
    std::string mModuleName{MODULE_NAME};
    std::string mHeartbeatBroadcastName{"Heartbeat"};
    std::string mServiceName{"Incrementer"};
    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    std::chrono::seconds heartBeatInterval{30};
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
};

class Incrementer : public UMPS::Modules::IProcess
{
public:
    Incrementer() = default;
    Incrementer(
        const std::string &moduleName,
        std::unique_ptr<UIncrementer::Service> &&incrementer,
        std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mIncrementer(std::move(incrementer)),
        mLogger(logger)
    {
        if (mIncrementer == nullptr)
        {
            throw std::invalid_argument("Incrementer is NULL");
        }
        if (!mIncrementer->isInitialized())
        {
            throw std::invalid_argument("Incrementer not initialized");
        }
        if (mLogger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> ();
        }
        // Create local command replier
        mLocalCommand
            = std::make_unique<UMPS::Services::Command::Service> (mLogger);
        UMPS::Services::Command::ServiceOptions localServiceOptions;
        localServiceOptions.setModuleName(moduleName);
        localServiceOptions.setCallback(
            std::bind(&Incrementer::commandCallback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
        mLocalCommand->initialize(localServiceOptions);

        mInitialized = true;
    }
    /// Destructor
    ~Incrementer()
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
    /// @result True indicates this is still running
    [[nodiscard]] bool isRunning() const noexcept override
    {
        return keepRunning();
    } 
    /// @brief Toggles this as running or not running
    void setRunning(const bool running)
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        mKeepRunning = running;
    }
    /// @brief Stops the process.
    void stop() override
    {
        setRunning(false);
        if (mIncrementer != nullptr)
        {
            if (mIncrementer->isRunning()){mIncrementer->stop();}
        }
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
            throw std::runtime_error("Incrementer not initialized");
        }
        setRunning(true);
        mLogger->debug("Starting the incrementer service...");
        mIncrementer->start();
        mLogger->debug("Starting the local command proxy..."); 
        mLocalCommand->start();
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
        mLogger->error("Unhandled message: " + messageType);
        USC::AvailableCommandsResponse commandsResponse;
        commandsResponse.setCommands(getInputOptions());
        return commandsResponse.clone();
    }
private:
    mutable std::mutex mMutex;
    std::unique_ptr<UIncrementer::Service> mIncrementer{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<UMPS::Services::Command::Service> mLocalCommand{nullptr};
    std::unique_ptr<UMPS::ProxyServices::Command::Replier>
        mModuleRegistryReplier{nullptr};
    bool mKeepRunning{true};
    bool mInitialized{false};
};

///--------------------------------------------------------------------------///
///                               Driver Program                             ///
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
    auto logger = createLogger(programOptions.mModuleName,
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
        // Connect to the operator
        logger->debug("Connecting to uOperator...");
        const std::string operatorSection{"uOperator"};
        auto uOperator = UCI::createRequestor(iniFile, operatorSection,
                                              context, logger);
        programOptions.mZAPOptions = uOperator->getZAPOptions();

        // Create a heartbeat
        logger->debug("Creating heartbeat process...");
        namespace UHeartbeat = UMPS::ProxyBroadcasts::Heartbeat;
        auto heartbeat = UHeartbeat::createHeartbeatProcess(*uOperator, iniFile,
                                                            "Heartbeat",
                                                            context, logger);
        processManager.insert(std::move(heartbeat));

        // Create the module command replier
        logger->debug("Creating module registry replier process...");
        namespace URemoteCommand = UMPS::ProxyServices::Command;
        URemoteCommand::ModuleDetails moduleDetails;
        moduleDetails.setName(programOptions.mModuleName);
        // Get the backend service connection details
        if (!programOptions.mIncrementerServiceOptions.haveAddress())
        {
            auto address = uOperator->getProxyServiceBackendDetails(
                              programOptions.mServiceName).getAddress();
            programOptions.mIncrementerServiceOptions.setAddress(address);
        }
        programOptions.mIncrementerServiceOptions.setZAPOptions(
            programOptions.mZAPOptions);
        // Create the incrementer service
        auto incrementerService
            = std::make_unique<UIncrementer::Service> (context, logger);
        incrementerService->initialize(
            programOptions.mIncrementerServiceOptions);

        auto incrementerProcess
            = std::make_unique<::Incrementer> (programOptions.mModuleName,
                                               std::move(incrementerService),
                                               logger);

        auto callbackFunction = std::bind(&Incrementer::commandCallback,
                                          &*incrementerProcess,
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          std::placeholders::_3);
        auto remoteReplier
            = URemoteCommand::createReplierProcess(*uOperator,
                                                   moduleDetails,
                                                   callbackFunction,
                                                   iniFile,
                                                   "ModuleRegistry",
                                                   nullptr, // Make new context
                                                   logger);
        // Add the remote replier and incrementer
        processManager.insert(std::move(remoteReplier));
        processManager.insert(std::move(incrementerProcess));
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
    boost::program_options::options_description desc(
R"""(
The incrementer service allows other modules to uniquely increment values of
interest - e.g., pick identifiers, arrival identifiers, origin identifiers,
event identifiers, etc.  Example usage:

    incrementer --ini=incrementer.ini

Allowed options)""");
    desc.add_options()
        ("help", "Produces this help message")
        ("ini",  boost::program_options::value<std::string> (),
                 "The initialization file for this executable");
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
