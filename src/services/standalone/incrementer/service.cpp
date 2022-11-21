#include <chrono>
#include <mutex>
#include <set>
#include <thread>
#ifndef NDEBUG
#include <cassert>
#endif
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/routerDealer/reply.hpp>
#include <umps/messaging/routerDealer/replyOptions.hpp>
#include <umps/messaging/context.hpp>
#include <umps/messageFormats/failure.hpp>
#include <umps/authentication/zapOptions.hpp>
#include "urts/services/standalone/incrementer/service.hpp"
#include "urts/services/standalone/incrementer/serviceOptions.hpp"
#include "urts/services/standalone/incrementer/counter.hpp"
#include "urts/services/standalone/incrementer/incrementRequest.hpp"
#include "urts/services/standalone/incrementer/incrementResponse.hpp"
#include "urts/services/standalone/incrementer/itemsRequest.hpp"
#include "urts/services/standalone/incrementer/itemsResponse.hpp"

using namespace URTS::Services::Standalone::Incrementer;

class Service::ServiceImpl
{
public:
    /// Constructors
    ServiceImpl() = delete;
    ServiceImpl(const std::shared_ptr<UMPS::Messaging::Context> &context,
                const std::shared_ptr<UMPS::Logging::ILog> &logger)
    {
        if (context == nullptr)
        {
            mContext = std::make_shared<UMPS::Messaging::Context> (1);
        }
        else
        {
            mContext = context;
        }
        if (logger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> ();
        }
        else
        {
            mLogger = logger;
        }
        mIncrementerReplier
            = std::make_unique<UMPS::Messaging::RouterDealer::Reply>
              (mContext, mLogger);
        mCounter = std::make_unique<Counter> ();
    }
    /// Stops the proxy and authenticator and joins threads
    void stop()
    {
        mLogger->debug("Incrementer stopping threads...");
        setRunning(false);
        if (mIncrementerReplier != nullptr)
        {
            if (mIncrementerReplier->isRunning()){mIncrementerReplier->stop();}
        }
    }
    /// Starts the proxy and authenticator and creates threads
    void start()
    {
        stop();
        setRunning(true);
        std::lock_guard<std::mutex> lockGuard(mMutex);
#ifndef NDEBUG
        assert(mIncrementerReplier->isInitialized());
#endif
        mLogger->debug("Starting the incrementer replier...");
        mIncrementerReplier->start();
#ifndef NDEBUG
        assert(mIncrementerReplier->isRunning());
#endif
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
    /// Destructor
    ~ServiceImpl()
    {
        stop();
    }
    /// Callback function
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage>
        callback(const std::string &messageType,
                 const void *messageContents, const size_t length) noexcept
    {   
        if (mLogger->getLevel() >= UMPS::Logging::Level::Debug)
        {
            mLogger->debug("ReplierImpl::callback: Message of type: "
                         + messageType 
                         + " with length: " + std::to_string(length)
                         + " bytes was received.  Processing...");
        }
        ItemsRequest itemsRequest;
        IncrementRequest incrementRequest;
        if (messageType == incrementRequest.getMessageType())
        {
            auto response = std::make_unique<IncrementResponse> (); 
            // Unpack the request
            std::string itemName;
            try
            {
                auto messagePtr = static_cast<const char *> (messageContents);
                incrementRequest.fromMessage(messagePtr, length);
                itemName = incrementRequest.getItem();
            }
            catch (const std::exception &e) 
            {
                mLogger->error("Request serialization failed with: "
                             + std::string(e.what()));
                response->setReturnCode(
                    IncrementResponse::ReturnCode::InvalidMessage);
                return response;
            }
            // Set the identifier to help out the recipient
            response->setIdentifier(incrementRequest.getIdentifier());
            // Process the request
            try
            {
                auto nextValue = mCounter->getNextValue(itemName);
                response->setValue(nextValue);
                response->setReturnCode(
                    IncrementResponse::ReturnCode::Success);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Incrementer failed with: "
                             + std::string(e.what()));
                response->setReturnCode(
                    IncrementResponse::ReturnCode::AlgorithmFailure);
            }
            return response;
        }
        else if (messageType == itemsRequest.getMessageType())
        {
            auto response = std::make_unique<ItemsResponse> ();
            try
            {
                auto messagePtr = static_cast<const char *> (messageContents);
                itemsRequest.fromMessage(messagePtr, length);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Request serialization failed with: "
                             + std::string(e.what()));
                response->setReturnCode(
                    ItemsResponse::ReturnCode::InvalidMessage);
                return response;
            }
            response->setIdentifier(itemsRequest.getIdentifier());
            try
            {
                auto items = mCounter->getItems();
                response->setItems(items);
                response->setReturnCode(ItemsResponse::ReturnCode::Success);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Incrementer getItems failed with: "
                             + std::string(e.what()));
                response->setReturnCode(
                    ItemsResponse::ReturnCode::AlgorithmFailure);
            }
            return response;
        }
        else
        {
            mLogger->error("Expecting message type: "
                         + itemsRequest.getMessageType() + " or "
                         + incrementRequest.getMessageType()
                         + " but received: " + messageType);
        }
        auto response = std::make_unique<UMPS::MessageFormats::Failure> ();
        response->setDetails("Unhandled message type in callback: "
                           + messageType);
        return response;
    }
///private:
    mutable std::mutex mMutex;
    std::shared_ptr<UMPS::Messaging::Context> mContext{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<UMPS::Messaging::RouterDealer::Reply>
        mIncrementerReplier{nullptr};
    std::unique_ptr<Counter> mCounter{nullptr};
    ServiceOptions mOptions;
    const std::string mName{"Incrementer"};
    bool mKeepRunning{true};
    bool mInitialized{false};
};

/// C'tor
Service::Service() :
    pImpl(std::make_unique<ServiceImpl> (nullptr, nullptr))
{
}

Service::Service(std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ServiceImpl> (nullptr, logger))
{
}

Service::Service(std::shared_ptr<UMPS::Messaging::Context> &context) :
    pImpl(std::make_unique<ServiceImpl> (context, nullptr))
{
}

Service::Service(std::shared_ptr<UMPS::Messaging::Context> &context,
                 std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ServiceImpl> (context, logger))
{
}

/// Destructor
Service::~Service() = default;

/// Is the service running
bool Service::isRunning() const noexcept
{
    return pImpl->keepRunning();
}

/// Initialize
void Service::initialize(const ServiceOptions &options)
{
    stop(); // Ensure the service is stopped
    if (!options.haveAddress()){throw std::runtime_error("Address not set");}
    // Setup counter
    auto sqlite3File = options.getSqlite3FileName();
    auto deleteIfExists = options.deleteSqlite3FileIfExists();
    pImpl->mCounter->initialize(sqlite3File, deleteIfExists);
    // Make sure default items are in table
    auto initialValue = options.getInitialValue();
    auto increment = options.getIncrement();
    for (auto item : IncrementRequest::getItemSet())
    {
        auto itemString = IncrementRequest::itemToString(item);
        if (!pImpl->mCounter->haveItem(itemString))
        {
            pImpl->mCounter->addItem(itemString, initialValue, increment);
        }
    }
    // Replier options
    UMPS::Messaging::RouterDealer::ReplyOptions replyOptions;
    replyOptions.setAddress(options.getAddress());
    replyOptions.setZAPOptions(options.getZAPOptions());
    replyOptions.setSendHighWaterMark(options.getSendHighWaterMark());
    replyOptions.setReceiveHighWaterMark(options.getReceiveHighWaterMark());
    replyOptions.setPollingTimeOut(replyOptions.getPollingTimeOut());
    replyOptions.setCallback(std::bind(&ServiceImpl::callback,
                                       &*this->pImpl,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3));
    pImpl->mIncrementerReplier->initialize(replyOptions);
    pImpl->mOptions = options;
    pImpl->mInitialized = pImpl->mIncrementerReplier->isInitialized();
}

/// Is the service initialized?
bool Service::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// Runs the service
void Service::start()
{
    if (!isInitialized())
    {
        throw std::runtime_error("Service not initialized");
    }
    pImpl->mLogger->debug("Beginning service " + pImpl->mName + "...");
    pImpl->start();
}

/// Stop the service
void Service::stop()
{
    pImpl->stop();
}
