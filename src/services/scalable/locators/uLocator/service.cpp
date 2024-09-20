#include <thread>
#include <cmath>
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
#include <uLocator/travelTimeCalculator.hpp>
#include <uLocator/travelTimeCalculatorMap.hpp>
#include <uLocator/uussRayTracer.hpp>
#include <uLocator/optimizers/nlopt/dividedRectangles.hpp>
#include <uLocator/optimizers/pagmo/particleSwarm.hpp>
#include <uLocator/position/knownUtahEvent.hpp>
#include <uLocator/position/knownUtahQuarry.hpp>
#include <uLocator/position/knownYNPEvent.hpp>
#include <uLocator/position/utahRegion.hpp>
#include <uLocator/position/ynpRegion.hpp>
#include <uLocator/position/wgs84.hpp>
#include "urts/services/scalable/locators/uLocator/service.hpp"
#include "urts/services/scalable/locators/uLocator/serviceOptions.hpp"
#include "urts/services/scalable/locators/uLocator/locationRequest.hpp"
#include "urts/services/scalable/locators/uLocator/locationResponse.hpp"
#include "urts/services/scalable/locators/uLocator/arrival.hpp"
#include "urts/services/scalable/locators/uLocator/origin.hpp"
//#include "urts/broadcasts/internal/pick/pick.hpp"
#include "urts/database/connection/connection.hpp"
#include "urts/database/aqms/stationDataTable.hpp"
#include "urts/database/aqms/stationData.hpp"

#include "services/scalable/associators/massociate/createTravelTimeCalculator.hpp"

namespace ULoc = ULocator;
using namespace URTS::Services::Scalable::Locators::ULocator;

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

    /// @brief Respond to travel time calculation requests
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage>
        callback(const std::string &messageType,
                 const void *messageContents, const size_t length) noexcept
    {
        mLogger->debug("Beginning location...");
 
        LocationRequest locationRequest;
        if (messageType == locationRequest.getMessageType())
        {
            mLogger->debug("Location request received");
            LocationResponse response;
            // Unpack the message
            try
            {
                locationRequest.fromMessage(
                    reinterpret_cast<const char *> (messageContents), length);
                response.setIdentifier(locationRequest.getIdentifier());
            }
            catch (const std::exception &e)
            {
                response.setReturnCode(
                    LocationResponse::ReturnCode::InvalidRequest);
                return response.clone();
            }

            const auto &arrivals = locationRequest.getArrivalsReference();
            if (arrivals.empty())
            {
                response.setReturnCode(
                    LocationResponse::ReturnCode::InvalidRequest);
                return response.clone();
            }

        }
        mLogger->error("Unhandled message type: " + messageType);
        UMPS::MessageFormats::Failure response;
        response.setDetails("Unhandled message type");
        return response.clone();
    }
//private:
    mutable std::mutex mMutex;
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::shared_ptr<URTS::Database::Connection::IConnection>
        mAQMSConnection{nullptr};
    std::unique_ptr<UMPS::Messaging::RouterDealer::Reply> mReplier{nullptr};
    ServiceOptions mOptions;
    //std::unique_ptr<MASS::Associator> mAssociator{nullptr};
    std::unique_ptr<ULoc::Position::IGeographicRegion> mRegion{nullptr};
    std::unique_ptr<ULoc::TravelTimeCalculatorMap> mTravelTimeCalculators{nullptr};
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

/// Initialize
void Service::initialize(
    const ServiceOptions &options,
    std::shared_ptr<URTS::Database::Connection::IConnection> &connection)
{
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
        pImpl->mRegion = ULoc::Position::UtahRegion {}.clone();
        pImpl->mIsUtah = true;
    }
    else
    {
        pImpl->mRegion = ULoc::Position::YNPRegion {}.clone();
        pImpl->mIsUtah = false;
    }
/*
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
*/
}

/// Initialized?
bool Service::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/*
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
*/
