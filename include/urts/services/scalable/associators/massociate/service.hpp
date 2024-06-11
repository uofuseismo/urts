#ifndef URTS_SERVICES_SCALABLE_SERVICE_ASSOCIATORS_MASSOCIATE_SERVICE_HPP
#define URTS_SERVICES_SCALABLE_SERVICE_ASSOCIATORS_MASSOCIATE_SERVICE_HPP
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
namespace ULocator
{
 class TravelTimeCalculatorMap;
}
namespace URTS::Database::Connection
{
 class IConnection;
}
namespace URTS::Services::Scalable::Associators::MAssociate
{
 class ServiceOptions;
}
namespace URTS::Services::Scalable::Associators::MAssociate
{
/// @class Service "service.hpp" "urts/services/scalable/associators/massociate/service.hpp"
/// @brief Runs the MAssociate pick association service.
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
    /// @brief Constructor with a given context for the replier and a logger.
    Service(std::shared_ptr<UMPS::Messaging::Context> &context,
            std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @}

    /// @name Step 1: Initialization
    /// @{

    /// @brief Initializes the class.
    /// @param[in] options  The options defining the replier and associator
    ///                     options.
    /// @throws std::invalid_argument if the replier information is not set
    ///         or the associator options are invalid.
    /// @throws std::runtime_error if there is an error connecting or
    ///         initializing associator.
    void initialize(const ServiceOptions &options);
    /// @result True indicates the class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;

    void setTravelTimeCalculatorMap(std::unique_ptr<ULocator::TravelTimeCalculatorMap> &&map);
    /// @}

    /// @name Step 2: Start 
    /// @{

    /// @brief Starts the service.
    /// @throws std::runtime_error if \c isInitialized() is false.
    void start();
    /// @result True indicates the service is running.
    [[nodiscard]] bool isRunning() const noexcept;
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
