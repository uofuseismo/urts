#ifndef URTS_SERVICES_SCALABLE_TRAVEL_TIMES_STATION_RESPONSE_HPP
#define URTS_SERVICES_SCALABLE_TRAVEL_TIMES_STATION_RESPONSE_HPP
#include <string>
#include <memory>
#include <umps/messageFormats/message.hpp>
#include <urts/services/scalable/travelTimes/stationRequest.hpp>
namespace URTS::Services::Scalable::TravelTimes
{
/// @class StationResponse "stationResponse.hpp" "urts/services/scalable/travelTimes/stationResponse.hpp"
/// @brief Resposne to a travel time request for a given station.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class StationResponse : public UMPS::MessageFormats::IMessage
{
public:
    /// @brief Defines the return code for the station request.
    enum class ReturnCode
    {
         Success = 0,    /*!< The travel time was successfully computed. */
         InvalidMessage = 2, /*!< The message could not be parsed. */
         InvalidParameter = 3, /*!< The StationRequest parameters were invalid. */
         AlgorithmicFailure = 4  /*!< There was a server-side error. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    StationResponse();
    /// @brief Copy constructor.
    /// @param[in] response   The response from which to initialize this class.
    StationResponse(const StationResponse &response);
    /// @brief Move constructor.
    /// @param[in,out] response  The response from which to initialize this class.
    ///                         On exit, response's behavior is undefined.
    StationResponse(StationResponse &&response) noexcept;
    /// @}

    /// @name Required Properties
    /// @{

    /// @brief Sets the source-to-receiver travel time in seconds.
    /// @param[in] time  The travel time in seconds.
    /// @throws std::invalid_argument if the time is negative.
    void setTravelTime(double time);
    /// @result The source-to-receiver travel time in seconds.
    /// @throws std::invalid_argument if \c haveTravelTime() is false.
    [[nodiscard]] double getTravelTime() const;
    /// @result True indicates the travel time was set.
    [[nodiscard]] bool haveTravelTime() const noexcept;

    /// @brief Sets the return code.
    /// @param[in] returnCode  The return code.
    void setReturnCode(ReturnCode returnCode) noexcept;
    /// @result The return code. 
    /// @throws std::runtime_error if \c haveReturnCode() is false. 
    [[nodiscard]] ReturnCode getReturnCode() const;
    /// @result True indicates the return code was set.
    [[nodiscard]] bool haveReturnCode() const noexcept;
    /// @}

    /// @name Optional Properties
    /// @{

    /// @brief This defines the region's velocity model in which the travel
    ///        time was computed.
    /// @param[in] region   The region in which the travel time was computed.
    void setRegion(StationRequest::Region region) noexcept;
    /// @result The region used for computing the travel time.
    [[nodiscard]] StationRequest::Region getRegion() const noexcept;

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
    ~StationResponse();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] response  The response to copy to this.
    /// @result A deep copy of the response.
    StationResponse& operator=(const StationResponse &response);
    
    /// @brief Move assignment operator.
    /// @result The memory from response moved to this.
    /// @result The memory from response moved to this.
    StationResponse& operator=(StationResponse &&response) noexcept;
    /// @}
private:
    class StationResponseImpl;
    std::unique_ptr<StationResponseImpl> pImpl;
};
}
#endif
