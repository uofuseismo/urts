#include <string>
#include "urts/database/aqms/origin.hpp"
#include "urts/database/aqms/arrival.hpp"
#include "utilities.hpp"

using namespace URTS::Database::AQMS;

class Origin::OriginImpl
{
public:
    std::string mAlgorithm;
    std::string mAuthority;
    std::string mSubSource;
    double mTime;
    double mLatitude;
    double mLongitude;
    double mDepth;
    double mGap;
    double mDistanceToNearestStation;
    double mWRMSE;
    int64_t mIdentifier;
    int64_t mEventIdentifier;
    int64_t mPreferredMagnitudeIdentifier;
    int64_t mPreferredMechanismIdentifier;
    ReviewFlag mReviewFlag;
    GeographicType mGeographicType;
    bool mHaveTime{false};
    bool mHaveLatitude{false};
    bool mHaveLongitude{false};
    bool mHaveDepth{false};
    bool mHaveIdentifier{false};
    bool mHaveEventIdentifier{false};
    bool mHavePreferredMagnitudeIdentifier{false};
    bool mHavePreferredMechanismIdentifier{false};
    bool mHaveReviewFlag{false};
    bool mHaveGeographicType{false};
    bool mBogusFlag{false};
    bool mHaveGap{false};
    bool mHaveDistanceToNearestStation{false};
    bool mHaveWRMSE{false};
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

/// Move construtor
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

/// Reset class 
void Origin::clear() noexcept
{
    pImpl = std::make_unique<OriginImpl> ();
}   
    
/// Destructor
Origin::~Origin() = default;

/// Bogus event?
void Origin::setBogus() noexcept
{
    pImpl->mBogusFlag = true;
}

void Origin::unsetBogus() noexcept
{
    pImpl->mBogusFlag = false;
}

bool Origin::isBogus() const noexcept
{
    return pImpl->mBogusFlag;
}

/// Identifier
void Origin::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
    pImpl->mHaveIdentifier = true;
}

int64_t Origin::getIdentifier() const
{
    if (!haveIdentifier()){throw std::runtime_error("Identifier not set");}
    return pImpl->mIdentifier;
}

bool Origin::haveIdentifier() const noexcept
{
    return pImpl->mHaveIdentifier;
}

/// Event Identifier
void Origin::setEventIdentifier(const int64_t identifier) noexcept
{
    pImpl->mEventIdentifier = identifier;
    pImpl->mHaveEventIdentifier = true;
}

int64_t Origin::getEventIdentifier() const
{
    if (!haveEventIdentifier())
    {
        throw std::runtime_error("Event identifier not set");
    }
    return pImpl->mEventIdentifier;
}

bool Origin::haveEventIdentifier() const noexcept
{
    return pImpl->mHaveEventIdentifier;
}

/// Magnitude identifier
void Origin::setPreferredMagnitudeIdentifier(const int64_t identifier) noexcept
{
    pImpl->mPreferredMagnitudeIdentifier = identifier;
    pImpl->mHavePreferredMagnitudeIdentifier = true;
}

std::optional<int64_t> Origin::getPreferredMagnitudeIdentifier() const noexcept
{
    return pImpl->mHavePreferredMagnitudeIdentifier ?
           std::optional<int64_t> {pImpl->mPreferredMagnitudeIdentifier} :
           std::nullopt;
}

/// Mechanism identifier
void Origin::setPreferredMechanismIdentifier(const int64_t identifier) noexcept
{
    pImpl->mPreferredMechanismIdentifier = identifier;
    pImpl->mHavePreferredMechanismIdentifier = true;
}

std::optional<int64_t> Origin::getPreferredMechanismIdentifier() const noexcept
{
    return pImpl->mHavePreferredMechanismIdentifier ?
           std::optional<int64_t> {pImpl->mPreferredMechanismIdentifier} :
           std::nullopt;
}

/// Review flag
void Origin::setReviewFlag(const Origin::ReviewFlag reviewFlag) noexcept
{
    pImpl->mReviewFlag = reviewFlag;
    pImpl->mHaveReviewFlag = true;
}

std::optional<Origin::ReviewFlag> Origin::getReviewFlag() const noexcept
{
    return pImpl->mHaveReviewFlag ?
           std::optional<Origin::ReviewFlag> {pImpl->mReviewFlag} :
           std::nullopt;
}

/// Geographic type
void Origin::setGeographicType(const Origin::GeographicType type) noexcept
{
    pImpl->mGeographicType = type;
    pImpl->mHaveGeographicType = true;
}

std::optional<Origin::GeographicType> Origin::getGeographicType() const noexcept
{
    return pImpl->mHaveGeographicType ?
           std::optional<Origin::GeographicType> {pImpl->mGeographicType} :
           std::nullopt;
}

/// Time
void Origin::setTime(const std::chrono::microseconds &time) noexcept
{
    setTime(time.count()*1.e-6);
}

void Origin::setTime(const double time) noexcept
{
    pImpl->mTime = time;
    pImpl->mHaveTime = true;
}

double Origin::getTime() const
{
    if (!haveTime()){throw std::runtime_error("Time not set");}
    return pImpl->mTime;
}

bool Origin::haveTime() const noexcept
{
    return pImpl->mHaveTime;
}

/// Latitude
void Origin::setLatitude(const double latitude)
{
    if (latitude < -90 || latitude > 90) 
    {   
        throw std::invalid_argument("Latitude must be in [-90,90]");
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
void Origin::setLongitude(const double lonIn) noexcept
{
    pImpl->mLongitude = ::lonTo180(lonIn);
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
void Origin::setDepth(const double depth)
{
    if (depth < -10 || depth > 1000)
    {
        throw std::invalid_argument("Depth must be in range [-10,1000]");
    }
    pImpl->mDepth = depth;
    pImpl->mHaveDepth = true;
}

std::optional<double> Origin::getDepth() const noexcept
{
    return pImpl->mHaveDepth ?
           std::optional<double> {pImpl->mDepth} : std::nullopt;
}

/// Authority
void Origin::setAuthority(const std::string &authorityIn)
{
    auto authority = ::convertString(authorityIn);
    if (authority.empty()){throw std::invalid_argument("Authority is empty");}
    if (authority.size() > 15) 
    {   
        throw std::invalid_argument("Authority must be 15 characters or less");
    }   
    pImpl->mAuthority = authority;
}

std::string Origin::getAuthority() const
{
    if (!haveAuthority()){throw std::runtime_error("Authority not set");}
    return pImpl->mAuthority;
}

bool Origin::haveAuthority() const noexcept
{
    return !pImpl->mAuthority.empty();
}

/// Subsource
void Origin::setSubSource(const std::string &subSource)
{
    if (subSource.size() > 8)
    {
        throw std::invalid_argument("Sub-source must be 8 characters or less");
    }
    pImpl->mSubSource = subSource;
}

std::optional<std::string> Origin::getSubSource() const noexcept
{
    return !pImpl->mSubSource.empty() ?
           std::optional<std::string> {pImpl->mSubSource} :
           std::nullopt;    
}

/// Algorithm
void Origin::setAlgorithm(const std::string &algorithm)
{
    if (algorithm.size() > 15)
    {   
        throw std::invalid_argument("Algorithm must be 15 characters or less");
    }   
    pImpl->mAlgorithm = algorithm;
}

std::optional<std::string> Origin::getAlgorithm() const noexcept
{
    return !pImpl->mAlgorithm.empty() ?
           std::optional<std::string> {pImpl->mAlgorithm} :
           std::nullopt;    
}

/// Gap
void Origin::setGap(const double gap)
{
    if (gap < 0 || gap > 360)
    {
        throw std::invalid_argument("Gap must be in range [0,360]");
    }
    pImpl->mGap = gap;
    pImpl->mHaveGap = true;
}

std::optional<double> Origin::getGap() const noexcept
{
    return pImpl->mHaveGap ? std::optional<double> {pImpl->mGap} : std::nullopt;
}

/// Distance
void Origin::setDistanceToNearestStation(const double distance)
{
    if (distance < 0)
    {
        throw std::invalid_argument("Distance cannot be negative");
    }
    pImpl->mDistanceToNearestStation = distance;
    pImpl->mHaveDistanceToNearestStation = true;
}

std::optional<double> Origin::getDistanceToNearestStation() const noexcept
{
    return pImpl->mHaveDistanceToNearestStation ?
           std::optional<double> {pImpl->mDistanceToNearestStation} :
           std::nullopt;
}

/// WRMSE
void Origin::setWeightedRootMeanSquaredError(const double wrmse)
{
    if (wrmse < 0)
    {
        throw std::invalid_argument("Weighted RMSE cannot be negative");
    }
    pImpl->mWRMSE = wrmse;
    pImpl->mHaveWRMSE = true;
}

std::optional<double> Origin::getWeightedRootMeanSquaredError() const noexcept
{
    return pImpl->mHaveWRMSE ?
           std::optional<double> {pImpl->mWRMSE} : std::nullopt;
}
