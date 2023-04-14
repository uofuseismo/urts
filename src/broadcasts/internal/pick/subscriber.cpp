#include <umps/authentication/zapOptions.hpp>
#include <umps/messaging/xPublisherXSubscriber/subscriber.hpp>
#include <umps/messaging/xPublisherXSubscriber/subscriberOptions.hpp>
#include <umps/messageFormats/messages.hpp>
#include <umps/messageFormats/message.hpp>
#include <umps/logging/log.hpp>
#include "urts/broadcasts/internal/pick/subscriber.hpp"
#include "urts/broadcasts/internal/pick/subscriberOptions.hpp"
#include "urts/broadcasts/internal/pick/pick.hpp"
#include "private/staticUniquePointerCast.hpp"

using namespace URTS::Broadcasts::Internal::Pick;
namespace UCI = UMPS::Services::ConnectionInformation;
namespace UAuth = UMPS::Authentication;
namespace UPubSub = UMPS::Messaging::XPublisherXSubscriber;

class Subscriber::SubscriberImpl
{
public:
    SubscriberImpl(std::shared_ptr<UMPS::Messaging::Context> context,
                   std::shared_ptr<UMPS::Logging::ILog> logger)
    {
        mSubscriber = std::make_unique<UPubSub::Subscriber> (context, logger);
        std::unique_ptr<UMPS::MessageFormats::IMessage> pickMessageType
            = std::make_unique<Pick> (); 
        mMessageTypes.add(pickMessageType);
    }
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
    UPubSub::SubscriberOptions subscriberOptions;
    subscriberOptions.setAddress(options.getAddress());
    subscriberOptions.setZAPOptions(options.getZAPOptions());
    subscriberOptions.setReceiveTimeOut(options.getTimeOut());
    subscriberOptions.setReceiveHighWaterMark(options.getHighWaterMark());
    subscriberOptions.setMessageTypes(pImpl->mMessageTypes);
    pImpl->mSubscriber->initialize(subscriberOptions);
    pImpl->mOptions = options;
}

/// Initialized?
bool Subscriber::isInitialized() const noexcept
{
    return pImpl->mSubscriber->isInitialized();
}

/// Destructor
Subscriber::~Subscriber() = default;

/// Receive
std::unique_ptr<Pick> Subscriber::receive() const
{
    auto message = pImpl->mSubscriber->receive();
    if (message != nullptr)
    {
        try
        {
            return static_unique_pointer_cast<Pick> (std::move(message));
        }
        catch (const std::exception &e)
        {
            std::string errorMessage
                = "Error deserializing pick message.  Failed with "
                + std::string{e.what()};
            throw std::runtime_error(errorMessage);
        }
    }
    return nullptr;
}
