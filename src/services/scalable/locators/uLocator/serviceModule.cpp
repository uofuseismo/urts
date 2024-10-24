#include <iostream>
#include <iostream>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <string>
#include <chrono>
#include <memory>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <uLocator/station.hpp>
#include <uLocator/travelTimeCalculatorMap.hpp>
#include <uLocator/position/wgs84.hpp>
#include <uLocator/position/utahRegion.hpp>
#include <uLocator/position/ynpRegion.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/dailyFile.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/modules/process.hpp>
#include <umps/modules/processManager.hpp>
#include <umps/proxyBroadcasts/heartbeat/publisherProcess.hpp>
#include <umps/proxyServices/command.hpp>
#include <umps/services/command.hpp>
#include <umps/services/connectionInformation.hpp>
#include "urts/services/scalable/locators/uLocator/service.hpp"
#include "urts/services/scalable/locators/uLocator/serviceOptions.hpp"
#include "urts/database/connection/postgresql.hpp"
#include "private/isEmpty.hpp"

#define MODULE_NAME "uLocator"
#define UTM_ZONE 12

namespace ULoc = URTS::Services::Scalable::Locators::ULocator;

struct ProgramOptions;
[[nodiscard]] std::pair<std::string, int> parseCommandLineOptions(int argc, char *argv[]);
[[nodiscard]] std::shared_ptr<UMPS::Logging::ILog>
    createLogger(const std::string &moduleName,
                 const std::filesystem::path logFileDirectory,
                 const UMPS::Logging::Level verbosity,
                 const int instance,
                 const int hour = 0,
                 const int minute = 0);
[[nodiscard]] std::string getInputOptions() noexcept;
[[nodiscard]] std::shared_ptr<URTS::Database::Connection::IConnection> 
    createAQMSDatabaseConnection(const ::ProgramOptions options);

struct ProgramOptions
{
    /// Constructor with instance defaulting to 0
    ProgramOptions() :
       ProgramOptions(0)
    {
    }
    /// Constructor with given instance 
    explicit ProgramOptions(const int instance)
    {
        if (instance < 0)
        {
            throw std::invalid_argument("Instance must be positive");
        }
        mInstance = instance;
    }
    /// @brief Load the module options from an initialization file
    void parseInitializationFile(const std::filesystem::path &iniFile)
    {
        if (!std::filesystem::exists(iniFile))
        {
            throw std::invalid_argument("Initialization file "
                                      + iniFile.string()
                                      + " does not exist"); 
        }
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
        //----------------Backend Service Connection Information--------------//
        const std::string section{"uLocator"};
        mServiceName = propertyTree.get<std::string>
                       (section + ".proxyServiceName", mServiceName);
        auto backendAddress = propertyTree.get<std::string>
                              (section + ".proxyServiceAddress", "");
        if (!::isEmpty(backendAddress))
        {
            mULocatorServiceOptions.setAddress(backendAddress);
        }
        if (::isEmpty(mServiceName) && ::isEmpty(backendAddress))
        {
            throw std::runtime_error("Service backend address indeterminable");
        }
        mULocatorServiceOptions.setReceiveHighWaterMark(
            propertyTree.get<int> (
                section + ".proxyServiceReceiveHighWaterMark",
                mULocatorServiceOptions.getReceiveHighWaterMark())
        );
        mULocatorServiceOptions.setSendHighWaterMark(
            propertyTree.get<int> (
                section + ".proxyServiceSendHighWaterMark",
                mULocatorServiceOptions.getSendHighWaterMark())
        );

        auto pollingTimeOut 
            = static_cast<int> (
               mULocatorServiceOptions.getPollingTimeOut().count()
              );
        pollingTimeOut = propertyTree.get<int> (
              section + ".proxyServicePollingTimeOut", pollingTimeOut);
        mULocatorServiceOptions.setPollingTimeOut(
            std::chrono::milliseconds {pollingTimeOut} );

        //---------------------------- Database ------------------------------//
        const auto databaseHost = std::getenv("URTS_AQMS_DATABASE_HOST");
        const auto databaseName = std::getenv("URTS_AQMS_DATABASE_NAME");
        const auto readOnlyUser = std::getenv("URTS_AQMS_RDONLY_USER");
        const auto readOnlyPassword = std::getenv("URTS_AQMS_RDONLY_PASSWORD");
        if (databaseHost != nullptr)
        {
            mDatabaseAddress = std::string {databaseHost};
        }
        if (databaseName != nullptr)
        {
            mDatabaseName = std::string {databaseName};
        }
        if (readOnlyUser != nullptr)
        {
            mDatabaseReadOnlyUser = std::string {readOnlyUser};
        }
        if (readOnlyPassword != nullptr)
        {
            mDatabaseReadOnlyPassword = std::string {readOnlyPassword};
        }
        mDatabaseReadOnlyUser
            = propertyTree.get<std::string> (
                 section + ".databaseReadOnlyUser",
                 mDatabaseReadOnlyUser);
        if (mDatabaseReadOnlyUser.empty())
        {
            throw std::runtime_error("Database read-only user not set");
        }
        mDatabaseReadOnlyPassword
            = propertyTree.get<std::string> (
                section + ".databaseReadOnlyPassword",
                mDatabaseReadOnlyPassword);
        if (mDatabaseReadOnlyPassword.empty())
        {
            throw std::runtime_error("Database read-only password not set");
        }
        mDatabasePort
            = propertyTree.get<int> (section + ".databasePort", mDatabasePort);
        mDatabaseAddress
            = propertyTree.get<std::string> (
                  section + ".databaseAddress",
                  mDatabaseAddress);
        if (mDatabaseAddress.empty())
        {
            throw std::runtime_error("Database address not set");
        }
        mDatabaseName
            = propertyTree.get<std::string>
              (section + ".databaseName", mDatabaseName);
        if (mDatabaseName.empty())
        {
            throw std::runtime_error("Database name not set");
        }
        //--------------------------uLocator options--------------------------//
        mULocatorServiceOptions.setUTMZone(UTM_ZONE);

        auto region = propertyTree.get<std::string> (section + ".region");
        double initialSearchDepth{4780};
        double horizontalRefinement{50000};
        double maximumSearchDepth{65000};
        double originTimeSearchWindow{140};
        int nParticles{20};
        int nGenerations{140};
        if (region == "utah" || region == "Utah")
        {
            mULocatorServiceOptions.setRegion(
                ULoc::ServiceOptions::Region::Utah);
            initialSearchDepth = 4780;
            horizontalRefinement = 50000;
            maximumSearchDepth = 65000;
            originTimeSearchWindow = 140;
            nParticles = 20;
            nGenerations = 140;
        }
        else if (region == "YNP" || region == "ynp" || region == "Yellowstone")
        {
            mULocatorServiceOptions.setRegion(
                ULoc::ServiceOptions::Region::YNP);
            initialSearchDepth = 6600;
            horizontalRefinement = 35000;
            maximumSearchDepth = 30000;
            originTimeSearchWindow = 60;
            nParticles = 15;
            nGenerations = 120;
        }
        else
        {
            throw std::invalid_argument("Unhandled region: " + region);
        }
        // Norm
        auto pNorm = propertyTree.get<double> (section + ".pNorm", 1.5);
        if (pNorm < 1)
        {
            throw std::invalid_argument("P-norm must be >= 1");
        }
        if (std::abs(pNorm - 1) < 1.e-4)
        {
            mULocatorServiceOptions.setNorm(ULoc::ServiceOptions::Norm::L1, 1);
        }
        else if (std::abs(pNorm - 2) < 1.e-4)
        {
            mULocatorServiceOptions.setNorm(ULoc::ServiceOptions::Norm::L2, 2);
        }
        else
        {
            mULocatorServiceOptions.setNorm(ULoc::ServiceOptions::Norm::Lp,
                                            pNorm);
        }
        // Correction and topography files
        auto staticCorrectionFile
            = propertyTree.get<std::string>
              (section + ".staticCorrections", "");
        if (!staticCorrectionFile.empty() &&
            std::filesystem::exists(staticCorrectionFile))
        {
            mULocatorServiceOptions.setStaticCorrectionFile(
               staticCorrectionFile);
        }
        auto sourceSpecificCorrectionFile
            = propertyTree.get<std::string>
              (section + ".sourceSpecificCorrections", ""); 
        if (!sourceSpecificCorrectionFile.empty() &&
            std::filesystem::exists(sourceSpecificCorrectionFile))
        {
            mULocatorServiceOptions.setSourceSpecificCorrectionFile(
               sourceSpecificCorrectionFile);
        }
        auto topographyFile
            = propertyTree.get<std::string>
              (section + ".topography", ""); 
        if (!topographyFile.empty() &&
            std::filesystem::exists(topographyFile))
        {
            mULocatorServiceOptions.setTopographyFile(
               topographyFile);
        }

        originTimeSearchWindow
            = propertyTree.get<double> (section + ".originTimeSearchWindow",
                                        originTimeSearchWindow);
        mULocatorServiceOptions.setOriginTimeSearchWindow(
            originTimeSearchWindow);
        // Initial search + DIRECT
        initialSearchDepth
            = propertyTree.get<double> (section + ".initialSearchDepth",
                                        initialSearchDepth);
        mULocatorServiceOptions.setInitialSearchDepth(initialSearchDepth); 
        auto nDIRECTFunctionEvaluations
            = mULocatorServiceOptions.getNumberOfFunctionEvaluations();
        nDIRECTFunctionEvaluations
            = propertyTree.get<int> (section + ".numberOfDIRECTFunctionEvaluations",
                                     nDIRECTFunctionEvaluations);
        mULocatorServiceOptions.setNumberOfFunctionEvaluations(
            nDIRECTFunctionEvaluations);
        // Refined search + PSO
        horizontalRefinement
            = propertyTree.get<double> (section + ".horizontalRefinement",
                                        horizontalRefinement);
        mULocatorServiceOptions.setHorizontalRefinement(horizontalRefinement);
        maximumSearchDepth
            = propertyTree.get<double> (section + ".maximumSearchDepth", 
                                        maximumSearchDepth);
        mULocatorServiceOptions.setMaximumSearchDepth(maximumSearchDepth);
        nGenerations
            = propertyTree.get<int>
              (section + ".numberOfGenerations", nGenerations);
        mULocatorServiceOptions.setNumberOfGenerations(nGenerations);
        nParticles
            = propertyTree.get<int>
              (section + ".numberOfParticles", nParticles);
        mULocatorServiceOptions.setNumberOfParticles(nParticles);


    }
    std::string mModuleName{MODULE_NAME};
    std::string mServiceName{"uLocator"};
    std::string mDatabaseAddress;
    std::string mDatabaseName;
    std::string mDatabaseReadOnlyUser;
    std::string mDatabaseReadOnlyPassword;
    std::string mOriginBroadcastName{"Origin"};
    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    std::chrono::seconds heartBeatInterval{30};
    ULoc::ServiceOptions mULocatorServiceOptions;
    UMPS::Authentication::ZAPOptions mZAPOptions;
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
    int mDatabasePort{5432};
    int mInstance{0};

};

class LocatorProcess : public UMPS::Modules::IProcess
{
public:
    LocatorProcess() = delete;
    /// @brief Constructor.
    LocatorProcess(const std::string &moduleName,
                   std::unique_ptr<ULoc::Service> &&locator,
                   std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mLocator(std::move(locator)),
        mLogger(logger)
    {
        if (moduleName.empty())
        {
            throw std::invalid_argument("Module name is empty");
        }
        if (mLocator == nullptr)
        {
            throw std::invalid_argument("uLocator is NULL");
        }
        if (!mLocator->isInitialized())
        {
            throw std::invalid_argument("uLocator not initialized");
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
            std::bind(&LocatorProcess::commandCallback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
        mLocalCommand->initialize(localServiceOptions);
        mInitialized = true;
    }
    /// @brief Destructor
    ~LocatorProcess() override
    {
        stop();
    }
    /// @brief Stops the processes.
    void stop() override
    {
        setRunning(false);
        if (mLocator != nullptr)
        {
            if (mLocator->isRunning()){mLocator->stop();}
        }
        if (mLocalCommand != nullptr)
        {
            if (mLocalCommand->isRunning()){mLocalCommand->stop();}
        }
    }
    /// @brief Starts the processes.
    void start() override
    {
        stop();
        if (!isInitialized())
        {
            throw std::invalid_argument("uLocator not initialized");
        }
        setRunning(true);
        mLogger->debug("Starting the locator service...");
        mLocator->start();
        mLogger->debug("Starting the local command proxy...");
        mLocalCommand->start();
    }
    /// @result True indicates this should keep running
    [[nodiscard]] bool keepRunning() const
    {
        return mKeepRunning; 
    }
    /// @result True indicates this is still running
    [[nodiscard]] bool isRunning() const noexcept override
    {
        return keepRunning();
    }
    /// @result True indicates the class is initialized
    [[nodiscard]] bool isInitialized() const noexcept
    {
        return mInitialized;
    }
    /// @brief Toggles this as running or not running
    void setRunning(const bool running)
    {
        mKeepRunning = running;
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
            if (command == "someCommand") // TODO
            {
                mLogger->debug("Issuing someCommand command...");
                try
                {
                    throw std::invalid_argument("Unhandled case");
                }
                catch (const std::exception &e)
                {
                    mLogger->error("Error issuing someCommand: "
                                 + std::string {e.what()});
                    response.setResponse("Server error detected");
                    response.setReturnCode(
                        USC::CommandResponse::ReturnCode::ApplicationError);
                }
            }
            else
            {
                response.setResponse(::getInputOptions());
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
private:
    //mutable std::mutex mMutex;
    std::unique_ptr<ULoc::Service> mLocator{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<UMPS::Services::Command::Service> mLocalCommand{nullptr};
    std::atomic<bool> mKeepRunning{true};
    std::atomic<bool> mInitialized{false};
};


int main(int argc, char *argv[])
{
    // Get the ini file from the command line
    std::string iniFile;
    int instance{0};
    try  
    {    
        auto arguments = ::parseCommandLineOptions(argc, argv);
        iniFile = arguments.first;
        instance = arguments.second;
        if (iniFile.empty()){return EXIT_SUCCESS;}
    }
    catch (const std::exception &e)  
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    // Parse the initialization file
    ::ProgramOptions programOptions(instance);
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
    constexpr int hour{0};
    constexpr int minute{0};
    auto logger = ::createLogger(programOptions.mModuleName,
                                 programOptions.mLogFileDirectory,
                                 programOptions.mVerbosity,
                                 programOptions.mInstance,
                                 hour, minute);
    // Create the database connection
    std::shared_ptr<URTS::Database::Connection::IConnection>
        aqmsDatabaseConnection{nullptr};
    try
    {
        aqmsDatabaseConnection = ::createAQMSDatabaseConnection(programOptions);
    }
    catch (const std::exception &e)
    {
        logger->error(e.what());
        return EXIT_FAILURE;
    }
    // Create a communication context
    auto context = std::make_shared<UMPS::Messaging::Context> (1);
    // Initialize the various processes
    logger->info("Initializing processes...");
    UMPS::Modules::ProcessManager processManager{logger};
    try
    {
        // Connect to the operator
        logger->debug("Connecting to uOperator...");
        const std::string operatorSection{"uOperator"};
        auto uOperator
            = UMPS::Services::ConnectionInformation::createRequestor(
                 iniFile, operatorSection, context, logger);
        programOptions.mZAPOptions = uOperator->getZAPOptions();
/*
        // Create a heartbeat
        logger->debug("Creating heartbeat process...");
        namespace UHeartbeat = UMPS::ProxyBroadcasts::Heartbeat;
        auto heartbeat = UHeartbeat::createHeartbeatProcess(*uOperator, iniFile,
                                                            "Heartbeat",
                                                            context,
                                                            logger);
        processManager.insert(std::move(heartbeat));
        // Create the module command replier
        logger->debug("Creating module registry replier process...");
        namespace URemoteCommand = UMPS::ProxyServices::Command;
        URemoteCommand::ModuleDetails moduleDetails;
        moduleDetails.setName(programOptions.mModuleName);
        moduleDetails.setInstance(instance);
*/
        // Get the backend service connection details
        if (!programOptions.mULocatorServiceOptions.haveAddress())
        {
            auto address = uOperator->getProxyServiceBackendDetails(
                              programOptions.mServiceName).getAddress();
            programOptions.mULocatorServiceOptions.setAddress(address);
        }
        programOptions.mULocatorServiceOptions.setZAPOptions(
            programOptions.mZAPOptions);

        // Create the service and the subsequent process
        auto locatorService
            = std::make_unique<ULoc::Service> (context, logger);
        locatorService->initialize(programOptions.mULocatorServiceOptions,
                                   aqmsDatabaseConnection);

    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        logger->error(e.what());
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}

/// Read the program options from the command line
std::pair<std::string, int> parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(
R"""(
This service allows a client to locate events.  This is a scalable service so
it is conceivable the application will have multiple instances running
simultaneously.  Example usage:

    uLocatorService --ini=uLocatorService.ini --instance=0

Allowed options)""");
    desc.add_options()
        ("help", "Produces this help message")
        ("ini",  boost::program_options::value<std::string> (),
                 "Defines the initialization file for this executable")
        ("instance",
         boost::program_options::value<uint16_t> ()->default_value(0),
         "Defines the module instance");
    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), vm); 
    boost::program_options::notify(vm);
    int instance{0};
    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return std::pair{iniFile, instance};
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
    if (vm.count("instance"))
    {
        instance = static_cast<int> (vm["instance"].as<uint16_t> ());
        if (instance < 0)
        {
            throw std::invalid_argument("Instance must be positive");
        }
    }
    return std::pair {iniFile, instance};
}

/// Creates the application logger
std::shared_ptr<UMPS::Logging::ILog>
    createLogger(const std::string &moduleName,
                 const std::filesystem::path logFileDirectory,
                 const UMPS::Logging::Level verbosity,
                 const int instance,
                 const int hour,
                 const int minute)
{
{
auto logger = std::make_shared<UMPS::Logging::StandardOut> (verbosity);
logger->error("fix me");
return logger;
}

    auto logFileName = moduleName + "_" + std::to_string(instance) + ".log";
    auto fullLogFileName = logFileDirectory / logFileName;
    auto logger = std::make_shared<UMPS::Logging::DailyFile> ();
    logger->initialize(moduleName,
                       fullLogFileName,
                       verbosity,
                       hour, minute);
    logger->info("Starting logging for " + moduleName);
    return logger;
};

/// Creates an AQMS database connection.
std::shared_ptr<URTS::Database::Connection::IConnection> 
createAQMSDatabaseConnection(const ::ProgramOptions options)
{
    auto databaseConnection
       = std::make_shared<URTS::Database::Connection::PostgreSQL> ();
    databaseConnection->setAddress(options.mDatabaseAddress);
    databaseConnection->setDatabaseName(options.mDatabaseName);
    databaseConnection->setUser(options.mDatabaseReadOnlyUser);
    databaseConnection->setPassword(options.mDatabaseReadOnlyPassword);
    databaseConnection->setPort(options.mDatabasePort);
    databaseConnection->connect();
    return databaseConnection;
}

/// Input options
std::string getInputOptions() noexcept
{
    std::string commands{
R"""(
Commands: 
   help       Displays this message.
)"""};
    return commands;
}

