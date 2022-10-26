#include <urts/proxyBroadcasts/dataPacket/publisher.hpp>
#include <urts/proxyBroadcasts/dataPacket/publisherOptions.hpp>
#include <urts/proxyBroadcasts/dataPacket/subscriber.hpp>
#include <urts/proxyBroadcasts/dataPacket/subscriberOptions.hpp>
#include <umps/messaging/context.hpp>
#include <umps/logging/log.hpp>
#include <python/messaging.hpp>
#include "proxyBroadcasts.hpp"

using namespace URTS::Python::ProxyBroadcasts;
namespace UPB = URTS::ProxyBroadcasts;

DataPacket::Publisher::Publisher() :
    mPublisher(std::make_unique<UPB::DataPacket::Publisher> ())
{
}

DataPacket::Publisher::Publisher(
    UMPS::Python::Messaging::Context &context)
{
    auto umpsContext = context.getSharedPointer();
    mPublisher = std::make_unique<UPB::DataPacket::Publisher> (umpsContext);
}

DataPacket::Publisher::~Publisher() = default;
