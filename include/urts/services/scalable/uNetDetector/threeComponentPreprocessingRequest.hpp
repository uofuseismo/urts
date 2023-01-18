#ifndef URTS_SERVICES_SCALABLE_UNET_DETECTOR_THREE_COMPONENT_PREPROCESSING_REQUEST_HPP
#define URTS_SERVICES_SCALABLE_UNET_DETECTOR_THREE_COMPONENT_PREPROCESSING_REQUEST_HPP
#include <memory>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::UNetDetector
{
/// @brief Requests a waveform snippet be pre-processed.
/// @note Typically you would use a service request that will pre-process and
///       apply the model as that is more efficient.
class ThreeComponentPreproccessingRequest : public UMPS::MessageFormats::IMessage
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    ThreeComponentPreprocessingRequest(); 
    /// @brief Copy constructor.
    /// @param[in] request  The request from which to initialize this class.
    ThreeComponentPreprocessingRequest(const ThreeComponentPreprocessingRequest &request);
    /// @brief Move constructor.
    /// @param[in,out] request  The request from which to initialize this class.
    ///                         On exit, request's behavior is undefined.
    ThreeComponentPreprocessingRequest(ThreeComponentPreprocessingRequest &&request) noexcept;
    /// @}

    /// @name Signals to Process 
    /// @{

    /// @brief Sets the signals on the vertical, north, and east channels that
    ///        will be pre-processed.
    /// @param[in] verticalSignal  The signal on the vertical channel.
    /// @param[in] northSignal     The signal on the north (1) channel.
    /// @param[in] eastSignal      The signal on the east (2) channel.
    /// @throws std::invalid_argument the signals are not the same size
    ///         or empty.
    void setVerticalNorthEastSignal(const std::vector<double> &verticalSignal,
                                    const std::vector<double> &northSignal,
                                    const std::vector<double> &eastSignal);
    /// @result True indicates the signals were set.
    [[nodisard]] bool haveSignals() const noexcept;
    /// @}


    /// @name Sampling Rate
    /// @{

    /// @brief Sets the sampling rate of the signal.
    /// @param[in] samplingRate  The sampling rate of the signal in Hz.
    /// @throws std::invalid_argument if the sampling rate is not positive.
    void setSamplingRate(double samplingRate);
    /// @result The sampling rate of the signal.
    /// @note By default this is 100 Hz.
    [[nodiscard]] getSamplingRate() const noexcept;
    /// @}

    /// @name Request Identifier
    /// @{

    /// @brief Sets a request identifier.
    /// @param[in] identifier  The request identifier.  The service will return
    ///                        this number in a successful response. 
    void setIdentifier(int64_t identifier);
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
    ThreeComponentPreprocessingRequest& operator=(const ThreeComponentPreprocessingRequest &request);
    /// @result The memory moved from the request to this.
    ThreeComponentPreprocessingRequest& operator=(ThreeComponentPreprocessingRequest &&request) noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief 
    void clear() noexcept;
    /// @brief Destructor.
    ~PreprocessingRequest();
    /// @} 
private:
    class PreproccessingRequestImpl;
    std::unique_ptr<PreproccessingRequestImpl> pImpl;
};
}
#endif
