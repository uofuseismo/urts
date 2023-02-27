#ifndef URTS_SERVICES_SCALABLE_DETECTORS_UNET_THREE_COMPONENT_P_PROCESSING_RESPONSE_HPP
#define URTS_SERVICES_SCALABLE_DETECTORS_UNET_THREE_COMPONENT_P_PROCESSING_RESPONSE_HPP
#include <memory>
#include <vector>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::Pickers::CNNOneComponentP
{
/// @class ProcessingResponse "processingResponse.hpp" "urts/services/scalable/pickers/cnnOneComponentP/processingResponse.hpp"
/// @brief Given a vertical waveform with the initial pick centered in the
///        middle of the waveform this is the correction to add to that pick
///        in seconds.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class ProcessingResponse : public UMPS::MessageFormats::IMessage
{
public:
    /// @brief Defines the service's return code.
    enum ReturnCode
    {
        Success = 0,                 /*!< Inference was successfully performed
                                          on the signals. */
        InvalidMessage = 1,          /*!< The request message was invalid. */
        PreprocessingFailure = 2,    /*!< The preprocessing failed. */
        ProcessedSignalTooSmall = 3, /*!< After preprocessing the resulting
                                          signal is too small on which to
                                          perform inference. */
        InvalidProcessedSignalLength = 4, /*!< The processed signal has an
                                               invalid length  .*/
        InferenceFailure = 5         /*!< The inference algorithm failed.  */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    ProcessingResponse();
    /// @brief Copy constructor.
    /// @param[in] response  The response from which to initialize this class.
    ProcessingResponse(const ProcessingResponse &response);
    /// @brief Move constructor.
    /// @param[in,out] response  The response from which to initialize this class.
    ///                         On exit, response's behavior is undefined.
    ProcessingResponse(ProcessingResponse &&response) noexcept;
    /// @}

    /// @name Pick Correction
    /// @{

    /// @brief Sets the correction to add to the P pick.
    /// @param[in] correction  The correction, in seconds, to add to the 
    ///                        original P pick.
    void setCorrection(double correction) noexcept;
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

    /// @name Sampling Rate
    /// @{

    /// @brief Sets the sampling rate of the processed signals.
    /// @param[in] samplingRate  The sampling rate of the signal in Hz.
    /// @throws std::invalid_argument if the sampling rate is not positive.
    void setSamplingRate(double samplingRate);
    /// @result The sampling rate of the processed signals.
    /// @note By default this is 100 Hz.
    [[nodiscard]] double getSamplingRate() const noexcept;
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
    ProcessingResponse& operator=(const ProcessingResponse &response);
    /// @result The memory moved from the response to this.
    ProcessingResponse& operator=(ProcessingResponse &&response) noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~ProcessingResponse() override;
    /// @} 
private:
    class ResponseImpl;
    std::unique_ptr<ResponseImpl> pImpl;
};
}
#endif
