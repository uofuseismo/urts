#ifndef URTS_DATABASE_AQMS_STATION_DATA_TABLE_HPP
#define URTS_DATABASE_AQMS_STATION_DATA_TABLE_HPP
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
  class StationData;
 }
 namespace Connection
 {
  class IConnection;
 }
}
namespace URTS::Database::AQMS
{
/// @class StationDataTable "stationDataTable.hpp" "urts/database/aqms/stationDataTable.hpp"
/// @brief A container for working with the station_data table. 
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class StationDataTable : public URTS::ObserverPattern::ISubject
{
public:
    /// @name Constructors
    /// @{

    /// @brief Constructor.
    StationDataTable();
    /// @brief Constructor with given logger.
    explicit StationDataTable(std::shared_ptr<UMPS::Logging::ILog> &logger);
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

    /// @brief Queries all station data in the database.
    void queryAll();
    /// @brief Queries only station data whose ontime is less than the current
    ///        time and whose offtime is greater than the current time. 
    void queryCurrent();
    /// @}

    /// @name Query Results
    /// @{

    /// @result The station data for all the queried stations.
    [[nodiscard]] std::vector<StationData> getStationData() const;
    /// @param[in] network       The network code to match.
    /// @param[in] station       The station code to match.
    /// @result The station data matching this network and station.
    ///         Note, if the queryAll was invoked then you
    ///         may get multiple results that correspond to different epochs.
    [[nodiscard]] std::vector<StationData> getStationData(const std::string &network,
                                                          const std::string &station) const;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Destructor.
    virtual ~StationDataTable();
    /// @}

    StationDataTable& operator=(const StationDataTable &) = delete;
    StationDataTable(const StationDataTable &) = delete;
    StationDataTable(StationDataTable &&data) noexcept = delete;
    StationDataTable& operator=(StationDataTable &&) = delete;
private:
    class StationDataTableImpl;
    std::unique_ptr<StationDataTableImpl> pImpl;
};
}
#endif
