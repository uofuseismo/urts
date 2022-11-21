#ifndef URTS_SERVICES_STANDALONE_INCREMENTER_INCREMENT_REQUEST_HPP
#define URTS_SERVICES_STANDALONE_INCREMENTER_INCREMENT_REQUEST_HPP
#include <memory>
#include <set>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Standalone::Incrementer
{
/// @class IncrementRequest "incrementRequest.hpp" "urts/services/standalone/incrementer/incrementRequest.hpp"
/// @brief To identify items (picks, events, origins, amplitudes, etc.) in the
///        processing pipeline we must assign them a unique object identifier.
///        The challenge in a distributed setting is that we need a single
///        entity that performs a lock-increment-unlock operation.  This is the
///        mechanism for requesting an identifier from that entity. 
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class IncrementRequest : public UMPS::MessageFormats::IMessage
{
public:
    /// @brief These are predefined set of items that can be incremented by
    ///        URTS.  Of course the API allows you to define custom items.
    /// @copyright Ben Baker (University of Utah) distributed under the MIT license.
    enum Item
    {
        Amplitude = 0,     /*!< An amplitude.  This is used in computing a magnitude. */
        Event = 1,         /*!< An event identifier. */
        Magnitude = 2,     /*!< A magnitude. */
        Origin = 3,        /*!< An event can have multiple origins as the location
                                is refined. */
        PhaseArrival = 4,  /*!< An arrival item (this is an associated pick). */
        PhasePick = 5      /*!< A pick item (this is an unassociated arrival). */ 
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    IncrementRequest();
    /// @brief Copy constructor.
    /// @param[in] request   The request from which to initialize this class.
    IncrementRequest(const IncrementRequest &request);
    /// @brief Move constructor.
    /// @param[in,out] request  The request from which to initialize this class.
    ///                         On exit, request's behavior is undefined.
    IncrementRequest(IncrementRequest &&request) noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] request   The request to copy to this.
    /// @result A deep copy of the the input request.
    IncrementRequest& operator=(const IncrementRequest &request);
    /// @brief Move assignment operator.
    /// @param[in,out] request  The request whose memory will be moved to this.
    ///                         On exit, request's behavior is undefined.
    /// @result The memory from request moved to this.
    IncrementRequest& operator=(IncrementRequest &&request) noexcept;
    /// @}

    /// @name Required Information
    /// @{

    /// @brief This represents a custom item to be incremented.
    /// @param[in] item  The item that (e.g., table name) that we are
    ///                  requesting be incremented.   
    /// @throws std::invalid_argument if the item is empty.
    void setItem(const std::string &item);
    /// @brief This defines a standard item to be incremented.
    /// @param[in] item  The item whose identifier we are requesting be
    ///                  incremented.
    void setItem(Item item) noexcept;
    /// @result The item identifier.
    [[nodiscard]] std::string getItem() const;
    /// @result True indicates that the item was set.
    [[nodiscard]] bool haveItem() const noexcept;
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

    /// @name Message Abstract Base Class Properties
    /// @{

    /// @result A copy of this class.
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> clone() const final;
    /// @result An instance of an uninitialized class.
    [[nodiscard]] std::unique_ptr<IMessage> createInstance() const noexcept final;
    /// @brief Converts the request class to a string message.
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
    /// @result The message type.
    [[nodiscard]] std::string getMessageType() const noexcept final;
    /// @result The message version.
    [[nodiscard]] std::string getMessageVersion() const noexcept final;
    /// @}

    /// @name Debugging Utilities
    /// @{

    /// @brief Creates the class from a JSON request message.
    /// @throws std::runtime_error if the message is invalid.
    void fromJSON(const std::string &message);
    /// @brief Converts the request class to a JSON message.  This is useful
    ///        for debugging.
    /// @param[in] nIndent  The number of spaces to indent.
    /// @note -1 disables indentation which is preferred for message
    ///       transmission.
    /// @result A JSON representation of this class.
    [[nodiscard]] std::string toJSON(int nIndent =-1) const;
    /// @brief Convenience function to initialize this class from a CBOR
    ///        message.
    /// @param[in] cbor  The CBOR message held in a string container.
    /// @throws std::runtime_error if the message is invalid.
    /// @throws std::invalid_argument if the required information is not set.
    void fromCBOR(const std::string &cbor);
    /// @brief Creates the class from a CBOR message.
    /// @param[in] data    The contents of the CBOR message.  This is an
    ///                    array whose dimension is [length] 
    /// @param[in] length  The length of data.
    /// @throws std::runtime_error if the message is invalid.
    /// @throws std::invalid_argument if data is NULL or length is 0. 
    void fromCBOR(const uint8_t *data, size_t length);
    /// @brief Converts the request class to a CBOR message.
    /// @result The class expressed in Compressed Binary Object Representation
    ///         (CBOR) format.
    /// @throws std::runtime_error if the required information is not set. 
    [[nodiscard]] std::string toCBOR() const;
    /// @}

    /// @result Maps an item to a string representation.
    [[nodiscard]] static std::string itemToString(Item item);
    /// @result The items.
    [[nodiscard]] static std::set<IncrementRequest::Item> getItemSet() noexcept;
    /// @name Destructors
    /// @{

    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~IncrementRequest() override;
    /// @}
private:
    class IncrementRequestImpl;
    std::unique_ptr<IncrementRequestImpl> pImpl;
};
}
#endif
