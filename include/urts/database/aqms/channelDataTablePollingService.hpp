#ifndef URTS_DATABASE_AQMS_CHANNEL_DATA_TABLE_POLLING_SERVICE_HPP
#define URTS_DATABASE_AQMS_CHANNEL_DATA_TABLE_POLLING_SERVICE_HPP
#include <memory>
#include <vector>
#include <chrono>
namespace UMPS::Logging
{
 class ILog;
}
namespace URTS::Database::Connection
{
 class IConnection;
}
namespace URTS::Database::AQMS
{
 class ChannelData;
}
namespace URTS::Database::AQMS
{
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
    void setConnection(std::shared_ptr<URTS::Database::Connection::IConnection> &connection);
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
    /// @result The channel data for all the queried stations.
    [[nodiscard]] std::vector<ChannelData> getChannelData() const;
    /// @param[in] network       The network code to match.
    /// @param[in] station       The station code to match.
    /// @param[in] channel       The channel code to match.
    /// @param[in] locationCode  The location code to match.
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
}
#endif
