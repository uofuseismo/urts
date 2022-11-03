#ifndef URTS_PYTHON_PROXY_BROADCASTS_HPP
#define URTS_PYTHON_PROXY_BROADCASTS_HPP
#include <memory>
#include <vector>
#include <umps/messageFormats/message.hpp>
#include <python/messageFormats.hpp>
#include <python/messaging.hpp>
#include <python/logging.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
namespace URTS::ProxyBroadcasts
{
 namespace DataPacket
 {
  class DataPacket;
  class Publisher;
  class Subscriber;
 } 
}
namespace URTS::Python::ProxyBroadcasts
{
namespace DataPacket
{
    /// @brief Wraps a data packet message.
    class DataPacket : public UMPS::Python::MessageFormats::IMessage
    {
    public:
        DataPacket();
        DataPacket(const DataPacket &packet);
        DataPacket(const URTS::ProxyBroadcasts::DataPacket::DataPacket &packet);
        DataPacket(DataPacket &&packet) noexcept;
        DataPacket& operator=(const DataPacket &packet);
        DataPacket& operator=(const URTS::ProxyBroadcasts::DataPacket::DataPacket &packet);
        DataPacket& operator=(DataPacket &&packet) noexcept;
        [[nodiscard]] const URTS::ProxyBroadcasts::DataPacket::DataPacket& getNativeClassReference() const noexcept;
        [[nodiscard]] URTS::ProxyBroadcasts::DataPacket::DataPacket getNativeClass() const noexcept;
        [[nodiscard]] std::unique_ptr<UMPS::Python::MessageFormats::IMessage> createInstance() const override;
        [[nodiscard]] std::unique_ptr<UMPS::Python::MessageFormats::IMessage> clone(const std::unique_ptr<UMPS::MessageFormats::IMessage> &message) const override;
        [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> getInstanceOfBaseClass() const noexcept override;
        void fromBaseClass(UMPS::MessageFormats::IMessage &message) override;
        [[nodiscard]] std::string getMessageType() const noexcept override;
        void setNetwork(const std::string &network);
        [[nodiscard]] std::string getNetwork() const;
        void setStation(const std::string &station);
        [[nodiscard]] std::string getStation() const;
        void setChannel(const std::string &channel);
        [[nodiscard]] std::string getChannel() const;
        void setLocationCode(const std::string &location);
        [[nodiscard]] std::string getLocationCode() const;
        void setSamplingRate(const double samplingRate);
        [[nodiscard]] double getSamplingRate() const;
        void setStartTime(const std::chrono::microseconds &time) noexcept;
        [[nodiscard]] std::chrono::microseconds getStartTime() const noexcept;
        [[nodiscard]] int getNumberOfSamples() const noexcept;
        void setData(const pybind11::array_t<double, pybind11::array::c_style | pybind11::array::forcecast> &x); 
        [[nodiscard]] std::vector<double> getData() const noexcept;
        void clear() noexcept;
        ~DataPacket() override;
    private:
        std::unique_ptr<URTS::ProxyBroadcasts::DataPacket::DataPacket> pImpl{nullptr};
    };
    /// @brief Wraps a datapacket publisher.
    class Publisher
    {
    public:
        Publisher();
        explicit Publisher(UMPS::Python::Messaging::Context &context);
        ~Publisher();
    //private:
        std::unique_ptr<URTS::ProxyBroadcasts::DataPacket::Publisher> mPublisher{nullptr};
    };
    /// @brief Wraps a datapacket subscriber.
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
/// Initialize
void initialize(pybind11::module &m);
}
#endif
