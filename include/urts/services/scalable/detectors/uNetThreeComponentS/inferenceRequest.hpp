#ifndef URTS_SERVICES_SCALABLE_DETECTORS_UNET_THREE_COMPONENT_S_INFERENCE_REQUEST_HPP
#define URTS_SERVICES_SCALABLE_DETECTORS_UNET_THREE_COMPONENT_S_INFERENCE_REQUEST_HPP
#include <memory>
#include <vector>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::Detectors::UNetThreeComponentS
{
/// @class InferenceRequest "inferenceRequest.hpp" "urts/services/scalable/detectors/uNetThreeComponentS/inferenceRequest.hpp"
/// @brief Requests inference be performed on a preprocessed waveform snippet.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class InferenceRequest : public UMPS::MessageFormats::IMessage
{
public:
    /// @brief Defines the method used to apply the inference utility.
    ///        Inference can be performed using a sliding window for
    ///        signals exceeding some minimum length or can be applied
    ///        for a fixed-sized window.
    enum class InferenceStrategy
    {
        SlidingWindow = 0, /*!< By default a sliding window will be applied
                                to the input signals.  In this case, the signals
                                must be at least \c getMiniumumSignalLength(). */ 
        FixedWindow = 1,   /*!< An inference will be performed on a single 
                                window.  In this case, the input signals must
                                be a valid signal length. */
    };
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

    /// @name Signals to Process
    /// @{

    /// @brief Sets the signals on the vertical, north, and east channels
    ///        on which inference will be performed.
    /// @param[in] verticalSignal   The signal on the vertical channel.
    /// @param[in] northSignal      The signal on the north (1) channel.
    /// @param[in] eastSignal       The signal on the east (2) channel.
    /// @param[in] strategy         The inference strategy.
    /// @throws std::invalid_argument the signals are not the same size,
    ///         If the inference strategy is SlidingWindow then the signal
    ///         must be at least \c getMinimumSignalLength().  If the
    ///         inference straetgy is FixedWindow then the 
    ///         \c isValidSignalLength() must be true.
    void setVerticalNorthEastSignal(const std::vector<double> &verticalSignal,
                                    const std::vector<double> &northSignal,
                                    const std::vector<double> &eastSignal,
                                    InferenceStrategy strategy = InferenceStrategy::SlidingWindow);
    /// @brief Sets the signals on the vertical, north, and east channels
    ///        on which inference will be performed.
    /// @param[in,out] verticalSignal  The signal on the vertical channel.
    ///                                On exit, verticalSignal's behavior is
    ///                                undefined.
    /// @param[in,out] northSignal     The signal on the north (1) channel.
    ///                                On exit, northSignal's behavior is
    ///                                undefined.
    /// @param[in,out] eastSignal      The signal on the east (2) channel.
    ///                                On exit, eastSignal's behavior is
    ///                                undefined.
    /// @param[in] strategy            The inference strategy.
    /// @throws std::invalid_argument the signals are not the same size,
    ///         If the inference strategy is SlidingWindow then the signal
    ///         must be at least \c getMinimumSignalLength().  If the
    ///         inference straetgy is FixedWindow then the 
    ///         \c isValidSignalLength() must be true.
    void setVerticalNorthEastSignal(std::vector<double> &&verticalSignal,
                                    std::vector<double> &&northSignal,
                                    std::vector<double> &&eastSignal,
                                    InferenceStrategy strategy = InferenceStrategy::SlidingWindow);
    /// @result The vertical signal.
    /// @throws std::runtime_error if \c haveSignals() is false.
    [[nodiscard]] std::vector<double> getVerticalSignal() const;
    /// @result The north signal.
    /// @throws std::runtime_error if \c haveSignals() is false.
    [[nodiscard]] std::vector<double> getNorthSignal() const;
    /// @result The east signal.
    /// @throws std::runtime_error if \c haveSignals() is false.
    [[nodiscard]] std::vector<double> getEastSignal() const;

    /// @result A reference to the vertical signal.
    /// @throws std::runtime_error if \c haveSignals() is false.
    /// @note This exists for performance reasons.  You should use
    ///       \c getVerticalSignal(). 
    [[nodiscard]] const std::vector<double> &getVerticalSignalReference() const;
    /// @result A reference to the north signal.
    /// @throws std::runtime_error if \c haveSignals() is false.
    /// @note This exists for performance reasons.  You should use
    ///       \c getNorthSignal(). 
    [[nodiscard]] const std::vector<double> &getNorthSignalReference() const;
    /// @result A reference to the east signal.
    /// @throws std::runtime_error if \c haveSignals() is false.
    /// @note This exists for performance reasons.  You should use
    ///       \c getEastSignal(). 
    [[nodiscard]] const std::vector<double> &getEastSignalReference() const;

    /// @result True indicates the signals were set.
    [[nodiscard]] bool haveSignals() const noexcept;
    /// @result The inference strategy.
    /// @throws std::invalid_argument if \c haveSignals() is false.
    [[nodiscard]] InferenceStrategy getInferenceStrategy() const;
    /// @result The minimum signal length. 
    [[nodiscard]] static int getMinimumSignalLength() noexcept;
    /// @result True indicates this is a valid signal length.
    [[nodiscard]] bool isValidSignalLength(int nSamples) noexcept;
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
