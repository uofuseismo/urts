#include <thread>
#include <mutex>
#include <string>
#include <algorithm>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messageFormats/failure.hpp>
#include <umps/messageFormats/message.hpp>
#include <umps/messaging/routerDealer/reply.hpp>
#include <umps/messaging/routerDealer/replyOptions.hpp>
#include <massociate/pick.hpp>
#include <massociate/associator.hpp>
#include <massociate/waveformIdentifier.hpp>
#include <massociate/dbscan.hpp>
#include <massociate/event.hpp>
#include <massociate/migrator.hpp>
#include <uLocator/station.hpp>
#include <uLocator/uussRayTracer.hpp>
#include <uLocator/position/knownUtahEvent.hpp>
#include <uLocator/position/knownUtahQuarry.hpp>
#include <uLocator/position/knownYNPEvent.hpp>
#include <uLocator/position/utahRegion.hpp>
#include <uLocator/position/ynpRegion.hpp>
#include <uLocator/position/wgs84.hpp>
#include "urts/services/scalable/associators/massociate/service.hpp"
#include "urts/services/scalable/associators/massociate/serviceOptions.hpp"
#include "urts/services/scalable/associators/massociate/associationRequest.hpp"
#include "urts/services/scalable/associators/massociate/associationResponse.hpp"
#include "urts/services/scalable/associators/massociate/arrival.hpp"
#include "urts/services/scalable/associators/massociate/origin.hpp"
#include "urts/services/scalable/associators/massociate/pick.hpp"
#include "urts/broadcasts/internal/pick/pick.hpp"
#include "urts/database/connection/connection.hpp"
#include "urts/database/aqms/stationDataTable.hpp"
#include "urts/database/aqms/stationData.hpp"
#include "createTravelTimeCalculator.hpp"

namespace MASS = MAssociate;
using namespace URTS::Services::Scalable::Associators::MAssociate;

namespace
{

MASS::Pick fromPick(const Pick &pick)
{
    MASS::Pick result;
    MASS::WaveformIdentifier waveformIdentifier;

    waveformIdentifier.setNetwork(pick.getNetwork());
    waveformIdentifier.setStation(pick.getStation());
    waveformIdentifier.setChannel(pick.getChannel());
    waveformIdentifier.setLocationCode(pick.getLocationCode());

    result.setWaveformIdentifier(waveformIdentifier);
    result.setTime(pick.getTime());
    result.setStandardError(pick.getStandardError());
    result.setIdentifier(pick.getIdentifier());
    if (pick.getPhaseHint() == Pick::PhaseHint::P)
    {
        result.setPhaseHint(MASS::Pick::PhaseHint::P);
    }
    else
    {
        result.setPhaseHint(MASS::Pick::PhaseHint::S);
    }

    return result;
}

Pick fromPick(const MASS::Pick &pick)
{
    Pick result;
    const auto &waveformIdentifier = pick.getWaveformIdentifier();
    result.setNetwork(waveformIdentifier.getNetwork());
    result.setStation(waveformIdentifier.getStation());
    result.setChannel(waveformIdentifier.getChannel());
    result.setLocationCode(waveformIdentifier.getLocationCode());
    result.setIdentifier(pick.getIdentifier());
    result.setTime(pick.getTime());
    result.setStandardError(pick.getStandardError());
    if (pick.getPhaseHint() == "P")
    {
        result.setPhaseHint(Pick::PhaseHint::P);
    }
    else
    {
        result.setPhaseHint(Pick::PhaseHint::S);
    }
    return result;
}

std::vector<Pick> fromPicks(const std::vector<MASS::Pick> &picks)
{
    std::vector<Pick> result;
    result.reserve(picks.size());
    for (const auto &pick : picks)
    {
        try
        {
            result.push_back(::fromPick(pick));
        }
        catch (...)
        {
        }
    }
    return result;
}

Arrival fromArrival(const MASS::Arrival &arrival)
{
    Arrival result;
    auto waveformIdentifier = arrival.getWaveformIdentifier();
    result.setNetwork(waveformIdentifier.getNetwork());
    result.setStation(waveformIdentifier.getStation());
    result.setChannel(waveformIdentifier.getChannel());
    result.setLocationCode(waveformIdentifier.getLocationCode());
    auto phase = arrival.getPhase();
    if (phase == "P")
    {
        result.setPhase(Arrival::Phase::P);
    }
    else if (phase == "S")
    {
        result.setPhase(Arrival::Phase::S);
    }
    else
    {
        throw std::invalid_argument("Unhandled phase type " + phase);
    }
    result.setTime(arrival.getTime());
    result.setIdentifier(arrival.getIdentifier());
    try
    {
        result.setTravelTime(arrival.getTravelTime());
    }
    catch (...)
    {
    }
    return result;
}

std::vector<Arrival> fromArrivals(const std::vector<MASS::Arrival> &arrivals,
                                  std::shared_ptr<UMPS::Logging::ILog> &logger)
{
    std::vector<Arrival> result;
    result.reserve(arrivals.size());
    for (const auto &arrival : arrivals)
    {
        try
        {
            result.push_back(::fromArrival(arrival));
        }
        catch (const std::exception &e)
        {
            if (logger)
            {
                logger->warn("Failed to add arrival because "
                           + std::string {e.what()});
            }
        }
    }
    return result;
}

Origin fromEvent(const MASS::Event &event,
                 std::shared_ptr<UMPS::Logging::ILog> &logger)
{
    Origin origin;
    origin.setLatitude(event.getLatitude());
    origin.setLongitude(event.getLongitude());
    origin.setDepth(event.getDepth());
    origin.setTime(event.getOriginTime());
    const auto &eventArrivals = event.getArrivalsReference();
    origin.setArrivals(::fromArrivals(eventArrivals, logger));
    return origin;
}

std::vector<Origin> fromEvents(const std::vector<MASS::Event> &events,
                               std::shared_ptr<UMPS::Logging::ILog> &logger)
{
    std::vector<Origin> origins;
    if (events.empty()){return origins;}
    origins.reserve(events.size());
    for (const auto &event : events)
    {
        try
        {
            origins.push_back(::fromEvent(event, logger));
        }
        catch (const std::exception &e)
        {
            if (logger)
            {
                logger->warn("Failed to add origin because "
                           + std::string {e.what()});
            }
        }
    }
    return origins;
}

std::vector<MASS::Pick> fromPicks(const std::vector<Pick> &picks,
                                  std::shared_ptr<UMPS::Logging::ILog> &logger)
{
    std::vector<MASS::Pick> result;
    result.reserve(picks.size());
    for (const auto &pick : picks)
    {
        try
        {
            result.push_back(::fromPick(pick));
        }
        catch (const std::exception &e)
        {
            if (logger)
            {
                logger->warn("Failed to add pick; failed with: "
                           + std::string {e.what()});
            }
        }
    }
    return result;
}

/*
ULocator::Station
getStationInformation(
    const std::string &networkCode,
    const std::string &stationName,
    const ULocator::Position::IGeographicRegion &region,
    std::shared_ptr<URTS::Database::Connection::IConnection> &aqmsConnection,
    std::shared_ptr<UMPS::Logging::ILog> &logger)
{
    if (networkCode.empty())
    {
        throw std::invalid_argument("Network code is empty");
    }
    if (stationName.empty())
    {
        throw std::invalid_argument("Station name is empty");
    }
    if (!aqmsConnection) 
    {
        throw std::invalid_argument("The connection is NULL");
    }
    auto name = networkCode + "." + stationName;
    // Grab the station information
    URTS::Database::AQMS::StationDataTable stationTable{logger};
    constexpr bool getCurrent{true};
    stationTable.query(networkCode, stationName, getCurrent);
    auto stationsData = stationTable.getStationData();
    if (stationsData.empty())
    {
        throw std::runtime_error("Could not find "
                               + name + " in AQMS database"); 
    }
    if (stationsData.size() != 1)
    {
        logger->warn("Multiple entries for " + name
                   + "; taking first one");
    }
    ULocator::Station stationInformation;
    stationInformation.setNetwork(networkCode);
    stationInformation.setName(stationName);
    stationInformation.setElevation(stationsData.at(0).getElevation());
    ULocator::Position::WGS84 position{stationsData.at(0).getLatitude(),
                                       stationsData.at(0).getLongitude()};
    stationInformation.setGeographicPosition(position, region);
    return stationInformation;
}
*/

}

class Service::ServiceImpl
{
public:
    /// @brief Constructor
    ServiceImpl(std::shared_ptr<UMPS::Messaging::Context> responseContext = nullptr,
                const std::shared_ptr<UMPS::Logging::ILog> &logger = nullptr)
    {
        if (logger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> ();
        }
        else
        {
            mLogger = logger;
        }
        mReplier
            = std::make_unique<UMPS::Messaging::RouterDealer::Reply>
              (responseContext, mLogger);
    }
/*
    void initialize(std::unique_ptr<ULocator::TravelTimeCalculatorMap> &&map)
    {
        // Geographic region
        bool isUtah{true};
        std::vector<std::unique_ptr<ULocator::Position::IKnownLocalLocation>>
            searchLocations;
        if (mOptions.getRegion() == ServiceOptions::Region::Utah)
        {
            mRegion = ULocator::Position::UtahRegion ().clone();
            // Default search locations
            isUtah = true;
            auto knownEvents = ULocator::Position::getKnownUtahEvents();
            for (const auto &event : knownEvents)
            {
                searchLocations.push_back(event.clone());
            }
            // And the quarry locations
        }
        else
        {
            mRegion = ULocator::Position::YNPRegion ().clone();
            // Default search
            isUtah = false;
            auto knownEvents = ULocator::Position::getKnownYNPEvents();
            for (const auto &event : knownEvents)
            {
                searchLocations.push_back(event.clone());
            }
        }
        // Create the clusterer
        auto clusterer = std::make_unique<MASS::DBSCAN> ();
        clusterer->initialize(mOptions.getDBSCANEpsilon(),
                              mOptions.getDBSCANMinimumClusterSize());
        // Create the migrator
        auto migrator = std::make_unique<MASS::IMigrator> (mLogger);
        migrator->setDefaultSearchLocations(searchLocations);
        migrator->setTravelTimeCalculatorMap(std::move(map));
        migrator->setGeographicRegion(*mRegion->clone());
        migrator->setPickSignalToMigrate(
            MASS::IMigrator::PickSignal::Boxcar);
        migrator->setMaximumEpicentralDistance(
            mOptions.getMaximumDistanceToAssociate());

    }
*/
    /// @brief Starts the service
    void start()
    {
        stop();
        setRunning(true);
        std::lock_guard<std::mutex> lockGuard(mMutex);
        mLogger->debug("Starting replier service...");
        mReplier->start();
    }
    /// @brief Stops the threads
    void stop()
    {
        mLogger->debug("MAssociate associator service stopping threads...");
        setRunning(false);
        if (mReplier != nullptr){mReplier->stop();}
    }
    /// @result True indicates the threads should keep running
    [[nodiscard]] bool keepRunning() const
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        return mKeepRunning;
    }
    /// @brief Toggles this as running or not running
    void setRunning(const bool running)
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        mKeepRunning = running;
    }
    /// @brief Respond to travel time calculation requests
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage>
        callback(const std::string &messageType,
                 const void *messageContents, const size_t length) noexcept
    {
        mLogger->debug("Beginning association...");
 
        AssociationRequest associationRequest;
        if (messageType == associationRequest.getMessageType())
        {
            AssociationResponse response;
            // Unpack the message
            try
            {
                associationRequest.fromMessage(
                    reinterpret_cast<const char *> (messageContents), length);
                response.setIdentifier(associationRequest.getIdentifier());
            }
            catch (const std::exception &e)
            {
                response.setReturnCode(
                    AssociationResponse::ReturnCode::InvalidRequest);
                return response.clone();
            }

            mLogger->debug("Association request received");
            auto picks = associationRequest.getPicks();
            // Try again when you have something
            auto nPicks = static_cast<int> (picks.size());
            if (nPicks < 4)
            {
                response.setUnassociatedPicks(picks);
                response.setReturnCode(
                    AssociationResponse::ReturnCode::Success);
                return response.clone(); 
            }
            // If any pick identifiers collide then override them
            try
            {
                auto newIdentifierCounter = picks.at(0).getIdentifier();
                for (int i = 1; i < nPicks; ++i)
                {
                    newIdentifierCounter
                        = std::max(newIdentifierCounter,
                                   picks.at(i).getIdentifier());
                }
                bool collision{false};
                for (int i = 0; i < nPicks; ++i)
                {
                    for (int j = i + 1; j < nPicks; ++j)
                    {
                        if (picks.at(i).getIdentifier() ==
                            picks.at(j).getIdentifier())
                        {
                            collision = true;
                            newIdentifierCounter = newIdentifierCounter + 1;
                            picks[j].setIdentifier(newIdentifierCounter);
                        }
                    }
                }
                if (collision)
                {
                    mLogger->warn("Duplicate identifiers detected");
                }
            }
            catch (const std::exception &e)
            {
                response.setReturnCode(
                    AssociationResponse::ReturnCode::AlgorithmicFailure);
                mLogger->error("Failed to adjust identifiers "
                             + std::string {e.what()});
                return response.clone();
            }
            // Convert to MAssociate picks
            std::vector<MASS::Pick> massPicks;
            try
            {
                massPicks = ::fromPicks(picks, mLogger);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to convert picks");
                response.setReturnCode(
                   AssociationResponse::ReturnCode::AlgorithmicFailure);
                return response.clone();
            }
            // Insufficient number of picks were converted
            if (static_cast<int> (massPicks.size()) < 4)
            {
                response.setUnassociatedPicks(picks);
                response.setReturnCode(
                    AssociationResponse::ReturnCode::Success);
                return response.clone(); 
            }
            // Ensure we have the travel time tables for the pick.
            if (mAQMSConnection)
            {
                for (const auto &pick : massPicks)
                {
                    if (!mAssociator->haveTravelTimeCalculator(pick))
                    {
                        // Attempt ot create the table
                        try
                        {
                            // Create the station information from the database
                            auto identifier = pick.getWaveformIdentifier();
                            auto calculatorName = identifier.getNetwork()
                                                + identifier.getStation()
                                                + pick.getPhaseHint();
                            mLogger->debug("Attempting to create 1D calculator "
                                         + calculatorName);
                            auto uStation
                                = ::createStation(identifier.getNetwork(),
                                                  identifier.getStation(),
                                                  mIsUtah,
                                                  mAQMSConnection,
                                                  mLogger);
                            // If we haven't read it now then we don't have the
                            // corrections
                            std::filesystem::path staticCorrectionsFile; 
                            std::filesystem::path sourceSpecificCorrectionsFile;
                            auto calculator
                                = ::createTravelTimeCalculator(
                                      uStation,
                                      pick.getPhaseHint(),
                                      mIsUtah,
                                      staticCorrectionsFile,
                                      sourceSpecificCorrectionsFile,
                                      mLogger);
                            mAssociator->addTravelTimeCalculator(
                                uStation,
                                pick.getPhaseHint(),
                                std::move(calculator));
                        }
                        catch (const std::exception &e)
                        {
                            mLogger->warn(e.what());
                        } 
                    }
                }
            }
            // Now, if we don't have a table we purge the pick
            massPicks.erase
            (
                std::remove_if(massPicks.begin(), massPicks.end(),
                               [&](const MASS::Pick &massPick)
                {
                    try
                    {
                        return !mAssociator->haveTravelTimeCalculator(massPick);
                    }
                    catch (const std::exception &e)
                    {
                        mLogger->warn(e.what());
                        return true;
                    }
                }),
                massPicks.end()
            );
            // Insufficient number of picks have travel time calculators
            if (static_cast<int> (massPicks.size()) < 4)
            {
                response.setUnassociatedPicks(picks);
                response.setReturnCode(
                    AssociationResponse::ReturnCode::Success);
                return response.clone();
            }
            // Time to associate 
            try
            {
                //auto massPicks = ::fromPicks(picks, mLogger);
                mLogger->debug("Performing association with "
                             + std::to_string(massPicks.size())
                             + " picks");
                mAssociator->setPicks(massPicks);
                mAssociator->associate();
                auto associatedEvents = mAssociator->getEvents(); 
                // No events - this is easy
                if (associatedEvents.empty())
                {
                    mLogger->debug("Not events created"); 
                    response.setUnassociatedPicks(picks);
                    response.setReturnCode(
                        AssociationResponse::ReturnCode::Success);
                }
                else
                {
                    // Build the events
                    auto origins = ::fromEvents(associatedEvents, mLogger);
                    mLogger->debug("Created " + std::to_string(origins.size()) + " events!");
                    response.setOrigins(origins); 
                    // Set the unassociated picks
                    auto massUnassociatedPicks
                        = mAssociator->getUnassociatedPicks();
                    auto unassociatedPicks = ::fromPicks(massUnassociatedPicks);
                    // And for any pick not in the unassociated picks b/c of
                    // random problems along the way - add it
                    for (auto &pick : picks)
                    {
                        bool exists{false};
                        for (const auto &unassociatedPick : unassociatedPicks)
                        {
                            if (unassociatedPick.getIdentifier() ==
                                pick.getIdentifier())
                            {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists)
                        {
                            unassociatedPicks.push_back(std::move(pick));
                        }
                    }
                    response.setUnassociatedPicks(unassociatedPicks);
                    response.setReturnCode(
                        AssociationResponse::ReturnCode::Success);
                }
            }
            catch (const std::exception &e)
            {
                response.setReturnCode(
                    AssociationResponse::ReturnCode::AlgorithmicFailure); 
                mLogger->error("Failed to associate because "
                             + std::string {e.what()});
            }
            return response.clone();
        }
        mLogger->error("Unhandled message type: " + messageType);
        UMPS::MessageFormats::Failure response;
        response.setDetails("Unhandled message type");
        return response.clone();
    }
    mutable std::mutex mMutex;
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::shared_ptr<URTS::Database::Connection::IConnection>
        mAQMSConnection{nullptr};
    std::unique_ptr<UMPS::Messaging::RouterDealer::Reply> mReplier{nullptr};
    ServiceOptions mOptions;
    std::unique_ptr<MASS::Associator> mAssociator{nullptr};
    std::unique_ptr<ULocator::Position::IGeographicRegion> mRegion{nullptr};
    bool mIsUtah{true};
    bool mKeepRunning{false};
    bool mInitialized{false};
};

/// Constructor
Service::Service() :
    pImpl(std::make_unique<ServiceImpl> (nullptr, nullptr))
{
}

Service::Service(std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ServiceImpl> (nullptr, logger))
{
}

Service::Service(std::shared_ptr<UMPS::Messaging::Context> &responseContext,
                 std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ServiceImpl> (responseContext, logger))
{
}


/// Destructor
Service::~Service() = default;

/// Initialize
void Service::initialize(
    const ServiceOptions &options,
    std::unique_ptr<::MAssociate::Associator> &&associator,
    std::shared_ptr<URTS::Database::Connection::IConnection> &connection)
{
    if (associator == nullptr)
    {
        throw std::invalid_argument("Associator is NULL");
    }
    if (!associator->haveOptimizer())
    {
        throw std::invalid_argument("Optimizer not set on associator");
    }
    if (!associator->haveClusterer())
    {
        throw std::invalid_argument("Clustered not set on associator");
    }
    // Database connection
    pImpl->mAQMSConnection = nullptr;
    if (connection != nullptr){pImpl->mAQMSConnection = connection;}
    // Create the replier
    pImpl->mLogger->debug("Creating replier...");
    UMPS::Messaging::RouterDealer::ReplyOptions replierOptions;
    replierOptions.setAddress(options.getAddress());
    replierOptions.setZAPOptions(options.getZAPOptions());
    replierOptions.setPollingTimeOut(options.getPollingTimeOut());
    replierOptions.setSendHighWaterMark(options.getSendHighWaterMark());
    replierOptions.setReceiveHighWaterMark(
        options.getReceiveHighWaterMark());
    replierOptions.setCallback(std::bind(&ServiceImpl::callback,
                                         &*this->pImpl,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3));
    pImpl->mReplier->initialize(replierOptions); 
    std::this_thread::sleep_for(std::chrono::milliseconds {10});
    // Create the associator
    if (options.getRegion() == ServiceOptions::Region::Utah)
    {
        pImpl->mRegion = ULocator::Position::UtahRegion {}.clone();
        pImpl->mIsUtah = true;
    }
    else
    {
        pImpl->mRegion = ULocator::Position::YNPRegion {}.clone();
        pImpl->mIsUtah = false;
    }
    pImpl->mAssociator = std::move(associator);
    // Initialized?
    pImpl->mInitialized = pImpl->mReplier->isInitialized();
    if (pImpl->mInitialized)
    {
        pImpl->mLogger->debug("Service initialized!");
        pImpl->mOptions = options;
    }
    else
    {
        pImpl->mLogger->error("Failed to initialize service.");
    }
}

/// Initialized?
bool Service::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// Start the service
void Service::start()
{
    if (!isInitialized()){throw std::runtime_error("Class not initialized");}
    pImpl->mLogger->debug("Starting MAssociator associator service");
    pImpl->start();
}

/// Stop the service
void Service::stop()
{
    pImpl->mLogger->debug("stopping MAssociator associator service");
    pImpl->stop();
}

/// Running?
bool Service::isRunning() const noexcept
{
    return pImpl->keepRunning();
}

