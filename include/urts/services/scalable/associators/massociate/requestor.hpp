#ifndef URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_REQUESTOR_HPP
#define URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_REQUESTOR_HPP
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
namespace URTS::Services::Scalable::Associators::MAssociate
{
 class RequestorOptions;
 class AssociationRequest;
 class AssociationResponse;
}
namespace URTS::Services::Scalable::Associators::MAssociate
{
/// @class Requestor "requestor.hpp" "urts/services/scalable/associators/massociate/requestor.hpp"
/// @brief A requestor that will submit jobs for association.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class Requestor
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    Requestor();
    /// @brief Constructor with a given logger.
    /// @param[in] logger  The logger.
    explicit Requestor(std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @brief Constructor with a given context.
    /// @param[in] context  The ZeroMQ context to use.
    explicit Requestor(std::shared_ptr<UMPS::Messaging::Context> &context);
    /// @brief Constructor with a given context and logger.
    Requestor(std::shared_ptr<UMPS::Messaging::Context> &context,
              std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @brief Move constructor.
    /// @param[in,out] requestor  The request class from which to initialize
    ///                           this class.  On exit, request's behavior is
    ///                           undefined.
    Requestor(Requestor &&requestor) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Move assignment operator.
    /// @param[in,out] requestor  The request class whose memory will be moved
    ///                           to this.  On exit, request's behavior is
    ///                           undefined.
    Requestor& operator=(Requestor &&requestor) noexcept;
    /// @result The memory from request moved to this.
    /// @} 

    /// @name Step 1: Initialization
    /// @{

    /// @brief Initializes the request.
    /// @param[in] options   The request options.
    /// @throws std::invalid_argument if the endpoint.
    void initialize(const RequestorOptions &options);
    /// @result True indicates the class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;
    /// @}

    /// @name Step 2: Request 
    /// @{

    /// @brief Performs a blocking association request. 
    /// @param[in] request  The assication request to make to the server via
    ///                     the router. 
    /// @result The response to the association request from the server.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] std::unique_ptr<AssociationResponse> request(const AssociationRequest &request);
    /// @}

    /// @name Step 3: Disconnecting
    /// @{

    /// @brief Disconnects the requestor from the router-dealer.
    /// @note This step is optional as it will be done by the destructor.
    void disconnect();
    /// @}
 
    /// @name Destructors
    /// @{

    /// @brief Destructor.
    ~Requestor();
    /// @}

    Requestor(const Requestor &request) = delete;
    Requestor& operator=(const Requestor &request) = delete;
private:
    class RequestorImpl;
    std::unique_ptr<RequestorImpl> pImpl;
};
}
#endif
