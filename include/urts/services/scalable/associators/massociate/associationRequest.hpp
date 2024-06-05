#ifndef URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_ASSOCATION_REQUEST_HPP
#define URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_ASSOCATION_REQUEST_HPP
#include <vector>
#include <string>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::Associators::MAssociate
{
 class Pick;
}
namespace URTS::Services::Scalable::Associators::MAssociate
{
/// @class AssociationRequest "assocationRequest.hpp" "urts/services/scalable/associators/massociate/associationRequest.hpp"
/// @brief Requests the association of the provided picks into event(s).
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class AssociationRequest : public UMPS::MessageFormats::IMessage
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    AssociationRequest();
    /// @brief Copy constructor.
    /// @param[in] request  The request from which to initialize this class.
    AssociationRequest(const AssociationRequest &request);
    /// @brief Move constructor.
    /// @param[in] request  The request from which to initialize this class.
    ///                     On exit, request's behavior is undefined.
    AssociationRequest(AssociationRequest &&request) noexcept;
    /// @}

    /// @name Properties
    /// @{

    /// @brief Sets the unassociated arrivals (i.e., picks).
    /// @param[in] picks  Sets the picks to associate.  The pick must have the
    ///                   network, station, channel, and location code set as
    ///                   well as the pick time and phase hint.
    void setPicks(const std::vector<Pick> &picks);
    /// @result The picks.
    [[nodiscard]] std::vector<Pick> getPicks() const noexcept;
    /// @result A reference to the picks.
    [[nodiscard]] const std::vector <Pick> &getPicksReference() const noexcept;

    /// @brief Sets the request identifier.
    /// @param[in] identifier   A unique identifier for this request.
    void setIdentifier(int64_t identifier) noexcept;
    /// @result The request identifier.
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
    /// @param[in] data    The contents of the message.  This is an
    ///                    array whose dimension is [length] 
    /// @param[in] length  The length of data.
    /// @throws std::runtime_error if the message is invalid.
    /// @throws std::invalid_argument if data is NULL or length is 0. 
    void fromMessage(const char *data, size_t length) final;
    /// @result Defines the message type.
    [[nodiscard]] std::string getMessageType() const noexcept final;
    /// @result The message version.
    [[nodiscard]] std::string getMessageVersion() const noexcept final;
    /// @result A copy of this class.
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> clone() const final;
    /// @result An uninitialized instance of this class.
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage> createInstance() const noexcept final;
    /// @}
 
    /// @name Destructors
    /// @{

    /// @brief Resets the class and releases memory.
    void clear() noexcept;
    /// @brief Destructor.
    ~AssociationRequest();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] request  The request to copy to this.
    /// @result A deep copy of the request.
    AssociationRequest& operator=(const AssociationRequest &request);
 
    /// @brief Move assignment operator.
    /// @param[in,out] request  The request whose memory will be moved to this.
    ///                         On exit, request's behavior is undefined.
    /// @result The memory from the request moved to this.
    AssociationRequest& operator=(AssociationRequest &&request) noexcept;
    /// @}
private:
    class AssociationRequestImpl;
    std::unique_ptr<AssociationRequestImpl> pImpl;
};
}
#endif
