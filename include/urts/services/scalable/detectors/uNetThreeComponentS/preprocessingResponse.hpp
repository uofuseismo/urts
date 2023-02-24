#ifndef URTS_SERVICES_SCALABLE_DETECTORS_UNET_THREE_COMPONENT_S_PREPROCESSING_RESPONSE_HPP
#define URTS_SERVICES_SCALABLE_DETECTORS_UNET_THREE_COMPONENT_S_PREPROCESSING_RESPONSE_HPP
#include <memory>
#include <vector>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::Detectors::UNetThreeComponentS
{
/// @class PreprocessingResponse "preprocessingResponse.hpp" "urts/services/scalable/detectors/uNetThreeComponentS/preprocessingResponse.hpp"
/// @brief Response from a waveform snippet preprocessing request.
/// @note Typically you would use an inference request which can preprocess
///       and apply the model as that is more efficient.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class PreprocessingResponse : public UMPS::MessageFormats::IMessage
{
public:
    /// @brief Defines the service's return code.
    enum ReturnCode
    {
        Success = 0,          /*!< The signals were successfully preprocessed. */
        InvalidMessage = 1,   /*!< The request message was invalid. */
        AlgorithmFailure = 2  /*!< The preprocessing algorithm failed. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    PreprocessingResponse();
    /// @brief Copy constructor.
    /// @param[in] response  The response from which to initialize this class.
    PreprocessingResponse(const PreprocessingResponse &response);
    /// @brief Move constructor.
    /// @param[in,out] response  The response from which to initialize this class.
    ///                         On exit, response's behavior is undefined.
    PreprocessingResponse(PreprocessingResponse &&response) noexcept;
    /// @}

    /// @name Processed Signals
    /// @{

    /// @brief Sets the processed signals on the vertical, north, and east
    ///        channels.
    /// @param[in] verticalSignal  The signal on the vertical channel.
    /// @param[in] northSignal     The signal on the north (1) channel.
    /// @param[in] eastSignal      The signal on the east (2) channel.
    /// @throws std::invalid_argument the signals are not the same size
    ///         or empty.
    void setVerticalNorthEastSignal(const std::vector<double> &verticalSignal,
                                    const std::vector<double> &northSignal,
                                    const std::vector<double> &eastSignal);
    /// @brief Sets the processed signals on the vertical, north, and east
    ///        channels.
    /// @param[in,out] verticalSignal  The signal on the vertical channel.
    ///                                On exit, verticalSignal's behavior is
    ///                                undefined.
    /// @param[in,out] northSignal     The signal on the north (1) channel.
    ///                                On exit, northSignal's behavior is
    ///                                undefined.
    /// @param[in,out] eastSignal      The signal on the east (2) channel.
    ///                                On exit, eastSignal's behavior is
    ///                                undefined.
    /// @throws std::invalid_argument the signals are not the same size
    ///         or empty.
    void setVerticalNorthEastSignal(std::vector<double> &&verticalSignal,
                                    std::vector<double> &&northSignal,
                                    std::vector<double> &&eastSignal);
    /// @result The vertical signal.
    /// @throws std::runtime_error if \c haveSignals() is false.
    [[nodiscard]] std::vector<double> getVerticalSignal() const;
    /// @result The north signal.
    /// @throws std::runtime_error if \c haveSignals() is false.
    [[nodiscard]] std::vector<double> getNorthSignal() const;
    /// @result The east signal.
    /// @throws std::runtime_error if \c haveSignals() is false.
    [[nodiscard]] std::vector<double> getEastSignal() const;
    /// @result True indicates the signals were set.
    [[nodiscard]] bool haveSignals() const noexcept;
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
    /// @result Uniquely defines this message type.
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
    PreprocessingResponse& operator=(const PreprocessingResponse &response);
    /// @result The memory moved from the response to this.
    PreprocessingResponse& operator=(PreprocessingResponse &&response) noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~PreprocessingResponse() override;
    /// @} 
private:
    class ResponseImpl;
    std::unique_ptr<ResponseImpl> pImpl;
};
}
#endif
