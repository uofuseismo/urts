#ifndef UMPS_PROXY_SERVICES_PACKET_CACHE_BULK_DATA_RESPONSE_HPP
#define UMPS_PROXY_SERVICES_PACKET_CACHE_BULK_DATA_RESPONSE_HPP
#include <memory>
#include <vector>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::PacketCache
{
 class DataResponse;
}
namespace URTS::Services::Scalable::PacketCache
{
/// @name BulkDataResponse "bulkDataResponse.hpp" "urts/services/scalable/packetCache/bulkDataResponse.hpp"
/// @brief This represents the data response for multiple sensors.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class BulkDataResponse : public UMPS::MessageFormats::IMessage
{
public:
    /// @brief Defines the return code for a bulk data request.
    enum ReturnCode
    {   
        Success = 0,             /*!< No errors were detected; the request was successful. */
        NoSensor = 1,            /*!< The data for the requested sensor 
                                      (Network, Station, Channel, Location Code) does not exist. */
        InvalidMessageType = 2,  /*!< The received message type is not supported. */
        InvalidMessage = 3,      /*!< The request message could not be parsed. */
        InvalidTimeQuery = 4,    /*!< The time query parameters are invalid. */
        AlgorithmicFailure = 5   /*!< An internal error was detected .*/
    }; 
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    BulkDataResponse();
    /// @brief Copy constructor.
    /// @param[in] response  The response from which to initialize this class.
    BulkDataResponse(const BulkDataResponse &response);
    /// @brief Move constructor.
    /// @param[in,out] response  The response from which to initialize
    ///                          this class.  On exit, responses' behavior is
    ///                          undefined.
    BulkDataResponse(BulkDataResponse &&response) noexcept;
    /// @}
    
    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] response  The response to copy to this.
    /// @result A deep copy of the input response.
    BulkDataResponse& operator=(const BulkDataResponse &response); 
    /// @brief Move assignment operator.
    /// @param[in,out] response  The response whose memory will be moved to
    ///                          this.  On exit, response's behavior is
    ///                          undefined.
    /// @result The memory from response moved to this.
    BulkDataResponse& operator=(BulkDataResponse &&response) noexcept;
    /// @}

    /// @name Responses
    /// @{

    /// @param[in] response  Adds a data response.  
    void addDataResponse(const DataResponse &response);
    /// @param[in,out] response  Adds a data response.  On exit, responses's
    ///                          behavior will be undefined.
    void addDataResponse(DataResponse &&response);
    /// @result The data responses.
    [[nodiscard]] std::vector<DataResponse> getDataResponses() const noexcept;
    /// @result A pointer to the data responses.  This is an array whose 
    ///         dimension is [\c getNumberOfDataResponses()].
    [[nodiscard]] const DataResponse *getDataResponsesPointer() const noexcept;
    /// @result The number of data responses.
    [[nodiscard]] int getNumberOfDataResponses() const noexcept;
    /// @}

    /// @name Additional Information
    /// @{

    /// @brief Allows the service to set its return code and signal to
    ///        the requester whether or not the request was successfully
    ///        processed.
    /// @param[in] code  The return code.
    void setReturnCode(ReturnCode code) noexcept;
    /// @result The return code from the service.
    [[nodiscard]] BulkDataResponse::ReturnCode getReturnCode() const noexcept;

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

    /// @brief Converts the this class to a string message.
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

    /// @brief Creates the class from a JSON data reseponse message.
    /// @throws std::runtime_error if the message is invalid.
    void fromJSON(const std::string &message);
    /// @brief Converts the data response class to a JSON message.
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
    ~BulkDataResponse() override;
    /// @}
private:
    class BulkDataResponseImpl;
    std::unique_ptr<BulkDataResponseImpl> pImpl;
};
}
#endif
