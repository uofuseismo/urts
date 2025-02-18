#include <string>
#include <boost/format.hpp>
#include "urts/database/aqms/event.hpp"
#include "utilities.hpp"

using namespace URTS::Database::AQMS;

namespace
{
[[nodiscard]] std::string typeToString(const Event::Type type)
{
    if (type == Event::Type::Unknown)
    {
        return "uk";
    }
    else if (type == Event::Type::Earthquake)
    {
        return "eq";
    }
    else if (type == Event::Type::QuarryBlast)
    {
        return "qb";
    }
    else if (type == Event::Type::SubnetTrigger)
    {
        return "st";
    }
    else if (type == Event::Type::MiningInduced)
    {

    }
    else if (type == Event::Type::Explosion ||
             type == Event::Type::ChemicalExplosion ||
             type == Event::Type::NuclearExplosion)
    {
        return "ex";
    }
    else if (type == Event::Type::NuclearTest)
    {
        return "nt";
    }
    else if (type == Event::Type::Sonic)             /*!< Sonic. */
    {
        return "sn";
    }
    else if (type == Event::Type::Avalanche)
    {
        return "av";
    }
    else if (type == Event::Type::Collapse)
    {
        return "co";
    }
    else if (type == Event::Type::Landslide) 
    {
        return "ls";
    }
    return "uk"; 
}

}

class Event::EventImpl
{
public:
    std::string mAuthority;
    std::string mSubSource;
    int64_t mIdentifier;
    int64_t mPreferredOriginIdentifier;
    int64_t mPreferredMechanismIdentifier;
    int64_t mPreferredMagnitudeIdentifier;
    int64_t mCommentIdentifier;
    int mVersion{0};
    Event::Type mType;
    bool mSelectFlag{false}; 
    bool mHaveVersion{false};
    bool mHaveIdentifier{false};
    bool mHaveType{false};
    bool mHavePreferredOriginIdentifier{false};
    bool mHavePreferredMagnitudeIdentifier{false};
    bool mHavePreferredMechanismIdentifier{false};
    bool mHaveCommentIdentifier{false};
};

/// Constructor
Event::Event() :
    pImpl(std::make_unique<EventImpl> ())
{
}

/// Copy constructor
Event::Event(const Event &event)
{
    *this = event;
}

/// Move constructor
Event::Event(Event &&event) noexcept
{
    *this = std::move(event);
}

/// Copy assignment
Event& Event::operator=(const Event &event)
{
    if (&event == this){return *this;}
    pImpl = std::make_unique<EventImpl> (*event.pImpl);
    return *this;
}

/// Move assignment
Event& Event::operator=(Event &&event) noexcept
{
    if (&event == this){return *this;}
    pImpl = std::move(event.pImpl);
    return *this;
}

/// Reset class 
void Event::clear() noexcept
{
    pImpl = std::make_unique<EventImpl> ();
}

/// Destructor
Event::~Event() = default;

/// Identifier
void Event::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
    pImpl->mHaveIdentifier = true;
}

int64_t Event::getIdentifier() const
{
    if (!haveIdentifier()){throw std::runtime_error("Identifier not set");}
    return pImpl->mIdentifier;
}

bool Event::haveIdentifier() const noexcept
{
    return pImpl->mHaveIdentifier;
}

/// Authority
void Event::setAuthority(const std::string &authorityIn)
{
    auto authority = ::convertString(authorityIn);
    if (authority.empty()){throw std::invalid_argument("Authority is empty");}
    if (authority.size() > 15)
    {   
        throw std::invalid_argument("Authority must be 15 characters or less");
    }   
    pImpl->mAuthority = authority;
}

std::string Event::getAuthority() const
{
    if (!haveAuthority()){throw std::runtime_error("Authority not set");}
    return pImpl->mAuthority;
}

bool Event::haveAuthority() const noexcept
{
    return !pImpl->mAuthority.empty();
}

/// Version
void Event::setVersion(const uint16_t version) noexcept
{
    pImpl->mVersion = version;
    pImpl->mHaveVersion = true;
}

int Event::getVersion() const
{
    if (!haveVersion()){throw std::runtime_error("Version not set");}
    return pImpl->mVersion;
}

bool Event::haveVersion() const noexcept
{
    return pImpl->mHaveVersion;
}

/// Subsource
void Event::setSubSource(const std::string &subSource)
{
    if (subSource.size() > 8)
    {   
        throw std::invalid_argument("Sub-source must be 8 characters or less");
    }   
    pImpl->mSubSource = subSource;
}

std::optional<std::string> Event::getSubSource() const noexcept
{
    return !pImpl->mSubSource.empty() ?
           std::optional<std::string> {pImpl->mSubSource} :
           std::nullopt;    
}

/// Origin identifier
void Event::setPreferredOriginIdentifier(const int64_t identifier) noexcept
{
    pImpl->mPreferredOriginIdentifier = identifier;
    pImpl->mHavePreferredOriginIdentifier = true;
}

std::optional<int64_t> Event::getPreferredOriginIdentifier() const noexcept
{
    return pImpl->mHavePreferredOriginIdentifier ?
           std::optional<int64_t> {pImpl->mPreferredOriginIdentifier} :
           std::nullopt;
}

/// Magnitude identifier
void Event::setPreferredMagnitudeIdentifier(const int64_t identifier) noexcept
{
    pImpl->mPreferredMagnitudeIdentifier = identifier;
    pImpl->mHavePreferredMagnitudeIdentifier = true;
}

std::optional<int64_t> Event::getPreferredMagnitudeIdentifier() const noexcept
{
    return pImpl->mHavePreferredMagnitudeIdentifier ?
           std::optional<int64_t> {pImpl->mPreferredMagnitudeIdentifier} :
           std::nullopt;
}

/// Mechanism identifier
void Event::setPreferredMechanismIdentifier(const int64_t identifier) noexcept
{
    pImpl->mPreferredMechanismIdentifier = identifier;
    pImpl->mHavePreferredMechanismIdentifier = true;
}

std::optional<int64_t> Event::getPreferredMechanismIdentifier() const noexcept
{
    return pImpl->mHavePreferredMechanismIdentifier ?
           std::optional<int64_t> {pImpl->mPreferredMechanismIdentifier} :
           std::nullopt;
}

/// Comment identifier
void Event::setCommentIdentifier(const int64_t identifier) noexcept
{
    pImpl->mCommentIdentifier = identifier;
    pImpl->mHaveCommentIdentifier = true;
}

std::optional<int64_t> Event::getCommentIdentifier() const noexcept
{
    return pImpl->mHaveCommentIdentifier ?
           std::optional<int64_t> (pImpl->mCommentIdentifier) : std::nullopt;
}

/// Geographic type
void Event::setType(const Event::Type type) noexcept
{
    pImpl->mType = type;
    pImpl->mHaveType = true;
}

std::optional<Event::Type> Event::getType() const noexcept
{
    return pImpl->mHaveType ?
           std::optional<Event::Type> {pImpl->mType} :
           std::nullopt;
}

/// Select flag
void Event::setSelectFlag() noexcept
{
    pImpl->mSelectFlag = true;
}

void Event::unsetSelectFlag() noexcept
{
    pImpl->mSelectFlag = false;
}

bool Event::getSelectFlag() const noexcept
{
    return pImpl->mSelectFlag;
}

/// Insertion string
std::string URTS::Database::AQMS::toInsertString(const Event &event)
{
    if (!event.haveAuthority())
    {   
        throw std::invalid_argument("Authority not set");
    }
    if (event.getType() == std::nullopt)
    {
        throw std::invalid_argument("Event type not set");
    }
    if (!event.haveVersion())
    {
        throw std::invalid_argument("Version not set");
    }
    std::string insertStatement{"INSERT INTO event "};

    auto eventType = ::typeToString(*event.getType());
    std::string keys = {"(auth, etype, version,"};
    std::string values
        = str(boost::format(" VALUES ('%1%', '%2%', %3$d,")
              %event.getAuthority()
              %eventType
              %event.getVersion()
             );

    if (event.haveIdentifier())
    {   
        keys = keys + " evid,";
        values = values + str(boost::format(" %1$d,")% event.getIdentifier() );
    } 
    if (event.getPreferredOriginIdentifier())
    {   
        keys = keys + " prefor,";
        values = values + str(boost::format(" %1$d,")% *event.getPreferredOriginIdentifier() );
    }   
    if (event.getPreferredMagnitudeIdentifier())
    {   
        keys = keys + " prefmag,";
        values = values + str(boost::format(" %1$d,")% *event.getPreferredMagnitudeIdentifier() );
    }   
    if (event.getPreferredMechanismIdentifier())
    {   
        keys = keys + " prefmec,";
        values = values + str(boost::format(" %1$d,")% *event.getPreferredMechanismIdentifier() );
    }   
    if (event.getCommentIdentifier())
    {
        keys = keys + " commid,";
        values = values + str(boost::format(" %1$d,")% *event.getCommentIdentifier() );
    }
    if (event.getSubSource())
    {
        keys = keys + " subsource,";
        values = values + str(boost::format(" '%1%',")% *event.getSubSource() );
    }
    if (event.getSelectFlag())
    {
        keys = keys + " selectflag,";
        values = values + " 1,";
    }
    else
    {
        keys = keys + " selectflag,";
        values = values + " 0,";
    }

    // Delete the trailing comma and make it a ")"
    keys.back() = ')';
    values.back()= ')';
    // Put it all together
    insertStatement = insertStatement + keys + values + ";";
    return insertStatement;
}
