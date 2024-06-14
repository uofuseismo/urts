#ifndef URTS_DATABASE_AQMS_CHANNEL_DATA_HPP
#define URTS_DATABASE_AQMS_CHANNEL_DATA_HPP
#include <ostream>
#include <memory>
#include <vector>
#include <chrono>
namespace UMPS::Logging
{
 class ILog;
}
namespace URTS::Database::Connection
{
 class PostgreSQL;
}
namespace URTS::Database::AQMS
{
/// @class ChannelData "channelData.hpp" "urts/database/aqms/channelData.hpp"
/// @brief A (subset of the) channel_data table that is used in AQMS.  
///        This is useful if you only need to know a channel's sampling rate or
///        orientation.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class ChannelData
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    ChannelData();
    /// @brief Copy constructor.
    /// @param[in] channelData   The class from which to initialize this class.
    ChannelData(const ChannelData &channelData);
    /// @brief Move constructor.
    /// @param[in,out] channelData  The class from which to initialize this
    ///                             class.  On exit, channelData's behavior
    ///                             is undefined.
    ChannelData(ChannelData &&channelData) noexcept;
    /// @}
 
    /// @name Operators
    /// @{

    /// @brief Copy assignment.
    /// @param[in] channelData  The channel data to copy to this.
    /// @result A deep copy of the channel data.
    ChannelData& operator=(const ChannelData &channelData);
    /// @brief Move assignment.
    /// @param[in,out] channelData   The channel data whose memory will be
    ///                              moved to this.  On exit, channelData's
    ///                              behavior is undefined.
    /// @result The channelData's memory moved to this.
    ChannelData& operator=(ChannelData &&channelData) noexcept;
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

    /// @brief Sets the channel name.
    /// @param[in] channel  The channel name.
    void setChannel(const std::string &channel);
    /// @result The channel name.
    /// @throws std::invalid_argument if \c haveChannel() is false.
    [[nodiscard]] std::string getChannel() const;
    /// @result True indicates that the channel name was set.
    [[nodiscard]] bool haveChannel() const noexcept;
    /// @}

    /// @name Location Code
    /// @{

    /// @brief Sets the location code.
    /// @param[in] locationCode  The location code.
    void setLocationCode(const std::string &locationCode);
    /// @result The location code.
    /// @throws std::invalid_argument if \c haveLocationCode() is false.
    [[nodiscard]] std::string getLocationCode() const; 
    /// @result True indicates that the location code was set.
    [[nodiscard]] bool haveLocationCode() const noexcept;
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

    /// @brief Sets the channel's elevation.
    /// @param[in] elevation  The channel's elevation in meters.
    void setElevation(double elevation) noexcept;
    /// @result The channel elevation in meters.
    /// @throws std::runtime_error if \c haveElevation() is false.
    [[nodiscard]] double getElevation() const;
    /// @result True indicates that channel was set.
    [[nodiscard]] bool haveElevation() const noexcept;
    /// @}

    /// @name Sampling Rate
    /// @{

    /// @brief Sets the sampling rate.
    /// @param[in] samplingRate  The sampling rate in Hz.
    /// @throws std::invalid_argument if this is not postiive.
    void setSamplingRate(double samplingRate);
    /// @result The sampling rate in Hz.
    /// @throws std::runtime_error if \c haveSamplingRate() is false.
    [[nodiscard]] double getSamplingRate() const;
    /// @result True indicates the sampling rate was set.
    [[nodiscard]] bool haveSamplingRate() const noexcept;
    /// @}

    /// @name Dip
    /// @{
    /// @brief Sets the dip of the channel.
    /// @param[in] dip  The dip, in degrees, of the instrument measured 
    ///                 positive down horizontal.  This must be in the
    ///                 range [-90,90].
    /// @throws std::invalid_argument if this is not in the range [-90,90].
    void setDip(double dip);
    /// @result The dip in degrees.
    /// @throws std::runtime_error if \c haveDip() is false.
    [[nodiscard]] double getDip() const;
    /// @result True indicates the dip was set.
    [[nodiscard]] bool haveDip() const noexcept;
    /// @}

    /// @name Azimuth 
    /// @{

    /// @brief Sets the azimuth of the channel.
    /// @param[in] azimuth  The azimuth in degrees.  This is measured positive
    ///                     clockwise from north.  This must be in the range
    ///                     [0, 360].
    /// @throws std::invalid_argument if this is not in the range [0,360].
    void setAzimuth(double azimuth);
    /// @result The azimuth in degrees.
    /// @throws std::runtime_error if \c haveAzimuth() is false.
    [[nodiscard]] double getAzimuth() const;
    /// @result True indicates the azimuth was set.
    [[nodiscard]] bool haveAzimuth() const noexcept;
    /// @}

    /// @name On/Off Date
    /// @{

    /// @brief Sets the on and off date of the station.
    /// @param[in] onOffDate  onOffDate.first is the on date of the station
    ///                       and onOffDate.second is the off date.
    /// @throws std::invalid_argument if onOffDate.first >= onOffDate.second.
    /// @note Times are UTC and specified in microseconds since the epoch.
    void setOnOffDate(const std::pair<std::chrono::microseconds, std::chrono::microseconds> &onOffDate);
    /// @result The time (UTC) when this station was turned on in microseconds
    ///         since the epoch.
    /// @throws std::runtime_error if \c haveOnOffDate() is false.
    [[nodiscard]] std::chrono::microseconds getOnDate() const;
    /// @result The time (UTC) when this station may be turned off in
    ///          microseconds since the epoch.
    /// @throws std::runtime_error if \c haveOnOffDate() is false.
    [[nodiscard]] std::chrono::microseconds getOffDate() const;
    /// @result True indicates the on/off date was set.
    [[nodiscard]] bool haveOnOffDate() const noexcept;
    /// @}

    /// @name Load Date
    /// @{

    /// @brief Sets the load date of the channel.
    /// @param[in] loadDate  The time (UTC) in microseconds since the epoch when
    ///                      the station was loaded in the database.
    void setLoadDate(const std::chrono::microseconds &loadDate) noexcept;
    /// @result The channel's load date (UTC) in micrseconds since the epoch.
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
    ~ChannelData();
    /// @}
private:
    class ChannelDataImpl;
    std::unique_ptr<ChannelDataImpl> pImpl; 
};
/// @result True indicates the channel data classes are equal.
[[nodiscard]] bool operator==(const ChannelData &lhs,
                              const ChannelData &rhs);
/// @result True indicates the channel data classes are not equal.
[[nodiscard]] bool operator!=(const ChannelData &lhs,
                              const ChannelData &rhs);
/// @brief Outputs a channel as a JSON object.
/// @param[in] os           An output stream object.
/// @param[in] channelData  The channel data.
/// @return A formatted channelData JSON string.
std::ostream& operator<<(std::ostream &os, const ChannelData &channelData);
}
#endif
