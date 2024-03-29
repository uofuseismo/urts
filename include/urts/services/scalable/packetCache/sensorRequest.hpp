#ifndef URTS_SERVICES_SCALABLE_PACKET_CACHE_SENSOR_REQUEST_HPP
#define URTS_SERVICES_SCALABLE_PACKET_CACHE_SENSOR_REQUEST_HPP
#include <memory>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::PacketCache
{
/// @name SensorRequest "sensorRequest.hpp" "urts/services/scalable/packetCache/sensorRequest.hpp"
/// @brief This is a request message for querying the available sensors - i.e.,
///        all unique Network, Station, Channel, Location Codes.
/// @note Since the underlying messaging is asynchronous it is to your advantage
///       to provide your request a unique identifier since the requests are
///       not required to filled in the order that they are put on the wire.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class SensorRequest : public UMPS::MessageFormats::IMessage
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    SensorRequest();
    /// @brief Copy constructor.
    /// @param[in] request  The request from which to initialize this class.
    SensorRequest(const SensorRequest &request);
    /// @brief Move constructor.
    /// @param[in,out] request  The request from which to initialize this class.
    ///                         On exit, request's behavior is undefined.
    SensorRequest(SensorRequest &&request) noexcept;
    /// @}
    
    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] request  The request to copy to this.
    /// @result A deep copy of the input request.
    SensorRequest& operator=(const SensorRequest &request); 
    /// @brief Move assignment operator.
    /// @param[in,out] request  The request whose memory will be moved to this.
    ///                         On exit, request's behavior is undefined.
    /// @result The memory from request moved to this.
    SensorRequest& operator=(SensorRequest &&request) noexcept;
    /// @}

    /// @name Optional Information
    /// @{

    /// @brief For asynchronous messaging this allows the requester to index
    ///        the request.  This value will be returned so the requester
    ///        can track which request was filled by the response.
    /// @param[in] identifier   The request identifier.
    void setIdentifier(uint64_t identifier) noexcept;
    /// @result The request identifier.
    [[nodiscard]] uint64_t getIdentifier() const noexcept;
    /// @}

    /// @name Message Properties
    /// @{

    /// @brief Converts this class to a string message.
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

    /// @name Debugging Utilities
    /// @{

    /// @brief Creates the class from a JSON data request message.
    /// @throws std::runtime_error if the message is invalid.
    void fromJSON(const std::string &message);
    /// @brief Converts the data request class to a JSON message.
    /// @param[in] nIndent  The number of spaces to indent.
    /// @note -1 disables indentation which is preferred for message
    ///       transmission.
    /// @result A JSON representation of this class.
    [[nodiscard]] std::string toJSON(int nIndent =-1) const;
    /// @brief Converts the pick class to a CBOR message.
    /// @result The class expressed in Compressed Binary Object Representation
    ///         (CBOR) format.
    /// @throws std::runtime_error if the required information is not set. 
    [[nodiscard]] std::string toCBOR() const;
    /// @brief Creates the class from a CBOR message.
    /// @param[in] cbor  The CBOR message.
    void fromCBOR(const std::string &cbor);
    /// @brief Creates the class from a CBOR message.
    /// @param[in] data    The contents of the CBOR message.  This is an
    ///                    array whose dimension is [length] 
    /// @param[in] length  The length of data.
    /// @throws std::runtime_error if the message is invalid.
    /// @throws std::invalid_argument if data is NUL or length is 0.
    void fromCBOR(const uint8_t *data, size_t length);
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~SensorRequest() override;
    /// @}
private:
    class SensorRequestImpl;
    std::unique_ptr<SensorRequestImpl> pImpl;
};
}
#endif
