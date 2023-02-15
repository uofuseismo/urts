#include <iomanip>
#include <cmath>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <soci/soci.h>
#include <time/utc.hpp>
#include <umps/logging/standardOut.hpp>
#include "urts/database/aqms/channelDataTable.hpp"
#include "urts/database/aqms/channelData.hpp"
#include "urts/database/connection/postgresql.hpp"

#define COLUMNS "net, sta, seedchan, location, lat, lon, elev, azimuth, dip, samprate, EXTRACT(epoch FROM ondate), EXTRACT(epoch FROM offdate), EXTRACT(epoch FROM lddate) "

using namespace URTS::Database::AQMS;

template<> struct soci::type_conversion<ChannelData>
{
    typedef values base_type;
    static void from_base(const values &v, indicator , ChannelData &data)
    {
        // Required by schema
        data.setNetwork(v.get<std::string> ("net"));
        data.setStation(v.get<std::string> ("sta"));
        data.setChannel(v.get<std::string> ("seedchan"));
        data.setLocationCode(v.get<std::string> ("location"));
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

        try
        {
            data.setAzimuth(v.get<double> ("azimuth"));
        }
        catch (...)
        {
        }

        try
        {
            data.setDip(v.get<double> ("dip"));
        }
        catch (...)
        {
        }

        data.setSamplingRate(v.get<double> ("samprate"));
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

class ChannelDataTable::ChannelDataTableImpl
{
public:
    /// Constructor
    ChannelDataTableImpl(std::shared_ptr<UMPS::Logging::ILog> logger) :
        mLogger(logger)
    {
        if (mLogger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StandardOut> ();
        }
    }
    [[nodiscard]] URTS::ObserverPattern::Message update(
        const std::vector<ChannelData> &newData,
        const std::string &queryType)
    {
        std::scoped_lock lock(mMutex);
        if (newData != mChannelData)
        {
            mChannelData = newData;
            mHaveCurrent = false;
            mHaveAll = false;
            if (queryType == "current"){mHaveCurrent = true;}
            if (queryType == "all"){mHaveAll = true;}
            return URTS::ObserverPattern::Message::Update;
        }
        return URTS::ObserverPattern::Message::NoChange;
    }
    /// Gets the channel data
    [[nodiscard]] std::vector<ChannelData> getData() const
    {
        std::scoped_lock lock(mMutex);
        return mChannelData;
    }
    /// Gets the channel data
    [[nodiscard]] std::vector<ChannelData>
        getData(const std::string &network,
                const std::string &station,
                const std::string &channel,
                const std::string &locationCode) const
    {
        auto channelData = getData();
        if (channelData.empty()){return channelData;}
        channelData.erase(
                std::remove_if(channelData.begin(), channelData.end(),
                               [network, station, channel, locationCode]
                               (const ChannelData &cd)
                               {
                                   bool lMatch = false;
                                   if (cd.haveNetwork() &&
                                       cd.haveStation() &&
                                       cd.haveChannel() &&
                                       cd.haveLocationCode())
                                   {
                                       if (cd.getNetwork() == network &&
                                           cd.getStation() == station &&
                                           cd.getChannel() == channel &&
                                           cd.getLocationCode() == locationCode)
                                       {
                                           lMatch = true;
                                       }
                                   }
                                   return !lMatch;
                               }),
                channelData.end());
        return channelData;
    }
//private:
    mutable std::mutex mMutex;
    std::shared_ptr<UMPS::Logging::ILog> mLogger;
    std::shared_ptr<URTS::Database::Connection::IConnection>
        mConnection{nullptr};
    std::vector<ChannelData> mChannelData;
    std::chrono::microseconds mMostRecentLoadDate{-2208988800000000}; //1900-01-01T00:00:00
    int mChannelSpaceEstimate{5000};
    bool mConnected{false};
    bool mHaveAll{false}; 
    bool mHaveCurrent{false};
};

/// C'tor
ChannelDataTable::ChannelDataTable() :
    pImpl(std::make_unique<ChannelDataTableImpl> (nullptr)) 
{
}

/// C'tor
ChannelDataTable::ChannelDataTable(
    std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ChannelDataTableImpl> (logger))
{
}

/// Destructor
ChannelDataTable::~ChannelDataTable() = default;

void ChannelDataTable::setConnection(
    std::shared_ptr<URTS::Database::Connection::IConnection> &connection)
{
    pImpl->mConnection = connection;
    pImpl->mConnected = true;
}

/// Connected?
bool ChannelDataTable::isConnected() const noexcept
{
    return pImpl->mConnected;
}

/// Query every channel
void ChannelDataTable::queryAll()
{
    if (!isConnected()){throw std::runtime_error("Class not connected");}
    pImpl->mLogger->debug("Checking for new channel_date information...");
    auto session
        = reinterpret_cast<soci::session *> (pImpl->mConnection->getSession());
    // Do I really need an update?
    auto mostRecentLoadDate
        = static_cast<int64_t> (std::round((pImpl->mMostRecentLoadDate.count())
                                           *1.e-6));
    soci::rowset<int>
       rowCount(session->prepare << "SELECT COUNT(*) FROM channel_data WHERE "
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
        pImpl->mLogger->debug("Unhandled case - querying table");
        doQuery = true;
    }
    if (!doQuery && pImpl->mHaveAll)
    {
        pImpl->mLogger->debug("No update detected");
        this->notify(URTS::ObserverPattern::Message::NoChange);
        return;
    }
    // Yeah, let's query
    pImpl->mLogger->debug("Querying channel_data information...");
    pImpl->mChannelData.clear();
    soci::rowset<ChannelData>
        rows(session->prepare << "SELECT " << COLUMNS << " FROM channel_data");
    std::vector<ChannelData> data;
    data.reserve(pImpl->mChannelSpaceEstimate);
    for (auto &it : rows)
    {
        // Plus 1 deals with truncation of microseconds
        std::chrono::microseconds thisLoadDate{it.getLoadDate().count() + 1};
        pImpl->mMostRecentLoadDate
            = std::max(pImpl->mMostRecentLoadDate, thisLoadDate);
        data.push_back(it);
    }
    auto message = pImpl->update(data, "all");
    pImpl->mChannelSpaceEstimate
         = std::max(pImpl->mChannelSpaceEstimate,
                    static_cast<int> (pImpl->mChannelData.size()));
    this->notify(message);
}

/// Query channels currently running
void ChannelDataTable::queryCurrent()
{
    if (!isConnected()){throw std::runtime_error("Class not connected");}
    pImpl->mLogger->debug("Checking for new channel_date information...");
    auto session
        = reinterpret_cast<soci::session *> (pImpl->mConnection->getSession());
    // Do I really need an update?
    auto mostRecentLoadDate
        = static_cast<int64_t> (std::round(pImpl->mMostRecentLoadDate.count()
                                          *1.e-6));
    soci::rowset<int> 
        rowCount(session->prepare << "SELECT COUNT(*) FROM channel_data WHERE "
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
        pImpl->mLogger->debug("Unhandled case - querying table");
        doQuery = true;
    }   
    if (!doQuery && pImpl->mHaveCurrent)
    {   
        pImpl->mLogger->debug("No update detected");
        this->notify(URTS::ObserverPattern::Message::NoChange);
        return;
    }
    // Yeah, let's actually query
    pImpl->mLogger->debug("Querying channel_data information...");
    Time::UTC now;
    now.now();
    auto currentTime = static_cast<int64_t> (now.getEpoch());
    pImpl->mChannelData.clear();
    soci::rowset<ChannelData>
        rows(session->prepare << "SELECT "
                              << COLUMNS
                              << " FROM channel_data WHERE "
                              << "TO_TIMESTAMP("
                              << std::to_string(currentTime)
                              << ") AT TIME ZONE 'UTC'"
                              << " BETWEEN ondate AND offdate");
    std::vector<ChannelData> data;
    data.reserve(pImpl->mChannelSpaceEstimate);
    for (auto &it : rows)
    {
        // Plus 1 deals with truncation on microseconds
        std::chrono::microseconds thisLoadDate{it.getLoadDate().count() + 1}; 
        pImpl->mMostRecentLoadDate
            = std::max(pImpl->mMostRecentLoadDate, thisLoadDate);
        data.push_back(it);
    }
    auto message = pImpl->update(data, "current");
    pImpl->mChannelSpaceEstimate
         = std::max(pImpl->mChannelSpaceEstimate,
                    static_cast<int> (pImpl->mChannelData.size()));
    this->notify(message);
}

/// Return the queried data
std::vector<ChannelData> ChannelDataTable::getChannelData() const
{
    auto result = pImpl->getData();
    return result;
}

/// Return the queried data
std::vector<ChannelData>
    ChannelDataTable::getChannelData(const std::string &network,
                                     const std::string &station,
                                     const std::string &channel,
                                     const std::string &locationCode) const
{
    return pImpl->getData(network, station, channel, locationCode);
}

