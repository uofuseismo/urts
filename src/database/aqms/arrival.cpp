#include <string>
#include "urts/database/aqms/arrival.hpp"
#include "utilities.hpp"

using namespace URTS::Database::AQMS;

class Arrival::ArrivalImpl
{
public:
    double mTime;
    double mQuality{-1}; // Valid range is [0, 1]
    int64_t mIdentifier;
    std::string mAuthority;
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    std::string mPhase;
    std::string mSubSource;
    ReviewFlag mReviewFlag;
    FirstMotion mFirstMotion{FirstMotion::Unknown};
    bool mHaveIdentifier{false};
    bool mHaveLocationCode{false};
    bool mHaveReviewFlag{false};
    bool mHaveTime{false};
};

/// Constructor
Arrival::Arrival() :
    pImpl(std::make_unique<ArrivalImpl> ())
{
}

/// Copy constructor
Arrival::Arrival(const Arrival &arrival)
{
    *this = arrival;
}

/// Move construtor
Arrival::Arrival(Arrival &&arrival) noexcept
{
    *this = std::move(arrival);
}

/// Copy assignment
Arrival& Arrival::operator=(const Arrival &arrival)
{
    if (&arrival == this){return *this;}
    pImpl = std::make_unique<ArrivalImpl> (*arrival.pImpl);
    return *this;
}

/// Move assignment
Arrival& Arrival::operator=(Arrival &&arrival) noexcept
{
    if (&arrival == this){return *this;}
    pImpl = std::move(arrival.pImpl);
    return *this;
}

/// Destructor
Arrival::~Arrival() = default;

/// Authority
void Arrival::setAuthority(const std::string &authorityIn)
{
    auto authority = ::convertString(authorityIn);
    if (authority.empty()){throw std::invalid_argument("Authority is empty");}
    if (authority.size() > 15)
    {
        throw std::invalid_argument("Authority must be 15 characters or less");
    }
    pImpl->mAuthority = authority;
}

std::string Arrival::getAuthority() const
{
    if (!haveAuthority()){throw std::runtime_error("Authority not set");}
    return pImpl->mAuthority;
}

bool Arrival::haveAuthority() const noexcept
{
    return !pImpl->mAuthority.empty();
}

/// Station
void Arrival::setStation(const std::string &stationIn)
{
    auto station = ::convertString(stationIn);
    if (station.empty()){throw std::invalid_argument("Station is empty");}
    if (station.size() > 6)
    {
        throw std::invalid_argument("Station must be 6 characters or less");
    }
    pImpl->mStation = station;
}

std::string Arrival::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set");}
    return pImpl->mStation;
}

bool Arrival::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Time
void Arrival::setTime(const std::chrono::microseconds &time) noexcept
{
    setTime(time.count()*1.e-6);
}

void Arrival::setTime(const double time) noexcept
{
    pImpl->mTime = time;
    pImpl->mHaveTime = true;
}

double Arrival::getTime() const
{
    if (!haveTime()){throw std::runtime_error("Time not set");}
    return pImpl->mTime;
}

bool Arrival::haveTime() const noexcept
{
    return pImpl->mHaveTime;
}

/// Network
void Arrival::setNetwork(const std::string &networkIn)
{
    auto network = ::convertString(networkIn);
    if (network.empty()){throw std::invalid_argument("Network is empty");}
    if (network.size() > 8)
    {
        throw std::invalid_argument("Network must be 8 characters or less");
    }
    pImpl->mNetwork = network;
}

std::optional<std::string> Arrival::getNetwork() const noexcept
{
    return !pImpl->mNetwork.empty() ?
           std::optional<std::string> {pImpl->mNetwork} : std::nullopt;
}

/// SEED channel
void Arrival::setSEEDChannel(const std::string &channelIn)
{
    auto channel = ::convertString(channelIn);
    if (channel.empty()){throw std::invalid_argument("Channel is empty");}
    if (channel.size() > 3)
    {   
        throw std::invalid_argument(
            "SEED channel must be 3 characters or less");
    }   
    pImpl->mChannel = channel;
}

std::optional<std::string> Arrival::getSEEDChannel() const noexcept 
{
    return !pImpl->mChannel.empty() ?
           std::optional<std::string> {pImpl->mChannel} : std::nullopt;
}

/// Location code
void Arrival::setLocationCode(const std::string &locationCode)
{
    if (locationCode.size() > 2)
    {
        throw std::invalid_argument(
             "Location code must be 2 characters or less");
    }
    pImpl->mLocationCode = locationCode;
    pImpl->mHaveLocationCode = true;
}

std::optional<std::string> Arrival::getLocationCode() const noexcept 
{
    return pImpl->mHaveLocationCode ?
           std::optional<std::string> {pImpl->mLocationCode} : std::nullopt;
}

/// Phase
void Arrival::setPhase(const std::string &phaseIn)
{
    auto phase = phaseIn;
    std::remove_if(phase.begin(), phase.end(), ::isspace);
    if (phase.empty()){throw std::invalid_argument("Phase cannot be empty");}
    if (phase.size() > 8)
    {
        throw std::invalid_argument("Phase must be 8 characters or less");
    }
    pImpl->mPhase = phase;
}

std::optional<std::string> Arrival::getPhase() const noexcept
{
    return !pImpl->mPhase.empty() ?
           std::optional<std::string> {pImpl->mPhase} : std::nullopt;
}

/// Quality
void Arrival::setQuality(double quality)
{
    if (quality < 0 || quality > 1)
    {
        throw std::invalid_argument("Quality must be in range [0,1]");
    }
    pImpl->mQuality = quality;
}

std::optional<double> Arrival::getQuality() const noexcept
{
    return pImpl->mQuality > -1 ?
           std::optional<double> {pImpl->mQuality} : std::nullopt;
}

/// First motion
void Arrival::setFirstMotion(const Arrival::FirstMotion firstMotion) noexcept
{
    pImpl->mFirstMotion = firstMotion;
}

Arrival::FirstMotion Arrival::getFirstMotion() const noexcept
{
    return pImpl->mFirstMotion;
}

/// Identifier
void Arrival::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
    pImpl->mHaveIdentifier = true;
}

std::optional<int64_t> Arrival::getIdentifier() const noexcept
{
    return pImpl->mHaveIdentifier ?
           std::optional<int64_t> {pImpl->mIdentifier} : std::nullopt;
}

/// Review flag
void Arrival::setReviewFlag(const Arrival::ReviewFlag reviewFlag) noexcept
{
    pImpl->mReviewFlag = reviewFlag;
    pImpl->mHaveReviewFlag = true;
}

std::optional<Arrival::ReviewFlag> Arrival::getReviewFlag() const noexcept
{
    return pImpl->mHaveReviewFlag ?
           std::optional<Arrival::ReviewFlag> {pImpl->mReviewFlag} :
           std::nullopt;
}

/// Subsource
void Arrival::setSubSource(const std::string &subSource)
{
    pImpl->mSubSource = subSource;
}

std::optional<std::string> Arrival::getSubSource() const noexcept
{
    return !pImpl->mSubSource.empty() ?
           std::optional<std::string> {pImpl->mSubSource} :
           std::nullopt;       
}
