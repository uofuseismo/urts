#ifndef URTS_DATABASE_AQMS_CHANNEL_DATA_TABLE_POLLER_HPP
#define URTS_DATABASE_AQMS_CHANNEL_DATA_TABLE_POLLER_HPP
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
/// @class ChannelDataTablePoller "channelDataTablePoller.hpp" "urts/database/aqms/ChannelDataTablePoller.hpp"
/// @brief A utility service that polls the database and updates the
///        channel data.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class ChannelDataTablePoller
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
    ChannelDataTablePoller();
    /// @brief Constructor with given logger.
    explicit ChannelDataTablePoller(std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @}

    /// @name Initialization
    /// @{

    /// @brief Sets a connection to the AQMS database. 
    /// @param[in] connection       A connection to the AQMS database.
    /// @param[in] queryMode        Defines whether the poller will get all
    ///                             channel data or only open channels.
    /// @param[in] pollingInterval  The poller will query the database
    ///                             this many seconds.
    /// @throws std::invalid_argument if the connection->isConnected() is false
    ///         or the refresh rate is negative.
    void initialize(std::shared_ptr<URTS::Database::Connection::IConnection> &connection,
                    QueryMode mode = QueryMode::Current,
                    const std::chrono::seconds &pollingInterval = std::chrono::seconds {3600});
    /// @result True indicates that the poller class is initialized and
    ///          ready to be started.
    [[nodiscard]] bool isInitialized() const noexcept;
    /// @result The database polling interval.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] std::chrono::seconds getPollingInterval() const noexcept;
    /// @}

    /// @name Start/Stop Polling Service
    /// @{

    /// @brief Starts a service that continually queries the database
    ///        for the channel data. 
    /// @throws std::runtime_error if \c isInitialized() is false.
    void start();
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
    ~ChannelDataTablePoller();
    /// @}

    ChannelDataTablePoller& operator=(const ChannelDataTablePoller &) = delete;
    ChannelDataTablePoller(const ChannelDataTablePoller &) = delete;
    ChannelDataTablePoller(ChannelDataTablePoller &&) noexcept = delete;
    ChannelDataTablePoller& operator=(ChannelDataTablePoller &&) = delete;
private:
    class ChannelDataTablePollerImpl;
    std::unique_ptr<ChannelDataTablePollerImpl> pImpl;
};
}
#endif
