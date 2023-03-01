#include <mutex>
#include <iostream>
#include <filesystem>
#include <string>
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
#include <umps/services/connectionInformation/details.hpp>
#include <umps/services/connectionInformation/requestorOptions.hpp>
#include <umps/services/connectionInformation/requestor.hpp>
#include <umps/services/connectionInformation/socketDetails/dealer.hpp>
#include <umps/services/connectionInformation/details.hpp>
#include <umps/services/command/availableCommandsRequest.hpp>
#include <umps/services/command/availableCommandsResponse.hpp>
#include <umps/services/command/commandRequest.hpp>
#include <umps/services/command/commandResponse.hpp>
#include <umps/services/command/service.hpp>
#include <umps/services/command/serviceOptions.hpp>
#include <umps/services/command/terminateRequest.hpp>
#include <umps/services/command/terminateResponse.hpp>
#include "urts/services/scalable/pickers/cnnOneComponentP/serviceOptions.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/service.hpp"
#include "private/isEmpty.hpp"

namespace UAuth = UMPS::Authentication;
namespace UCI = UMPS::Services::ConnectionInformation;
namespace PPicker = URTS::Services::Scalable::Pickers::CNNOneComponentP;

#define MODULE_NAME "PPickRegressorService"

/// @result Gets the command line input options as a string.
[[nodiscard]] std::string getInputOptions() noexcept
{
    std::string commands{
R"""(
Commands: 
   help       Displays this message.
)"""};
    return commands;
}

/// Parse the command line options
[[nodiscard]] std::pair<std::string, int>
    parseCommandLineOptions(int argc, char *argv[]);

/// @result The logger for this application.
std::shared_ptr<UMPS::Logging::ILog>
    createLogger(const std::string &moduleName = MODULE_NAME,
                 const std::filesystem::path logFileDirectory = "/var/log/urts",
                 const UMPS::Logging::Level verbosity = UMPS::Logging::Level::Info,
                 const int instance = 0,
                 const int hour = 0,
                 const int minute = 0)
{
    auto logFileName = moduleName + "_" + std::to_string(instance) + ".log";
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
        //-----------------------ML Model Options-----------------------------//
        std::string weightsFileGuess
        {
            "/usr/local/share/UUSSMLModels/pickersPPickRegressor.onnx"
        };
        std::string weightsFile{""};
        if (std::filesystem::exists(weightsFileGuess))
        {
            weightsFile = weightsFileGuess;
        }
        weightsFile
            = propertyTree.get<std::string> ("PPickRegressor.weightsFile",
                                             weightsFile);
        mServiceOptions.setModelWeightsFile(weightsFile);

        PPicker::ServiceOptions::Device device
            = PPicker::ServiceOptions::Device::CPU;
        auto deviceString
            = propertyTree.get<std::string> ("PPickRegressor.device",
                                             "cpu");
        std::transform(deviceString.begin(), deviceString.end(),
                       deviceString.begin(), ::toupper);
        if (deviceString == std::string {"GPU"})
        {
            device = PPicker::ServiceOptions::Device::GPU;
        }
        mServiceOptions.setDevice(device);
        //----------------Backend Service Connection Information--------------//
        mServiceName
             = propertyTree.get<std::string>
               ("PPickRegressor.proxyServiceName", mServiceName);
    
        auto backendAddress
             = propertyTree.get<std::string>
               ("PPickRegressor.proxyServiceAddress", "");
        if (!::isEmpty(backendAddress))
        {
            mServiceOptions.setAddress(backendAddress);
        }
        if (::isEmpty(mServiceName) && ::isEmpty(backendAddress))
        {
            throw std::runtime_error("Service backend address indeterminable");
        }
        mServiceOptions.setReceiveHighWaterMark(
            propertyTree.get<int> (
                "PPickRegressor.proxyServiceReceiveHighWaterMark",
                mServiceOptions.getReceiveHighWaterMark())
        );
        mServiceOptions.setSendHighWaterMark(
            propertyTree.get<int> (
                "PPickRegressor.proxyServiceSendHighWaterMark",
                mServiceOptions.getSendHighWaterMark())
        );
        auto pollingTimeOut 
            = static_cast<int> (mServiceOptions.getPollingTimeOut().count()
              );
        pollingTimeOut = propertyTree.get<int> (
              "PPickRegressor.proxyServicePollingTimeOut", pollingTimeOut);
        mServiceOptions.setPollingTimeOut(
            std::chrono::milliseconds {pollingTimeOut} );
    }
//public:
    PPicker::ServiceOptions mServiceOptions;
    std::string mServiceName{"PPickRegressor"};
    std::string mHeartbeatBroadcastName{"Heartbeat"};
    std::string mModuleName{MODULE_NAME};
    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    UAuth::ZAPOptions mZAPOptions;
    std::chrono::seconds heartBeatInterval{30};
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
    int mInstance{0};
};

/// The process that manages the machine learning inference engine.
class InferenceEngine : public UMPS::Modules::IProcess
{
public:
    /// @brief Default constructor.
    InferenceEngine() = delete;
    /// @brief Constructor.
    InferenceEngine(const std::string &moduleName,
                    std::unique_ptr<PPicker::Service> &&inferenceService,
                    std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mInferenceService(std::move(inferenceService)),
        mLogger(logger)
    {
        if (mInferenceService == nullptr)
        {
            throw std::invalid_argument("Inference service is NULL");
        }
        if (!mInferenceService->isInitialized())
        {
            throw std::invalid_argument("Inference service not initialized");
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
            std::bind(&InferenceEngine::commandCallback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
        mLocalCommand->initialize(localServiceOptions);

        mInitialized = true;
    }
    /// @brief Destructor.
    ~InferenceEngine()
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
        if (mInferenceService != nullptr)
        {
            if (mInferenceService->isRunning()){mInferenceService->stop();}
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
            throw std::runtime_error("Inference engine not initialized");
        }
        setRunning(true);
        mLogger->debug("Starting the inference service...");
        mInferenceService->start();
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
        commandsResponse.setCommands(getInputOptions());
        return commandsResponse.clone();
    }
private:
    mutable std::mutex mMutex;
    std::unique_ptr<PPicker::Service> mInferenceService{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<UMPS::Services::Command::Service> mLocalCommand{nullptr};
    std::unique_ptr<UMPS::ProxyServices::Command::Replier>
        mModuleRegistryReplier{nullptr};
    bool mKeepRunning{true};
    bool mInitialized{false};
};


int main(int argc, char *argv[])
{
    // Get the ini file from the command line
    std::string iniFile;
    int instance = 0;
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
    ProgramOptions programOptions(instance);
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
                               programOptions.mInstance,
                               hour, minute);
    // Create a context
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
                                                            context,
                                                            logger);
        processManager.insert(std::move(heartbeat));

        // Create the module command replier
        logger->debug("Creating module registry replier process...");
        namespace URemoteCommand = UMPS::ProxyServices::Command;
        URemoteCommand::ModuleDetails moduleDetails;
        moduleDetails.setName(programOptions.mModuleName);
        moduleDetails.setInstance(instance);

        // Get the backend service connection details
        if (!programOptions.mServiceOptions.haveAddress())
        {
            auto address = uOperator->getProxyServiceBackendDetails(
                              programOptions.mServiceName).getAddress();
            programOptions.mServiceOptions.setAddress(address);
        }
        programOptions.mServiceOptions.setZAPOptions(
            programOptions.mZAPOptions);

        // Create the service and the subsequent process
        auto inferenceService
            = std::make_unique<PPicker::Service> (context, logger);
        inferenceService->initialize(programOptions.mServiceOptions);
        auto inferenceProcess
           = std::make_unique<::InferenceEngine> (programOptions.mModuleName,
                                                  std::move(inferenceService),
                                                  logger);

        // Create the remote replier
        auto callbackFunction = std::bind(&InferenceEngine::commandCallback,
                                          &*inferenceProcess,
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
        processManager.insert(std::move(inferenceProcess));
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


///--------------------------------------------------------------------------///
///                            Utility Functions                             ///
///--------------------------------------------------------------------------///
/// Read the program options from the command line
std::pair<std::string, int> parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(
R"""(
The pPickRegressorService regressor allows a module to improve an initial P
pick.  This is a scalable service so it is likely that the user will have
multiple instances running simultaneously so as to satisfy the overall
application's inference needs.  Example usage:
    pRegressorService --ini=pRegressor.ini --instance=0
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
    int instance = 0;
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

