#include <string>
#include <boost/format.hpp>
#include "urts/database/aqms/assocaro.hpp"
#include "utilities.hpp"

using namespace URTS::Database::AQMS;

class AssocArO::AssocArOImpl
{
public:
    std::string mAuthority;
    std::string mSubSource;
    std::string mPhase;
    int64_t mOriginIdentifier;
    int64_t mArrivalIdentifier;
    double mSourceReceiverDistance{-1};
    double mSourceReceiverAzimuth{-1};
    double mInputWeight{-1};
    double mTakeOffAngle;
    double mTravelTimeResidual;
    AssocArO::ReviewFlag mReviewFlag;
    bool mHaveOriginIdentifier{false};
    bool mHaveArrivalIdentifier{false};
    bool mHaveReviewFlag{false};
    bool mHaveTakeOffAngle{false};
    bool mHaveTravelTimeResidual{false};
};

/// Constructor
AssocArO::AssocArO() :
    pImpl(std::make_unique<AssocArOImpl> ())
{
}

/// Copy constructor
AssocArO::AssocArO(const AssocArO &assocaro)
{
    *this = assocaro;
}

/// Move constructor
AssocArO::AssocArO(AssocArO &&assocaro) noexcept
{
    *this = std::move(assocaro);
}

/// Copy assignment
AssocArO& AssocArO::operator=(const AssocArO &assocaro)
{
    if (&assocaro == this){return *this;}
    pImpl = std::make_unique<AssocArOImpl> (*assocaro.pImpl);
    return *this;
}

/// Move assignment
AssocArO& AssocArO::operator=(AssocArO &&assocaro) noexcept
{
    if (&assocaro == this){return *this;}
    pImpl = std::move(assocaro.pImpl);
    return *this;
}

/// Destructor
AssocArO::~AssocArO() = default;

/// Reset the class
void AssocArO::clear() noexcept
{
    pImpl = std::make_unique<AssocArOImpl> ();
}

/// Authority
void AssocArO::setAuthority(const std::string &authorityIn)
{
    auto authority = ::convertString(authorityIn);
    if (authority.empty()){throw std::invalid_argument("Authority is empty");}
    if (authority.size() > 15) 
    {   
        throw std::invalid_argument("Authority must be 15 characters or less");
    }   
    pImpl->mAuthority = authority;
}

std::string AssocArO::getAuthority() const
{
    if (!haveAuthority()){throw std::runtime_error("Authority not set");}
    return pImpl->mAuthority;
}

bool AssocArO::haveAuthority() const noexcept
{
    return !pImpl->mAuthority.empty();
}


/// Origin identifier
void AssocArO::setOriginIdentifier(const int64_t identifier) noexcept
{
    pImpl->mOriginIdentifier = identifier;
    pImpl->mHaveOriginIdentifier = true;
}

int64_t AssocArO::getOriginIdentifier() const
{
    if (!haveOriginIdentifier())
    {
        throw std::runtime_error("Origin identifier not set");
    }
    return pImpl->mOriginIdentifier;
}

bool AssocArO::haveOriginIdentifier() const noexcept
{
    return pImpl->mHaveOriginIdentifier;
}

/// AssocArO identifier
void AssocArO::setArrivalIdentifier(const int64_t identifier) noexcept
{
    pImpl->mArrivalIdentifier = identifier;
    pImpl->mHaveArrivalIdentifier = true;
}

int64_t AssocArO::getArrivalIdentifier() const
{
    if (!haveArrivalIdentifier())
    {   
        throw std::runtime_error("Arrival identifier not set");
    }   
    return pImpl->mArrivalIdentifier;
}

bool AssocArO::haveArrivalIdentifier() const noexcept
{
    return pImpl->mHaveArrivalIdentifier;
}

/// Review flag
void AssocArO::setReviewFlag(const AssocArO::ReviewFlag reviewFlag) noexcept
{
    pImpl->mReviewFlag = reviewFlag;
    pImpl->mHaveReviewFlag = true;
}

std::optional<AssocArO::ReviewFlag> AssocArO::getReviewFlag() const noexcept
{
    return pImpl->mHaveReviewFlag ?
           std::optional<AssocArO::ReviewFlag> {pImpl->mReviewFlag} :
           std::nullopt;
}

/// Subsource
void AssocArO::setSubSource(const std::string &subSource)
{
    if (subSource.size() > 8)
    {   
        throw std::invalid_argument("Sub-source must be 8 characters or less");
    }   
    pImpl->mSubSource = subSource;
}

std::optional<std::string> AssocArO::getSubSource() const noexcept
{
    return !pImpl->mSubSource.empty() ?
           std::optional<std::string> {pImpl->mSubSource} :
           std::nullopt;    
}

/// Source receiver distance
void AssocArO::setSourceReceiverDistance(const double distance)
{
    if (distance < 0){throw std::invalid_argument("Distance must be positive");}
    pImpl->mSourceReceiverDistance = distance;
}

std::optional<double> AssocArO::getSourceReceiverDistance() const noexcept
{
    return (pImpl->mSourceReceiverDistance >= 0) ?
           std::optional<double> {pImpl->mSourceReceiverDistance} :
           std::nullopt;
}

/// Azimuth
void AssocArO::setSourceToReceiverAzimuth(const double azimuth)
{
    if (azimuth < 0 || azimuth >= 360)
    {
        throw std::invalid_argument("Azimuth must be in range [0,360)");
    }
    pImpl->mSourceReceiverAzimuth = azimuth;
}

std::optional<double> AssocArO::getSourceToReceiverAzimuth() const noexcept
{
    return (pImpl->mSourceReceiverAzimuth >= 0) ?
           std::optional<double> {pImpl->mSourceReceiverAzimuth} :
           std::nullopt;
}

/// Phase
void AssocArO::setPhase(const std::string &phaseIn)
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

std::optional<std::string> AssocArO::getPhase() const noexcept
{
    return !pImpl->mPhase.empty() ?
           std::optional<std::string> {pImpl->mPhase} : std::nullopt;
}

/// Azimuth
void AssocArO::setInputWeight(const double weight)
{
    if (weight < 0)
    {
        throw std::invalid_argument("Weight must be non-negative");
    }
    pImpl->mInputWeight = weight;
}

std::optional<double> AssocArO::getInputWeight() const noexcept
{   
    return (pImpl->mInputWeight >= 0) ?
           std::optional<double> {pImpl->mInputWeight} :
           std::nullopt;
}

/// Take off angle
void AssocArO::setTakeOffAngle(const double angle)
{
    if (angle < 0 || angle > 180)
    {
        throw std::invalid_argument("Takeoff angle must be in range [0,180]");
    }
    pImpl->mTakeOffAngle = angle;
    pImpl->mHaveTakeOffAngle = true;
}

std::optional<double> AssocArO::getTakeOffAngle() const noexcept
{
    return pImpl->mHaveTakeOffAngle ?
           std::optional<double> {pImpl->mTakeOffAngle} :
           std::nullopt;
}

/// Travel time residual
void AssocArO::setTravelTimeResidual(const double residual) noexcept
{
    pImpl->mTravelTimeResidual = residual;
    pImpl->mHaveTravelTimeResidual = true;
}

std::optional<double> AssocArO::getTravelTimeResidual() const noexcept
{
    return pImpl->mHaveTravelTimeResidual ?
           std::optional<double> {pImpl->mTravelTimeResidual} :
           std::nullopt;
}

/// Insertion string
std::string URTS::Database::AQMS::toInsertString(const AssocArO &assocaro)
{
    if (!assocaro.haveAuthority())
    {
        throw std::invalid_argument("Authority not set");
    }   
    if (!assocaro.haveOriginIdentifier())
    {
        throw std::invalid_argument("Origin identifier not set");
    }
    if (!assocaro.haveArrivalIdentifier())
    {   
        throw std::invalid_argument("Arrival identifier not set");
    }   
    std::string insertStatement{"INSERT INTO assocaro "};

    std::string keys = {"(auth, orid, arid,"};
    std::string values
        = str(boost::format(" VALUES ('%1%', %2$d, %3$d,")
              %assocaro.getAuthority()
              %assocaro.getOriginIdentifier()
              %assocaro.getArrivalIdentifier()
             );

    if (assocaro.getSubSource())
    {   
        keys = keys + " subsource,";
        values = values + str(boost::format(" '%1%',")% *assocaro.getSubSource() );  
    }   
    if (assocaro.getPhase())
    {
        keys = keys + " iphase,";
        values = values + str(boost::format(" '%1%',")% *assocaro.getPhase() );
    }
    if (assocaro.getSourceReceiverDistance())
    {   
        keys = keys + " delta,";
        values = values + str(boost::format(" %1$.3f,")% *assocaro.getSourceReceiverDistance() );
    } 
    if (assocaro.getSourceToReceiverAzimuth())
    {
        keys = keys + " seaz,";
        values = values + str(boost::format(" %1$.3f,")% *assocaro.getSourceToReceiverAzimuth() );
    }  
    if (assocaro.getInputWeight())
    {
        keys = keys + " in_wgt,";
        values = values + str(boost::format(" %1$.6f,")% *assocaro.getInputWeight() );
    }
    if (assocaro.getTravelTimeResidual())
    {   
        keys = keys + " timeres,";
        values = values + str(boost::format(" %1$.6f,")% *assocaro.getTravelTimeResidual() );
    }
    if (assocaro.getReviewFlag())
    {
        if (*assocaro.getReviewFlag() == AssocArO::ReviewFlag::Automatic)
        {
            keys = keys + " rflag,";
            values = values + " 'A',";
        }
        else if (*assocaro.getReviewFlag() == AssocArO::ReviewFlag::Human)
        {
            keys = keys + " rflag,";
            values = values + " 'H',";
        }
        else if (*assocaro.getReviewFlag() == AssocArO::ReviewFlag::Finalized)
        {
            keys = keys + " rflag,";
            values = values + " 'F',";
        }
    }

    // Delete the trailing comma and make it a ")"
    keys.back() = ')';
    values.back()= ')';
    // Put it all together
    insertStatement = insertStatement + keys + values + ";";
    return insertStatement;
}
