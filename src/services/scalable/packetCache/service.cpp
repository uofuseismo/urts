#include <iostream>
#include <mutex>
#include <thread>
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/messaging/routerDealer/reply.hpp>
#include <umps/messaging/routerDealer/replyOptions.hpp>
#include <umps/messageFormats/failure.hpp>
#include "urts/services/scalable/packetCache/service.hpp"
#include "urts/services/scalable/packetCache/serviceOptions.hpp"
#include "urts/services/scalable/packetCache/cappedCollection.hpp"
#include "urts/services/scalable/packetCache/dataRequest.hpp"
#include "urts/services/scalable/packetCache/dataResponse.hpp"
#include "urts/services/scalable/packetCache/bulkDataRequest.hpp"
#include "urts/services/scalable/packetCache/bulkDataResponse.hpp"
#include "urts/services/scalable/packetCache/sensorRequest.hpp"
#include "urts/services/scalable/packetCache/sensorResponse.hpp"
#include "urts/broadcasts/internal/dataPacket/subscriber.hpp"
#include "urts/broadcasts/internal/dataPacket/subscriberOptions.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "private/threadSafeQueue.hpp"

using namespace URTS::Services::Scalable::PacketCache;
namespace URouterDealer = UMPS::Messaging::RouterDealer;
namespace UMF = UMPS::MessageFormats;
namespace UDP = URTS::Broadcasts::Internal::DataPacket; 

namespace
{

void performDataRequest(const DataRequest &dataRequest,
                        const CappedCollection &mCappedCollection,
                        DataResponse *response)
{
    // Does this SNCL exist in the cache?
    auto name = dataRequest.getNetwork() + "."
              + dataRequest.getStation() + "."
              + dataRequest.getChannel() + "."
              + dataRequest.getLocationCode();
    auto haveSensor = mCappedCollection.haveSensor(name);
    if (haveSensor)
    {
        auto [startTime, endTime] = dataRequest.getQueryTimes();
        try
        {
            auto packets = mCappedCollection.getPackets(name,
                                                        startTime,
                                                        endTime);
            response->setPackets(packets);
        }
        catch (const std::exception &e)
        {
            auto errorMessage = "Query failed with: " + std::string {e.what()};
            response->setReturnCode(
                DataResponse::ReturnCode::AlgorithmicFailure);
            throw std::runtime_error(errorMessage);
        }
    }
    else
    {
        response->setReturnCode(DataResponse::ReturnCode::NoSensor);
    }
}

}

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
        mDataPacketSubscriber
            = std::make_unique<UDP::Subscriber> (dataPacketContext, mLogger);
        mPacketCacheReplier
            = std::make_unique<URouterDealer::Reply> (responseContext, mLogger);
        mCappedCollection = std::make_unique<CappedCollection> (mLogger);
    }
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
    /// @brief Starts the service
    void start()
    {
        stop();
        setRunning(true); 
        std::lock_guard<std::mutex> lockGuard(mMutex);
        // Start thread to read messages from broadcast and put into queue
        mLogger->debug("Starting data packet subscriber thread...");
        mDataPacketSubscriberThread
           = std::thread(&ServiceImpl::getPackets, this);
        // Start thread to read messages from queue and put into packetCache 
        mLogger->debug("Starting queue to packetCache thread...");
        mQueueToPacketCacheThread
           = std::thread(&ServiceImpl::queueToPacketCache, this);
        // Start thread to read / respond to messages
        mLogger->debug("Starting replier service...");
        mPacketCacheReplier->start();
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
        if (mPacketCacheReplier != nullptr)
        {
            if (mPacketCacheReplier->isRunning()){mPacketCacheReplier->stop();}
        }
        if (mDataPacketSubscriberThread.joinable())
        {
            mDataPacketSubscriberThread.join();
        }
        if (mQueueToPacketCacheThread.joinable())
        {
            mQueueToPacketCacheThread.join();
        }
        if (mResponseThread.joinable())
        {
            mResponseThread.join();
        }
    }
    // Respond to data requests
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage>
        callback(const std::string &messageType,
                 const void *messageContents, const size_t length) noexcept
    {
        // Get data
        mLogger->debug("Request received");
        DataRequest dataRequest;
        if (messageType == dataRequest.getMessageType())
        {
            mLogger->debug("Data request received");
            // Deserialize the message
            DataResponse response;
            try
            {
                dataRequest.fromMessage(
                    static_cast<const char *> (messageContents), length);
                response.setIdentifier(dataRequest.getIdentifier());
            }
            catch (...)
            {
                mLogger->error("Received invalid data request");
                response.setReturnCode(
                    DataResponse::ReturnCode::InvalidMessage);
                return response.clone();
            }
            // Does this SNCL exist in the cache?
            try
            {
                ::performDataRequest(dataRequest,
                                     *mCappedCollection,
                                     &response);
            }
            catch (const std::exception &e) 
            {
                mLogger->error(e.what());
            }
            mLogger->debug("Replying to data request");
            return response.clone();
        }
        // Bulk data request
        BulkDataRequest bulkDataRequest;
        if (messageType == bulkDataRequest.getMessageType())
        {
            mLogger->debug("Bulk data request received");
            // Deserialize the message
            PacketCache::BulkDataResponse response;
            try
            {
                bulkDataRequest.fromMessage(
                    static_cast<const char *> (messageContents), length);
                response.setIdentifier(bulkDataRequest.getIdentifier());
            }
            catch (...)
            {
                mLogger->error("Received invalid data request");
                response.setReturnCode(
                     BulkDataResponse::ReturnCode::InvalidMessage);
                return response.clone();
            }
            auto nRequests = bulkDataRequest.getNumberOfDataRequests();
            const auto dataRequests = bulkDataRequest.getDataRequestsPointer();
            for (int iRequest = 0; iRequest < nRequests; ++iRequest)
            {
                DataResponse dataResponse;
                dataResponse.setIdentifier(
                    dataRequests[iRequest].getIdentifier());
                try
                {
                    ::performDataRequest(dataRequests[iRequest],
                                         *mCappedCollection,
                                         &dataResponse);
                }
                catch (const std::exception &e)
                {
                    mLogger->error(e.what());
                }
                response.addDataResponse(std::move(dataResponse));
            }
            mLogger->debug("Replying to bulk data request");
            return response.clone();
        }
        // Get sensors
        SensorRequest sensorRequest;
        if (messageType == sensorRequest.getMessageType())
        {
            mLogger->debug("Sensor request received");
            SensorResponse response;
            try
            {
                sensorRequest.fromMessage(
                    static_cast<const char *> (messageContents), length);
                response.setIdentifier(sensorRequest.getIdentifier());
            }
            catch (...)
            {
                mLogger->error("Received invalid sensor request");
                response.setReturnCode(
                    SensorResponse::ReturnCode::InvalidMessage);
                return response.clone();
            }
            // Now get the result
            try
            {
                response.setNames(mCappedCollection->getSensorNames());
            }
            catch (...)
            {
                mLogger->error("Failed to set sensor names");
                response.setReturnCode(
                    SensorResponse::ReturnCode::AlgorithmicFailure);
            }
            mLogger->debug("Replying to sensor request");
            return response.clone();
        }
        // Send something back so they don't wait forever
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
    std::thread mDataPacketSubscriberThread;
    std::thread mQueueToPacketCacheThread;
    std::thread mResponseThread;
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<UDP::Subscriber> mDataPacketSubscriber{nullptr};
    std::unique_ptr<CappedCollection> mCappedCollection{nullptr};
    std::unique_ptr<URouterDealer::Reply> mPacketCacheReplier{nullptr};
    ::ThreadSafeQueue<UDP::DataPacket> mDataPacketQueue;
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

/// Constructor logger
Service::Service(std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ServiceImpl> (nullptr, nullptr, logger))
{
}

/*
/// Constructor logger
Service::Service(std::shared_ptr<UMPS::Messaging::Context> &context,
                 std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ServiceImpl> (context, logger))
{
}


*/

/// Destructor
Service::~Service() = default;

/*
/// Initialize the class
void Service::initialize(const ServiceOptions &options)
//    const int maxPackets,
//    const UDataPacket::SubscriberOptions<T> &dataPacketSubscriberOptions,
//    const ReplierOptions &packetCacheReplierOptions)
{
    stop(); // Ensure the service is stopped
    if (maxPackets <= 0)
    {
        throw std::invalid_argument("Max packets must be positive");
    }
    pImpl->mDataPacketSubscriber->initialize(dataPacketSubscriberOptions);
    pImpl->mCappedCollection->initialize(maxPackets);
    pImpl->mPacketCacheReplier->initialize(packetCacheReplierOptions,
                                           pImpl->mCappedCollection);
    pImpl->mInitialized = pImpl->mDataPacketSubscriber->isInitialized() &&
                          pImpl->mPacketCacheReplier->isInitialized() &&
                          pImpl->mCappedCollection->isInitialized();
}
*/

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

/// Get total number of packets
int Service::getTotalNumberOfPackets() const noexcept
{
    return pImpl->mCappedCollection->getTotalNumberOfPackets();
}

