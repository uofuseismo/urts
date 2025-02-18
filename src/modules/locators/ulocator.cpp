#include <iostream>
#include <iomanip>
#include <atomic>
#include <thread>
#include <vector>
#include <filesystem>
#include <string>
#include <chrono>
#ifndef NDEBUG
#include <cassert>
#endif
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/logging/dailyFile.hpp>
#include <umps/messaging/context.hpp>
#include <umps/modules/process.hpp>
#include <umps/modules/processManager.hpp>
#include <umps/modules/module.hpp>
#include <umps/proxyBroadcasts/heartbeat.hpp>
#include <umps/proxyServices/command.hpp>
#include <umps/services/command.hpp>
#include <umps/services/connectionInformation.hpp>
#include "urts/broadcasts/internal/origin.hpp"
#include "urts/services/standalone/incrementer.hpp"
#include "urts/services/scalable/locators/uLocator/arrival.hpp"
#include "urts/services/scalable/locators/uLocator/origin.hpp"
#include "urts/services/scalable/locators/uLocator/requestor.hpp"
#include "urts/services/scalable/locators/uLocator/requestorOptions.hpp"
#include "urts/services/scalable/locators/uLocator/locationRequest.hpp"
#include "urts/services/scalable/locators/uLocator/locationResponse.hpp"
#include "private/threadSafeQueue.hpp"
#include "private/isEmpty.hpp"
#include "examples.hpp"

#define MODULE_NAME "uLocator"

/// @brief Converts a origin arrival to a ulocator arrival.
/// TODO put this in arrival
URTS::Services::Scalable::Locators::ULocator::Arrival fromArrival(
    const URTS::Broadcasts::Internal::Origin::Arrival &arrival)
{
    URTS::Services::Scalable::Locators::ULocator::Arrival result;
    result.setTime(arrival.getTime());
    result.setNetwork(arrival.getNetwork());
    result.setStation(arrival.getStation());
    if (arrival.getPhase() == URTS::Broadcasts::Internal::Origin::Arrival::Phase::P)
    {
        result.setPhase(URTS::Services::Scalable::Locators::ULocator::Arrival::Phase::P);
    }
    else if (arrival.getPhase() == URTS::Broadcasts::Internal::Origin::Arrival::Phase::S)
    {
        result.setPhase(URTS::Services::Scalable::Locators::ULocator::Arrival::Phase::S);
    }
    else
    {
        throw std::runtime_error("Unhandled phase");
    } 
    return result; 
}

/*
URTS::Broadcasts::Internal::Origin::Arrival fromArrival(
    const URTS::Services::Scalable::Locators::ULocator::Arrival &arrival,
    const double originTime)
{
    URTS::Broadcasts::Internal::Origin::Arrival result;
    if (arrival.getIdentifier())
    {
        result.setIdentifier(*arrival.getIdentifier());
    }
    result.setTime(arrival.getTime());
    result.setNetwork(arrival.getNetwork());
    result.setStation(arrival.getStation());
    //result.setChannel(arrival.getChannel());
    //result.setLocationCode(arrival.getLocationCode());
    if (arrival.getPhase() == URTS::Services::Scalable::Locators::ULocator::Arrival::Phase::P)
    {   
        result.setPhase(URTS::Broadcasts::Internal::Origin::Arrival::Phase::P);
    }   
    else
    {   
        result.setPhase(URTS::Broadcasts::Internal::Origin::Arrival::Phase::S);
    }   
    auto travelTime = arrival.getTravelTime();
    if (travelTime)
    {
        result.setResidual(result.getTime().count()*1.e-6
                         - (originTime + *travelTime));
    }
    if (arrival.getIdentifier())
    {
        result.setIdentifier(*arrival.getIdentifier());
    }
    return result;
}
*/

URTS::Broadcasts::Internal::Origin::Origin
    fromOrigin(const URTS::Broadcasts::Internal::Origin::Origin &initialOrigin,
               const URTS::Services::Scalable::Locators::ULocator::Origin &origin)
{
    auto result = initialOrigin;
    // Get the refined origin
    result.setLatitude(origin.getLatitude());
    result.setLongitude(origin.getLongitude());
    result.setDepth(origin.getDepth());
    result.setTime(origin.getTime());
    auto originTime = origin.getTime().count()*1.e-6;
    // Now we do this tricky business of matching the initial arrivals with
    // the arrivals used in location.  Basically, the arrivals riding on the
    // origin to relocate have much richer information than what the locator
    // needs
    auto initialArrivals = result.getArrivals();
    auto relocatedArrivals = origin.getArrivals();
    std::vector<URTS::Broadcasts::Internal::Origin::Arrival> arrivals;
    for (auto &initialArrival : initialArrivals)
    {
        auto network = initialArrival.getNetwork();
        auto station = initialArrival.getStation();
        std::string phase{"P"};
        if (initialArrival.getPhase() ==
            URTS::Broadcasts::Internal::Origin::Arrival::Phase::P)
        {
            phase = "P";
        }
        else if (initialArrival.getPhase() ==
                 URTS::Broadcasts::Internal::Origin::Arrival::Phase::S)
        {
            phase = "S";
        }
        else
        {
            std::cerr << "unhandled phase" << std::endl;
            continue;
        }
        // Hunt for match
        bool found{false};
        for (auto &relocatedArrival : relocatedArrivals)
        {
            std::string relocatedArrivalPhase{"P"};
            if (relocatedArrival.getPhase() ==
                URTS::Services::Scalable::Locators::ULocator::Arrival::Phase::P)
            {
                relocatedArrivalPhase = "P";
            }
            else if (relocatedArrival.getPhase() ==
                     URTS::Services::Scalable::Locators::ULocator::Arrival::Phase::S)
            {
                relocatedArrivalPhase = "S";
            }
            else
            {
                std::cerr << "unhandled phase 2" << std::endl;
                continue;
            } 
            // Update the residual (if possible) and (re)save the initial
            // arrival
            if (relocatedArrival.getNetwork() == network &&
                relocatedArrival.getStation() == station &&
                relocatedArrivalPhase == phase)
            {
                auto travelTime = relocatedArrival.getTravelTime();
                if (travelTime)
                {
                    initialArrival.setResidual(
                        initialArrival.getTime().count()*1.e-6
                      - (originTime + *travelTime));
                }   
                arrivals.push_back(initialArrival);
                found = true;
                break;
            }
        }
        if (!found)
        {
            std::cerr << "failed to find arrival match" << std::endl;
        }
    }
    result.setArrivals(arrivals); 
    return result;
}

/// @brief Gets the input options for the (remote) command callback
[[nodiscard]] std::string getInputOptions() noexcept;
/// @result The logger for this application.
std::shared_ptr<UMPS::Logging::ILog>
createLogger(const std::string &moduleName = MODULE_NAME,
             const std::filesystem::path logFileDirectory = "/var/log/urts",
             const UMPS::Logging::Level verbosity = UMPS::Logging::Level::Info,
             const int hour = 0, 
             const int minute = 0) 
{
auto logger = std::make_shared<UMPS::Logging::StandardOut> (UMPS::Logging::Level::Debug);//verbosity);
return logger;
/*
    auto logFileName = moduleName + ".log";
    auto fullLogFileName = logFileDirectory / logFileName;
    auto logger = std::make_shared<UMPS::Logging::DailyFile> ();  
    logger->initialize(moduleName,
                       fullLogFileName,
                       verbosity,
                       hour, minute);
    logger->info("Starting logging for " + moduleName);
    return logger;
*/
}

/// @result Gets the command line input options as a string.
std::string getInputOptions() noexcept
{
    std::string commands{
R"""(
Commands: 
   help    Displays this message.
)"""};
    return commands;
}

/// @brief Parses the command line options.
[[nodiscard]] std::string parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(
R"""(
The uLocator drives the locators that refine preliminary events built by the
associator.
Example usage:
    uLocator --ini=uLocator.ini
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

struct ProgramOptions
{
    void parseInitializationFile(const std::filesystem::path &iniFile)
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
        //------------------------- Associator Options -----------------------//
        std::string section{"uLocator"};

        auto region = propertyTree.get<std::string> (section + ".region");
        if (region == "utah" || region == "Utah")
        {
            mIsUtah = true;
        }
        else if (region == "YNP" || region == "ynp" || region == "Yellowstone")
        {
            mIsUtah = false;
        }
        else
        {
            throw std::invalid_argument("Unhandled region: " + region);
        }

        mInputOriginBroadcastName
            = propertyTree.get<std::string> (
                 section + ".inputOriginBroadcastName",
                 mInputOriginBroadcastName);
        mInputOriginBroadcastAddress
            = propertyTree.get<std::string> (
                 section + ".inputOriginBroadcastAddress",
                 mInputOriginBroadcastAddress);
        if (::isEmpty(mInputOriginBroadcastName) &&
            ::isEmpty(mInputOriginBroadcastAddress))
        {
            throw std::runtime_error("Input origin broadcast indeterminable");
        }

        mOutputOriginBroadcastName
            = propertyTree.get<std::string> (
                 section + ".outputOriginBroadcastName",
                 mOutputOriginBroadcastName);
        mOutputOriginBroadcastAddress
            = propertyTree.get<std::string> (
                 section + ".outputOriginBroadcastAddress",
                 mOutputOriginBroadcastAddress);
        if (::isEmpty(mOutputOriginBroadcastName) &&
            ::isEmpty(mOutputOriginBroadcastAddress))
        {   
            throw std::runtime_error("Output origin broadcast indeterminable");
        }

        // Location request timeout
        mLocatorServiceName
            = propertyTree.get<std::string> (
                 section + ".locatorServiceName",
                 mLocatorServiceName);
        mLocatorServiceAddress
            = propertyTree.get<std::string> (
                 section + ".locatorServiceAddress",
                 mLocatorServiceAddress);
        if (::isEmpty(mLocatorServiceName) &&
            ::isEmpty(mLocatorServiceAddress))
        {
            throw std::runtime_error("Location service address indeterminable");
        }

        auto requestTimeOut
            = propertyTree.get<int> (section + ".locatorRequestTimeOut",
                                     mLocatorRequestReceiveTimeOut.count());
        if (requestTimeOut < 0)
        {
            throw std::invalid_argument("Requests cannot indefinitely block");
        }
        mLocatorRequestReceiveTimeOut
            = std::chrono::seconds {requestTimeOut};

        // Incrementer
        mIncrementerServiceName
            = propertyTree.get<std::string> (
                section + ".incrementerServiceName",
                mIncrementerServiceName);
        mIncrementerServiceAddress
            = propertyTree.get<std::string> (
                section + ".incrementerServiceAddress",
                mIncrementerServiceAddress);
        if (::isEmpty(mIncrementerServiceName) &&
            ::isEmpty(mIncrementerServiceAddress))
        {
            throw std::runtime_error("Incrementer service indeterminable");
        }
        // Increment time out
        requestTimeOut
            = propertyTree.get<int>
                (section + ".incrementRequestTimeOut",
                 mIncrementRequestReceiveTimeOut.count());
        if (requestTimeOut < 0) 
        {
            throw std::invalid_argument(
                "Increment request cannot indefinitely block");
        }
        mIncrementRequestReceiveTimeOut
            = std::chrono::milliseconds {requestTimeOut};
    }
    std::string mModuleName{MODULE_NAME};
    std::string mInputOriginBroadcastName{"PreliminaryOrigin"};
    std::string mInputOriginBroadcastAddress;
    std::string mOutputOriginBroadcastName{"Origin"};
    std::string mOutputOriginBroadcastAddress;
    std::string mLocatorServiceName{"uLocator"};
    std::string mLocatorServiceAddress;
    std::string mIncrementerServiceName{"Incrementer"};
    std::string mIncrementerServiceAddress;
    std::filesystem::path mLogFileDirectory{"/var/log/urts"};
    std::chrono::seconds mLocatorRequestReceiveTimeOut{45};
    std::chrono::milliseconds mIncrementRequestReceiveTimeOut{1000}; // 1 s
    UMPS::Logging::Level mVerbosity{UMPS::Logging::Level::Info};
    URTS::Broadcasts::Internal::Origin::SubscriberOptions
        mInputOriginSubscriberOptions;
    URTS::Broadcasts::Internal::Origin::PublisherOptions
        mOutputOriginPublisherOptions;
    URTS::Services::Scalable::Locators::ULocator::RequestorOptions
        mLocatorRequestorOptions;
    URTS::Services::Standalone::Incrementer::RequestorOptions
        mIncrementerRequestorOptions;
    bool mIsUtah{true};
};

///--------------------------------------------------------------------------///

class Locator : public UMPS::Modules::IProcess
{
public:
    Locator(const ::ProgramOptions &programOptions,
            std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mContext(std::make_shared<UMPS::Messaging::Context> (1)),
        mProgramOptions(programOptions),
        mLogger(logger)
    {
        mModuleName = mProgramOptions.mModuleName;
        mIsUtah = mProgramOptions.mIsUtah;
        if (mIsUtah)
        {
            mLogger->info("Creating a Utah locator client");
            mMonitoringRegion = URTS::Broadcasts::Internal::Origin::Origin::MonitoringRegion::Utah;
        }
        else
        {
            mLogger->info("Creating a Yellowstone locator client");
            mMonitoringRegion = URTS::Broadcasts::Internal::Origin::Origin::MonitoringRegion::Yellowstone;
        }
        // Create an origin subscriber
        mOriginSubscriber
            = std::make_unique<URTS::Broadcasts::Internal::Origin::Subscriber>
                (mContext, mLogger);
        mOriginSubscriber->initialize(
            mProgramOptions.mInputOriginSubscriberOptions);

        // Create the origin publisher
        mOriginPublisher
            = std::make_unique<URTS::Broadcasts::Internal::Origin::Publisher>
                 (mContext, mLogger);
        mOriginPublisher->initialize(
            mProgramOptions.mOutputOriginPublisherOptions);

        // Create the incrementer client
        mIncrementerClient
            = std::make_unique<URTS::Services::Standalone::Incrementer
                                   ::Requestor> (mContext, mLogger);
        mIncrementerClient->initialize(
            mProgramOptions.mIncrementerRequestorOptions);

        // Create the locator client
        mLocatorClient
            = std::make_unique<URTS::Services::Scalable::Locators
                                   ::ULocator::Requestor> (mContext, mLogger);
        mLocatorClient->initialize(
            mProgramOptions.mLocatorRequestorOptions);

        std::this_thread::sleep_for(std::chrono::milliseconds {250});
        if (!mOriginSubscriber->isInitialized())
        {
            throw std::runtime_error("Origin subscriber not initialized");
        }
        if (!mOriginPublisher->isInitialized())
        {
            throw std::runtime_error("Origin publisher not initialized");
        }
        if (!mIncrementerClient->isInitialized())
        {
            throw std::runtime_error("Incrementer client not initialized");
        }
        if (!mLocatorClient->isInitialized())
        {
            throw std::runtime_error("Locator client not initialized");
        }

        // Instantiate the local command replier
        mLocalCommand
            = std::make_unique<UMPS::Services::Command::Service> (mLogger);
        UMPS::Services::Command::ServiceOptions localServiceOptions;
        localServiceOptions.setModuleName(getName());
        localServiceOptions.setCallback(
            std::bind(&Locator::commandCallback,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
        mLocalCommand->initialize(localServiceOptions);

        mInitialized = true;
    }
    /// @brief Destructor
    ~Locator() override
    {
        stop();
    }
    /// @brief Stops the locator processes
    void stop() override
    {
        setRunning(false);
        if (mLocatorThread.joinable()){mLocatorThread.join();}
        if (mOriginSubscriberThread.joinable()){mOriginSubscriberThread.join();}
        if (mOriginPublisherThread.joinable()){mOriginPublisherThread.join();}
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
        mOriginPublisherThread
             = std::thread(&::Locator::publishOrigins, this);
        mOriginSubscriberThread
             = std::thread(&::Locator::getOrigins, this);
        mLocatorThread
             = std::thread(&::Locator::locate, this);
        if (mLocalCommand != nullptr)
        {
#ifndef NDEBUG
            assert(mLocalCommand->isInitialized());
#endif
            mLocalCommand->start();
        }
    }
    /// Initialized?
    [[nodiscard]] bool isInitialized() const noexcept
    {
        return mInitialized;
    }
    /// @brief Toggles this as running or not running
    void setRunning(const bool running)
    {
        mKeepRunning = running;
    }
    /// @result True indicates this is running.
    [[nodiscard]] bool isRunning() const noexcept override
    {
        return mKeepRunning;
    }
    /// @result The module name.
    [[nodiscard]] std::string getName() const noexcept override
    {
        return mModuleName;
    }
    /// @brief Perform the location.
    void locate()
    {
        mLogger->debug("Starting the locator...");
        int64_t locationRequestIdentifier{0};
        constexpr std::chrono::milliseconds timeOut{10};
        while (isRunning())
        {
            URTS::Broadcasts::Internal::Origin::Origin origin;
            auto gotOrigin
                = mOriginSubscriberQueue.wait_until_and_pop(&origin,
                                                            timeOut);
            if (gotOrigin)
            {
                // Create a location request
                try
                {
                    auto arrivalsIn = origin.getArrivals();
                    std::vector<URTS::Services::Scalable::Locators::ULocator::Arrival> arrivals;
                    arrivals.reserve(arrivalsIn.size());
                    for (const auto &arrival : arrivalsIn)
                    {
                        try
                        {
                            arrivals.push_back(::fromArrival(arrival));
                        }
                        catch (const std::exception &e)
                        {
                            mLogger->warn("Did not add arrival because "
                                        + std::string{e.what()});
                        }
                    }
                    std::sort(arrivals.begin(), arrivals.end(),
                              [](const auto &lhs, const auto &rhs)
                              {
                                  return lhs.getTime() < rhs.getTime();
                              });
                    URTS::Services::Scalable::Locators::ULocator::LocationRequest 
                        request;
                    request.setIdentifier(locationRequestIdentifier);
                    request.setArrivals(arrivals);
                    request.setLocationStrategy(
                        URTS::Services::Scalable::Locators::ULocator::
                        LocationRequest::LocationStrategy::General);
                    locationRequestIdentifier = locationRequestIdentifier + 1;
                    auto response = mLocatorClient->request(request);
                    if (response)
                    {
                        auto refinedOrigin = response->getOrigin();
                        if (refinedOrigin)
                        {
                            auto newOrigin
                                = ::fromOrigin(origin, *refinedOrigin);
                            mOriginPublisherQueue.push(std::pair{newOrigin, true});
                        }
                        else
                        {
                            throw std::runtime_error("Origin not on response");
                        }
                    }
                    else
                    {
                        throw std::runtime_error("Location request failed");
                    }
                }
                catch (const std::exception &e)
                {
                    mLogger->warn("Failed to refine origin because " 
                                + std::string {e.what()});
                    mOriginPublisherQueue.push(std::pair {origin, false}); 
                }
            }
        }
        mLogger->debug("Locator thread leaving...");
    }
    /// @brief Publish output origins.
    void publishOrigins()
    {
#ifndef NDEBUG
        assert(mOriginPublisher->isInitialized());
#endif
        mLogger->debug("Starting the origin publisher...");
        URTS::Services::Standalone::Incrementer::IncrementRequest originRequest;
        originRequest.setItem(URTS::Services::Standalone::
                              Incrementer::IncrementRequest::Item::Origin);
        URTS::Services::Standalone::Incrementer::IncrementRequest arrivalRequest;
        arrivalRequest.setItem(URTS::Services::Standalone::Incrementer::
                               IncrementRequest::Item::PhaseArrival);
        int64_t requestIdentifier{0};
        constexpr std::chrono::milliseconds timeOut{10};
        while (isRunning())
        {
            std::pair<URTS::Broadcasts::Internal::Origin::Origin, bool>
                originPair;
            auto gotOrigin
                = mOriginPublisherQueue.wait_until_and_pop(&originPair,
                                                           timeOut);
            if (gotOrigin)
            {
                if (!originPair.second)
                {
                    mLogger->debug("Propagating unrefined origin...");
                }
                else
                {
                    // Update the identifiers and processing algorithms
                    mLogger->debug("Updating refined origin information...");
                    auto initialOriginIdentifier = originPair.first.getIdentifier();
                    auto originIdentifier = initialOriginIdentifier;
                    try
                    {
                        requestIdentifier = requestIdentifier + 1;
                        originRequest.setIdentifier(requestIdentifier);
                        auto response
                            = mIncrementerClient->request(originRequest);
                        originIdentifier = response->getValue();
                        originPair.first.setIdentifier(originIdentifier);

                        auto previousIdentifiers
                            = originPair.first.getPreviousIdentifiers();
                        previousIdentifiers.push_back(
                            initialOriginIdentifier);
                        originPair.first.setPreviousIdentifiers(
                            previousIdentifiers);

                        auto processingAlgorithms
                             = originPair.first.getAlgorithms();
                        processingAlgorithms.push_back(getName());
                        originPair.first.setAlgorithms(processingAlgorithms);
                    }
                    catch (const std::exception &e)
                    {
                        mLogger->warn("Failed get new origin identifier");
                    }
                    auto arrivals = originPair.first.getArrivals();
                    for (auto &arrival : arrivals)
                    {
                        requestIdentifier = requestIdentifier + 1;
                        arrivalRequest.setIdentifier(requestIdentifier);
                        arrival.setOriginIdentifier(originIdentifier);
                        try
                        {
                            auto response
                                = mIncrementerClient->request(arrivalRequest);
                            arrival.setIdentifier(response->getValue());
                        }
                        catch (const std::exception &e)
                        {
                            mLogger->warn(
                              "Failed to get arrival identifier.  Failed with: "
                             + std::string {e.what()});
                        }
                    }
                    originPair.first.setArrivals(arrivals);
                }
                // Write out some information
                if (mLogger->getLevel() >= UMPS::Logging::Level::Info)
                {
                    mLogger->info("Origin "
                                + std::to_string(originPair.first.getIdentifier()) + " "
                                + std::to_string(originPair.first.getTime().count()*1.e-6) + " " 
                                + std::to_string(originPair.first.getLatitude()) + " " 
                                + std::to_string(originPair.first.getLongitude()) + " " 
                                + std::to_string(originPair.first.getDepth()));
                    for (const auto &arrival : originPair.first.getArrivals())
                    {
                        std::string phase{"P"};
                        if (arrival.getPhase() == URTS::Broadcasts::Internal::Origin::Arrival::Phase::S)
                        {
                            phase = "S";
                        }
                        mLogger->info(std::to_string(arrival.getIdentifier()) + " "
                                    + arrival.getNetwork() + "."
                                    + arrival.getStation() + "."
                                    + arrival.getChannel() + "." 
                                    + arrival.getLocationCode() + "." 
                                    + phase + " " 
                                    + std::to_string(arrival.getTime().count()*1.e-6));
                    }
                }
                // Finally publish it
                try
                {
                    mLogger->debug("Publishing origin...");
                    mOriginPublisher->send(originPair.first);
                }
                catch (const std::exception &e)
                {
                    mLogger->error("Failed to send origin.  Failed with "
                                 + std::string{e.what()});
                }
            }
        }
        mLogger->debug("Origin publisher thread leaving...");
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
        // Return something
        mLogger->error("Unhandled message: " + messageType);
        USC::AvailableCommandsResponse commandsResponse;
        commandsResponse.setCommands(::getInputOptions());
        return commandsResponse.clone();
    }
    /// @brief Gets input origins.
    void getOrigins()
    {
#ifndef NDEBUG
        assert(mOriginSubscriber->isInitialized());
#endif
        mLogger->debug("Starting the origin subscriber...");
        while (isRunning())
        {
            auto origin = mOriginSubscriber->receive();
            if (origin)
            {
                if (origin->getMonitoringRegion() == mMonitoringRegion)
                {
                    mLogger->debug("Adding a location to the queue");
                    mOriginSubscriberQueue.push(std::move(*origin));
                }
                else
                {
                    mLogger->debug("Origin not in region; skipping...");
                }
            }
        }
        mLogger->debug("Origin subscriber thread leaving...");
    }
private:
    std::string mModuleName{MODULE_NAME};
    std::shared_ptr<UMPS::Messaging::Context> mContext{nullptr};
    ::ProgramOptions mProgramOptions;
    std::unique_ptr<URTS::Broadcasts::Internal::Origin::Subscriber>
        mOriginSubscriber{nullptr};
    std::unique_ptr<URTS::Broadcasts::Internal::Origin::Publisher>
        mOriginPublisher{nullptr};
    std::unique_ptr<URTS::Services::Standalone::Incrementer::Requestor>
        mIncrementerClient{nullptr};
    std::unique_ptr<URTS::Services::Scalable::Locators::ULocator::Requestor>
        mLocatorClient{nullptr};
    ::ThreadSafeQueue<URTS::Broadcasts::Internal::Origin::Origin>
        mOriginSubscriberQueue;
    ::ThreadSafeQueue<std::pair<URTS::Broadcasts::Internal::Origin::Origin, bool>>
        mOriginPublisherQueue;
    std::unique_ptr<UMPS::Services::Command::Service> mLocalCommand{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::thread mLocatorThread;
    std::thread mOriginPublisherThread;
    std::thread mOriginSubscriberThread;
    URTS::Broadcasts::Internal::Origin::Origin::MonitoringRegion
       mMonitoringRegion{URTS::Broadcasts::Internal::Origin::Origin::MonitoringRegion::Unknown};
    std::atomic<bool> mKeepRunning{true};
    std::atomic<bool> mInitialized{false};
    bool mIsUtah{false};
};

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
    constexpr int hour{0};
    constexpr int minute{0};
    auto logger = ::createLogger(programOptions.mModuleName,
                                 programOptions.mLogFileDirectory,
                                 programOptions.mVerbosity,
                                 hour, minute);
    // Create the contexts (not much to communicate so this is sufficent)
    auto generalContext = std::make_shared<UMPS::Messaging::Context> (1);
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

        // Input origin subscriber
        logger->debug("Defining origin subscriber...");
        URTS::Broadcasts::Internal::Origin::SubscriberOptions subscriberOptions;
        if (!programOptions.mInputOriginBroadcastAddress.empty())
        {
            subscriberOptions.setAddress(
                programOptions.mInputOriginBroadcastAddress);
        }
        else
        {
            subscriberOptions.setAddress(
                uOperator->getProxyBroadcastBackendDetails(
                  programOptions.mInputOriginBroadcastName).getAddress());
        }
        subscriberOptions.setZAPOptions(zapOptions);
        subscriberOptions.setHighWaterMark(0); // Infinite 
        programOptions.mInputOriginSubscriberOptions = subscriberOptions;

        // Output origin publisher 
        logger->debug("Defining origin publisher...");
        URTS::Broadcasts::Internal::Origin::PublisherOptions publisherOptions;
        if (!programOptions.mOutputOriginBroadcastAddress.empty())
        {
            publisherOptions.setAddress(
                programOptions.mOutputOriginBroadcastAddress);
        }
        else
        {
            publisherOptions.setAddress(
                uOperator->getProxyBroadcastFrontendDetails(
                  programOptions.mOutputOriginBroadcastName).getAddress());
        }
        publisherOptions.setZAPOptions(zapOptions);
        publisherOptions.setHighWaterMark(0); // Infinite 
        programOptions.mOutputOriginPublisherOptions = publisherOptions;

        // Incrementer service
        logger->debug("Defining incrementer service...");
        URTS::Services::Standalone::Incrementer::RequestorOptions
             incrementerOptions;
        if (!programOptions.mIncrementerServiceAddress.empty())
        {
            incrementerOptions.setAddress(
                programOptions.mIncrementerServiceAddress);
        }
        else
        {
            incrementerOptions.setAddress(
               uOperator->getProxyServiceFrontendDetails(
                   programOptions.mIncrementerServiceName).getAddress());
        }
        incrementerOptions.setReceiveTimeOut(
             programOptions.mIncrementRequestReceiveTimeOut);
        incrementerOptions.setZAPOptions(zapOptions);
        programOptions.mIncrementerRequestorOptions = incrementerOptions;

        // Locator requestor
        logger->debug("Defining locator service...");
        URTS::Services::Scalable::Locators::ULocator::RequestorOptions
            locatorClientOptions;
        if (!programOptions.mLocatorServiceAddress.empty())
        {
            locatorClientOptions.setAddress(
               programOptions.mLocatorServiceAddress);
        }
        else
        {
            locatorClientOptions.setAddress(
               uOperator->getProxyServiceFrontendDetails(
                    programOptions.mLocatorServiceName).getAddress()
            );
        }
        locatorClientOptions.setZAPOptions(zapOptions);
        locatorClientOptions.setReceiveTimeOut(
            programOptions.mLocatorRequestReceiveTimeOut);
        programOptions.mLocatorRequestorOptions
            = locatorClientOptions;

        // Create a locator
        auto locatorProcess
            = std::make_unique<::Locator> (programOptions, logger);
 
/*
        auto locatorClient
            = std::make_unique<URTS::Services::Scalable::Locators
                                   ::ULocator::Requestor> (generalContext, logger);
        locatorClient->initialize(programOptions.mLocatorRequestorOptions);

        logger->debug("Creating module registry replier process...");
        namespace URemoteCommand = UMPS::ProxyServices::Command;
        URemoteCommand::ModuleDetails moduleDetails;
        moduleDetails.setName(programOptions.mModuleName);
        auto callbackFunction = std::bind(&Locator::commandCallback,
                                          &*locatorProcess,
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
        processManager.insert(std::move(remoteReplierProcess));
*/
        processManager.insert(std::move(locatorProcess));

/*
if (programOptions.mIsUtah)
{
        auto request1 = testUtahEventRequest();
        auto response1 = locatorClient->request(request1);
        auto origin = response1->getOrigin();
        std::cout << std::setprecision(16)
                  << origin->getTime().count()*1.e-6 << ","
                  << origin->getLatitude() << ","
                  << origin->getLongitude() << ","
                  << origin->getDepth() << std::endl; 
        auto request2 = testUtahQuarryEventRequest();
        auto response2 = locatorClient->request(request2);
        origin = response2->getOrigin();
        std::cout << std::setprecision(16)
                  << origin->getTime().count()*1.e-6 << "," 
                  << origin->getLatitude() << "," 
                  << origin->getLongitude() << "," 
                  << origin->getDepth() << std::endl; 
}
else
{
        auto request1 = testYNPEventRequest();
        auto response1 = locatorClient->request(request1);
        auto origin = response1->getOrigin();
        std::cout << std::setprecision(16)
                  << origin->getTime().count()*1.e-6 << ","
                  << origin->getLatitude() << ","
                  << origin->getLongitude() << ","
                  << origin->getDepth() << std::endl; 
}
*/

    }
    catch (const std::exception &e)
    {
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
        logger->error(e.what());
        return EXIT_FAILURE;
    }
    // The main thread waits and, when requested, sends a stop to all processes
    logger->info("Starting main thread...");
    processManager.handleMainThread();
    return EXIT_SUCCESS;
}


