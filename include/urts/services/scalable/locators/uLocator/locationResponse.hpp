#ifndef URTS_SERVICES_SCALABLE_LOCATORS_ULOCATOR_LOCATION_RESPONSE_HPP
#define URTS_SERVICES_SCALABLE_LOCATORS_ULOCATOR_LOCATION_RESPONSE_HPP
#include <vector>
#include <string>
#include <optional>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::Locators::ULocator
{
 class Origin;
}
namespace URTS::Services::Scalable::Locators::ULocator
{
/// @class LocationResponse "locationResponse.hpp" "urts/services/scalable/locators/uLocate/locationResponse.hpp"
/// @brief Response to the location request given the provided arrivals.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class LocationResponse : public UMPS::MessageFormats::IMessage
{
public:
    /// @brief Defines the locator's return code.
    enum class ReturnCode
    {
        Success = 0,             /*!< The algorithm successfully ran. */
        InvalidRequest = 1,      /*!< The request message could not be parsed. */
        AlgorithmicFailure = 2   /*!< There was a server-side error. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    LocationResponse();
    /// @brief Copy constructor.
    /// @param[in] response  The response from which to initialize this class.
    LocationResponse(const LocationResponse &response);
    /// @brief Move constructor.
    /// @param[in] response  The response from which to initialize this class.
    ///                      On exit, response's behavior is undefined.
    LocationResponse(LocationResponse &&response) noexcept;
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
    /// @}

    /// @name Properties
    /// @{

    /// @brief Sets the origin.
    /// @param[in] origin   The origin must have a time, latitude, longitude,
    ///                     and depth.
    void setOrigin(const Origin &origin);
    /// @result The arrivals.
    [[nodiscard]] std::optional<Origin> getOrigin() const noexcept;

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
    void fromMessage(const std::string &message) final;
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
    ~LocationResponse();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] response  The response to copy to this.
    /// @result A deep copy of the response.
    LocationResponse& operator=(const LocationResponse &response);
 
    /// @brief Move assignment operator.
    /// @param[in,out] response  The response whose memory will be moved to this.
    ///                         On exit, response's behavior is undefined.
    /// @result The memory from the response moved to this.
    LocationResponse& operator=(LocationResponse &&response) noexcept;
    /// @}
private:
    class LocationResponseImpl;
    std::unique_ptr<LocationResponseImpl> pImpl;
};
}
#endif
