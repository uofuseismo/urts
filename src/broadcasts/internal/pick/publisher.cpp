#include <string>
#include <chrono>
#include <thread>
#ifndef NDEBUG
#include <cassert>
#endif
#include <umps/messaging/xPublisherXSubscriber/publisherOptions.hpp>
#include <umps/messaging/xPublisherXSubscriber/publisher.hpp>
#include <umps/logging/log.hpp>
#include "urts/broadcasts/internal/pick/publisher.hpp"
#include "urts/broadcasts/internal/pick/publisherOptions.hpp"
#include "urts/broadcasts/internal/pick/pick.hpp"

using namespace URTS::Broadcasts::Internal::Pick;
namespace UXPubXSub = UMPS::Messaging::XPublisherXSubscriber;

class Publisher::PublisherImpl
{
public:
    PublisherImpl(std::shared_ptr<UMPS::Messaging::Context> context,
                  std::shared_ptr<UMPS::Logging::ILog> logger) :
        mPublisher(std::make_unique<UXPubXSub::Publisher> (context, logger)),
        mLogger(logger)
    {
    }
    std::unique_ptr<UXPubXSub::Publisher> mPublisher{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    PublisherOptions mOptions;
};

/// C'tor
Publisher::Publisher() :
    pImpl(std::make_unique<PublisherImpl> (nullptr, nullptr))
{
}

/// C'tor
Publisher::Publisher(std::shared_ptr<UMPS::Messaging::Context> &context) :
    pImpl(std::make_unique<PublisherImpl> (context, nullptr)) 
{
}

/// C'tor
Publisher::Publisher(std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<PublisherImpl> (nullptr, logger))
{
}

/// C'tor
Publisher::Publisher(std::shared_ptr<UMPS::Messaging::Context> &context,
                     std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<PublisherImpl> (context, logger)) 
{
}

/// Move c'tor
Publisher::Publisher(Publisher &&publisher) noexcept
{
    *this = std::move(publisher);
}

/// Move assignment
Publisher& Publisher::operator=(Publisher &&publisher) noexcept
{
    if (&publisher == this){return *this;}
    pImpl = std::move(publisher.pImpl);
    return *this;
}

/// Initialize
void Publisher::initialize(const PublisherOptions &options)
{
    if (!options.haveAddress()){throw std::runtime_error("Address not set");}
    auto publisherOptions = options.getPublisherOptions();
    pImpl->mPublisher->initialize(publisherOptions);
#ifndef NDEBUG
    assert(pImpl->mPublisher->isInitialized());
#endif
    // Slow joiner problem
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pImpl->mOptions = options;
}

/// Initialized?
bool Publisher::isInitialized() const noexcept
{
    return pImpl->mPublisher->isInitialized();
}

/// Destructor
Publisher::~Publisher() = default;

/// Send
void Publisher::send(const Pick &message)
{
    if (pImpl->mLogger)
    {
        if (pImpl->mLogger->getLevel() == UMPS::Logging::Level::Debug)
        {
            try
            {
                auto pickDetails = message.getNetwork() + "."
                                 + message.getStation() + "."
                                 + message.getChannel() + "."
                                 + message.getLocationCode() + "."
                                 + message.getPhaseHint() + " "
                                 + std::to_string(
                                      message.getTime().count()*1.e-6);
                pImpl->mLogger->debug("Publishing " + pickDetails);
            }
            catch (...)
            {
            } 
        }
    }
    pImpl->mPublisher->send(message); 
}
