#include <cmath>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#ifndef NDEBUG
#include <cassert>
#endif
#include "urts/services/scalable/locators/uLocator/origin.hpp"
#include "urts/services/scalable/locators/uLocator/arrival.hpp"
#include "database/aqms/utilities.hpp"

using namespace URTS::Services::Scalable::Locators::ULocator;

class Origin::OriginImpl
{
public:
    std::vector<Arrival> mArrivals;
    std::chrono::microseconds mTime{0};
    double mLatitude{0};
    double mLongitude{0};
    double mDepth{0};
    int64_t mIdentifier{0};
    bool mHaveTime{false};
    bool mHaveLatitude{false};
    bool mHaveLongitude{false};
    bool mHaveDepth{false};
    bool mDepthFixedToFreeSurface{false};
    bool mHaveIdentifier{false};
};

/// Constructor
Origin::Origin() :
    pImpl(std::make_unique<OriginImpl> ())
{
}

/// Copy constructor
Origin::Origin(const Origin &origin)
{
    *this = origin;
}

/// Move constructor
Origin::Origin(Origin &&origin) noexcept
{
    *this = std::move(origin);
}

/// Copy assignment
Origin& Origin::operator=(const Origin &origin)
{
    if (&origin == this){return *this;}
    pImpl = std::make_unique<OriginImpl> (*origin.pImpl);
    return *this;
}

/// Move assignment
Origin& Origin::operator=(Origin &&origin) noexcept
{
    if (&origin == this){return *this;}
    pImpl = std::move(origin.pImpl);
    return *this;
}

/// Destructor
Origin::~Origin() = default;

/// Reset the class 
void Origin::clear() noexcept
{
    pImpl = std::make_unique<OriginImpl> ();
}

/// Latitude
void Origin::setLatitude(double latitude)
{
    if (latitude <-90 || latitude > 90)
    {
        throw std::invalid_argument("Latitude must be in range [-90,90]");
    }
    pImpl->mLatitude = latitude;
    pImpl->mHaveLatitude = true;
}

double Origin::getLatitude() const
{
    if (!haveLatitude()){throw std::runtime_error("Latitude not set");}
    return pImpl->mLatitude;
}

bool Origin::haveLatitude() const noexcept
{
    return pImpl->mHaveLatitude;
}

/// Longitude
void Origin::setLongitude(double longitude) noexcept
{
    pImpl->mLongitude = ::lonTo180(longitude);
    pImpl->mHaveLongitude = true;
}

double Origin::getLongitude() const
{
    if (!haveLongitude()){throw std::runtime_error("Longitude not set");}
    return pImpl->mLongitude;
}

bool Origin::haveLongitude() const noexcept
{
    return pImpl->mHaveLongitude;
}

/// Depth
void Origin::setDepth(double depth)
{
    if (depth < -8600 || depth > 800000)
    {
        throw std::invalid_argument("Depth must be in range [-8600 800000] m");
    }
    pImpl->mDepth = depth;
    pImpl->mHaveDepth = true;
}

double Origin::getDepth() const
{
    if (!haveDepth()){throw std::runtime_error("Depth not set");}
    return pImpl->mDepth;
}

bool Origin::haveDepth() const noexcept
{
    return pImpl->mHaveDepth;
}

void Origin::toggleDepthFixedToFreeSurface(const bool fixed) noexcept
{
    pImpl->mDepthFixedToFreeSurface = fixed;
}

bool Origin::depthFixedToFreeSurface() const noexcept
{
    return pImpl->mDepthFixedToFreeSurface;
}

/// Time
void Origin::setTime(const double time) noexcept
{
    auto originTime
       = std::chrono::microseconds{static_cast<int64_t>
         (std::round(time*1.e6))};
    setTime(originTime);
}

void Origin::setTime(const std::chrono::microseconds &time) noexcept
{
    pImpl->mTime = time;
    pImpl->mHaveTime = true;
}

std::chrono::microseconds Origin::getTime() const
{
    if (!haveTime()){throw std::runtime_error("Time not yet set");}
    return pImpl->mTime;
}

bool Origin::haveTime() const noexcept
{
    return pImpl->mHaveTime;
}

/// Identifier
void Origin::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
    pImpl->mHaveIdentifier = true;
}

std::optional<int64_t> Origin::getIdentifier() const noexcept
{
    return pImpl->mHaveIdentifier ?
           std::optional<int64_t> (pImpl->mIdentifier) : std::nullopt;
}

/// Arrivals
void Origin::setArrivals(const std::vector<Arrival> &arrivalsIn)
{
    auto arrivals = arrivalsIn;
    for (const auto &arrival : arrivals)
    {
        if (!arrival.haveNetwork())
        {
            throw std::invalid_argument("Network not set");
        }
        if (!arrival.haveStation())
        {
            throw std::invalid_argument("Station not set");
        }
        if (!arrival.havePhase()){throw std::invalid_argument("Phase not set");}
        if (!arrival.haveTime()){throw std::invalid_argument("Time not set");}
    }
    pImpl->mArrivals = std::move(arrivals);
}

std::vector<Arrival> Origin::getArrivals() const noexcept
{
    return pImpl->mArrivals;
}

const std::vector<Arrival> &Origin::getArrivalsReference() const noexcept
{
    return *&pImpl->mArrivals;
}

