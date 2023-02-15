#include <iomanip>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <soci/soci.h>
#include <soci/postgresql/soci-postgresql.h>
#include <time/utc.hpp>
#include <umps/logging/standardOut.hpp>
#include "urts/database/aqms/channelData.hpp"
#include "urts/database/connection/postgresql.hpp"
#include "utilities.hpp"

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
        auto onDate = ::fromTM(v.get<std::tm> ("ondate"));
        Time::UTC offDate(2114380800); // 2037
        try
        {
            auto offDate = ::fromTM(v.get<std::tm> ("offdate"));
        }
        catch (...)
        {
        }
        data.setOnOffDate(std::pair(onDate, offDate));

        auto loadDate = ::fromTM(v.get<std::tm> ("lddate"));
        data.setLoadDate(loadDate); 
    }   
};

/// Compares station data
bool URTS::Database::AQMS::operator==(
    const ChannelData &lhs, const ChannelData &rhs)
{
    constexpr double tol = 1.e-7;
    if (lhs.haveNetwork() && rhs.haveNetwork())
    {
        if (lhs.getNetwork() != rhs.getNetwork()){return false;}
    }
    else
    {
        return false;
    }

    if (lhs.haveStation() && rhs.haveStation())
    {
        if (lhs.getStation() != rhs.getStation()){return false;}
    }
    else
    {
        return false;
    }

    if (lhs.haveChannel() && rhs.haveChannel())
    {
        if (lhs.getChannel() != rhs.getChannel()){return false;}
    }
    else
    {
        return false;
    }

    if (lhs.haveLocationCode() && rhs.haveLocationCode())
    {
        if (lhs.getLocationCode() != rhs.getLocationCode()){return false;}
    }
    else
    {
        return false;
    }

    if (lhs.haveOnOffDate() && rhs.haveOnOffDate())
    {
        if (lhs.getOnDate()  != rhs.getOnDate()){return false;}
        if (lhs.getOffDate() != rhs.getOffDate()){return false;}
    }
    else
    {
        return false;
    }

    if (lhs.haveLoadDate() && rhs.haveLoadDate())
    {
        if (lhs.getLoadDate() != rhs.getLoadDate()){return false;}
    }
    else
    {
        return false;
    }

    if (lhs.haveLatitude() && rhs.haveLatitude())
    {
        if (std::abs(lhs.getLatitude() - rhs.getLatitude()) > tol)
        {
            return false;
        }
    }
    else
    {
        return false;
    }   

    if (lhs.haveLongitude() && rhs.haveLongitude())
    {
        if (std::abs(lhs.getLongitude() - rhs.getLongitude()) > tol)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (lhs.haveElevation() && rhs.haveElevation())
    {
        if (std::abs(lhs.getElevation() - rhs.getElevation()) > tol)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (lhs.haveDip() && rhs.haveDip())
    {
        if (std::abs(lhs.getDip() - rhs.getDip()) > tol)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    if (lhs.haveAzimuth() && rhs.haveAzimuth())
    {
        if (std::abs(lhs.getAzimuth() - rhs.getAzimuth()) > tol)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool URTS::Database::AQMS::operator!=(
    const ChannelData &lhs, const ChannelData &rhs)
{
    return !(lhs == rhs);
}


///--------------------------------------------------------------------------///
///                          Station Data                                    ///
///--------------------------------------------------------------------------///
class ChannelData::ChannelDataImpl
{
public:
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    Time::UTC mOnDate;
    Time::UTC mOffDate;
    Time::UTC mLoadDate;
    double mLatitude{0};
    double mLongitude{0};
    double mElevation{0};
    double mSamplingRate{0};
    double mDip{0};
    double mAzimuth{0};
    bool mHaveOnOffDate{false};
    bool mHaveLatitude{false};
    bool mHaveLongitude{false};
    bool mHaveElevation{false};
    bool mHaveDip{false};
    bool mHaveAzimuth{false};
    bool mHaveLoadDate{false};
};

/// C'tor
ChannelData::ChannelData() :
    pImpl(std::make_unique<ChannelDataImpl> ())
{
}

/// Copy c'tor
ChannelData::ChannelData(const ChannelData &channelData)
{
    *this = channelData;
}

/// Move c'tor
ChannelData::ChannelData(ChannelData &&channelData) noexcept
{
    *this = std::move(channelData);
}

/// Copy assignment
ChannelData& ChannelData::operator=(const ChannelData &data)
{
    if (&data == this){return *this;}
    pImpl = std::make_unique<ChannelDataImpl> (*data.pImpl);
    return *this;
}

/// Move assignment
ChannelData& ChannelData::operator=(ChannelData &&data) noexcept
{
    if (&data == this){return *this;}
    pImpl = std::move(data.pImpl);
    return *this;
}

/// Destructor
ChannelData::~ChannelData() = default;

/// Network
void ChannelData::setNetwork(const std::string &network)
{
    if (network.empty()){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string ChannelData::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set");}
    return pImpl->mNetwork;
}

bool ChannelData::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void ChannelData::setStation(const std::string &station)
{
    if (station.empty()){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = station;
}

std::string ChannelData::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set");}
    return pImpl->mStation;
}

bool ChannelData::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void ChannelData::setChannel(const std::string &channel)
{
    if (channel.empty()){throw std::invalid_argument("Channel is empty");}
    pImpl->mChannel = channel;
}

std::string ChannelData::getChannel() const
{
    if (!haveChannel()){throw std::runtime_error("Channel not set");}
    return pImpl->mChannel;
}

bool ChannelData::haveChannel() const noexcept
{
    return !pImpl->mChannel.empty();
}

/// Latitude
void ChannelData::setLatitude(const double latitude)
{
    if (latitude < -90 || latitude > 90)
    {
        throw std::invalid_argument("Latitude must be in [-90,90]");
    }
    pImpl->mLatitude = latitude;
    pImpl->mHaveLatitude = true;
}

double ChannelData::getLatitude() const
{
    if (!haveLatitude()){throw std::runtime_error("Latitude not set");}
    return pImpl->mLatitude;
}

bool ChannelData::haveLatitude() const noexcept
{
    return pImpl->mHaveLatitude;
}

/// Longitude
void ChannelData::setLongitude(const double lonIn)
{
    pImpl->mLongitude = ::lonTo180(lonIn);
    pImpl->mHaveLongitude = true;
}

double ChannelData::getLongitude() const
{
    if (!haveLongitude()){throw std::runtime_error("Longitude not set");}
    return pImpl->mLongitude;
}

bool ChannelData::haveLongitude() const noexcept
{
    return pImpl->mHaveLongitude;
}

/// Elevation
void ChannelData::setElevation(const double elevation) noexcept
{
    pImpl->mElevation = elevation;
    pImpl->mHaveElevation = true;
}

double ChannelData::getElevation() const
{
    if (!haveElevation()){throw std::runtime_error("Elevation not set");}
    return pImpl->mElevation;
}

bool ChannelData::haveElevation() const noexcept
{
    return pImpl->mHaveElevation;
}

/// Location code
void ChannelData::setLocationCode(const std::string &locationCode)
{
    // N.B. At UUSS an empty channel code from the database will be '  '
    if (locationCode.empty())
    {
        throw std::invalid_argument("Location code is empty");
    }
    pImpl->mLocationCode = locationCode;
}

std::string ChannelData::getLocationCode() const
{
    if (!haveLocationCode())
    {
        throw std::runtime_error("Location code not set");
    }
    return pImpl->mLocationCode;
}

bool ChannelData::haveLocationCode() const noexcept
{
    return !pImpl->mLocationCode.empty();
}

/// Sampling rate
void ChannelData::setSamplingRate(const double samplingRate)
{
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("Sampling rate not positive");
    }
    pImpl->mSamplingRate = samplingRate;
}

double ChannelData::getSamplingRate() const
{
    if (!haveSamplingRate()){throw std::runtime_error("Sampling rate not set");}
    return pImpl->mSamplingRate;
}

bool ChannelData::haveSamplingRate() const noexcept
{
    return (pImpl->mSamplingRate > 0);
}

/// Azimuth 
void ChannelData::setAzimuth(const double azimuth)
{
    if (azimuth < 0 || azimuth > 360)
    {
        throw std::invalid_argument("Azimuth not in range [0,360]");
    }
    pImpl->mAzimuth = azimuth;
    pImpl->mHaveAzimuth = true;
}

double ChannelData::getAzimuth() const
{
    if (!haveAzimuth()){throw std::runtime_error("Azimuth not set");}
    return pImpl->mAzimuth;
}

bool ChannelData::haveAzimuth() const noexcept
{
    return pImpl->mHaveAzimuth;
}

/// Dip
void ChannelData::setDip(const double dip)
{
    if (dip < -90 || dip > 90)
    {
        throw std::invalid_argument("Dip not in range [-90,90]");
    }
    pImpl->mDip = dip;
    pImpl->mHaveDip = true;
}

double ChannelData::getDip() const
{
    if (!haveDip()){throw std::runtime_error("Dip not set");}
    return pImpl->mDip;
}

bool ChannelData::haveDip() const noexcept
{
    return pImpl->mHaveDip;
}

/// On/off date
void ChannelData::setOnOffDate(const std::pair<Time::UTC, Time::UTC> &onOffDate)
{
    if (onOffDate.first.getEpoch() >= onOffDate.second.getEpoch())
    {   
        throw std::invalid_argument(
            "onOffDate.first must be less than onOffDate.second");
    }   
    pImpl->mOnDate = onOffDate.first;
    pImpl->mOffDate = onOffDate.second;
    pImpl->mHaveOnOffDate = true;
}

Time::UTC ChannelData::getOnDate() const
{
    if (!haveOnOffDate()){throw std::runtime_error("On/off date not set");}
    return pImpl->mOnDate;
}

Time::UTC ChannelData::getOffDate() const
{
    if (!haveOnOffDate()){throw std::runtime_error("On/off date not set");}
    return pImpl->mOffDate;
}

bool ChannelData::haveOnOffDate() const noexcept
{
    return pImpl->mHaveOnOffDate;
}

/// Last modified
void ChannelData::setLoadDate(const Time::UTC &loadDate) noexcept
{
    pImpl->mLoadDate = loadDate;
    pImpl->mHaveLoadDate = true;
}

Time::UTC ChannelData::getLoadDate() const 
{
    if (!haveLoadDate())
    {
        throw std::runtime_error("Load date not set");
    }
    return pImpl->mLoadDate;
}

bool ChannelData::haveLoadDate() const noexcept
{
    return pImpl->mHaveLoadDate;
}

/// Reset class
void ChannelData::clear() noexcept
{
    pImpl = std::make_unique<ChannelDataImpl> ();
}

/*
///--------------------------------------------------------------------------///
///                             Table                                        ///
///--------------------------------------------------------------------------///
class ChannelDataTable::ChannelDataTableImpl
{
public:
    ChannelDataTableImpl(std::shared_ptr<UMPS::Logging::ILog> logger)
    {
        if (logger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StdOut> ();
        }
        else
        {
            mLogger = logger;
        }
    }
    URTS::ObserverPattern::Message update(
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
            return URTS::ObserverPattern::Message::UPDATE;
        }
        return URTS::ObserverPattern::Message::NO_CHANGE;
    }
    std::vector<ChannelData> getData() const
    {
        std::scoped_lock lock(mMutex);
        return mChannelData;
    }
    std::vector<ChannelData>
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
    std::shared_ptr<URTS::Database::Connection::PostgreSQL>
        mConnection{nullptr};
    std::vector<ChannelData> mChannelData;
    Time::UTC mMostRecentLoadDate{"1900-01-01T00:00:00"};
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
    std::shared_ptr<URTS::Database::Connection::PostgreSQL> &connection)
{
    pImpl->mConnection = connection;
    pImpl->mConnected = true;
}

/// Connected?
bool ChannelDataTable::isConnected() const noexcept
{
    return pImpl->mConnected;
}

/// Query
void ChannelDataTable::queryAll()
{
    if (!isConnected()){throw std::runtime_error("Class not connected");}
    pImpl->mLogger->debug("Checking for new channel_date information...");
    auto session = pImpl->mConnection->getSession();
    // Do I really need an update?
    auto mostRecentLoadDate = pImpl->mMostRecentLoadDate.getEpoch();
    bool doQuery = true;
    soci::rowset<int>
       rowCount(session->prepare << "SELECT COUNT(*) FROM channel_data WHERE "
                                 << "lddate > TO_TIMESTAMP("
                                 << std::to_string(mostRecentLoadDate)
                                 << ") AT TIME ZONE 'UTC'");
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
        this->notify(URTS::ObserverPattern::Message::NO_CHANGE);
        return;
    }
    // Yeah, let's query
    pImpl->mLogger->debug("Querying channel_data information...");
    pImpl->mChannelData.clear();
    soci::rowset<ChannelData>
        rows(session->prepare << "SELECT * FROM channel_data");
    std::vector<ChannelData> data;
    data.reserve(pImpl->mChannelSpaceEstimate);
    for (auto &it : rows)
    {
        // Plus 1 deals with truncation of microseconds
        pImpl->mMostRecentLoadDate = std::max(pImpl->mMostRecentLoadDate,
                                              it.getLoadDate() + 1);
        data.push_back(it);
    }
    auto message = pImpl->update(data, "all");
    pImpl->mChannelSpaceEstimate
         = std::max(pImpl->mChannelSpaceEstimate,
                    static_cast<int> (pImpl->mChannelData.size()));
    this->notify(message);
}

void ChannelDataTable::queryCurrent()
{
    if (!isConnected()){throw std::runtime_error("Class not connected");}
    pImpl->mLogger->debug("Checking for new channel_date information...");
    Time::UTC now;
    now.now();
    auto currentTime = static_cast<int64_t> (now.getEpoch());
    auto session = pImpl->mConnection->getSession();
    // Do I really need an update?
    auto mostRecentLoadDate = pImpl->mMostRecentLoadDate.getEpoch();
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
        this->notify(URTS::ObserverPattern::Message::NO_CHANGE);
        return;
    }
    // Yeah, let's actually query
    pImpl->mLogger->debug("Querying channel_data information...");
    pImpl->mChannelData.clear();
    soci::rowset<ChannelData>
        rows(session->prepare << "SELECT * FROM channel_data WHERE "
                              << "TO_TIMESTAMP("
                              << std::to_string(currentTime)
                              << ") AT TIME ZONE 'UTC'"
                              << " BETWEEN ondate AND offdate");
    std::vector<ChannelData> data;
    data.reserve(pImpl->mChannelSpaceEstimate);
    for (auto &it : rows)
    {
        // Plus 1 deals with truncation on microseconds
        pImpl->mMostRecentLoadDate = std::max(pImpl->mMostRecentLoadDate,
                                              it.getLoadDate() + 1);
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


///-------------------------------------------------------------------------///
///                                Service                                  ///
///-------------------------------------------------------------------------///
class ChannelDataTableService::ChannelDataTableServiceImpl
{
public:
    ChannelDataTableServiceImpl(std::shared_ptr<UMPS::Logging::ILog> logger)
    {
        if (logger == nullptr)
        {
            mLogger = std::make_shared<UMPS::Logging::StdOut> ();
        }
        else
        {
            mLogger = logger;
        }
        mChannelDataTable = std::make_unique<ChannelDataTable> (mLogger);
    }
    ~ChannelDataTableServiceImpl()
    {
        stop();
    }
    // Stops the service
    void stop()
    {
        setKeepRunning(false);
        if (mUpdaterThread.joinable())
        {
            mUpdaterThread.join();
        }
    } 
    // Starts the service
    void start()
    {
        stop();
        setKeepRunning(true);
        mUpdaterThread = std::thread(&ChannelDataTableServiceImpl::update,
                                     this); 
    }
    // Update
    void update()
    {
        while (keepRunning())
        {
            auto now = std::chrono::system_clock::now().time_since_epoch();
            auto nowSeconds
                = std::chrono::duration_cast<std::chrono::seconds> (now);
            auto nextQueryTime = mLastQuery + mRefreshRate;
            if (nowSeconds > nextQueryTime)
            {
                mLogger->debug("Querying channel data...");
                if (mQueryMode == QueryMode::CURRENT)
                {
                    mChannelDataTable->queryCurrent();
                }
                else
                {
                    mChannelDataTable->queryAll();
                }
                mLastQuery = nowSeconds;
            }
            std::this_thread::sleep_for (std::chrono::seconds(1));
        }
    }
    // Keep running?
    [[nodiscard]] bool keepRunning() const noexcept
    {
        std::scoped_lock lock(mMutex);
        return mKeepRunning;
    }
    // Determines whether this should / should not keep running
    void setKeepRunning(const bool running)
    {
        std::scoped_lock lock(mMutex);
        mKeepRunning = running;
    }
    void setConnection(
        std::shared_ptr<URTS::Database::Connection::PostgreSQL> &connection)
    {
        mChannelDataTable->setConnection(connection);
        mConnected = mChannelDataTable->isConnected();
    }
    bool isConnected() const noexcept
    {
        std::scoped_lock lock(mMutex);
        return mConnected;
    }
    void setOptions(const std::chrono::seconds &refreshRate,
                    const QueryMode mode)
    {
        std::scoped_lock lock(mMutex);
        mRefreshRate = refreshRate;
        mQueryMode = mode;
    }
    std::vector<ChannelData>
        getChannelData(const std::string &network,
                       const std::string &station,
                       const std::string &channel,
                       const std::string &locationCode) const
    {
        std::scoped_lock lock(mMutex);
        return mChannelDataTable->getChannelData(network, station,
                                                 channel, locationCode);
    }
///private:
    mutable std::mutex mMutex;
    std::unique_ptr<ChannelDataTable> mChannelDataTable{nullptr};
    std::shared_ptr<UMPS::Logging::ILog> mLogger;
    std::chrono::seconds mRefreshRate{3600};
    std::chrono::seconds mLastQuery;
    std::thread mUpdaterThread;
    QueryMode mQueryMode = QueryMode::CURRENT;
    bool mKeepRunning = false;
    bool mConnected = false;
};

/// C'tor
ChannelDataTableService::ChannelDataTableService() :
    pImpl(std::make_unique<ChannelDataTableServiceImpl> (nullptr))
{
}

ChannelDataTableService::ChannelDataTableService(
    std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ChannelDataTableServiceImpl> (logger))
{
}

/// Destructor
ChannelDataTableService::~ChannelDataTableService() = default;

/// Set connection
void ChannelDataTableService::setConnection(
    std::shared_ptr<URTS::Database::Connection::PostgreSQL> &connection)
{
    stop();
    pImpl->setConnection(connection); 
}

/// Connected?
bool ChannelDataTableService::isConnected() const noexcept
{
    return pImpl->isConnected();
}

/// Stop the service
void ChannelDataTableService::stop()
{
    pImpl->stop();
}

/// Starts the service
void ChannelDataTableService::start(
    const std::chrono::seconds &refreshRate,
    const QueryMode mode)
{
    if (!isConnected()){throw std::runtime_error("Not connected");}
    if (refreshRate.count() < 0)
    {
        throw std::invalid_argument("Refresh rate must be positive");
    }
    pImpl->setOptions(refreshRate, mode);
    pImpl->start();
}

/// Find
std::vector<ChannelData> ChannelDataTableService::getChannelData(
    const std::string &network,
    const std::string &station,
    const std::string &channel,
    const std::string &locationCode) const
{
    return pImpl->getChannelData(network, station, channel, locationCode);
}
*/
