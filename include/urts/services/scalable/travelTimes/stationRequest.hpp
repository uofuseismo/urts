#ifndef URTS_SERVICES_SCALABLE_TRAVEL_TIMES_STATION_REQUEST_HPP
#define URTS_SERVICES_SCALABLE_TRAVEL_TIMES_STATION_REQUEST_HPP
#include <string>
#include <memory>
#include <umps/messageFormats/message.hpp>
namespace URTS::Services::Scalable::TravelTimes
{
/// @class StationRequest "stationRequest.hpp" "urts/services/scalable/travelTimes/stationRequest.hpp"
/// @brief Requests a travel time calculation from a source point to a station.
/// @copyright Ben Baker (UUSS) distributed under the MIT license.
class StationRequest : public UMPS::MessageFormats::IMessage
{
public:
    /// @brief Defines the geographic region in which to compute the
    ///        travel times.
    enum class Region
    {
        Utah = 0,        /*!< Compute the travel times in the Utah region. */
        Yellowstone = 1, /*!< Compute the travel times in the Yellowstone region. */
        EventBased = 2   /*!< Use the region based on the hypocenter's latitude. */
    };
    /// @brief Defines the phase.
    enum class Phase
    {
        P = 0, /*!< Compute the first-arriving P arrival time. */
        S = 1  /*!< Compute the first-arriving S arrival time. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    StationRequest();
    /// @brief Copy constructor.
    /// @param[in] request   The request from which to initialize this class.
    StationRequest(const StationRequest &request);
    /// @brief Move constructor.
    /// @param[in,out] request  The request from which to initialize this class.
    ///                         On exit, request's behavior is undefined.
    StationRequest(StationRequest &&request) noexcept;
    /// @}

    /// @name Required Properties
    /// @{

    /// @brief Sets the network code.
    /// @param[in] network  The network code - e.g., UU.
    /// @throw std::invalid_argument if the network code is empty.
    void setNetwork(const std::string &network);
    /// @result The network code.
    [[nodiscard]] std::string getNetwork() const;
    /// @result True indicates the network was set.
    [[nodiscard]] bool haveNetwork() const noexcept;

    /// @brief Sets the station name.
    /// @param[in] station   The station name - e.g., ARUT.
    /// @throw std::invalid_argument if the station name is empty.
    void setStation(const std::string &station);
    /// @result The station name.
    [[nodiscard]] std::string getStation() const;
    /// @result True indicates the network was set.
    [[nodiscard]] bool haveStation() const noexcept;

    /// @brief Sets the source's latitude.
    /// @param[in] latitude   The source's latitude in degrees.
    /// @throws std::invalid_argument if this is not in the range [-90,90].
    void setLatitude(double latitude);
    /// @result The latitude in degrees.
    /// @throws std::runtime_error if \c haveLatitude() is false.
    [[nodiscard]] double getLatitude() const;
    /// @result True indicates the latitude was set.
    [[nodiscard]] bool haveLatitude() const noexcept;

    /// @brief Sets the source's longitude.
    /// @param[in] longitude   The source's longitude in degrees.
    void setLongitude(double latitude) noexcept;
    /// @result The longitude in degrees.  This is in the range [-180,180).
    /// @throws std::runtime_error if \c haveLongitude() is false.
    [[nodiscard]] double getLongitude() const; 
    /// @result True indicates the longitude was set.
    [[nodiscard]] bool haveLongitude() const noexcept;

    /// @brief Sets the event's depth.
    /// @param[in] depth   The event depth in meters with respect to sea-level.
    /// @throws std::invalid_argument if the depth is higher than Mt. Everest
    ///         (~-8850 m) or deeper than 800,000 meters.
    void setDepth(double depth);
    /// @result The event depth in meters.
    [[nodiscard]] double getDepth() const; 
    /// @result True indicates the event depth was set.
    [[nodiscard]] bool haveDepth() const noexcept;

    /// @brief Sets the phase type.
    /// @param[in] phase   The phase type. 
    void setPhase(Phase phase) noexcept;
    /// @result The phase type.
    /// @throws std::runtime_error if \c havePhase() is true.
    [[nodiscard]] Phase getPhase() const;
    /// @result True indicates the phase was set.
    [[nodiscard]] bool havePhase() const noexcept;
    /// @}

    /// @name Optional Properties
    /// @{

    /// @brief This defines the region's velocity model in which to compute
    ///        the travel time.
    /// @param[in] region   The region in which to compute the travel times.
    void setRegion(Region region) noexcept;
    /// @result The region to be used for computing the travel times.
    [[nodiscard]] Region getRegion() const noexcept;

    /// @brief Enables the use of corrections (should they exist).
    void enableCorrections() noexcept;
    /// @brief Disables the use of corrections.
    void disableCorrections() noexcept; 
    /// @result True indicates corrections will be used if they exist.
    [[nodiscard]] bool useCorrections() const noexcept;

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
    ~StationRequest();
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] request  The request to copy to this.
    /// @result A deep copy of the request.
    StationRequest& operator=(const StationRequest &request);
    
    /// @brief Move assignment operator.
    /// @result The memory from request moved to this.
    /// @result The memory from request moved to this.
    StationRequest& operator=(StationRequest &&request) noexcept;
    /// @}
private:
    class StationRequestImpl;
    std::unique_ptr<StationRequestImpl> pImpl;
};
}
#endif
