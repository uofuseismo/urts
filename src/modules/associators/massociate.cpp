#include <atomic>
#include <cmath>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <string>
#ifndef NDEBUG
#include <cassert>
#endif
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <umps/logging/dailyFile.hpp> 
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/modules/processManager.hpp>
#include <umps/proxyBroadcasts/heartbeat/publisherProcess.hpp>
#include <umps/proxyBroadcasts/heartbeat/publisherProcessOptions.hpp>
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
#include <umps/services/connectionInformation/socketDetails/xPublisher.hpp>
#include <umps/services/connectionInformation/socketDetails/xSubscriber.hpp>
#include "urts/broadcasts/internal/origin/origin.hpp"
#include "urts/broadcasts/internal/origin/arrival.hpp"
#include "urts/broadcasts/internal/pick/pick.hpp"
#include "urts/broadcasts/internal/pick/subscriber.hpp"
#include "urts/broadcasts/internal/pick/subscriberOptions.hpp"
//#include "urts/broadcasts/internal/publisher/pick.hpp"
//#include "urts/broadcasts/internal/publisher/subscriber.hpp"
//#include "urts/broadcasts/internal/publisher/subscriberOptions.hpp"
#include "urts/services/scalable/associators/massociate/arrival.hpp"
#include "urts/services/scalable/associators/massociate/associationRequest.hpp"
#include "urts/services/scalable/associators/massociate/associationResponse.hpp"
#include "urts/services/scalable/associators/massociate/origin.hpp"
#include "urts/services/scalable/associators/massociate/pick.hpp"
#include "urts/services/scalable/associators/massociate/requestor.hpp"
#include "urts/services/scalable/associators/massociate/requestorOptions.hpp"
#include "private/threadSafeQueue.hpp"
#include "programOptions.hpp"

#define MODULE_NAME "mAssociator"

namespace
{

URTS::Broadcasts::Internal::Origin::Arrival fromArrival(const URTS::Services::Scalable::Associators::MAssociate::Arrival &arrival,
                                                        const double originTime)
{
    URTS::Broadcasts::Internal::Origin::Arrival result;
    result.setIdentifier(arrival.getIdentifier());
    result.setTime(arrival.getTime());
    result.setNetwork(arrival.getNetwork());
    result.setStation(arrival.getStation());
    result.setChannel(arrival.getChannel());
    result.setLocationCode(arrival.getLocationCode());
    if (arrival.getPhase() == URTS::Services::Scalable::Associators::MAssociate::Arrival::Phase::P)
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
        result.setResidual(result.getTime().count() - (originTime + *travelTime));
    }
    return result;
}

URTS::Broadcasts::Internal::Origin::Origin fromOrigin(const URTS::Services::Scalable::Associators::MAssociate::Origin &origin)
{
    URTS::Broadcasts::Internal::Origin::Origin result;
    result.setTime(origin.getTime());
    result.setLatitude(origin.getLatitude());
    result.setLongitude(origin.getLongitude());
    result.setDepth(origin.getDepth());
    const auto &arrivalsReference  = origin.getArrivalsReference();
    std::vector<URTS::Broadcasts::Internal::Origin::Arrival> arrivals;
    for (const auto &arrival : arrivalsReference)
    {
        try
        {
            arrivals.push_back(::fromArrival(arrival, origin.getTime().count()*1.e-6));
        }
        catch (...)
        {
        }   
    }   
    try 
    {
        result.setArrivals(std::move(arrivals));
    }   
    catch (...)
    {   
    }   
    return result;
}
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

/// @brief Parses the command line options.
[[nodiscard]] std::string parseCommandLineOptions(int argc, char *argv[])
{
    std::string iniFile;
    boost::program_options::options_description desc(
R"""(
The mAssociator drives the associators that turn picks into events.
Example usage:
    mAssociator --ini=mAssociator.ini
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

class Associator : public UMPS::Modules::IProcess
{
public:
    Associator(const ::ProgramOptions &programOptions,
               std::shared_ptr<UMPS::Logging::ILog> &logger) :
        mContext(std::make_shared<UMPS::Messaging::Context> (1)),
        mProgramOptions(programOptions),
        mLogger(logger)
    {
        // Create the pick subscriber 
        mPickSubscriber
            = std::make_unique<URTS::Broadcasts::Internal::Pick::Subscriber>
                (mContext, mLogger);
        mPickSubscriber->initialize(
            mProgramOptions.mPickSubscriberOptions);

        // Create the origin publisher
        mOriginPublisher
            = std::make_unique<URTS::Broadcasts::Internal::Origin::Publisher>
                 (mContext, mLogger);
        mOriginPublisher->initialize(
            mProgramOptions.mOriginPublisherOptions);

        // Create the associator client
        mAssociatorClient
            = std::make_unique<URTS::Services::Scalable::Associators
                                   ::MAssociate::Requestor> (mContext, mLogger);
        mAssociatorClient->initialize(
            mProgramOptions.mAssociatorRequestorOptions);

        std::this_thread::sleep_for(std::chrono::milliseconds {250});
        if (!mPickSubscriber->isInitialized())
        {
            throw std::runtime_error("Pick subscriber not initialized");
        }
        if (!mOriginPublisher->isInitialized())
        {
            throw std::runtime_error("Origin publisher not initialized");
        }

        // Instantiate the local command replier
        mLocalCommand
            = std::make_unique<UMPS::Services::Command::Service> (mLogger);
        UMPS::Services::Command::ServiceOptions localServiceOptions;
        localServiceOptions.setModuleName(getName());
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
    /// @brief Stops the associator processes
    void stop() override
    {
        setRunning(false);
        if (mAssociatorThread.joinable()){mAssociatorThread.join();}
        if (mPickSubscriberThread.joinable()){mPickSubscriberThread.join();}
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
             = std::thread(&::Associator::publishOrigins, this);
        mLogger->debug("Starting the pick subscriber...");
        mPickSubscriberThread
             = std::thread(&::Associator::getPicks, this);
        mLogger->debug("Starting the associator...");
        mAssociatorThread
             = std::thread(&::Associator::associate, this);
        if (mLocalCommand != nullptr)
        {
#ifndef NDEBUG
            assert(mLocalCommand->isInitialized());
#endif
            mLocalCommand->start();
        }
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
    /// @result The module name.
    [[nodiscard]] std::string getName() const noexcept override
    {
        return mModuleName;
    }
    /// @brief Gets input picks and puts them into the queue for processing.
    void getPicks()
    {
#ifndef NDEBUG
        assert(mPickSubscriber->isInitialized());
#endif
        mLogger->debug("Starting the pick subscriber...");
        while (keepRunning())
        {
            auto pick = mPickSubscriber->receive();
            if (pick != nullptr)
            {
                try
                {
                    URTS::Services::Scalable::Associators::MAssociate::Pick massPick{*pick};
                    auto network = massPick.getNetwork();
                    bool sendUtah{false};
                    bool sendYNP{false};
                    if (network == "UU" || network == "NN" ||
                        network == "AR" || network == "RE")
                    {
                        sendUtah = true;
                        sendYNP = false;
                    }
                    else if (network == "WY" || network == "MB" ||
                             network == "IW" || network == "PB")
                    {
                        sendUtah = false;
                        sendYNP = true;
                    }
                    else if (network == "US")
                    {
                        auto station = massPick.getStation();
                        if (station == "LKWY" || station == "RLMT" || station == "BOZ")
                        {
                            sendUtah = false;
                            sendYNP = true;
                        }
                    }
                    else
                    {
                        mLogger->warn("Unhandled network code " + network);
                    }
                    if (mIsUtah)
                    {
                        if (sendUtah)
                        {
                            mPickSubscriberQueue.push(std::move(massPick));
                        } 
                    }
                    else
                    {
                        if (sendYNP)
                        {
                            mPickSubscriberQueue.push(std::move(massPick));
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    mLogger->warn(e.what());
                } 
            }
        }
        mLogger->debug("Pick subscriber thread leaving...");
    }
    /// @brief Publishes origins
    void publishOrigins()
    {
int64_t identifier{1};
#ifndef NDEBUG
        assert(mOriginPublisher->isInitialized());
#endif
        mLogger->debug("Starting the origin publisher...");
        constexpr std::chrono::milliseconds timeOut{10};
        while (keepRunning())
        {
            URTS::Broadcasts::Internal::Origin::Origin origin;
            auto gotOrigin
                = mOriginPublisherQueue.wait_until_and_pop(&origin,
                                                           timeOut);
            if (gotOrigin)
            {
                try
                {
                    mLogger->debug("Publishing origin...");
origin.setIdentifier(identifier);
identifier++;
                    mOriginPublisher->send(origin);
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
    /// Runs the associator
    void associate()
    {
        int64_t associationRequestIdentifier{0};
// TODO assocation window should be > pickLatency + maximumMoveout
        auto associationWindow = std::chrono::duration_cast<std::chrono::microseconds> (mProgramOptions.mAssociationWindowDuration);
        auto pickLatency = std::chrono::duration_cast<std::chrono::microseconds> (mProgramOptions.mPickLatency);
        auto maximumMoveout = std::chrono::duration_cast<std::chrono::microseconds> (mProgramOptions.mMaximumMoveout);
#ifndef NDEBUG
        assert(mAssociatorClient->isInitialized());
#endif
        auto lastProcessingTime = std::chrono::duration_cast<std::chrono::microseconds> (
                 std::chrono::high_resolution_clock::now().time_since_epoch() );
        mLogger->debug("Starting the associator...");
        std::vector<URTS::Services::Scalable::Associators::MAssociate::Pick> picks;
        while (keepRunning())
        {
            auto now = std::chrono::duration_cast<std::chrono::microseconds> (
                 std::chrono::high_resolution_clock::now().time_since_epoch() );
            //constexpr std::chrono::microseconds fiveSeconds{5000000};
            auto maximumOriginTime = now
                                   - pickLatency
                                   - maximumMoveout;
            auto newestProcessingTime = now - pickLatency; //maximumOriginTime + maximumMoveout;
            auto oldestProcessingTime = newestProcessingTime - associationWindow; //maximumOriginTime - associationWindow;
/* 
            auto oldestProcessingTime = now 
                                      - pickLatency
                                      - associationWindow
                                      - fiveSeconds; // TODO need a smarter padding
            auto newestProcessingTime = oldestProcessingTime
                                      + associationWindow;
            auto minOriginTime = oldestProcessingTime;
*/
            // First purge the old picks
            picks.erase(
                std::remove_if(picks.begin(), picks.end(),
                               [&](const auto &pick)
                               {
                                   return pick.getTime() < oldestProcessingTime;
                               }),
                               picks.end()
            );
            // Now get the newest picks
            bool gotPicks{false};
            while (!mPickSubscriberQueue.empty()) 
            {
                auto pick = mPickSubscriberQueue.try_pop();
                if (pick)
                {
                    // Add the pick
                    if (pick->getTime() >= oldestProcessingTime)
                    {
                        std::string phase{"P"};
                        if (pick->getPhaseHint() == URTS::Services::Scalable::Associators::MAssociate::Pick::PhaseHint::S)
                        {
                            phase = "S";
                        }
/*
                        std::cout << pick->getNetwork() << " "
                                  << pick->getStation() << " "
                                  << pick->getChannel() << " " 
                                  << pick->getLocationCode() << " "
                                  << phase << " "
                                  << pick->getTime().count() << std::endl;
*/
                        picks.push_back(*pick); 
                        gotPicks = true;
                    }
                }
                else
                {
                    break;
                }
            }
            // Sort the pick list for my sanity
            if (gotPicks)
            {
                std::sort(picks.begin(), picks.end(),
                          [](const auto &lhs, const auto &rhs)
                          {
                             return lhs.getTime() < rhs.getTime();
                          });
            }
            // Are we over the limit?  Erase the oldest picks
            if (picks.size() > mMaximumNumberOfPicks)
            {
                auto nErase = picks.size() - mMaximumNumberOfPicks;
                mLogger->warn("Maximum picks exceeded.  Erasing "
                            + std::to_string(nErase) + " picks"); 
                picks.erase(picks.begin(), picks.begin() + nErase);
            }

            // Time to make a request
            if (now >= lastProcessingTime + pickLatency)
            {
                associationRequestIdentifier = associationRequestIdentifier + 1;
                lastProcessingTime = now;
                URTS::Services::Scalable::Associators::MAssociate::AssociationRequest request;
 
                request.setIdentifier(associationRequestIdentifier);
                std::vector<URTS::Services::Scalable::Associators::MAssociate::Pick> picksToAssociate;
                picksToAssociate.reserve(picks.size());
                for (const auto &pick : picks)
                {
                    if (pick.getTime() >= oldestProcessingTime)// &&
                    //    pick.getTime() <= newestProcessingTime)
                    {
                        picksToAssociate.push_back(pick);
                    }
                }
std::cout << picks.size() << " " << picksToAssociate.size() << std::endl;
                if (picksToAssociate.size() > 4)
                {
                    try
                    {
                        mLogger->debug("Issuing association request with "
                                     + std::to_string(picks.size()) + " picks");
                        request.setPicks(picksToAssociate);
                        auto response = mAssociatorClient->request(request);
                        if (response)
                        {
                            auto origins = response->getOrigins();
                            // Remove the origins that are too new (we'll get them the next go)
                            origins.erase(
                                std::remove_if(origins.begin(), origins.end(),
                                               [&](const auto &origin)
                                                {
                                                    return origin.getTime() > maximumOriginTime;
                                                }),
                                                origins.end()
                                                );
                            // Make the remaining origins publishable and remove associated picks
                            for (const auto &origin : origins)
                            {
                                mLogger->info("Created origin at "
                                            + std::to_string(origin.getTime().count()*1.e-6) + " "
                                            + std::to_string(origin.getLatitude()) + " " 
                                            + std::to_string(origin.getLongitude()) + " "
                                            + std::to_string(origin.getDepth())); 
                                try
                                {
                                    mOriginPublisherQueue.push(::fromOrigin(origin));
                                }
                                catch (const std::exception &e)
                                {
                                    mLogger->warn(e.what());
                                }
                                // Purge associated picks
                                const auto &arrivalsReference = origin.getArrivalsReference();
                                for (const auto &arrival : arrivalsReference)
                                {
                                    auto phase = "P";
                                    if (arrival.getPhase() == URTS::Services::Scalable::Associators::MAssociate::Arrival::Phase::S){phase = "S";} 
 auto time = arrival.getTime().count()*1.e-6;
                                    mLogger->info("Arrival " + arrival.getNetwork() + "." + arrival.getStation() + "." + arrival.getChannel() + "." + arrival.getLocationCode() + "." + phase + " " + std::to_string(time));
                                }
                                for (const auto &arrival : arrivalsReference)
                                {
                                    auto arrivalIdentifier = arrival.getIdentifier();
                                    picks.erase(std::remove_if(picks.begin(), picks.end(),
                                                [&](const auto &pick)
                                                {
                                                    return pick.getIdentifier() == arrivalIdentifier;
                                                }),
                                                picks.end()
                                                );
                                    // Also purge similar picks
                                    picks.erase(std::remove_if(picks.begin(), picks.end(),
                                                               [&](const auto &pick)
                                                               {
                                                                   constexpr std::chrono::microseconds tol{1000000};
                                                                   std::string pickPhase{"P"};
                                                                   if (pick.getPhaseHint() == URTS::Services::Scalable::Associators::MAssociate::Pick::PhaseHint::S)
                                                                   {
                                                                       pickPhase = "S";
                                                                   } 
                                                                   for (const auto &arrival : arrivalsReference)
                                                                   {
                                                                       std::string arrivalPhase = "P";
                                                                       if (arrival.getPhase() == URTS::Services::Scalable::Associators::MAssociate::Arrival::Phase::S)
                                                                       {
                                                                           arrivalPhase = "S";
                                                                       }

                                                                       if (arrival.getNetwork() == pick.getNetwork() &&
                                                                           arrival.getStation() == pick.getStation() &&
                                                                           std::abs(arrival.getTime().count() - pick.getTime().count()) < tol.count() &&
                                                                           arrivalPhase == pickPhase)
                                                                       {
                                                                           return true;
                                                                       }
                                                                   }
                                                                   return false;
                                                               }),
                                                               picks.end());
                                }
                            }
                        }
                    }
                    catch (const std::exception &e)
                    {
                        mLogger->warn(e.what());
                    }
                    
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds {1});
        }
        mLogger->debug("Exiting the associator client");
    }
    /// Initialized?
    [[nodiscard]] bool isInitialized() const noexcept
    {
        return mInitialized;
    }
    /// @result True indicates this should keep running
    [[nodiscard]] bool keepRunning() const
    {
        return mKeepRunning;
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
    std::string mModuleName{MODULE_NAME};
    std::shared_ptr<UMPS::Messaging::Context> mContext{nullptr};
    ::ProgramOptions mProgramOptions;
    std::unique_ptr<URTS::Broadcasts::Internal::Pick::Subscriber>
        mPickSubscriber{nullptr};
    std::unique_ptr<URTS::Broadcasts::Internal::Origin::Publisher>
        mOriginPublisher{nullptr};
    std::unique_ptr<URTS::Services::Scalable::Associators::MAssociate::Requestor> mAssociatorClient{nullptr};
    ::ThreadSafeQueue<URTS::Services::Scalable::Associators::MAssociate::Pick>
          mPickSubscriberQueue;//{nullptr};
    ::ThreadSafeQueue<URTS::Broadcasts::Internal::Origin::Origin>
          mOriginPublisherQueue;
    std::unique_ptr<UMPS::Services::Command::Service> mLocalCommand;//{nullptr};
    std::thread mOriginPublisherThread;
    std::thread mPickSubscriberThread;
    std::thread mAssociatorThread;
    std::chrono::seconds mProcessingWindow{120};
    std::chrono::seconds mPickLatency{60}; // Typically picks are 30 s late so this is conservative
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::atomic<bool> mKeepRunning{true};
    std::atomic<bool> mInitialized{false};
    size_t mMaximumNumberOfPicks{4096};
    bool mIsUtah{true}; 
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
    // Create the contexts (this is pretty low one should be fine)
    auto generalContext = std::make_shared<UMPS::Messaging::Context> (1);
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

        // Pick subscriber
        URTS::Broadcasts::Internal::Pick::SubscriberOptions
            subscriberOptions;
        if (!programOptions.mPickBroadcastAddress.empty())
        {
            subscriberOptions.setAddress(
                programOptions.mPickBroadcastAddress);
        }
        else
        {
            logger->debug("Fetching pick subscriber address..."); 
            subscriberOptions.setAddress(
                uOperator->getProxyBroadcastBackendDetails(
                  programOptions.mPickBroadcastName).getAddress());
        }
        subscriberOptions.setZAPOptions(zapOptions);
        subscriberOptions.setHighWaterMark(0); // Infinite 
        programOptions.mPickSubscriberOptions = subscriberOptions;

        // Output origin publisher 
        logger->debug("Creating origin publisher...");
        URTS::Broadcasts::Internal::Origin::PublisherOptions publisherOptions;
        if (!programOptions.mOriginBroadcastAddress.empty())
        {
            publisherOptions.setAddress(
                programOptions.mOriginBroadcastAddress);
        }
        else
        {
            publisherOptions.setAddress(
                uOperator->getProxyBroadcastFrontendDetails(
                  programOptions.mOriginBroadcastName).getAddress());
        }
        publisherOptions.setZAPOptions(zapOptions);
        publisherOptions.setHighWaterMark(0); // Infinite 
        programOptions.mOriginPublisherOptions = publisherOptions;

        // Associator requestor
        URTS::Services::Scalable::Associators::MAssociate::RequestorOptions
            associatorClientOptions;
        if (!programOptions.mAssociatorServiceAddress.empty())
        {
            associatorClientOptions.setAddress(
               programOptions.mAssociatorServiceAddress);
        }
        else
        {
            logger->debug("Fetching packetCache address...");
            associatorClientOptions.setAddress(
               uOperator->getProxyServiceFrontendDetails(
                    programOptions.mAssociatorServiceName).getAddress()
            );
        }
        associatorClientOptions.setZAPOptions(zapOptions);
        associatorClientOptions.setReceiveTimeOut(
            programOptions.mAssociatorRequestReceiveTimeOut);
        programOptions.mAssociatorRequestorOptions
            = associatorClientOptions;

        logger->debug("Creating associator process...");
        auto associatorProcess
            = std::make_unique<::Associator> (programOptions, logger);

/*
        logger->debug("Creating module registry replier process...");
        namespace URemoteCommand = UMPS::ProxyServices::Command;
        URemoteCommand::ModuleDetails moduleDetails;
        moduleDetails.setName(programOptions.mModuleName);
        auto callbackFunction = std::bind(&Associator::commandCallback,
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
        //processManager.insert(std::move(remoteReplierProcess));
*/

        processManager.insert(std::move(associatorProcess));
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
