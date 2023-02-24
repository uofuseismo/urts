#ifndef URTS_SERVICES_SCALABLE_PICKERS_CNN_ONE_COMPONENT_P_INFERENCE_REQUEST_HPP
#define URTS_SERVICES_SCALABLE_PICKERS_CNN_ONE_COMPONENT_P_INFERENCE_REQUEST_HPP
#include <memory>
#include <vector>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::Pickers::CNNOneComponentP
{
/// @class InferenceRequest "inferenceRequest.hpp" "urts/services/scalable/pickers/cnnOneComponentP/inferenceRequest.hpp"
/// @brief Requests inference be performed on a preprocessed waveform snippet.
///        It is assumed the initial pick estimate is at the center of the
///        waveform.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class InferenceRequest : public UMPS::MessageFormats::IMessage
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    InferenceRequest();
    /// @brief Copy constructor.
    /// @param[in] request  The request from which to initialize this class.
    InferenceRequest(const InferenceRequest &request);
    /// @brief Move constructor.
    /// @param[in,out] request  The request from which to initialize this class.
    ///                         On exit, request's behavior is undefined.
    InferenceRequest(InferenceRequest &&request) noexcept;
    /// @}

    /// @name Signal to Process
    /// @{

    /// @brief Sets the signals on the vertical channel on which inference
    ///        will be performed.
    /// @param[in] verticalSignal   The signal on the vertical channel.
    /// @throws std::invalid_argument the signal length is not equal
    ///         to \c getExpectedSignalLength().
    void setVerticalSignal(const std::vector<double> &verticalSignal);
    /// @brief Sets the signals on the vertical channel on which inference
    ///        will be performed.
    /// @param[in,out] verticalSignal  The signal on the vertical channel.
    ///                                On exit, verticalSignal's behavior is
    ///                                undefined.
    /// @throws std::invalid_argument the signal length is not equal
    ///         to \c getExpectedSignalLength().
    void setVerticalSignal(std::vector<double> &&verticalSignal);
    /// @result The vertical signal.
    /// @throws std::runtime_error if \c haveSignals() is false.
    [[nodiscard]] std::vector<double> getVerticalSignal() const;

    /// @result A reference to the vertical signal.
    /// @throws std::runtime_error if \c haveSignal() is false.
    /// @note This exists for performance reasons.  You should use
    ///       \c getVerticalSignal(). 
    [[nodiscard]] const std::vector<double> &getVerticalSignalReference() const;

    /// @result True indicates the vertical signal was set.
    [[nodiscard]] bool haveSignal() const noexcept;
    /// @result The expected signal length. 
    [[nodiscard]] static int getExpectedSignalLength() noexcept;
    /// @result The sampling rate of the input signals in Hz.
    [[nodiscard]] static double getSamplingRate() noexcept;
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

    /// @result A deep copy of the request.
    InferenceRequest& operator=(const InferenceRequest &request);
    /// @result The memory moved from the request to this.
    InferenceRequest& operator=(InferenceRequest &&request) noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~InferenceRequest() override;
    /// @} 
private:
    class RequestImpl;
    std::unique_ptr<RequestImpl> pImpl;
};
}
#endif
