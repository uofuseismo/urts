#ifndef URTS_DATABASE_AQMS_CHANNEL_DATA_TABLE_HPP
#define URTS_DATABASE_AQMS_CHANNEL_DATA_TABLE_HPP
#include <memory>
#include <vector>
#include "urts/observerPattern/subject.hpp"
namespace UMPS::Logging
{
 class ILog;
}
namespace URTS::Database
{
 namespace AQMS
 {
  class ChannelData;
 }
 namespace Connection
 {
  class IConnection;
 }
}
namespace URTS::Database::AQMS
{
/// @class ChannelDataTable "channelDataTable.hpp" "urts/database/aqms/channelDataTable.hpp"
/// @brief A container for working with the channel_data table. 
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class ChannelDataTable : public URTS::ObserverPattern::ISubject
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    ChannelDataTable();
    /// @brief Constructor with given logger.
    explicit ChannelDataTable(std::shared_ptr<UMPS::Logging::ILog> &logger);
    /// @brief Constructor with a given connection and logger.
    /// @note Provided the connection is set, you can skip the setConnection() method.
    explicit ChannelDataTable(std::shared_ptr<URTS::Database::Connection::IConnection> &connection,
                              std::shared_ptr<UMPS::Logging::ILog> logger = nullptr);
    /// @}

    /// @name Initialization
    /// @{

    /// @brief Sets a connection to the AQMS database. 
    void setConnection(std::shared_ptr<URTS::Database::Connection::IConnection> &connection); 
    /// @result True indicates that there is a connection to the AQMS database.
    [[nodiscard]] bool isConnected() const noexcept;
    /// @}

    /// @name Conveniecne Queries
    /// @{

    /// @brief Queries all channel data in the database.
    void queryAll();
    /// @brief Queries only channel data whose ontime is less than the current
    ///        time and whose offtime is greater than the current time. 
    void queryCurrent();
    /// @}

    /// @name Query Results
    /// @{

    /// @result The channel data for all the queried stations.
    [[nodiscard]] std::vector<ChannelData> getChannelData() const;
    /// @param[in] network       The network code to match.
    /// @param[in] station       The station code to match.
    /// @param[in] channel       The channel code to match.
    /// @param[in] locationCode  The location code to match.
    /// @result The channel data matching this network, station, channel,
    ///         location code.  Note, if the queryAll was invoked then you
    ///         may get multiple results that correspond to different epochs.
    [[nodiscard]] std::vector<ChannelData> getChannelData(const std::string &network,
                                                          const std::string &station,
                                                          const std::string &channel,
                                                          const std::string &locationCode) const;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Destructor.
    virtual ~ChannelDataTable();
    /// @}

    ChannelDataTable& operator=(const ChannelDataTable &) = delete;
    ChannelDataTable(const ChannelDataTable &) = delete;
    ChannelDataTable(ChannelDataTable &&data) noexcept = delete;
    ChannelDataTable& operator=(ChannelDataTable &&) = delete;
private:
    class ChannelDataTableImpl;
    std::unique_ptr<ChannelDataTableImpl> pImpl;
};
}
#endif
