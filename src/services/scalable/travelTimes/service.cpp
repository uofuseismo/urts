#include <mutex>
#include <cmath>
#include <thread>
#include <string>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/messaging/routerDealer/reply.hpp>
#include <umps/messaging/routerDealer/replyOptions.hpp>
#include <umps/messageFormats/failure.hpp>
#include <uLocator/travelTimeCalculatorMap.hpp>
#include <uLocator/uussRayTracer.hpp>
#include <uLocator/station.hpp>
#include <uLocator/position/wgs84.hpp>
#include <uLocator/position/utahRegion.hpp>
#include <uLocator/position/ynpRegion.hpp>
#include "urts/services/scalable/travelTimes/service.hpp"
#include "urts/services/scalable/travelTimes/serviceOptions.hpp"
#include "urts/services/scalable/travelTimes/stationRequest.hpp"
#include "urts/services/scalable/travelTimes/stationResponse.hpp"
#include "private/threadSafeQueue.hpp"

using namespace URTS::Services::Scalable::TravelTimes;
namespace URouterDealer = UMPS::Messaging::RouterDealer;
namespace UMF = UMPS::MessageFormats;

class Service::ServiceImpl
{
public:
    /// @brief Constructor
    ServiceImpl(std::shared_ptr<UMPS::Messaging::Context> responseContext = nullptr,
                std::shared_ptr<UMPS::Messaging::Context> dataPacketContext = nullptr,
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
        mTravelTimesReplier
            = std::make_unique<URouterDealer::Reply> (responseContext, mLogger);
//        mCappedCollection = std::make_unique<CappedCollection> (mLogger);
    }
/*
    /// @brief A thread running this function will read data packet messages
    ///        from the data packet broadcast and put them into the queue.
    void getPackets()
    {
        while (keepRunning())
        {
            try
            {
                auto dataPacket = mDataPacketSubscriber->receive();
                if (dataPacket == nullptr){continue;} // Time out
                mDataPacketQueue.push(std::move(*dataPacket));
            }
            catch (const std::exception &e)
            {
                mLogger->error("Error receiving packet: "
                             + std::string(e.what()));
                continue;
            }
        }
        mLogger->debug("Data packet broadcast to queue thread has exited");
    }
    /// @brief A thread running this function will read messages from the queue
    ///        and put them into the circular buffer.
    void queueToPacketCache()
    {
        while (keepRunning())
        {
            UDP::DataPacket dataPacket;
            if (mDataPacketQueue.wait_until_and_pop(&dataPacket))
            {
                if (dataPacket.getNumberOfSamples() > 0)
                {
                    try
                    {
                        mCappedCollection->addPacket(std::move(dataPacket));
                    }
                    catch (const std::exception &e)
                    {
                        mLogger->error("Failed to add packet.  Failed with: "
                                     + std::string{e.what()});
                    }
                }
            }
        }
        mLogger->debug("Queue to circular buffer thread has exited");
    }
*/
    /// @brief Starts the service
    void start()
    {
        stop();
        setRunning(true); 
        std::lock_guard<std::mutex> lockGuard(mMutex);
        // Start thread to read / respond to messages
        mLogger->debug("Starting replier service...");
        mTravelTimesReplier->start();
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
    /// @brief Stops the threads
    void stop()
    {
        mLogger->debug("PacketCache stopping threads...");
        setRunning(false);
        if (mTravelTimesReplier != nullptr)
        {
            if (mTravelTimesReplier->isRunning()){mTravelTimesReplier->stop();}
        }
    }
    /// @brief Updates the travel time calculator map and computes the travel
    ///        time.
    void updateTravelTimeCalculatorMap(const std::string &networkCode,
                                       const std::string &stationName,
                                       const std::string &phase,
                                       const double stationLatitude,
                                       const double stationLongitude,
                                       const double stationElevation)
    {
        if (networkCode.empty())
        {
            throw std::invalid_argument("Network code is empty");
        }
        if (stationName.empty())
        {
            throw std::invalid_argument("Station name is empty");
        }
        if (phase.empty())
        {
            throw std::invalid_argument("Phase is empty");
        }
        ULocator::Position::WGS84 stationLocation{stationLatitude,
                                                  stationLongitude, mUTMZone};
    }
    /// @brief Respond to travel time calculation requests
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage>
        callback(const std::string &messageType,
                 const void *messageContents, const size_t length) noexcept
    {
        // Figure out what type of request this is
        mLogger->debug("Request received");
        StationRequest stationRequest;
        if (messageType == stationRequest.getMessageType())
        {
            mLogger->debug("Station travel time request received");
            // Deserialize the message
            StationResponse response;
            try
            {
                stationRequest.fromMessage(
                    static_cast<const char *> (messageContents), length);
                response.setIdentifier(stationRequest.getIdentifier());
            }
            catch (...)
            {
                mLogger->error("Received invalid travel time request");
                response.setReturnCode(
                    StationResponse::ReturnCode::InvalidMessage);
                return response.clone();
            }
            // Unpack requisite information
            ULocator::Station uStation;
            uStation.setNetwork(stationRequest.getNetwork());
            uStation.setName(stationRequest.getStation());
            constexpr double eventTime{0};
            double eventLatitude{41.2};
            double eventLongitude{-112.};
            double eventDepth{0};
            std::string phase{"P"};
            bool applyCorrection{true};
            bool doUtah{true};
            try
            {
                // Source location
                eventLatitude = stationRequest.getLatitude();
                eventLongitude = stationRequest.getLongitude();
                eventDepth = stationRequest.getDepth();
                // Phase
                if (stationRequest.getPhase() == StationRequest::Phase::S)
                {
                    phase = "S";
                }
                else
                {
                    if (stationRequest.getPhase() != StationRequest::Phase::P)
                    {
                        throw std::invalid_argument("Unhandled phase");
                    }
                    phase = "P";
                }
                // Correction
                applyCorrection = stationRequest.useCorrections();
                // Region
                if (stationRequest.getRegion() == StationRequest::Region::Utah)
                {
                    doUtah = true;
                }
                else if (stationRequest.getRegion() ==
                         StationRequest::Region::Yellowstone)
                {
                    doUtah = false;
                }
                else
                {
                    if (stationRequest.getRegion() !=
                        StationRequest::Region::EventBased)
                    {
                        throw std::invalid_argument("Unhandled region");
                    }
                    if (eventLatitude < 43)
                    {
                        doUtah = true;
                    }
                    else
                    {
                        doUtah = false;
                    }
                }
            }
            catch (const std::exception &e)
            {
                mLogger->warn(e.what());
                response.setReturnCode(
                    StationResponse::ReturnCode::InvalidParameter);
                return response.clone();
            }
            try
            {
                if (doUtah)
                {
                    auto xySource
                        = mUtahRegion.geographicToLocalCoordinates(
                              eventLatitude, eventLongitude);
                    auto travelTime = mUtahMap->evaluate(uStation,
                                                         phase,
                                                         eventTime,
                                                         xySource.first,
                                                         xySource.second,
                                                         eventDepth,
                                                         applyCorrection);
                    response.setTravelTime(std::max(0.0, travelTime));
                }
                else
                {
                   auto xySource
                        = mYNPRegion.geographicToLocalCoordinates(
                             eventLatitude, eventLongitude);
                   auto travelTime = mYNPMap->evaluate(uStation,
                                                       phase,
                                                       eventTime,
                                                       xySource.first,
                                                       xySource.second,
                                                       eventDepth,
                                                       applyCorrection);
                   response.setTravelTime(std::max(0.0, travelTime));
                }
            }
            catch (const std::exception &e)
            {
                mLogger->error(e.what()); 
                response.setReturnCode(
                    StationResponse::ReturnCode::AlgorithmicFailure);
            }
            mLogger->debug("Replying to station travel time request");
            return response.clone();
        }
        mLogger->error("Unhandled message type: " + messageType);
        UMPS::MessageFormats::Failure response;
        response.setDetails("Unhandled message type");
        return response.clone();
    }
    /// Destructor
    ~ServiceImpl()
    {
        stop();
    }
//private:
    mutable std::mutex mMutex;
    //ULocator::TravelTimeCalculatorMap mTravelTimeCalculatorMap;
    //std::thread mDataPacketSubscriberThread;
    //std::thread mQueueToPacketCacheThread;
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    ULocator::Position::UtahRegion mUtahRegion;
    ULocator::Position::YNPRegion mYNPRegion;
    std::unique_ptr<ULocator::TravelTimeCalculatorMap> mYNPMap{nullptr};
    std::unique_ptr<ULocator::TravelTimeCalculatorMap> mUtahMap{nullptr};
    std::unique_ptr<URouterDealer::Reply> mTravelTimesReplier{nullptr};
    ServiceOptions mOptions;
    int mUTMZone{12};
    bool mKeepRunning{true};
    bool mInitialized{false};
};

/// Constructor
Service::Service() :
    pImpl(std::make_unique<ServiceImpl> (nullptr, nullptr, nullptr))
{
}

/*
/// Constructor context 
Service::Service(std::shared_ptr<UMPS::Messaging::Context> &context)
    pImpl(std::make_unique<ServiceImpl> (context, nullptr))
{
}
*/

/// Constructor with logger
Service::Service(std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ServiceImpl> (nullptr, nullptr, logger))
{
}

/// Constructor with context and a logger
Service::Service(std::shared_ptr<UMPS::Messaging::Context> &replierContext,
                 std::shared_ptr<UMPS::Messaging::Context> &broadcastContext,
                 std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ServiceImpl> (replierContext,
                                         broadcastContext,
                                         logger))
{
}

/// Destructor
Service::~Service() = default;

/// Initialize the class
void Service::initialize(const ServiceOptions &options)
{
    if (!options.haveAddress())
    {
        throw std::invalid_argument("Replier address not set");
    }
    // Ensure the service is stopped
    stop(); // Ensure the service is stopped
    // Create the replier
    pImpl->mLogger->debug("Creating travel times replier...");
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
    pImpl->mTravelTimesReplier->initialize(replierOptions); 
    std::this_thread::sleep_for(std::chrono::milliseconds {10});
    // Create the travel times calculator
/*
    pImpl->mLogger->debug("Creating capped collection...");
    auto maximumNumberOfPackets = options.getMaximumNumberOfPackets();
    pImpl->mCappedCollection->initialize(maximumNumberOfPackets);
    // Initialized?
    pImpl->mInitialized = pImpl->mDataPacketSubscriber->isInitialized() &&
                          pImpl->mTravelTimeReplier->isInitialized() &&
                          pImpl->mCappedCollection->isInitialized();
*/
    if (pImpl->mInitialized)
    {
        pImpl->mLogger->debug("Packet cache service initialized"); 
        pImpl->mOptions = options;
    }
    else
    {
        pImpl->mLogger->error("Failed to initialize packet cache service");
    }
}

/// Start service
void Service::start()
{
    if (!isInitialized())
    {
        throw std::runtime_error("PacketCache service not initialized");
    }
    pImpl->mLogger->debug("Starting service...");
    pImpl->start();
}

/// Running?
bool Service::isRunning() const noexcept
{
    return pImpl->keepRunning();
}

/// Stop service
void Service::stop()
{
    pImpl->mLogger->debug("Stopping service...");
    pImpl->stop();
}

/// Initialized?
bool Service::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/*
/// Get total number of packets
int Service::getTotalNumberOfPackets() const noexcept
{
    return pImpl->mCappedCollection->getTotalNumberOfPackets();
}
*/
