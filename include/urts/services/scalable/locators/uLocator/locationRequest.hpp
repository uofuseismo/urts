#ifndef URTS_SERVICES_SCALABLE_LOCATORS_ULOCATOR_LOCATION_REQUEST_HPP
#define URTS_SERVICES_SCALABLE_LOCATORS_ULOCATOR_LOCATION_REQUEST_HPP
#include <vector>
#include <string>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::Locators::ULocator
{
 class Arrival;
}
namespace URTS::Services::Scalable::Locators::ULocator
{
/// @class LocationRequest "locationRequest.hpp" "urts/services/scalable/locators/uLocator/locationRequest.hpp"
/// @brief Requests the location given the provided arrivals.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class LocationRequest : public UMPS::MessageFormats::IMessage
{
public:
    enum class LocationStrategy
    {
        General = 0,    /*!< This will perform a general event location. */
        FreeSurface = 1 /*!< This will constraint the event to the free surface.  This is useful for quarry blasts. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    LocationRequest();
    /// @brief Copy constructor.
    /// @param[in] request  The request from which to initialize this class.
    LocationRequest(const LocationRequest &request);
    /// @brief Move constructor.
    /// @param[in] request  The request from which to initialize this class.
    ///                     On exit, request's behavior is undefined.
    LocationRequest(LocationRequest &&request) noexcept;
    /// @}

    /// @name Required Properties
    /// @{

    /// @brief Sets the arrivals.
    /// @param[in] arrivals  Sets the arrivals from which to build the location.
    ///                      The arrivals must have the network, station,
    ///                      and phase as well as the time.  Additionally,
    ///                      the arrivals must be causal - i.e., no S arrivals
    ///                      can precede P arrivals at a station and no
    ///                      duplicate phases can be added for a station. 
    void setArrivals(const std::vector<Arrival> &arrivals);
    /// @result The arrivals.
    [[nodiscard]] std::vector<Arrival> getArrivals() const;
    /// @result A reference to the arrivals.
    [[nodiscard]] const std::vector<Arrival> &getArrivalsReference() const;
    /// @result True indicates the arrivals were set.
    [[nodiscard]] bool haveArrivals() const noexcept;
    /// }

    /// @name Optional Properties
    /// @{

    /// @brief Sets the request identifier.
    /// @param[in] identifier   A unique identifier for this request.
    void setIdentifier(int64_t identifier) noexcept;
    /// @result The request identifier.
    [[nodiscard]] int64_t getIdentifier() const noexcept;

    /// @brief Sets the event location strategy.
    /// @param[in] strategy   The location strategy.
    void setLocationStrategy(LocationStrategy strategy) noexcept;
    /// @result The location strategy.
    /// @note By default this is a general event location.
    [[nodiscard]] LocationStrategy getLocationStrategy() const noexcept;
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
    ~LocationRequest();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] request  The request to copy to this.
    /// @result A deep copy of the request.
    LocationRequest& operator=(const LocationRequest &request);
 
    /// @brief Move assignment operator.
    /// @param[in,out] request  The request whose memory will be moved to this.
    ///                         On exit, request's behavior is undefined.
    /// @result The memory from the request moved to this.
    LocationRequest& operator=(LocationRequest &&request) noexcept;
    /// @}
private:
    class LocationRequestImpl;
    std::unique_ptr<LocationRequestImpl> pImpl;
};
}
#endif
