#ifndef URTS_SERVICES_SCALABLE_PICKERS_CNN_ONE_COMPONENT_P_INFERENCE_RESPONSE_HPP
#define URTS_SERVICES_SCALABLE_PICKERS_CNN_ONE_COMPONENT_P_INFERENCE_RESPONSE_HPP
#include <memory>
#include <vector>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::Pickers::CNNOneComponentP
{
/// @class InferenceResponse "inferenceResponse.hpp" "urts/services/scalable/pickers/cnnOneComponentP/inferenceResponse.hpp"
/// @brief The correction to add to the P pick.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class InferenceResponse : public UMPS::MessageFormats::IMessage
{
public:
    /// @brief Defines the service's return code.
    enum ReturnCode
    {
        Success = 0,           /*!< Inference was successfully performed on the signals. */
        InvalidMessage = 1,    /*!< The request message was invalid. */
        AlgorithmFailure = 2   /*!< The inference algorithm failed. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    InferenceResponse();
    /// @brief Copy constructor.
    /// @param[in] response  The response from which to initialize this class.
    InferenceResponse(const InferenceResponse &response);
    /// @brief Move constructor.
    /// @param[in,out] response  The response from which to initialize this class.
    ///                         On exit, response's behavior is undefined.
    InferenceResponse(InferenceResponse &&response) noexcept;
    /// @}

    /// @name Pick Correction
    /// @{

    /// @brief Sets the correction to add to the P pick.
    /// @param[in] correction  The correction, in seconds, to add to the 
    ///                        original P pick.
    void setCorrection(double correction);
    /// @result The correction to add to the P pick.
    /// @throws std::runtime_error if \c haveCorrection() is false.
    [[nodiscard]] double getCorrection() const;
    /// @result True indicates that the P correction was set.
    [[nodiscard]] bool haveCorrection() const noexcept;
    /// @}

    /// @name Return Code
    /// @{

    /// @brief Sets the return code.
    /// @param[in] returnCode  The return code.
    void setReturnCode(ReturnCode returnCode) noexcept;
    /// @result The return code.
    /// @throws std::runtime_error if \c haveReturnCode() is false.
    [[nodiscard]] ReturnCode getReturnCode() const;
    /// @result True indicates the return code was set.
    [[nodiscard]] bool haveReturnCode() const noexcept;
    /// @}

    /// @name Response Identifier
    /// @{

    /// @brief Sets a response identifier.
    /// @param[in] identifier  The response identifier.  The service will return
    ///                        this number in a successful response. 
    void setIdentifier(int64_t identifier) noexcept;
    /// @result The response identifier.
    [[nodiscard]] int64_t getIdentifier() const noexcept;
    /// @}

    /// @name Message Properties
    /// @{

    /// @brief Converts the packet class to a string message.
    /// @result The class expressed as a string message.
    /// @throws std::runtime_error if the required information is not set. 
    /// @note Though the container is a string the message need not be
    ///       human readable.
    [[nodiscard]] std::string toMessage() const final;
    /// @brief Creates the class from a message.
    /// @param[in] message  The message from which to create this class.
    /// @throws std::invalid_argument if message.empty() is true.
    /// @throws std::runtime_error if the message is invalid.
    void fromMessage(const std::string &message) final;
    /// @brief Creates the class from a message.
    /// @param[in] data    The contents of the message.  This is an
    ///                    array whose dimension is [length] 
    /// @param[in] length  The length of data.
    /// @throws std::runtime_error if the message is invalid.
    /// @throws std::invalid_argument if data is NULL or length is 0. 
    void fromMessage(const char *data, size_t length) final;
    /// @result A message type indicating this is a pick message.
    [[nodiscard]] std::string getMessageType() const noexcept final;
    /// @result The message version.
    [[nodiscard]] std::string getMessageVersion() const noexcept final;
    /// @result A copy of this class.
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> clone() const final;
    /// @result An uninitialized instance of this class.
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> createInstance() const noexcept final;
    /// @}

    /// @name Operators
    /// @{

    /// @result A deep copy of the response.
    InferenceResponse& operator=(const InferenceResponse &response);
    /// @result The memory moved from the response to this.
    InferenceResponse& operator=(InferenceResponse &&response) noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~InferenceResponse() override;
    /// @} 
private:
    class ResponseImpl;
    std::unique_ptr<ResponseImpl> pImpl;
};
}
#endif
