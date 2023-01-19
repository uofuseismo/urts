#ifndef URTS_PYTHON_SERVICES_SCALABLE_PACKET_CACHE_HPP
#define URTS_PYTHON_SERVICES_SCALABLE_PACKET_CACHE_HPP
#include <memory>
#include <vector>
#include <umps/messageFormats/message.hpp>
#include <python/messageFormats.hpp>
#include <python/messaging.hpp>
#include <python/logging.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
namespace URTS::Services::Scalable::PacketCache
{
 class BulkDataRequest;
 class BulkDataResponse;
 class DataRequest;
 class DataResponse;
 class SensorRequest;
 class SensorResponse;
}
namespace URTS::Python::Services::Scalable::PacketCache
{
/// @class SensorRequest
/// @brief Wraps a sensor request message.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class SensorRequest : public UMPS::Python::MessageFormats::IMessage
{
public:
    SensorRequest();
    SensorRequest(const SensorRequest &request);
    SensorRequest(const URTS::Services::Scalable::PacketCache::SensorRequest &request);
    SensorRequest(SensorRequest &&request) noexcept;
    SensorRequest& operator=(const SensorRequest &request);
    SensorRequest& operator=(const URTS::Services::Scalable::PacketCache::SensorRequest &packet);
    SensorRequest& operator=(SensorRequest &&request) noexcept;
    [[nodiscard]] const URTS::Services::Scalable::PacketCache::SensorRequest& getNativeClassReference() const noexcept;
    [[nodiscard]] URTS::Services::Scalable::PacketCache::SensorRequest getNativeClass() const noexcept;
    [[nodiscard]] std::unique_ptr<UMPS::Python::MessageFormats::IMessage> createInstance() const override;
    [[nodiscard]] std::unique_ptr<UMPS::Python::MessageFormats::IMessage> clone(const std::unique_ptr<UMPS::MessageFormats::IMessage> &message) const override;
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> getInstanceOfBaseClass() const noexcept override;
    void fromBaseClass(UMPS::MessageFormats::IMessage &message) override;
    [[nodiscard]] std::string getMessageType() const noexcept override;
    void setIdentifier(int64_t identifier);
    [[nodiscard]] int64_t getIdentifier() const noexcept;
    void clear() noexcept;
    ~SensorRequest() override;
private:
    std::unique_ptr<URTS::Services::Scalable::PacketCache::SensorRequest> pImpl{nullptr};
};
/// Initialize
void initialize(pybind11::module &m);
}
#endif
