#include <iostream>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <string>
#include <chrono>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <massociate/associator.hpp>
#include <massociate/dbscan.hpp>
#include <massociate/migrator.hpp>
#include <massociate/optimizer.hpp>
#include <massociate/particleSwarm.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/dailyFile.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/modules/process.hpp>
#include <umps/modules/processManager.hpp>
#include <umps/proxyBroadcasts/heartbeat/publisherProcess.hpp>
#include <umps/proxyServices/command/moduleDetails.hpp>
#include <umps/proxyServices/command/replier.hpp>
#include <umps/proxyServices/command/replierOptions.hpp>
#include <umps/proxyServices/command/replierProcess.hpp>
#include <umps/services/command/availableCommandsRequest.hpp>
#include <umps/services/command/availableCommandsResponse.hpp>
#include <umps/services/command/commandRequest.hpp>
#include <umps/services/command/commandResponse.hpp>
#include <umps/services/command/service.hpp>
#include <umps/services/command/serviceOptions.hpp>
#include <umps/services/command/terminateRequest.hpp>
#include <umps/services/command/terminateResponse.hpp>
#include <umps/services/connectionInformation/details.hpp>
#include <umps/services/connectionInformation/requestorOptions.hpp>
#include <umps/services/connectionInformation/requestor.hpp>
#include <umps/services/connectionInformation/socketDetails/dealer.hpp>
#include <umps/services/connectionInformation/details.hpp>
#include <uLocator/travelTimeCalculatorMap.hpp>
#include <uLocator/uussRayTracer.hpp>
#include <uLocator/station.hpp>
#include <uLocator/corrections/sourceSpecific.hpp>
#include <uLocator/corrections/static.hpp>
#include <uLocator/position/wgs84.hpp>
#include <uLocator/position/utahRegion.hpp>
#include <uLocator/position/ynpRegion.hpp>
#include "urts/services/scalable/associators/massociate/service.hpp"
#include "urts/services/scalable/associators/massociate/serviceOptions.hpp"
#include "urts/broadcasts/internal/pick/subscriberOptions.hpp"
#include "urts/broadcasts/internal/pick/subscriber.hpp"
#include "urts/broadcasts/internal/origin/publisherOptions.hpp"
#include "urts/broadcasts/internal/origin/publisher.hpp"
#include "urts/database/aqms/stationData.hpp"
#include "urts/database/aqms/stationDataTable.hpp"
#include "urts/database/connection/postgresql.hpp"
#include "createTravelTimeCalculator.hpp"
#include "private/isEmpty.hpp"

namespace MASS = MAssociate;
namespace UAuth = UMPS::Authentication;
namespace UMASS = URTS::Services::Scalable::Associators::MAssociate;
namespace UDatabase = URTS::Database;

#define MODULE_NAME "mAssociate"

struct ProgramOptions;
std::pair<std::string, int> parseCommandLineOptions(int argc, char *argv[]);
std::shared_ptr<UMPS::Logging::ILog>
    createLogger(const std::string &moduleName = MODULE_NAME,
                 const std::filesystem::path logFileDirectory = "/var/log/urts",
                 const UMPS::Logging::Level verbosity = UMPS::Logging::Level::Info,
                 const int instance = 0,
                 const int hour = 0,
                 const int minute = 0);
[[nodiscard]] std::string getInputOptions() noexcept;
[[nodiscard]] std::shared_ptr<UDatabase::Connection::IConnection> 
    createAQMSDatabaseConnection(const ::ProgramOptions options);

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
/*
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
*/
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
        mMinimumNumberOfArrivals
            = propertyTree.get<int> (section + ".minimumNumberOfArrivals",
                                     mMinimumNumberOfArrivals);
        if (mMinimumNumberOfArrivals < 4)
        {
            throw std::invalid_argument(
               "Minimum number of arrivals must be positive");
        }
        mMinimumNumberOfPArrivals 
            = propertyTree.get<int> (section + ".minimumNumberOfPArrivals",
                                     mMinimumNumberOfPArrivals);
        mMinimumNumberOfPArrivals = std::max(mMinimumNumberOfPArrivals, 0);

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
    std::string mHeartbeatBroadcastName{"Heartbeat"}; // TODO do i need this?
    std::string mDatabaseAddress;
    std::string mDatabaseName;
    std::string mDatabaseReadOnlyUser;
    std::string mDatabaseReadOnlyPassword;
    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    URTS::Broadcasts::Internal::Pick::SubscriberOptions mPickSubscriberOptions;
    URTS::Broadcasts::Internal::Origin::PublisherOptions
        mOriginPublisherOptions;
    UMASS::ServiceOptions mMAssociateServiceOptions;
    UAuth::ZAPOptions mZAPOptions;
    std::chrono::seconds heartBeatInterval{30};
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
    int mMinimumNumberOfArrivals{7};
    int mMinimumNumberOfPArrivals{4};
    int mDatabasePort{5432};
    int mInstance{0};
};

class AssociatorProcess : public UMPS::Modules::IProcess
{
public:
    AssociatorProcess() = delete;
    /// @brief Constructor.
    AssociatorProcess(const std::string &moduleName,
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
            std::bind(&AssociatorProcess::commandCallback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
        mLocalCommand->initialize(localServiceOptions);
        mInitialized = true;
    }
    /// @brief Destructor
    ~AssociatorProcess() override
    {
        stop();
    }
    /// @result True indicates the class is initialized
    [[nodiscard]] bool isInitialized() const noexcept
    {
        //std::scoped_lock lock(mMutex); 
        return mInitialized;
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
    /// @brief Stops the processes.
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
    /// @brief Starts the processes.
    void start() override
    {
        stop();
        if (!isInitialized())
        {
            throw std::invalid_argument("Associator not initialized");
        }
        setRunning(true);
        mLogger->debug("Starting the associator service...");
        mAssociator->start();
        mLogger->debug("Starting the local command proxy...");
        mLocalCommand->start();
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
    //mutable std::mutex mMutex;
    std::unique_ptr<UMASS::Service> mAssociator{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<UMPS::Services::Command::Service> mLocalCommand{nullptr};
    std::atomic<bool> mKeepRunning{true};
    std::atomic<bool> mInitialized{false};
};

///--------------------------------------------------------------------------///

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
    // Get the database connection
    std::shared_ptr<UDatabase::Connection::IConnection>
        aqmsDatabaseConnection{nullptr};
    std::vector<UDatabase::AQMS::StationData> aqmsStations;
    try
    {
        aqmsDatabaseConnection = ::createAQMSDatabaseConnection(programOptions); 
        UDatabase::AQMS::StationDataTable
            stationDataTable{aqmsDatabaseConnection, logger}; 
        stationDataTable.queryCurrent();
        aqmsStations = stationDataTable.getStationData();
    }
    catch (const std::exception &e)
    {
        logger->error(std::string {e.what()});
        return EXIT_FAILURE;
    }
    // Initialize
    bool isUtah{true};
    std::unique_ptr<ULocator::Position::IGeographicRegion>
        geographicRegion{nullptr};
    if (programOptions.mMAssociateServiceOptions.getRegion() ==
        UMASS::ServiceOptions::Region::YNP)
    {
        isUtah = false;
        geographicRegion = std::make_unique<ULocator::Position::YNPRegion> ();
    }
    else if (programOptions.mMAssociateServiceOptions.getRegion() ==
        UMASS::ServiceOptions::Region::Utah)
    {
        geographicRegion = std::make_unique<ULocator::Position::UtahRegion> (); 
        isUtah = true;
    }
    else
    {
        logger->error("Unhandled region!");
        return EXIT_FAILURE;
    }

    // Get the travel time calculators.  Since 1D calculators are cheap we'll
    // make one for everything in the database.
    logger->info("Creating travel time calculators...");
    auto travelTimeCalculatorMap
        = std::make_unique<ULocator::TravelTimeCalculatorMap> (); 
    bool doStaticCorrections{false};
    auto staticCorrectionsFile
        = programOptions.mMAssociateServiceOptions.getStaticCorrectionFile();
    if (std::filesystem::exists(staticCorrectionsFile))
    {
        doStaticCorrections = true;
    }
    bool doSourceSpecificCorrections{false};
    auto sourceSpecificCorrectionsFile
        = programOptions.mMAssociateServiceOptions
                        .getSourceSpecificCorrectionFile();
    if (std::filesystem::exists(sourceSpecificCorrectionsFile))
    {
        doSourceSpecificCorrections = true;
    }
    for (const auto &aqmsStation : aqmsStations)
    {
        ULocator::Station station;
        if (isUtah)
        {
            station = ::createUtahStation(aqmsStation);
        }
        else
        {
            station = ::createYNPStation(aqmsStation);
        }
        // Travel time calculator for each phase 
        std::vector<std::string> phases{"P", "S"};
        for (const auto &thisPhase : phases)
        {
            try
            {
                std::filesystem::path staticsFile;
                if (doStaticCorrections){staticsFile = staticCorrectionsFile;}
                std::filesystem::path ssscFile;
                if (doSourceSpecificCorrections)
                {
                    ssscFile = sourceSpecificCorrectionsFile;
                }
                auto rayTracer = ::createTravelTimeCalculator(station,
                                                              thisPhase,
                                                              isUtah,
                                                              staticsFile,
                                                              ssscFile,
                                                              logger);
                travelTimeCalculatorMap->insert(station, thisPhase,
                                                std::move(rayTracer));
            }
            catch (const std::exception &e)
            {
                logger->warn("Did not add calculator because "
                           + std::string {e.what()});
            }

        } // Loop on phases
    } // Loop on stations in database
    // Create the clusterer
    auto clusterer = std::make_unique<MASS::DBSCAN> (); 
    clusterer->initialize(
        programOptions.mMAssociateServiceOptions.getDBSCANEpsilon(),
        programOptions.mMAssociateServiceOptions.getDBSCANMinimumClusterSize());
    // Create the migrator
    auto migrator = std::make_unique<MASS::IMigrator> (logger);
    if (isUtah)
    {
        migrator->setDefaultSearchLocations(::createKnownUtahSearchLocations());
    }
    else
    {
        migrator->setDefaultSearchLocations(::createKnownYNPSearchLocations());
    }
    migrator->setTravelTimeCalculatorMap(std::move(travelTimeCalculatorMap));
    migrator->setGeographicRegion(*geographicRegion->clone());
    migrator->setPickSignalToMigrate(MASS::IMigrator::PickSignal::Boxcar);
    migrator->setMaximumEpicentralDistance(
        programOptions.mMAssociateServiceOptions
                      .getMaximumDistanceToAssociate());
    // Create the optimizer for the migrator
    auto psoOptimizer = std::make_unique<MAssociate::ParticleSwarm> (logger);
    auto searchDepthExtent
        = programOptions.mMAssociateServiceOptions.getExtentInDepth();
    if (std::abs(searchDepthExtent.second - searchDepthExtent.first) > 1.)
    {
        psoOptimizer->setExtentInZ(searchDepthExtent);
        psoOptimizer->enableSearchDepth();
    }
    else
    {
        auto searchDepth
            = 0.5*(searchDepthExtent.first + searchDepthExtent.second); 
        psoOptimizer->setDepth(searchDepth);
        psoOptimizer->disableSearchDepth();
    }
    psoOptimizer->setNumberOfParticles(
        programOptions.mMAssociateServiceOptions.getNumberOfParticles());
    psoOptimizer->setNumberOfGenerations(
        programOptions.mMAssociateServiceOptions.getNumberOfEpochs());
    psoOptimizer->setMigrator(std::move(migrator));
    auto associator = std::make_unique<MASS::Associator> (logger); 
    try
    {
        associator->initialize(programOptions.mMinimumNumberOfArrivals,
                               programOptions.mMinimumNumberOfPArrivals);
        associator->setOptimizer(std::move(psoOptimizer));
        associator->setClusterer(std::move(clusterer));
    }
    catch (const std::exception &e)
    {
        logger->error(std::string {e.what()});
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
        if (!programOptions.mMAssociateServiceOptions.haveAddress())
        {
            auto address = uOperator->getProxyServiceBackendDetails(
                              programOptions.mServiceName).getAddress();
            programOptions.mMAssociateServiceOptions.setAddress(address);
        }
        programOptions.mMAssociateServiceOptions.setZAPOptions(
            programOptions.mZAPOptions);

        // Create the service and the subsequent process
        auto associatorService
            = std::make_unique<UMASS::Service> (context, logger);
        associatorService->initialize(programOptions.mMAssociateServiceOptions,
                                      std::move(associator),
                                      aqmsDatabaseConnection);
        auto associatorProcess
            = std::make_unique<::AssociatorProcess>
                 (programOptions.mModuleName,
                  std::move(associatorService),
                  logger);

        // Create the remote replier
        auto callbackFunction = std::bind(&AssociatorProcess::commandCallback,
                                          &*associatorProcess,
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
        processManager.insert(std::move(associatorProcess));
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

std::string getInputOptions() noexcept
{
    std::string commands{
R"""(
Commands: 
   help       Displays this message.
)"""};
    return commands;
}


std::shared_ptr<UDatabase::Connection::IConnection> 
createAQMSDatabaseConnection(const ::ProgramOptions options)
{
    auto databaseConnection
       = std::make_shared<UDatabase::Connection::PostgreSQL> ();
    databaseConnection->setAddress(options.mDatabaseAddress);
    databaseConnection->setDatabaseName(options.mDatabaseName);
    databaseConnection->setUser(options.mDatabaseReadOnlyUser);
    databaseConnection->setPassword(options.mDatabaseReadOnlyPassword);
    databaseConnection->setPort(options.mDatabasePort);
    databaseConnection->connect();
    return databaseConnection;
}

