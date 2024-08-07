#ifndef URTS_SERVICES_SCALABLE_DETECTORS_UNET_ONE_COMPONENT_P_PREPROCESSING_REQUEST_HPP
#define URTS_SERVICES_SCALABLE_DETECTORS_UNET_ONE_COMPONENT_P_PREPROCESSING_REQUEST_HPP
#include <memory>
#include <vector>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::Detectors::UNetOneComponentP
{
/// @class PreprocessingRequest "preprocessingRequest.hpp" "urts/services/scalable/detectors/uNetOneComponentP/preprocessingRequest.hpp"
/// @brief Requests a waveform snippet be pre-processed.
/// @note Typically you would use a service request that will pre-process and
///       apply the model as that is more efficient.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class PreprocessingRequest : public UMPS::MessageFormats::IMessage
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    PreprocessingRequest();
    /// @brief Copy constructor.
    /// @param[in] request  The request from which to initialize this class.
    PreprocessingRequest(const PreprocessingRequest &request);
    /// @brief Move constructor.
    /// @param[in,out] request  The request from which to initialize this class.
    ///                         On exit, request's behavior is undefined.
    PreprocessingRequest(PreprocessingRequest &&request) noexcept;
    /// @}

    /// @name Signals to Process
    /// @{

    /// @brief Sets the signal on the vertical channel that will be processed.
    /// @param[in] verticalSignal  The signal on the vertical channel.
    /// @throws std::invalid_argument if the signal is empty.
    void setSignal(const std::vector<double> &verticalSignal);
    /// @brief Sets the signal on the vertical channel that will be processed.
    /// @param[in,out] verticalSignal  The signal on the vertical channel.
    ///                                On exit, verticalSignal's behavior is
    ///                                undefined.
    /// @throws std::invalid_argument the signals are not the same size
    ///         or empty.
    void setSignal(std::vector<double> &&verticalSignal);
    /// @result The vertical signal.
    /// @throws std::runtime_error if \c haveSignal() is false.
    [[nodiscard]] std::vector<double> getSignal() const;

    /// @result A reference to the vertical signal.
    /// @throws std::runtime_error if \c haveSignal() is false.
    /// @note This exists for performance reasons.  You should use
    ///       \c getSignal(). 
    [[nodiscard]] const std::vector<double> &getSignalReference() const;

    /// @result True indicates the signals were set.
    [[nodiscard]] bool haveSignal() const noexcept;
    /// @}


    /// @name Sampling Rate
    /// @{

    /// @brief Sets the sampling rate of the signals to be processed.
    /// @param[in] samplingRate  The sampling rate of the signals in Hz.
    /// @throws std::invalid_argument if the sampling rate is not positive.
    void setSamplingRate(double samplingRate);
    /// @result The sampling rate of the signal.
    /// @note By default this is 100 Hz.
    [[nodiscard]] double getSamplingRate() const noexcept;
    /// @}

    /// @name Request Identifier
    /// @{

    /// @brief Sets a request identifier.
    /// @param[in] identifier  The request identifier.  The service will return
    ///                        this number in a successful response. 
    void setIdentifier(int64_t identifier) noexcept;
    /// @note
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

    /// @result A deep copy of the request.
    PreprocessingRequest& operator=(const PreprocessingRequest &request);
    /// @result The memory moved from the request to this.
    PreprocessingRequest& operator=(PreprocessingRequest &&request) noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~PreprocessingRequest() override;
    /// @} 
private:
    class RequestImpl;
    std::unique_ptr<RequestImpl> pImpl;
};
}
#endif
