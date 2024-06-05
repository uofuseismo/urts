#include <cmath>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#ifndef NDEBUG
#include <cassert>
#endif
#include "urts/services/scalable/associators/massociate/arrival.hpp"
#include "private/isEmpty.hpp"

using namespace URTS::Services::Scalable::Associators::MAssociate;

class Arrival::ArrivalImpl
{
public:
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    std::chrono::microseconds mTime{0};
    uint64_t mIdentifier{0};
    double mTravelTime{0};
    Phase mPhase;
    bool mHaveTime{false};
    bool mHaveTravelTime{false};
    bool mHaveIdentifier{false};
    bool mHavePhase{false};
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

/// Move constructor
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

/// Reset the class 
void Arrival::clear() noexcept
{
    pImpl = std::make_unique<ArrivalImpl> ();
}

/// Network
void Arrival::setNetwork(const std::string &network)
{
    if (::isEmpty(network)){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = network;
}

std::string Arrival::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set yet");}
    return pImpl->mNetwork;
}

bool Arrival::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void Arrival::setStation(const std::string &station)
{
    if (::isEmpty(station)){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = station;
}

std::string Arrival::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set yet");}
    return pImpl->mStation;
}

bool Arrival::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void Arrival::setChannel(const std::string &channel)
{
    if (::isEmpty(channel)){throw std::invalid_argument("Channel is empty");}
    pImpl->mChannel = channel;
}

std::string Arrival::getChannel() const
{
    if (!haveChannel()){throw std::runtime_error("Channel not set yet");}
    return pImpl->mChannel;
}

bool Arrival::haveChannel() const noexcept
{
    return !pImpl->mChannel.empty();
}

/// Location code
void Arrival::setLocationCode(const std::string &location)
{
    if (::isEmpty(location))
    {
        throw std::invalid_argument("Location code is empty");
    }
    pImpl->mLocationCode = location;
}

std::string Arrival::getLocationCode() const
{
    if (!haveLocationCode())
    {
        throw std::runtime_error("Location code not set yet");
    }
    return pImpl->mLocationCode;
}

bool Arrival::haveLocationCode() const noexcept
{
    return !pImpl->mLocationCode.empty();
}

/// Arrival time
void Arrival::setTime(const double time) noexcept
{
    auto arrivalTime
       = std::chrono::microseconds{static_cast<int64_t>
         (std::round(time*1.e6))};
    setTime(arrivalTime);
}

void Arrival::setTime(const std::chrono::microseconds &time) noexcept
{
    pImpl->mTime = time;
    pImpl->mHaveTime = true;
}

std::chrono::microseconds Arrival::getTime() const
{
    if (!haveTime()){throw std::runtime_error("Time not yet set");}
    return pImpl->mTime;
}

bool Arrival::haveTime() const noexcept
{
    return pImpl->mHaveTime;
}

/// Arrival identifier
void Arrival::setIdentifier(const uint64_t id) noexcept
{
    pImpl->mIdentifier = id;
    pImpl->mHaveIdentifier = true;
}

uint64_t Arrival::getIdentifier() const
{
    if (!haveIdentifier()){throw std::runtime_error("Identifier not yet set");}
    return pImpl->mIdentifier;
}

bool Arrival::haveIdentifier() const noexcept
{
    return pImpl->mHaveIdentifier;
}

/// Phase
void Arrival::setPhase(const Phase phase) noexcept
{
    pImpl->mPhase = phase;
    pImpl->mHavePhase = true;
}

Arrival::Phase Arrival::getPhase() const
{
    if (!havePhase()){throw std::runtime_error("Phase not set");}
    return pImpl->mPhase;
}

bool Arrival::havePhase() const noexcept
{
    return pImpl->mHavePhase;
}

/// Travel time
void Arrival::setTravelTime(double travelTime)
{
    if (travelTime < 0)
    {
        throw std::invalid_argument("Travel time must be positive");
    }
    pImpl->mTravelTime = travelTime;
    pImpl->mHaveTravelTime = true;
}

std::optional<double> Arrival::getTravelTime() const noexcept
{
    if (pImpl->mHaveTravelTime)
    {
        return pImpl->mTravelTime;
    }
    return std::nullopt;
}
