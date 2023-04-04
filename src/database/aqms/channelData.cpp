#include <string>
#include <chrono>
#include <nlohmann/json.hpp>
#include "urts/database/aqms/channelData.hpp"
#include "urts/database/connection/postgresql.hpp"
#include "utilities.hpp"

using namespace URTS::Database::AQMS;

/// Compares channel data
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
///                          Channel Data                                    ///
///--------------------------------------------------------------------------///
class ChannelData::ChannelDataImpl
{
public:
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    std::chrono::microseconds mOnDate{0};
    std::chrono::microseconds mOffDate{0};
    std::chrono::microseconds mLoadDate{0};
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
void ChannelData::setOnOffDate(
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

std::chrono::microseconds ChannelData::getOnDate() const
{
    if (!haveOnOffDate()){throw std::runtime_error("On/off date not set");}
    return pImpl->mOnDate;
}

std::chrono::microseconds ChannelData::getOffDate() const
{
    if (!haveOnOffDate()){throw std::runtime_error("On/off date not set");}
    return pImpl->mOffDate;
}

bool ChannelData::haveOnOffDate() const noexcept
{
    return pImpl->mHaveOnOffDate;
}

/// Last modified
void ChannelData::setLoadDate(
    const std::chrono::microseconds &loadDateMuS) noexcept
{
    pImpl->mLoadDate = loadDateMuS;
    pImpl->mHaveLoadDate = true;
}

std::chrono::microseconds ChannelData::getLoadDate() const 
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

/// Print
std::ostream& 
URTS::Database::AQMS::operator<<(std::ostream &os, const ChannelData &data)
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
    if (data.haveChannel())
    {
        obj["Channel"] = data.getChannel();
    }   
    if (data.haveLocationCode())
    {   
        obj["LocationCode"] = data.getLocationCode();
    }
    if (data.haveSamplingRate())
    {
        obj["SamplingRate"] = data.getSamplingRate();
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
    if (data.haveDip())
    {
        obj["Dip"] = data.getDip();
    }
    if (data.haveAzimuth())
    {
        obj["Azimuth"] = data.getAzimuth();
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

