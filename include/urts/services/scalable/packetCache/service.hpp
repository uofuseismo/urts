#ifndef URTS_SERVICES_SCALABLE_PACKET_CACHE_SERVICE_HPP
#define URTS_SERVICES_SCALABLE_PACKET_CACHE_SERVICE_HPP
#include <memory>
namespace UMPS
{
 namespace Logging
 {
  class ILog;
 }
 namespace Messaging
 {
  class Context;
 }
}
namespace URTS::Services::Scalable::PacketCache
{
 class ServiceOptions;
}
namespace URTS::Services::Scalable::PacketCache
{
/// @class Service "service.hpp" "urts/services/scalable/packetCache/service.hpp"
/// @brief This class reads data packets from the publisher broadcast, stores
///        those packets in a capped collection, and replies to data requests.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Service
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Service();
    /// @brief Constructor with a given logger.
    explicit Service(std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @brief Constructor with a given context for the replier, the broadcast,
    ///        and a logger
    Service(std::shared_ptr<UMPS::Messaging::Context> &responseContext,
            std::shared_ptr<UMPS::Messaging::Context> &broadcastContext,
            std::shared_ptr<UMPS::Logging::ILog> &logger); 
    /// @}

    /// @name Step 1: Initialization
    /// @{

    /// @brief initializes the class.
    /// @param[in] options  The packet cache options.
    /// @throws std::invalid_argument if the replier and broadcast connection
    ///         information are not set.
    /// @throws std::runtime_error if there is an error connecting.
    void initialize(const ServiceOptions &options);
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
