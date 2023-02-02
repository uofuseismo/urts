#include <umps/authentication/zapOptions.hpp>
#include <umps/messaging/publisherSubscriber/subscriber.hpp>
#include <umps/messaging/publisherSubscriber/subscriberOptions.hpp>
#include <umps/messageFormats/messages.hpp>
#include <umps/messageFormats/message.hpp>
#include <umps/logging/log.hpp>
#include "urts/broadcasts/internal/dataPacket/subscriber.hpp"
#include "urts/broadcasts/internal/dataPacket/subscriberOptions.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "private/staticUniquePointerCast.hpp"

using namespace URTS::Broadcasts::Internal::DataPacket;
namespace UCI = UMPS::Services::ConnectionInformation;
namespace UAuth = UMPS::Authentication;
namespace UPubSub = UMPS::Messaging::PublisherSubscriber;

class Subscriber::SubscriberImpl
{
public:
    SubscriberImpl(std::shared_ptr<UMPS::Messaging::Context> context,
                   std::shared_ptr<UMPS::Logging::ILog> logger) :
        mLogger(logger)
    {
        mSubscriber = std::make_unique<UPubSub::Subscriber> (context, logger);
        std::unique_ptr<UMPS::MessageFormats::IMessage> dataPacketMessageType
            = std::make_unique<DataPacket> (); 
        mMessageTypes.add(dataPacketMessageType);
    }
    std::shared_ptr<UMPS::Logging::ILog> mLogger;
    std::unique_ptr<UPubSub::Subscriber> mSubscriber;
    SubscriberOptions mOptions;
    UMPS::MessageFormats::Messages mMessageTypes;
};

/// C'tor
Subscriber::Subscriber() :
    pImpl(std::make_unique<SubscriberImpl> (nullptr, nullptr))
{
}

Subscriber::Subscriber(std::shared_ptr<UMPS::Messaging::Context> &context) :
    pImpl(std::make_unique<SubscriberImpl> (context, nullptr))
{
}

Subscriber::Subscriber(std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<SubscriberImpl> (nullptr, logger))
{
}

Subscriber::Subscriber(std::shared_ptr<UMPS::Messaging::Context> &context,
                       std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<SubscriberImpl> (context, logger))
{
}

/// Move c'tor
Subscriber::Subscriber(Subscriber &&subscriber) noexcept
{
    *this = std::move(subscriber);
}

/// Move assignment
Subscriber& Subscriber::operator=(Subscriber &&subscriber) noexcept
{
    if (&subscriber == this){return *this;}
    pImpl = std::move(subscriber.pImpl);
    return *this;
}

/// Initialize
void Subscriber::initialize(const SubscriberOptions &options)
{
    if (!options.haveAddress()){throw std::runtime_error("Address not set");}
    if (pImpl->mLogger != nullptr)
    {
        pImpl->mLogger->debug("Data packet subscriber connecting to "
                           + options.getAddress());
    }
    UMPS::Messaging::PublisherSubscriber::SubscriberOptions subscriberOptions;
    subscriberOptions.setAddress(options.getAddress());
    subscriberOptions.setZAPOptions(options.getZAPOptions());
    subscriberOptions.setReceiveTimeOut(options.getTimeOut());
    subscriberOptions.setReceiveHighWaterMark(options.getHighWaterMark());
    subscriberOptions.setMessageTypes(pImpl->mMessageTypes);
    //auto subscriberOptions = pImpl->mOptions.getSubscriberOptions();
    pImpl->mSubscriber->initialize(subscriberOptions);
    pImpl->mOptions = options;
    if (pImpl->mLogger != nullptr)
    {
        pImpl->mLogger->debug("Data packet subscriber connected!");
    }
}

/// Initialized?
bool Subscriber::isInitialized() const noexcept
{
    return pImpl->mSubscriber->isInitialized();
}

/// Destructor
Subscriber::~Subscriber() = default;

/// Receive
std::unique_ptr<DataPacket> Subscriber::receive() const
{
    auto dataPacket
        = static_unique_pointer_cast<DataPacket>
          (pImpl->mSubscriber->receive());
    return dataPacket;
}
