#include <umps/logging/standardOut.hpp>
#include <umps/messageFormats/failure.hpp>
#include <umps/messageFormats/message.hpp>
#include <massociate/pick.hpp>
#include <massociate/associator.hpp>
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
#include "urts/broadcasts/internal/pick/pick.hpp"
//#include "urts/services/scalable/associators/massociate/assocationResponse.hpp"

namespace MASS = MAssociate;
using namespace URTS::Services::Scalable::Associators::MAssociate;

namespace
{

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
 
std::vector<MASS::Pick> picks;
        if (true)
        {
            try
            {
                mAssociator->setPicks(picks);
                mAssociator->associate();
            }
            catch (const std::exception &e)
            {
            }
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
