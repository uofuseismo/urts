#ifndef URTS_DATABASE_AQMS_STATION_DATA_HPP
#define URTS_DATABASE_AQMS_STATION_DATA_HPP
#include <memory>
namespace URTS::Database::AQMS
{
/// @class StationData "stationData.hpp" "urts/database/aqms/stationData.hpp"
/// @brief A (subset of the) station data table that is used in AQMS.  
///        This is useful if you only need a station name, location, and 
///        description.  Station responses exist in other tables.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class StationData
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    StationData();
    /// @brief Copy constructor.
    /// @param[in] stationData   The class from which to initialize this class.
    StationData(const StationData &stationData);
    /// @brief Move constructor.
    /// @param[in,out] stationData  The class from which to initialize this
    ///                             class.  On exit, stationData's behavior
    ///                             is undefined.
    StationData(StationData &&stationData) noexcept;
    /// @}
 
    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] stationData  The station data to copy to this.
    /// @result A deep copy of the station data.
    StationData& operator=(const StationData &stationData);
    /// @brief Move assignment.
    /// @param[in,out] stationData   The station data whose memory will be
    ///                              moved to this.  On exit, stationData's
    ///                              behavior is undefined.
    /// @result The stationData's memory moved to this.
    StationData& operator=(StationData &&stationData) noexcept;
    /// @}

    /// @name Network
    /// @{

    /// @brief Sets the network name.
    /// @param[in] network  The network name.
    void setNetwork(const std::string &network);
    /// @result The network name.
    /// @throws std::invalid_argument if \c haveNetwork() is false.
    [[nodiscard]] std::string getNetwork() const;
    /// @result True indicates that the network name was set.
    [[nodiscard]] bool haveNetwork() const noexcept;
    /// @}

    /// @name Station
    /// @{

    /// @brief Sets the station name.
    /// @param[in] station  The station name.
    void setStation(const std::string &station);
    /// @result The station name.
    /// @throws std::invalid_argument if \c haveStation() is false.
    [[nodiscard]] std::string getStation() const;
    /// @result True indicates that the station name was set.
    [[nodiscard]] bool haveStation() const noexcept;
    /// @}

    /// @name Latitude
    /// @{

    /// @brief Sets the station latitude.
    /// @param[in] latitude  The station latitude in degrees.
    /// @throws std::invalid_argument if the latitude is not in
    ///         the range [-90,90].
    void setLatitude(double latitude);
    /// @result The station latitude in degrees.
    /// @throws std::runtime_error if \c haveLatitude() is false.
    [[nodiscard]] double getLatitude() const;
    /// @result True indicates that latitude was set.
    [[nodiscard]] bool haveLatitude() const noexcept;
    /// @}

    /// @name Longitude
    /// @{

    /// @brief Sets the station longitude.
    /// @param[in] longitude  The station latitude in degrees.
    /// @throws std::invalid_argument if the latitude is not in
    ///         the range [-540,540).
    void setLongitude(double longitude);
    /// @result The station longitude in degrees.
    /// @throws std::runtime_error if \c haveLatitude() is false.
    [[nodiscard]] double getLongitude() const;
    /// @result True indicates that longitude was set.
    [[nodiscard]] bool haveLongitude() const noexcept;
    /// @}

    /// @name Elevation 
    /// @{

    /// @brief Sets the station elevation.
    /// @param[in] elevation  The station elevation in meters.
    void setElevation(double elevation) noexcept;
    /// @result The station elevation in meters.
    /// @throws std::runtime_error if \c haveElevation() is false.
    [[nodiscard]] double getElevation() const;
    /// @result True indicates that longitude was set.
    [[nodiscard]] bool haveElevation() const noexcept;
    /// @}

    /// @name Description
    /// @{

    /// @brief Sets a string-based description of the station.
    /// @param[in] description  The station description - e.g., 
    ///                         Washington Dome, UT, USA.
    void setDescription(const std::string &description) noexcept;
    /// @result The station description.
    [[nodiscard]] std::string getDescription() const noexcept;
    /// @}

    /// @name On/Off Date
    /// @brief Sets the on and off date of the station.
    /// @param[in] onOffDate  onOffDate.first is the on date of the station
    ///                       and onOffDate.second is the off date.
    /// @throws std::invalid_argument if onOffDate.first >= onOffDate.second.
    void setOnOffDate(const std::pair<std::chrono::microseconds, std::chrono::microseconds> &onOffDate);
    /// @result The date when this station was turned on.
    /// @throws std::runtime_error if \c haveOnOffDate() is false.
    [[nodiscard]] std::chrono::microseconds getOnDate() const;
    /// @result The date when this station may be turned off.
    /// @throws std::runtime_error if \c haveOnOffDate() is false.
    [[nodiscard]] std::chrono::microseconds getOffDate() const;
    /// @result True indicates the on/off date was set.
    [[nodiscard]] bool haveOnOffDate() const noexcept;
    /// @}

    /// @name Load Date
    /// @{

    /// @brief Sets the load date of the station.
    void setLoadDate(const std::chrono::microseconds &loadDate) noexcept;
    /// @result The station's load date.
    /// @throws std::runtime_error if \c haveLoadDate() is false.
    [[nodiscard]] std::chrono::microseconds getLoadDate() const;
    /// @result True indicates the load date was set.
    [[nodiscard]] bool haveLoadDate() const noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Resets the class.
    void clear() noexcept;
    /// @brief Destructor.
    ~StationData();
    /// @}
private:
    class StationDataImpl;
    std::unique_ptr<StationDataImpl> pImpl;
};
/// @result True indicates the station data classes are equal.
[[nodiscard]] bool operator==(const StationData &lhs,
                              const StationData &rhs);
/// @result True indicates the station data classes are not equal.
[[nodiscard]] bool operator!=(const StationData &lhs,
                              const StationData &rhs);
/// @brief Outputs a station as a JSON object.
/// @param[in] os           An output stream object.
/// @param[in] stationData  The station data.
/// @return A formatted stationData JSON string.
std::ostream& operator<<(std::ostream &os, const StationData &stationData);
}
#endif
