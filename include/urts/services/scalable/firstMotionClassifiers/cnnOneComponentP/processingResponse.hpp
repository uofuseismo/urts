#ifndef URTS_SERVICES_SCALABLE_FIRST_MOTION_CLASSIFIERS_CNN_ONE_COMPONENT_P_PROCESSING_RESPONSE_HPP
#define URTS_SERVICES_SCALABLE_FIRST_MOTION_CLASSIFIERS_CNN_ONE_COMPONENT_P_PROCESSING_RESPONSE_HPP
#include <memory>
#include <vector>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::FirstMotionClassifiers::CNNOneComponentP
{
/// @class ProcessingResponse "processingResponse.hpp" "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/processingResponse.hpp"
/// @brief Given a vertical waveform with the P pick centered in the middle
///        of the waveform this will infer the corresponding first motion.
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
    /// @brief Defines the first motion as being up, down, or unknown.
    enum FirstMotion : int 
    {   
        Up =+1,      /*!< Indicates that the first motion is up. */
        Down =-1,    /*!< Indicates that the first motion is down. */
        Unknown = 0  /*!< Indicates taht the first motion is unknown. */
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

    /// @name Probability
    /// @{

    /// @brief Sets the probability of the first motion being up, down,
    ///        and unknown.
    /// @param[in] probability  The probability of the first motion being up,
    ///                         down, and unkonwn, respectively.
    /// @throws std::invalid_argument if the probabilities do not sum to unity.
    void setProbabilities(const std::tuple<double, double, double> &probability);
    /// @result The probability that the first motion of the P pick is up, down,
    ///         and unkonwn.
    /// @throws std::runtime_error if \c haveProbabilities() is false.
    [[nodiscard]] std::tuple<double, double, double> getProbabilities() const;
    /// @result True indicates that the probabilty was set.
    [[nodiscard]] bool haveProbabilities() const noexcept;
    /// @}

    /// @name First Motion
    /// @{
  
    /// @brief Sets the predicted class as up, down, or unknown.
    /// @param[in] firstMotion  The first motion corresponding to this pick.
    void setFirstMotion(const FirstMotion firstMotion) noexcept;
    /// @result Defines the P pick as having a first motion of up, down,
    ///         or unknown.
    /// @throws std::runtime_error if \c haveFirstMotion() is false.
    [[nodiscard]] FirstMotion getFirstMotion() const;
    /// @result True indicates the firstMotion was set.
    [[nodiscard]] bool haveFirstMotion() const noexcept;
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
    /// @result Defines this message type.
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
