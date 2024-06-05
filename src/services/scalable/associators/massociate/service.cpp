#include <umps/logging/standardOut.hpp>
#include <umps/messageFormats/failure.hpp>
#include <umps/messageFormats/message.hpp>
#include <massociate/pick.hpp>
#include <massociate/associator.hpp>
#include <massociate/waveformIdentifier.hpp>
#include <massociate/dbscan.hpp>
#include <massociate/migrator.hpp>
#include <uLocator/position/knownUtahEvent.hpp>
#include <uLocator/position/knownUtahQuarry.hpp>
#include <uLocator/position/knownYNPEvent.hpp>
#include <uLocator/position/utahRegion.hpp>
#include <uLocator/position/ynpRegion.hpp>
#include "urts/services/scalable/associators/massociate/service.hpp"
#include "urts/services/scalable/associators/massociate/serviceOptions.hpp"
#include "urts/services/scalable/associators/massociate/associationRequest.hpp"
#include "urts/services/scalable/associators/massociate/associationResponse.hpp"
#include "urts/services/scalable/associators/massociate/arrival.hpp"
#include "urts/services/scalable/associators/massociate/origin.hpp"
#include "urts/services/scalable/associators/massociate/pick.hpp"
#include "urts/broadcasts/internal/pick/pick.hpp"

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
    void initialize()
    {
        // Geographic region
        bool isUtah{true};
        std::vector<std::unique_ptr<ULocator::Position::IKnownLocalLocation>>
            searchLocations;
        if (mOptions.getRegion() == ServiceOptions::Region::Utah)
        {
            mRegion = ULocator::Position::UtahRegion ().clone();
            // Default search
            isUtah = true;
            auto knownEvents = ULocator::Position::getKnownUtahEvents();
            for (const auto &event : knownEvents)
            {
                searchLocations.push_back(event.clone());
            }
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
//        migrator->setTravelTimeCalculatorMap(
//            std::move(travelTimeCalculatorMap));
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
            mLogger->debug("Association request received");
            try
            {
                auto massPicks
                    = ::fromPicks(associationRequest.getPicksReference(),
                                  mLogger);
                mAssociator->setPicks(massPicks);
                mAssociator->associate();
            }
            catch (const std::exception &e)
            {
            }
            return response.clone();
        }
        mLogger->error("Unhandled message type: " + messageType);
        UMPS::MessageFormats::Failure response;
        response.setDetails("Unhandled message type");
        return response.clone();
    }
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    ServiceOptions mOptions;
    std::unique_ptr<MASS::Associator> mAssociator{nullptr};
    std::unique_ptr<ULocator::Position::IGeographicRegion> mRegion{nullptr};
};

/// Destructor
Service::~Service() = default;
