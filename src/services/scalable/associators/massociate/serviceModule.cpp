#include <iostream>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <string>
#include <chrono>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <umps/logging/dailyFile.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/modules/process.hpp>
#include <umps/services/command/service.hpp>
#include <umps/services/command/availableCommandsRequest.hpp>
#include <umps/services/command/availableCommandsResponse.hpp>
#include <umps/services/command/commandRequest.hpp>
#include <umps/services/command/commandResponse.hpp>
#include <umps/services/command/service.hpp>
#include <umps/services/command/serviceOptions.hpp>
#include <umps/services/command/terminateRequest.hpp>
#include <umps/services/command/terminateResponse.hpp>
#include "urts/services/scalable/associators/massociate/service.hpp"
#include "urts/services/scalable/associators/massociate/serviceOptions.hpp"
#include "urts/broadcasts/internal/pick/subscriberOptions.hpp"
#include "urts/broadcasts/internal/pick/subscriber.hpp"
#include "urts/broadcasts/internal/origin/publisherOptions.hpp"
#include "urts/broadcasts/internal/origin/publisher.hpp"
#include "private/isEmpty.hpp"

namespace UAuth = UMPS::Authentication;
namespace UMASS = URTS::Services::Scalable::Associators::MAssociate;

#define MODULE_NAME "mAssociate"

std::pair<std::string, int> parseCommandLineOptions(int argc, char *argv[]);
std::shared_ptr<UMPS::Logging::ILog>
    createLogger(const std::string &moduleName = MODULE_NAME,
                 const std::filesystem::path logFileDirectory = "/var/log/urts",
                 const UMPS::Logging::Level verbosity = UMPS::Logging::Level::Info,
                 const int instance = 0,
                 const int hour = 0,
                 const int minute = 0);
[[nodiscard]] std::string getInputOptions() noexcept;

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
        //----------------Backend Service Connection Information--------------//
        const std::string section{"MAssociate"};
        mServiceName = propertyTree.get<std::string>
                       (section + ".proxyServiceName", mServiceName);
        auto backendAddress = propertyTree.get<std::string>
                              (section + ".proxyServiceAddress", "");
        if (!::isEmpty(backendAddress))
        {
            mMAssociateServiceOptions.setAddress(backendAddress);
        }
        if (::isEmpty(mServiceName) && ::isEmpty(backendAddress))
        {
            throw std::runtime_error("Service backend address indeterminable");
        }
        mMAssociateServiceOptions.setReceiveHighWaterMark(
            propertyTree.get<int> (
                section + ".proxyServiceReceiveHighWaterMark",
                mMAssociateServiceOptions.getReceiveHighWaterMark())
        );
        mMAssociateServiceOptions.setSendHighWaterMark(
            propertyTree.get<int> (
                section + ".proxyServiceSendHighWaterMark",
                mMAssociateServiceOptions.getSendHighWaterMark())
        );

        auto pollingTimeOut 
            = static_cast<int> (
               mMAssociateServiceOptions.getPollingTimeOut().count()
              );
        pollingTimeOut = propertyTree.get<int> (
              section + ".proxyServicePollingTimeOut", pollingTimeOut);
        mMAssociateServiceOptions.setPollingTimeOut(
            std::chrono::milliseconds {pollingTimeOut} );
        //--------------------------Pick Subscriber---------------------------//
        mPickBroadcastName = propertyTree.get<std::string>
                             (section + ".pickBroadcastName",
                              mPickBroadcastName);
        auto pickBroadcastAddress = propertyTree.get<std::string>
                                    (section + ".pickBroadcastAddress", "");
        if (!::isEmpty(pickBroadcastAddress))
        {
            mPickSubscriberOptions.setAddress(pickBroadcastAddress);
        }
        if (::isEmpty(mPickBroadcastName) && ::isEmpty(pickBroadcastAddress))
        {
            throw std::runtime_error("Pick broadcast address indeterminable");
        }
        mPickSubscriberOptions.setHighWaterMark(
            propertyTree.get<int> (section + ".pickBroadcastHighWaterMark", 0));
        auto pickTimeOut
            = static_cast<int> (mPickSubscriberOptions.getTimeOut().count());
        pickTimeOut = propertyTree.get<int> (section + ".pickBroadcastTimeOut",
                                             pickTimeOut);
        mPickSubscriberOptions.setTimeOut(
            std::chrono::milliseconds {pickTimeOut} );
        //--------------------------Origin Publisher--------------------------//
        mOriginBroadcastName = propertyTree.get<std::string>
                               (section + ".originBroadcastName",
                                mOriginBroadcastName);
        auto originBroadcastAddress = propertyTree.get<std::string>
                                      (section + ".originBroadcastAddress", "");
        if (!::isEmpty(originBroadcastAddress))
        {
            mOriginPublisherOptions.setAddress(originBroadcastAddress);
        }
        if (::isEmpty(mOriginBroadcastName) &&
            ::isEmpty(originBroadcastAddress))
        {
            throw std::runtime_error("Origin broadcast address indeterminable");
        }
        mOriginPublisherOptions.setHighWaterMark(
            propertyTree.get<int>
            (section + ".originBroadcastHighWaterMark", 0));
        auto originTimeOut
            = static_cast<int> (mOriginPublisherOptions.getTimeOut().count());
        originTimeOut
            = propertyTree.get<int> (section + ".originBroadcastTimeOut",
                                     originTimeOut);
        mOriginPublisherOptions.setTimeOut(
            std::chrono::milliseconds {originTimeOut} );
        //--------------------------Associator options------------------------//
        auto region = propertyTree.get<std::string> (section + ".region");
        if (region == "utah" || region == "Utah")
        {
            mMAssociateServiceOptions.setRegion(
                UMASS::ServiceOptions::Region::Utah);
            mMAssociateServiceOptions.setExtentInDepth(
                std::pair {-1700, 22000});
            mMAssociateServiceOptions.setDBSCANEpsilon(0.25);
        }
        else if (region == "YNP" || region == "ynp" || region == "Yellowstone")
        {
            mMAssociateServiceOptions.setRegion(
                UMASS::ServiceOptions::Region::YNP);
            mMAssociateServiceOptions.setExtentInDepth(
                std::pair {-1000, 16000});
            mMAssociateServiceOptions.setDBSCANEpsilon(0.2);
        }
        else
        {
            throw std::invalid_argument("Unhandled region: " + region);
        }
        // Load the variables with defaults
        auto minimumSearchDepth
            = propertyTree.get<double>
              (section + ".minimumSearchDepth",
               mMAssociateServiceOptions.getExtentInDepth().first);
        auto maximumSearchDepth
            = propertyTree.get<double>
              (section + ".maximumSearchDepth",
               mMAssociateServiceOptions.getExtentInDepth().second);
        mMAssociateServiceOptions.setExtentInDepth(
            std::pair {minimumSearchDepth, maximumSearchDepth});
        auto maximumDistance 
            = propertyTree.get<double>
              (section + ".maximumDistanceToAssociate",
               mMAssociateServiceOptions.getMaximumDistanceToAssociate());
        mMAssociateServiceOptions.setMaximumDistanceToAssociate(
            maximumDistance);
        // DBSCAN: epsilon
        auto dbscanEpsilon
            = propertyTree.get<double>
              (section + ".dbscanEpsilon",
               mMAssociateServiceOptions.getDBSCANEpsilon());
        mMAssociateServiceOptions.setDBSCANEpsilon(dbscanEpsilon);
        // DBSCAN: minimum cluster size
        auto dbscanMinimumClusterSize
            = propertyTree.get<int>
              (section + ".dbscanMinimumClusterSize",
               mMAssociateServiceOptions.getDBSCANMinimumClusterSize());
        mMAssociateServiceOptions.setDBSCANMinimumClusterSize(
            dbscanMinimumClusterSize);
        // PSO: number of epochs
        auto nPSOEpochs
            = propertyTree.get<int>
              (section + ".numberOfEpochs",
               mMAssociateServiceOptions.getNumberOfEpochs());
        mMAssociateServiceOptions.setNumberOfEpochs(nPSOEpochs);
        auto nPSOParticles
            = propertyTree.get<int>
              (section + ".numberOfParticles",
               mMAssociateServiceOptions.getNumberOfParticles());
        mMAssociateServiceOptions.setNumberOfParticles(nPSOParticles);
        // Corrections files
        auto staticCorrectionFile
            = propertyTree.get<std::string>
              (section + ".staticCorrections", "");
        if (!staticCorrectionFile.empty())
        {
            mMAssociateServiceOptions.setStaticCorrectionFile(
               staticCorrectionFile);
        }
        auto sourceSpecificCorrectionFile
            = propertyTree.get<std::string>
              (section + ".sourceSpecificCorrections", ""); 
        if (!sourceSpecificCorrectionFile.empty())
        {
            mMAssociateServiceOptions.setSourceSpecificCorrectionFile(
               sourceSpecificCorrectionFile);
        }
    }
    std::string mModuleName{MODULE_NAME};
    std::string mServiceName{"MAssociate"};
    std::string mPickBroadcastName{"Pick"};
    std::string mOriginBroadcastName{"Origin"};
    std::string mHeartbeatBroadcastName{"Heartbeat"};
    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    URTS::Broadcasts::Internal::Pick::SubscriberOptions mPickSubscriberOptions;
    URTS::Broadcasts::Internal::Origin::PublisherOptions
        mOriginPublisherOptions;
    UMASS::ServiceOptions mMAssociateServiceOptions;
    UAuth::ZAPOptions mZAPOptions;
    std::chrono::seconds heartBeatInterval{30};
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
    int mInstance{0};
};

class Associator : public UMPS::Modules::IProcess
{
public:
    Associator() = delete;
    /// @brief Constructor.
    Associator(const std::string &moduleName,
               std::unique_ptr<UMASS::Service> &&associator,
               std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mAssociator(std::move(associator)),
        mLogger(logger)
    {
        if (moduleName.empty())
        {
            throw std::invalid_argument("Module name is empty");
        }
        if (mAssociator == nullptr)
        {
            throw std::invalid_argument("Associator is NULL");
        }
        if (!mAssociator->isInitialized())
        {
            throw std::invalid_argument("Associator not initialized");
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
            std::bind(&Associator::commandCallback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
        mLocalCommand->initialize(localServiceOptions);
        mInitialized = true;
    }
    /// @brief Destructor
    ~Associator() override
    {
        stop();
    }
    /// @result True indicates the class is initialized
    [[nodiscard]] bool isInitialized() const noexcept
    {
        std::scoped_lock lock(mMutex); 
        return mInitialized;
    }
    /// @result True indicates this should keep running
    [[nodiscard]] bool keepRunning() const
    {
        return mKeepRunning; 
    }
    /// @brief Stops the process.
    void stop() override
    {
        setRunning(false);
        if (mAssociator != nullptr)
        {
            if (mAssociator->isRunning()){mAssociator->stop();}
        }
        if (mLocalCommand != nullptr)
        {
            if (mLocalCommand->isRunning()){mLocalCommand->stop();}
        }
    }
    /// @brief Toggles this as running or not running
    void setRunning(const bool running)
    {
        //std::lock_guard<std::mutex> lockGuard(mMutex);
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
                    /*
                    auto cacheSize = std::to_string(
                        mPacketCache->getTotalNumberOfPackets());
                    response.setResponse(cacheSize);
                    response.setReturnCode(
                        USC::CommandResponse::ReturnCode::Success);
                    */
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
    mutable std::mutex mMutex;
    std::unique_ptr<UMASS::Service> mAssociator{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<UMPS::Services::Command::Service> mLocalCommand{nullptr};
    std::atomic<bool> mKeepRunning{true};
    bool mInitialized{false};
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
    auto logger = ::createLogger(programOptions.mModuleName,
                                 programOptions.mLogFileDirectory,
                                 programOptions.mVerbosity,
                                 programOptions.mInstance,
                                 hour, minute);

    return EXIT_SUCCESS;
}

/// Read the program options from the command line
std::pair<std::string, int> parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(
R"""(
This service allows a client to associate picks into an event.
This is a scalable service so it is conceivable the user will have multiple
instances running simultaneously so as to satisfy the overall application's
performance needs.  Example usage:

    massociateService --ini=massociateService.ini --instance=0

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

std::shared_ptr<UMPS::Logging::ILog>
    createLogger(const std::string &moduleName,
                 const std::filesystem::path logFileDirectory,
                 const UMPS::Logging::Level verbosity,
                 const int instance,
                 const int hour,
                 const int minute)
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
};

std::string getInputOptions() noexcept
{
    std::string commands{
R"""(
Commands: 
   help       Displays this message.
)"""};
    return commands;
}

