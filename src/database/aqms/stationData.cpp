#include <string>
#include <chrono>
#include <nlohmann/json.hpp>
#include "urts/database/aqms/stationData.hpp"
#include "urts/database/connection/postgresql.hpp"
#include "utilities.hpp"

using namespace URTS::Database::AQMS;

/// Compares station data
bool URTS::Database::AQMS::operator==(
    const StationData &lhs, const StationData &rhs)
{
    constexpr double tol = 1.e-7;
    if (lhs.haveNetwork() && rhs.haveNetwork())
    {
        if (lhs.getNetwork() != rhs.getNetwork()){return false;}
    }   
    else
    {
        if (lhs.haveNetwork() != rhs.haveNetwork()){return false;}
    }   

    if (lhs.haveStation() && rhs.haveStation())
    {
        if (lhs.getStation() != rhs.getStation()){return false;}
    }   
    else
    {
        if (lhs.haveStation() != rhs.haveStation()){return false;}
    }   

    if (lhs.haveOnOffDate() && rhs.haveOnOffDate())
    {
        if (lhs.getOnDate()  != rhs.getOnDate()){return false;}
        if (lhs.getOffDate() != rhs.getOffDate()){return false;}
    }   
    else
    {
        if (lhs.haveOnOffDate() != rhs.haveOnOffDate()){return false;}
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
        if (lhs.haveLatitude() != rhs.haveLatitude()){return false;}
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
        if (lhs.haveLongitude() != rhs.haveLongitude()){return false;}
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
        if (lhs.haveElevation() != rhs.haveElevation()){return false;}
    }

    if (lhs.getDescription() != rhs.getDescription()){return false;}

    if (lhs.haveOnOffDate() && rhs.haveOnOffDate())
    {
        if (lhs.getOnDate()  != rhs.getOnDate()){return false;}
        if (lhs.getOffDate() != rhs.getOffDate()){return false;}
    }
    else
    {
        if (lhs.haveOnOffDate() != rhs.haveOnOffDate()){return false;}
    }

    if (lhs.haveLoadDate() && rhs.haveLoadDate())
    {
        if (lhs.getLoadDate() != rhs.getLoadDate()){return false;}
    }
    else
    {
        if (lhs.haveLoadDate() != rhs.haveLoadDate()){return false;}
    }

    return true;
}

class StationData::StationDataImpl
{
public:
    std::string mNetwork;
    std::string mStation;
    std::string mDescription;
    std::chrono::microseconds mOnDate{0};
    std::chrono::microseconds mOffDate{0};
    std::chrono::microseconds mLoadDate{0};
    double mLatitude{0};
    double mLongitude{0};
    double mElevation{0};
    bool mHaveOnOffDate{false};
    bool mHaveLatitude{false};
    bool mHaveLongitude{false};
    bool mHaveElevation{false};
    bool mHaveLoadDate{false};
};

/// Constructor
StationData::StationData() :
    pImpl(std::make_unique<StationDataImpl> ()) 
{
}

/// Copy constructor
StationData::StationData(const StationData &stationData)
{
    *this = stationData;
}

/// Move constructor
StationData::StationData(StationData &&stationData) noexcept
{
    *this = std::move(stationData);
}

/// Copy assignment
StationData& StationData::operator=(const StationData &data)
{
    if (&data == this){return *this;}
    pImpl = std::make_unique<StationDataImpl> (*data.pImpl);
    return *this;
}

/// Move assignment
StationData& StationData::operator=(StationData &&data) noexcept
{
    if (&data == this){return *this;}
    pImpl = std::move(data.pImpl);
    return *this;
}

/// Destructor
StationData::~StationData() = default;

/// Network
void StationData::setNetwork(const std::string &network)
{
    if (network.empty()){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string StationData::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set");}
    return pImpl->mNetwork;
}

bool StationData::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void StationData::setStation(const std::string &station)
{
    if (station.empty()){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = station;
}

std::string StationData::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set");}
    return pImpl->mStation;
}

bool StationData::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Description
void StationData::setDescription(const std::string &description) noexcept
{
    pImpl->mDescription = description;
}

std::string StationData::getDescription() const noexcept
{
    return pImpl->mDescription;
}

/// Latitude
void StationData::setLatitude(const double latitude)
{
    if (latitude < -90 || latitude > 90)
    {
        throw std::invalid_argument("Latitude must be in [-90,90]");
    }
    pImpl->mLatitude = latitude;
    pImpl->mHaveLatitude = true;
}

double StationData::getLatitude() const
{
    if (!haveLatitude()){throw std::runtime_error("Latitude not set");}
    return pImpl->mLatitude;
}

bool StationData::haveLatitude() const noexcept
{
    return pImpl->mHaveLatitude;
}

/// Longitude
void StationData::setLongitude(const double lonIn)
{
    pImpl->mLongitude = ::lonTo180(lonIn);
    pImpl->mHaveLongitude = true;
}

double StationData::getLongitude() const
{
    if (!haveLongitude()){throw std::runtime_error("Longitude not set");}
    return pImpl->mLongitude;
}

bool StationData::haveLongitude() const noexcept
{
    return pImpl->mHaveLongitude;
}

/// Elevation
void StationData::setElevation(const double elevation) noexcept
{
    pImpl->mElevation = elevation;
    pImpl->mHaveElevation = true;
}

double StationData::getElevation() const
{
    if (!haveElevation()){throw std::runtime_error("Elevation not set");}
    return pImpl->mElevation;
}

bool StationData::haveElevation() const noexcept
{
    return pImpl->mHaveElevation;
}

/// On/off date
void StationData::setOnOffDate(
    const std::pair<std::chrono::microseconds,
                    std::chrono::microseconds> &onOffDate)
{
    if (onOffDate.first >= onOffDate.second)
    {
        throw std::invalid_argument(
            "onOffDate.first must be less than onOffDate.second");
    }
    pImpl->mOnDate = onOffDate.first;
    pImpl->mOffDate = onOffDate.second;
    pImpl->mHaveOnOffDate = true;
}

std::chrono::microseconds StationData::getOnDate() const
{
    if (!haveOnOffDate()){throw std::runtime_error("On/off date not set");}
    return pImpl->mOnDate;
}

std::chrono::microseconds StationData::getOffDate() const
{
    if (!haveOnOffDate()){throw std::runtime_error("On/off date not set");}
    return pImpl->mOffDate;
}

bool StationData::haveOnOffDate() const noexcept
{
    return pImpl->mHaveOnOffDate;
}

/// Last modified
void StationData::setLoadDate(
    const std::chrono::microseconds &loadDate) noexcept
{
    pImpl->mLoadDate = loadDate;
    pImpl->mHaveLoadDate = true;
}

std::chrono::microseconds StationData::getLoadDate() const
{
    if (!haveLoadDate())
    {
        throw std::runtime_error("Load date not set");
    }
    return pImpl->mLoadDate;
}

bool StationData::haveLoadDate() const noexcept
{
    return pImpl->mHaveLoadDate;
}

/// Reset class
void StationData::clear() noexcept
{
    pImpl = std::make_unique<StationDataImpl> ();
}

std::ostream&
URTS::Database::AQMS::operator<<(std::ostream &os, const StationData &data)
{
    nlohmann::json obj;
    if (data.haveNetwork())
    {
        obj["Network"] = data.getNetwork();
    }
    if (data.haveStation())
    {
        obj["Station"] = data.getStation();
    }
    if (data.haveLatitude())
    {
        obj["Latitude"] = data.getLatitude();
    }
    if (data.haveLongitude())
    {
        obj["Longitude"] = data.getLongitude();
    }
    if (data.haveElevation())
    {
        obj["Elevation"] = data.getElevation();
    }
    auto description = data.getDescription();
    if (!description.empty())
    {
       obj["Description"] = description;
    }
    if (data.haveOnOffDate())
    {
        obj["OnDate"] = ::formatTimeUTC(data.getOnDate());
        obj["OffDate"] = ::formatTimeUTC(data.getOffDate());
    }
    if (data.haveLoadDate())
    {
       obj["LoadDate"] = ::formatTimeUTC(data.getLoadDate());
    }
    return os << obj.dump(4);
}
