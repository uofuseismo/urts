#ifndef URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_ASSOCATION_RESPONSE_HPP
#define URTS_SERVICES_SCALABLE_ASSOCIATORS_MASSOCIATE_ASSOCATION_RESPONSE_HPP
#include <vector>
#include <string>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::Associators::MAssociate
{
 class Pick;
 class Arrival;
 class Origin;
}
namespace URTS::Services::Scalable::Associators::MAssociate
{
/// @class AssociationResponse "assocationResponse.hpp" "urts/services/scalable/associators/massociate/associationResponse.hpp"
/// @brief The response to an association request.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class AssociationResponse : public UMPS::MessageFormats::IMessage
{
public:
    enum class ReturnCode
    {
        Success = 0,             /*!< The algorithm successfully ran.  Note, this does not necessarily mean that there are associated origins. */
        InvalidRequest = 1,      /*!< The request message could not be parsed. */
        AlgorithmicFailure = 2   /*!< There was a server-side error. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    AssociationResponse();
    /// @brief Copy constructor.
    /// @param[in] response  The response from which to initialize this class.
    AssociationResponse(const AssociationResponse &response);
    /// @brief Move constructor.
    /// @param[in] response  The response from which to initialize this class.
    ///                      On exit, response's behavior is undefined.
    AssociationResponse(AssociationResponse &&response) noexcept;
    /// @}

    /// @name Required Properties
    /// @{

    /// @brief Sets the return code for this association request.
    /// @param[in] returnCode  The return code.
    void setReturnCode(ReturnCode returnCode) noexcept;
    /// @result The return code.
    [[nodiscard]] ReturnCode getReturnCode() const;
    /// @result True indicates the return code was set.
    [[nodiscard]] bool haveReturnCode() const noexcept;

    /// @name Properties
    /// @{

    /// @brief Sets the event origins (built from the associated picks).
    /// @param[in] origins  The event origins.  Each origin must have a
    ///                     hypocenter and time.
    void setOrigins(const std::vector<Origin> &origins);
    /// @result The event origins.
    [[nodiscard]] std::vector<Origin> getOrigins() const noexcept;
    /// @result A reference to the origins.
    [[nodiscard]] const std::vector <Origin> &getOriginsReference() const noexcept;

    /// @brief Sets the unassociated arrivals (i.e., picks).
    /// @param[in] picks  Sets the picks to associate.  The pick must have the
    ///                   network, station, channel, and location code set as
    ///                   well as the pick time and phase hint.
    void setUnassociatedPicks(const std::vector<Pick> &picks);
    /// @result The unassociated picks.
    [[nodiscard]] std::vector<Pick> getUnassociatedPicks() const noexcept;
    /// @result A reference to the unassociated picks.
    [[nodiscard]] const std::vector <Pick> &getUnassociatedPicksReference() const noexcept;

    /// @brief Sets the response identifier.
    /// @param[in] identifier   A unique identifier for this response.
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
    ~AssociationResponse();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] response  The response to copy to this.
    /// @result A deep copy of the response.
    AssociationResponse& operator=(const AssociationResponse &response);
 
    /// @brief Move assignment operator.
    /// @param[in,out] response  The response whose memory will be moved to this.
    ///                          On exit, response's behavior is undefined.
    /// @result The memory from the response moved to this.
    AssociationResponse& operator=(AssociationResponse &&response) noexcept;
    /// @}
private:
    class AssociationResponseImpl;
    std::unique_ptr<AssociationResponseImpl> pImpl;
};
}
#endif
