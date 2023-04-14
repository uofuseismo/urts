#include <iostream>
#include <iomanip>
#include <cmath>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <soci/soci.h>
#include <umps/logging/standardOut.hpp>
#include "urts/database/aqms/stationDataTable.hpp"
#include "urts/database/aqms/stationData.hpp"
#include "urts/database/connection/postgresql.hpp"

#define COLUMNS "net, sta, staname, lat, lon, elev, EXTRACT(epoch FROM ondate) as ondate, EXTRACT(epoch FROM offdate) as offdate, EXTRACT(epoch FROM lddate) as lddate "

using namespace URTS::Database::AQMS;

template<> struct soci::type_conversion<StationData>
{
    typedef values base_type;
    static void from_base(const values &v, indicator , StationData &data)
    {
        // Required by schema
        data.setNetwork(v.get<std::string> ("net"));
        data.setStation(v.get<std::string> ("sta"));
        try
        {
            data.setDescription(v.get<std::string> ("staname"));
        }
        catch (...)
        {
        }

        try
        {
            data.setLatitude(v.get<double> ("lat"));
        }
        catch (...)
        {
        }

        try
        {
            data.setLongitude(v.get<double> ("lon"));
        }
        catch (...)
        {
        }

        try
        {
            data.setElevation(v.get<double> ("elev"));
        }
        catch (...)
        {
        }

        double onDate{v.get<double> ("ondate")};
        double offDate(2114380800); // 2037
        try
        {
            offDate = v.get<double> ("offdate");
        }
        catch (...)
        {
        }
        double loadDate{v.get<double> ("lddate")};    
        // Conversions
        std::chrono::microseconds
            onDateMuS{ static_cast<int64_t> (std::round(onDate*1.e6)) };
        std::chrono::microseconds
            offDateMuS{ static_cast<int64_t> (std::round(offDate*1.e6)) };
        std::chrono::microseconds
            loadDateMuS{ static_cast<int64_t> (std::round(loadDate*1.e6)) };
        // Set it
        data.setOnOffDate(std::pair{onDateMuS, offDateMuS});
        data.setLoadDate(loadDateMuS);
    }   
};

class StationDataTable::StationDataTableImpl
{
public:
    /// Constructor
    StationDataTableImpl(std::shared_ptr<UMPS::Logging::ILog> logger) :
        mLogger(logger)
    {
        if (mLogger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> ();
        }
    }
    [[nodiscard]] URTS::ObserverPattern::Message update(
        const std::vector<StationData> &newData,
        const std::string &queryType)
    {
        std::scoped_lock lock(mMutex);
        if (newData != mStationData)
        {
            mStationData = newData;
            mHaveCurrent = false;
            mHaveAll = false;
            if (queryType == "current"){mHaveCurrent = true;}
            if (queryType == "all"){mHaveAll = true;}
            return URTS::ObserverPattern::Message::Update;
        }
        return URTS::ObserverPattern::Message::NoChange;
    }
    /// Gets the channel data
    [[nodiscard]] std::vector<StationData> getData() const
    {
        std::scoped_lock lock(mMutex);
        return mStationData;
    }
    /// Gets the station data for the given network + station
    [[nodiscard]] std::vector<StationData>
        getData(const std::string &network,
                const std::string &station) const
    {
        auto stationData = getData();
        if (stationData.empty()){return stationData;}
        stationData.erase(
                std::remove_if(stationData.begin(), stationData.end(),
                               [network, station]
                               (const StationData &sd)
                               {
                                   bool lMatch = false;
                                   if (sd.haveNetwork() &&
                                       sd.haveStation())
                                   {
                                       if (sd.getNetwork() == network &&
                                           sd.getStation() == station)
                                       {
                                           lMatch = true;
                                       }
                                   }
                                   return !lMatch;
                               }),
                stationData.end());
        return stationData;
    }
    /// Gets the station data for the given network
    [[nodiscard]] std::vector<StationData>
        getData(const std::set<std::string> &networks) const
    {   
        auto stationData = getData();
        if (stationData.empty()){return stationData;}
        stationData.erase(
                std::remove_if(stationData.begin(), stationData.end(),
                               [networks]
                               (const StationData &sd)
                               {
                                   bool lMatch = false;
                                   if (sd.haveNetwork())
                                   {
                                       if (networks.contains(sd.getNetwork()))
                                       {
                                           lMatch = true;
                                       }
                                   }   
                                   return !lMatch;
                               }), 
                stationData.end());
        return stationData;
    }
//private:
    mutable std::mutex mMutex;
    std::shared_ptr<UMPS::Logging::ILog> mLogger;
    std::shared_ptr<URTS::Database::Connection::IConnection>
        mConnection{nullptr};
    std::vector<StationData> mStationData;
    std::chrono::microseconds mMostRecentLoadDate{-2208988800000000}; //1900-01-01T00:00:00
    int mStationSpaceEstimate{5000};
    bool mConnected{false};
    bool mHaveAll{false}; 
    bool mHaveCurrent{false};
};

/// C'tor
StationDataTable::StationDataTable() :
    pImpl(std::make_unique<StationDataTableImpl> (nullptr)) 
{
}

/// C'tor
StationDataTable::StationDataTable(
    std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<StationDataTableImpl> (logger))
{
}

/// Destructor
StationDataTable::~StationDataTable() = default;

void StationDataTable::setConnection(
    std::shared_ptr<URTS::Database::Connection::IConnection> &connection)
{
    pImpl->mConnection = connection;
    pImpl->mConnected = true;
}

/// Connected?
bool StationDataTable::isConnected() const noexcept
{
    return pImpl->mConnected;
}

/// Query every station 
void StationDataTable::queryAll()
{
    if (!isConnected()){throw std::runtime_error("Class not connected");}
    pImpl->mLogger->debug("Checking for new station_data information...");
    auto session
        = reinterpret_cast<soci::session *> (pImpl->mConnection->getSession());
    // Do I really need an update?
    auto mostRecentLoadDate
        = static_cast<int64_t> (std::round((pImpl->mMostRecentLoadDate.count())
                                           *1.e-6));
    soci::rowset<int>
       rowCount(session->prepare << "SELECT COUNT(*) FROM station_data WHERE "
                                 << "lddate > TO_TIMESTAMP("
                                 << std::to_string(mostRecentLoadDate)
                                 << ") AT TIME ZONE 'UTC'");
    bool doQuery = true;
    size_t nRows = 0;
    for (auto &it : rowCount)
    {
        if (it == 0){doQuery = false;}
        nRows = nRows + 1;
    }
    if (nRows != 1)
    {
        pImpl->mLogger->warn("Unhandled case - querying table");
        doQuery = true;
    }
    if (!doQuery && pImpl->mHaveAll)
    {
        pImpl->mLogger->debug("No update detected");
        this->notify(URTS::ObserverPattern::Message::NoChange);
        return;
    }
    // Yeah, let's query
    pImpl->mLogger->debug("Querying station_data information...");
    pImpl->mStationData.clear();
    soci::rowset<StationData>
        rows(session->prepare << "SELECT " << COLUMNS << " FROM station_data");
    std::vector<StationData> data;
    data.reserve(pImpl->mStationSpaceEstimate);
    for (auto &it : rows)
    {
        // Plus 1 second deals with truncation issue when database
        // performs equality test.
        std::chrono::microseconds thisLoadDate{it.getLoadDate().count()
                                             + 1000000};
        pImpl->mMostRecentLoadDate
            = std::max(pImpl->mMostRecentLoadDate, thisLoadDate);
        data.push_back(it);
    }
    auto message = pImpl->update(data, "all");
    pImpl->mStationSpaceEstimate
         = std::max(pImpl->mStationSpaceEstimate,
                    static_cast<int> (pImpl->mStationData.size()));
    this->notify(message);
}

/// Query stations currently running
void StationDataTable::queryCurrent()
{
    if (!isConnected()){throw std::runtime_error("Class not connected");}
    pImpl->mLogger->debug("Checking for new station_data information...");
    auto session
        = reinterpret_cast<soci::session *> (pImpl->mConnection->getSession());
    // Do I really need an update?
    auto mostRecentLoadDate
        = static_cast<int64_t> (std::round(pImpl->mMostRecentLoadDate.count()
                                          *1.e-6));
    soci::rowset<int> 
        rowCount(session->prepare << "SELECT COUNT(*) FROM station_data WHERE "
                                  << "lddate > TO_TIMESTAMP("
                                  << std::to_string(mostRecentLoadDate)
                                  << ") AT TIME ZONE 'UTC'");
    size_t nRows = 0;
    bool doQuery = true;
    for (auto &it : rowCount)
    {   
        if (it == 0){doQuery = false;}
        nRows = nRows + 1;
    }   
    if (nRows != 1)
    {
        pImpl->mLogger->warn("Unhandled case - querying table");
        doQuery = true;
    }
    if (!doQuery && pImpl->mHaveCurrent)
    {   
        pImpl->mLogger->debug("No update detected");
        this->notify(URTS::ObserverPattern::Message::NoChange);
        return;
    }
    // Yeah, let's actually query
    pImpl->mLogger->debug("Querying station_data information...");
    pImpl->mStationData.clear();
    soci::rowset<StationData>
        rows(session->prepare << "SELECT "
                              << COLUMNS
                              << " FROM station_data WHERE "
                              << " now() BETWEEN ondate AND offdate");
    std::vector<StationData> data;
    data.reserve(pImpl->mStationSpaceEstimate);
    for (auto &it : rows)
    {
        // Plus 1 second deals with truncation issue when database
        // performs equality test.
        std::chrono::microseconds thisLoadDate{it.getLoadDate().count()
                                             + 1000000}; 
        pImpl->mMostRecentLoadDate
            = std::max(pImpl->mMostRecentLoadDate, thisLoadDate);
        data.push_back(it);
    }
    auto message = pImpl->update(data, "current");
    pImpl->mStationSpaceEstimate
         = std::max(pImpl->mStationSpaceEstimate,
                    static_cast<int> (pImpl->mStationData.size()));
    this->notify(message);
}

/// Return the queried data
std::vector<StationData> StationDataTable::getStationData() const
{
    auto result = pImpl->getData();
    return result;
}

/// Return the queried data
std::vector<StationData>
    StationDataTable::getStationData(const std::string &network,
                                     const std::string &station) const
{
    return pImpl->getData(network, station);
}

std::vector<StationData>
    StationDataTable::getStationData(const std::string &network) const
{
    return getStationData(std::set<std::string> {network});
}

std::vector<StationData>
StationDataTable::getStationData(
    const std::set<std::string> &networks) const
{
    return pImpl->getData(networks);
}
