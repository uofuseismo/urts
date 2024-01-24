#ifndef URTS_PYTHON_BROADCASTS_INTERNAL_PICK_HPP
#define URTS_PYTHON_BROADCASTS_INTERNAL_PICK_HPP
#include <memory>
#include <chrono>
#include <string>
#include <vector>
#include <umps/messageFormats/message.hpp>
#include <python/messageFormats.hpp>
#include <python/messaging.hpp>
#include <python/logging.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
namespace URTS::Broadcasts::Internal::Pick
{
 class Pick;
 class Publisher;
 class PublisherOptions;
 class Subscriber;
 class SubscriberOptions;
 class UncertaintyBound;
}
namespace URTS::Python::Broadcasts::Internal::Pick
{
/// @class UncertaintyBound
/// @brief Defines an uncerainty bound.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class UncertaintyBound 
{
public:
    UncertaintyBound();
    UncertaintyBound(const UncertaintyBound &bound);
    UncertaintyBound(UncertaintyBound &&bound) noexcept;
    UncertaintyBound(const URTS::Broadcasts::Internal::Pick::UncertaintyBound &bound);
    UncertaintyBound& operator=(const UncertaintyBound &bound);
    UncertaintyBound& operator=(const URTS::Broadcasts::Internal::Pick::UncertaintyBound &bound);
    UncertaintyBound& operator=(UncertaintyBound &&bound) noexcept;
    [[nodiscard]] const URTS::Broadcasts::Internal::Pick::UncertaintyBound& getNativeClassReference() const noexcept;
    void setPerturbation(double perturbation);
    [[nodiscard]] double getPerturbation() const;
    void setPercentile(double percentile);
    [[nodiscard]] double getPercentile() const;
    void clear() noexcept;
    ~UncertaintyBound();
private:
    std::unique_ptr<URTS::Broadcasts::Internal::Pick::UncertaintyBound> pImpl;
};

/// @class Pick
/// @brief Wraps a pick message.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Pick : public UMPS::Python::MessageFormats::IMessage
{
public:
    Pick();
    Pick(const Pick &pick);
    Pick(const URTS::Broadcasts::Internal::Pick::Pick &pick);
    Pick(Pick &&pick) noexcept;
    Pick& operator=(const Pick &pick);
    Pick& operator=(const URTS::Broadcasts::Internal::Pick::Pick &pick);
    Pick& operator=(Pick &&pick) noexcept;
    [[nodiscard]] const URTS::Broadcasts::Internal::Pick::Pick& getNativeClassReference() const noexcept;
    [[nodiscard]] URTS::Broadcasts::Internal::Pick::Pick getNativeClass() const noexcept;
    [[nodiscard]] std::unique_ptr<UMPS::Python::MessageFormats::IMessage> createInstance() const override;
    [[nodiscard]] std::unique_ptr<UMPS::Python::MessageFormats::IMessage> clone(const std::unique_ptr<UMPS::MessageFormats::IMessage> &message) const override;
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> getInstanceOfBaseClass() const noexcept override;
    void fromBaseClass(UMPS::MessageFormats::IMessage &message) override;
    [[nodiscard]] std::string getMessageType() const noexcept override;
    void setTime(double time) noexcept;
    [[nodiscard]] double getTime() const;
    void setNetwork(const std::string &network);
    [[nodiscard]] std::string getNetwork() const;
    void setStation(const std::string &station);
    [[nodiscard]] std::string getStation() const;
    void setChannel(const std::string &channel);
    [[nodiscard]] std::string getChannel() const;
    void setLocationCode(const std::string &location);
    [[nodiscard]] std::string getLocationCode() const;
    void setIdentifier(uint64_t identifier) const noexcept;
    [[nodiscard]] uint64_t getIdentifier() const;
    void setProcessingAlgorithms(std::vector<std::string> &algorithms);
    [[nodiscard]] std::vector<std::string> getProcessingAlgorithms() const noexcept;
    void setLowerAndUpperUncertaintyBounds(const std::pair<UncertaintyBound, UncertaintyBound> &bounds);
    [[nodiscard]] std::pair<UncertaintyBound, UncertaintyBound> getLowerAndUpperUncertaintyBounds() const;
/*
    void setSamplingRate(const double samplingRate);
    [[nodiscard]] double getSamplingRate() const;
    void setStartTime(const std::chrono::microseconds &time) noexcept;
    [[nodiscard]] std::chrono::microseconds getStartTime() const noexcept;
    [[nodiscard]] int getNumberOfSamples() const noexcept;
    void setData(const pybind11::array_t<double, pybind11::array::c_style | pybind11::array::forcecast> &x); 
    [[nodiscard]] std::vector<double> getData() const noexcept;
*/
    void clear() noexcept;
    ~Pick() override;
private:
    std::unique_ptr<URTS::Broadcasts::Internal::Pick::Pick> pImpl{nullptr};
};
/// @brief Wraps a datapacket publisher.
class Publisher
{
public:
    Publisher();
    explicit Publisher(UMPS::Python::Messaging::Context &context);
    ~Publisher();
//private:
    std::unique_ptr<URTS::Broadcasts::Internal::Pick::Publisher> mPublisher{nullptr};
};
class SubscriberOptions
{
public:
    SubscriberOptions();
    SubscriberOptions(const SubscriberOptions &options);
    SubscriberOptions(const URTS::Broadcasts::Internal::Pick::SubscriberOptions &);
    SubscriberOptions(SubscriberOptions &&options) noexcept;
    SubscriberOptions& operator=(const SubscriberOptions &);
    SubscriberOptions& operator=(SubscriberOptions &&) noexcept;
    SubscriberOptions& operator=(const URTS::Broadcasts::Internal::Pick::SubscriberOptions &);
    void setAddress(const std::string &address);
    [[nodiscard]] std::string getAddress() const;
    const URTS::Broadcasts::Internal::Pick::SubscriberOptions& getNativeClassReference() const noexcept;
    void setZAPOptions(const UMPS::Python::Authentication::ZAPOptions &options);
    [[nodiscard]] UMPS::Python::Authentication::ZAPOptions getZAPOptions() const;
    void setTimeOut(const std::chrono::milliseconds &timeOut);
    [[nodiscard]] std::chrono::milliseconds getTimeOut() const;
    void setHighWaterMark(int hwm);
    [[nodiscard]] int getHighWaterMark() const;
    void clear() noexcept;
    ~SubscriberOptions();
private:
    std::unique_ptr<URTS::Broadcasts::Internal::Pick::SubscriberOptions> pImpl{nullptr};
};
/// @brief Wraps a datapacket subscriber.
class Subscriber
{
public:
    Subscriber();
    explicit Subscriber(UMPS::Python::Messaging::Context &context);
    explicit Subscriber(UMPS::Python::Logging::ILog &logger);
    Subscriber(UMPS::Python::Messaging::Context &context,
               UMPS::Python::Logging::ILog &logger);
    void initialize(const SubscriberOptions &options);
    [[nodiscard]] bool isInitialized() const noexcept;
    [[nodiscard]] std::unique_ptr<Pick> receive() const;
    ~Subscriber();
private:
    std::unique_ptr<URTS::Broadcasts::Internal::Pick::Subscriber> mSubscriber{nullptr};
};
/// Initialize
void initialize(pybind11::module &m);
}
#endif
