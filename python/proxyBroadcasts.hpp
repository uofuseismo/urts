#ifndef URTS_PYTHON_PROXY_BROADCASTS_HPP
#define URTS_PYTHON_PROXY_BROADCASTS_HPP
#include <memory>
#include <python/messaging.hpp>
#include <python/logging.hpp>
#include <pybind11/pybind11.h>
namespace URTS::ProxyBroadcasts
{
 namespace DataPacket
 {
  class Publisher;
  class Subscriber;
 } 
}
namespace URTS::Python::ProxyBroadcasts
{
namespace DataPacket
{
    class Publisher
    {
    public:
        Publisher();
        explicit Publisher(UMPS::Python::Messaging::Context &context);
        ~Publisher();
    //private:
        std::unique_ptr<URTS::ProxyBroadcasts::DataPacket::Publisher> mPublisher{nullptr};
    };

    class Subscriber
    {
    public:
        Subscriber();
        explicit Subscriber(UMPS::Python::Messaging::Context &context);
        ~Subscriber();
    private:
        std::unique_ptr<URTS::ProxyBroadcasts::DataPacket::Subscriber> mSubscriber{nullptr};
    };
}
}
#endif
