#include <umps/logging/standardOut.hpp>
#include <umps/messageFormats/failure.hpp>
#include <umps/messageFormats/message.hpp>
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
    }
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
                associationRequest.fromMessage(reinterpret_cast<const char *> (messageContents), length);
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
            }
            // Ensure we have the travel time tables for the pick.
            // This is an iterative loop
            for (int k = 0; k < nPicks; ++k)
            {
                bool createdTables{false};
                if (!createdTables){break;}
            }
            // Time to associate 
            try
            {
                auto massPicks = ::fromPicks(picks, mLogger);
                mAssociator->setPicks(massPicks);
                mAssociator->associate();
                auto associatedEvents = mAssociator->getEvents(); 
                // No events - this is easy
                if (associatedEvents.empty())
                {
                    response.setUnassociatedPicks(picks);
                    response.setReturnCode(
                        AssociationResponse::ReturnCode::Success);
                }
                else
                {
                    // Build the events
                    auto origins = ::fromEvents(associatedEvents, mLogger);
                    response.setOrigins(origins); 
                    // Set the unassociated picks
                    mAssociator->getUnassociatedPicks();

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
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::shared_ptr<URTS::Database::Connection::IConnection>
        mAQMSConnection{nullptr};
    ServiceOptions mOptions;
    std::unique_ptr<MASS::Associator> mAssociator{nullptr};
    std::unique_ptr<ULocator::Position::IGeographicRegion> mRegion{nullptr};
};

/// Destructor
Service::~Service() = default;
