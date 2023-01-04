#ifndef URTS_SERVICES_SCALABLE_PACKET_CACHE_SERVICE_HPP
#define URTS_SERVICES_SCALABLE_PACKET_CACHE_SERVICE_HPP
#include <memory>
namespace UMPS::Logging
{
 class ILog;
}
namespace URTS::Services::Scalable::PacketCache
{
 class ServiceOptions;
}
namespace URTS::Services::Scalable::PacketCache
{
/// @class ServiceOptions "serviceOptions.hpp" "urts/services/scalable/packetCache/serviceOptions.hpp"
/// @brief The options that define the backend packet cache service.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Service
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Service();
    /// @brief Constructor with a given logger.
    Service(std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @}

    /// @name Step 1: Initialization
    /// @{

    /// @result True indicates the class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;
    /// @}

    /// @name Step 2: Start 
    /// @{

    /// @brief Starts the service.
    /// @throws std::runtime_error if \c isInitialized() is false.
    void start();
    /// @result True indicates the service is running.
    [[nodiscard]] bool isRunning() const noexcept;
    /// @result The total number of packets in the packet cache.
    [[nodiscard]] int getTotalNumberOfPackets() const noexcept;
    /// @}

    /// @name Step 3: Stop
    /// @{
 
    /// @brief Stops the service.
    void stop();
    /// @} 

    /// @name Destructors
    /// @{

    /// @brief Destructor.
    ~Service();
    /// @}

    Service(const Service &) = delete;
    Service(Service &&) noexcept = delete;
    Service& operator=(const Service &) = delete;
    Service& operator=(Service &&) noexcept = delete;
private:
    class ServiceImpl;
    std::unique_ptr<ServiceImpl> pImpl;
};
}
#endif
