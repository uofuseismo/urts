#ifndef URTS_DATABASE_AQMS_CHANNEL_DATA_HPP
#define URTS_DATABASE_AQMS_CHANNEL_DATA_HPP
#include <memory>
#include <vector>
#include <chrono>
namespace Time
{
 class UTC;
}
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

    /// @brief Sets the station elevation.
    /// @param[in] elevation  The station elevation in meters.
    void setElevation(double elevation) noexcept;
    /// @result The station elevation in meters.
    /// @throws std::runtime_error if \c haveElevation() is false.
    [[nodiscard]] double getElevation() const;
    /// @result True indicates that longitude was set.
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
    void setOnOffDate(const std::pair<Time::UTC, Time::UTC> &onOffDate);
    /// @result The date when this station was turned on.
    /// @throws std::runtime_error if \c haveOnOffDate() is false.
    [[nodiscard]] Time::UTC getOnDate() const;
    /// @result The date when this station may be turned off.
    /// @throws std::runtime_error if \c haveOnOffDate() is false.
    [[nodiscard]] Time::UTC getOffDate() const;
    /// @result True indicates the on/off date was set.
    [[nodiscard]] bool haveOnOffDate() const noexcept;
    /// @}

    /// @name Load Date
    /// @{

    /// @brief Sets the load date of the channel.
    void setLoadDate(const Time::UTC &loadDate) noexcept;
    /// @result The channel's load date.
    /// @throws std::runtime_error if \c haveLoadDate() is false.
    [[nodiscard]] Time::UTC getLoadDate() const;
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

/*
/// @class ChannelDataTable "channelDataTable.hpp" "urts/database/aqms/ChannelDataTable.hpp"
/// @brief A container for working with the channel_data table. 
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class ChannelDataTable : public URTS::ObserverPattern::ISubject
{
public:
    /// @brief Constructor.
    ChannelDataTable();
    /// @brief Constructor with given logger.
    explicit ChannelDataTable(std::shared_ptr<UMPS::Logging::ILog> &logger);

    /// @brief Destructor.
    virtual ~ChannelDataTable();
    /// @brief Sets a connection to the AQMS database. 
    void setConnection(std::shared_ptr<URTS::Database::Connection::PostgreSQL> &connection); 
    /// @result True indicates that there is a connection to the AQMS database.
    [[nodiscard]] bool isConnected() const noexcept;
    /// @brief Queries all channel data in the database.
    void queryAll();
    /// @brief Queries only channel data whose ontime is less than the current
    ///        time and whose offtime is greater than the current time. 
    void queryCurrent();
    /// @result The data for all the queried stations.
    [[nodiscard]] std::vector<ChannelData> getChannelData() const;
    /// @result The channel data matching this network, station, channel,
    ///         location code.  Note, if the QueryMode is ALL then you
    ///         get multiple results.
    [[nodiscard]] std::vector<ChannelData> getChannelData(const std::string &network,
                                                          const std::string &station,
                                                          const std::string &channel,
                                                          const std::string &locationCode) const;


    ChannelDataTable& operator=(const ChannelDataTable &) = delete;
    ChannelDataTable(const ChannelDataTable &) = delete;
    ChannelDataTable(ChannelDataTable &&data) noexcept = delete;
    ChannelDataTable& operator=(ChannelDataTable &&) = delete;
private:
    class ChannelDataTableImpl;
    std::unique_ptr<ChannelDataTableImpl> pImpl;
};
*/

/// @class ChannelDataTablePollingService "channelDataTablePollingService.hpp" "urts/database/aqms/ChannelDataTablePollingService.hpp"
/// @brief A utility service that polls the database and updates channel the
///        channel information.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class ChannelDataTablePollingService
{
public:
    /// @brief Defines whether or not queries return currently running
    ///        or all channels.
    enum class QueryMode
    {
        All,     /*!< Query all channels in the database. */
        Current  /*!< Query only active channels in the database. */
    };
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    ChannelDataTablePollingService();
    /// @brief Constructor with given logger.
    explicit ChannelDataTablePollingService(std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @}

    /// @name Initialization
    /// @{

    /// @brief Sets a connection to the AQMS database. 
    void setConnection(std::shared_ptr<URTS::Database::Connection::PostgreSQL> &connection);
    /// @result True indicates that there is a connection to the AQMS database.
    [[nodiscard]] bool isConnected() const noexcept;
    /// @}

    /// @name Start/Stop Polling Service
    /// @{

    /// @brief Starts a service that continually queries the database
    ///        for the channel data. 
    void start(const std::chrono::seconds &refreshRate = std::chrono::seconds {3600},
               QueryMode mode = QueryMode::Current);
    /// @brief Stops the query service.
    void stop();
    /// @result True indicates the service is running.
    [[nodiscard]] bool isRunning() const noexcept;
    /// @result The channel data matching this network, station, channel,
    ///         location code.  Note, if the QueryMode is ALL then you
    ///         may get multiple matches that correspond to different epochs.
    [[nodiscard]] std::vector<ChannelData> getChannelData(const std::string &network,
                                                          const std::string &station,
                                                          const std::string &channel,
                                                          const std::string &locationCode) const;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Destructor.
    ~ChannelDataTablePollingService();
    /// @}

    ChannelDataTablePollingService& operator=(const ChannelDataTablePollingService &) = delete;
    ChannelDataTablePollingService(const ChannelDataTablePollingService &) = delete;
    ChannelDataTablePollingService(ChannelDataTablePollingService &&) noexcept = delete;
    ChannelDataTablePollingService& operator=(ChannelDataTablePollingService &&) = delete;
private:
    class ChannelDataTablePollingServiceImpl;
    std::unique_ptr<ChannelDataTablePollingServiceImpl> pImpl;
};
[[nodiscard]] bool operator==(const ChannelData &lhs,
                              const ChannelData &rhs);
[[nodiscard]] bool operator!=(const ChannelData &lhs,
                              const ChannelData &rhs);
}
#endif
