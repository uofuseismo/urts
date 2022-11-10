#include <chrono>
#include <mutex>
#include <set>
#include <thread>
#ifndef NDEBUG
#include <cassert>
#endif
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <ummps/authentication/zapOptions.hpp>
#include "urts/services/standalone/incrementer/service.hpp"
#include "urts/services/standalone/incrementer/serviceOptions.hpp"
#include "urts/services/standalone/incrementer/counter.hpp"
#include "urts/services/standalone/incrementer/options.hpp"
#include "urts/services/standalone/incrementer/counter.hpp"

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
        mIncrementerReplier = std::make_unique<Replier> (mContext, mLogger);
        mCounter = std::make_shared<Counter> ();
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
        mLogger->debug("Starting the replier...");
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
    // Respond to data requests
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
                response->setReturnCode(Items::ReturnCode::AlgorithmFailure);
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
    Options mOptions;
    std::string mName = "Incrementer";
    bool mKeepRunning = true;
    bool mInitialized = false;
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
void Service::initialize(const Options &options)
{
    stop(); // Ensure the service is stopped
    if (!options.haveBackendAddress())
    {
        throw std::runtime_error("Backend address not set");
    }
    // Counter
    auto sqlite3File = options.getSqlite3FileName();
    auto deleteIfExists = options.deleteSqlite3FileIfExists();
    pImpl->mCounter->initialize(sqlite3File, deleteIfExists);
    int64_t initialValue = options.getInitialValue();
    int increment = options.getIncrement();
    std::set<std::string> defaultItems{"Amplitude",
                                       "Event",
                                       "Magnitude",
                                       "Origin",
                                       "PhasePick",
                                       "PhaseArrival"};
    for (const auto &item : defaultItems)
    {
        if (!pImpl->mCounter->haveItem(item))
        {
            pImpl->mCounter->addItem(item, initialValue, increment);
        }
    }
    // Replier options
    ReplierOptions replierOptions;
    replierOptions.setAddress(options.getBackendAddress());
    replierOptions.setZAPOptions(options.getZAPOptions());
    pImpl->mIncrementerReplier->initialize(replierOptions,
                                           pImpl->mCounter);
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
