#ifndef URTS_SERVICES_SCALABLE_DETECTORS_UNET_ONE_COMPONENT_P_REQUESTOR_HPP
#define URTS_SERVICES_SCALABLE_DETECTORS_UNET_ONE_COMPONENT_P_REQUESTOR_HPP
#include <memory>
// Forward declarations
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
namespace URTS::Services::Scalable::Detectors::UNetOneComponentP
{
 class RequestorOptions;
 class InferenceRequest;
 class InferenceResponse;
 class PreprocessingRequest;
 class PreprocessingResponse;
 class ProcessingRequest;
 class ProcessingResponse;
}
namespace URTS::Services::Scalable::Detectors::UNetOneComponentP
{
/// @class Requestor "requestor.hpp" "urts/services/scalable/detectors/uNetOneComponentP/requestor.hpp"
/// @brief A requestor that will interact with the inference service.
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

    /// @brief Performs a blocking processing (preprocessing followed by 
    ///        inference) request.
    /// @param[in] request  The processing request to make to the
    ///                     the server via the router.
    /// @result The response to the processing request.
    /// @throws std::invalid_argument if there are no data requests in the
    ///         bulk request.
    /// @throws std::runtime_error if \c isInitialized() is false.
    /// @note This is faster than performing a preprocessing then inference
    ///       request.
    [[nodiscard]] std::unique_ptr<ProcessingResponse> request(const ProcessingRequest &request);
    /// @brief Performs a blocking inference request.
    /// @param[in] request  The inference request to make to the server via
    ///                     the router. 
    /// @result The response to the inference request from the server.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] std::unique_ptr<InferenceResponse> request(const InferenceRequest &request);
    /// @brief Performs a blocking data preprocessing request.
    /// @param[in] request  The preprocessing request to make to the server via
    ///                     the router.
    /// @result The response to the preprocessing request from the server.
    ///         This should contain data that can be used to form an
    ///         inference request.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] std::unique_ptr<PreprocessingResponse> request(const PreprocessingRequest &request);
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
