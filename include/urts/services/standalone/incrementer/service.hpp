#ifndef URTS_SERVICES_STANDALONE_INCREMENTER_SERVICE_HPP
#define URTS_SERVICES_STANDALONE_INCREMENTER_SERVICE_HPP
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
namespace URTS::Services::Standalone::Incrementer
{
 class ServiceOptions;
}
namespace URTS::Services::Standalone::Incrementer
{
/// @class Service service.hpp "urts/services/standalone/incrementer/service.hpp"
/// @brief Implements the incrementer service.
/// @copyright Ben Baker (Univeristy of Utah) distributed under the MIT license.
class Service
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Service();
    /// @brief Constructor with a given logger.
    explicit Service(std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @brief Constructor with a given context and a stdout logger.
    explicit Service(std::shared_ptr<UMPS::Messaging::Context> &context);
    /// @brief Constructor with a given logger and context.
    Service(std::shared_ptr<UMPS::Messaging::Context> &context,
            std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @}
     
    /// @brief Initializes the service.
    void initialize(const ServiceOptions &options);
    /// @result True indicates that the service is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;

    /// @brief Starts the service.
    void start();

    /// @result True indicates that the service is running.
    [[nodiscard]] bool isRunning() const noexcept;

    /// @brief Stops the service.
    void stop();

    /// @name Destructors
    /// @{

    /// @brief Destructor.
    ~Service();
    /// @}
 
    Service(const Service &service) = delete;
    Service& operator=(const Service &service) = delete;
    Service(Service &&service) noexcept = delete;
    Service& operator=(Service &&service) noexcept = delete;
private:
    class ServiceImpl;
    std::unique_ptr<ServiceImpl> pImpl;
};
}
#endif
