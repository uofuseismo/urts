#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "urts/broadcasts/internal/origin/origin.hpp"
#include "urts/broadcasts/internal/origin/arrival.hpp"
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"
#include "database/aqms/utilities.hpp"

#define MESSAGE_TYPE "URTS::Broadcasts::Internal::Origin::Origin"
#define MESSAGE_VERSION "1.0.0"

using namespace URTS::Broadcasts::Internal::Origin;

namespace
{

nlohmann::json toJSONObject(const Origin &origin)
{
    nlohmann::json obj;
    obj["MessageType"] = origin.getMessageType();
    obj["MessageVersion"] = origin.getMessageVersion();
    obj["Latitude"] = origin.getLatitude();
    obj["Longitude"] = origin.getLongitude();
    obj["Depth"] = origin.getDepth();
    obj["Time"] = static_cast<int64_t> (origin.getTime().count());
    obj["Identifier"] = origin.getIdentifier();
    obj["ReviewStatus"] = static_cast<int> (origin.getReviewStatus());
    obj["Algorithms"] = origin.getAlgorithms();
    const auto &arrivals = origin.getArrivalsReference();
    if (!arrivals.empty())
    {

    }
    return obj;
}

Origin objectToOrigin(const nlohmann::json &obj)
{
    Origin origin;
    if (obj["MessageType"] != origin.getMessageType())
    {
        throw std::invalid_argument("Message has invalid message type");
    }
    origin.setLatitude( obj["Latitude"].template get<double> () );
    origin.setLongitude( obj["Longitude"].template get<double> () );
    origin.setDepth( obj["Depth"].template get<double> () );
    origin.setTime(
        std::chrono::microseconds {obj["Time"].template get<int64_t> ()}
    );
    origin.setIdentifier(obj["Identifier"].template get<int64_t> ());
    origin.setReviewStatus(
        static_cast<Origin::ReviewStatus>
            (obj["ReviewStatus"].template get<int> ()) 
    );
    origin.setAlgorithms(
        (obj["Algorithms"].template get<std::vector<std::string>> ())
    ); 
    if (obj.contains("Arrivals"))
    {
        for (const auto &arrivalObject : obj["Arrivals"])
        {
        }
    }
    return origin;
}
 
Origin fromCBORMessage(const uint8_t *message, const size_t length)
{
    auto obj = nlohmann::json::from_cbor(message, message + length);
    return ::objectToOrigin(obj);
}

}

class Origin::OriginImpl
{
public:
    std::vector<Arrival> mArrivals;
    std::vector<std::string> mAlgorithms;
    std::chrono::microseconds mTime{0};
    double mDepth{0};
    double mLatitude{0};
    double mLongitude{0};
    int64_t mIdentifier{0};
    Origin::ReviewStatus mReviewStatus{Origin::ReviewStatus::Automatic};
    bool mHaveDepth{false};
    bool mHaveIdentifier{false};
    bool mHaveLatitude{false};
    bool mHaveLongitude{false};
    bool mHaveTime{false};
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

/// Reset the class
void Origin::clear() noexcept
{
    pImpl = std::make_unique<OriginImpl> ();
}

/// Destructor
Origin::~Origin() = default;

/// Time
void Origin::setTime(const std::chrono::microseconds &time) noexcept
{
    pImpl->mTime = time;
    pImpl->mHaveTime = true;
}

std::chrono::microseconds Origin::getTime() const
{
    if (!haveTime()){throw std::runtime_error("Time not set");}
    return pImpl->mTime;
}

bool Origin::haveTime() const noexcept
{
    return pImpl->mHaveTime;
}

/// Event depth
void Origin::setDepth(const double depth)
{
    if (depth < -8900 || depth > 800000)
    {
        throw std::invalid_argument("Depth must be in range [-8900,80000] m");
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

/// Latitude
void Origin::setLatitude(const double latitude)
{
    if (latitude <-90 || latitude > 90)
    {
        throw std::runtime_error("Latitude must be in range [-90,90]");
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
void Origin::setLongitude(const double longitude) noexcept
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

/// Identifier
void Origin::setIdentifier(const int64_t identifier) noexcept
{
    pImpl->mIdentifier = identifier;
    pImpl->mHaveIdentifier = true;
    for (auto &arrivals : pImpl->mArrivals)
    {   
        arrivals.setIdentifier(identifier);
    }   
}

int64_t Origin::getIdentifier() const
{
    if (!haveIdentifier())
    {   
        throw std::runtime_error("Origin identifier not set");
    }   
    return pImpl->mIdentifier;
}

bool Origin::haveIdentifier() const noexcept
{
    return pImpl->mHaveIdentifier;
}

/// Sets the arrivals
void Origin::setArrivals(const std::vector<Arrival> &arrivals)
{
    std::vector<Arrival> arrivalsCopy{arrivals};
    setArrivals(std::move(arrivalsCopy));
}

void Origin::setArrivals(std::vector<Arrival> &&arrivals)
{
    pImpl->mArrivals = std::move(arrivals);
    if (haveIdentifier())
    {
        auto identifier = getIdentifier();
        for (auto &arrival : pImpl->mArrivals)
        {
            arrival.setOriginIdentifier(identifier);
        }
    }
}

std::vector<Arrival> Origin::getArrivals() const noexcept
{
    return pImpl->mArrivals;
}

const std::vector<Arrival> &Origin::getArrivalsReference() const noexcept
{
    return *&pImpl->mArrivals;
}

/// Review status
void Origin::setReviewStatus(const ReviewStatus status) noexcept
{
    pImpl->mReviewStatus = status;
}

Origin::ReviewStatus Origin::getReviewStatus() const noexcept
{
    return pImpl->mReviewStatus;
}

/// Algorithms
void Origin::setAlgorithms(const std::vector<std::string> &algorithms) noexcept
{
    pImpl->mAlgorithms = algorithms;
}

std::vector<std::string> Origin::getAlgorithms() const noexcept
{
    return pImpl->mAlgorithms;
}

/// Copy this class
std::unique_ptr<UMPS::MessageFormats::IMessage> Origin::clone() const
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<Origin> (*this);
    return result;
}

/// Create an instance of this class 
std::unique_ptr<UMPS::MessageFormats::IMessage>
    Origin::createInstance() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> result
        = std::make_unique<Origin> (); 
    return result;
}

/// Message type
std::string Origin::getMessageType() const noexcept
{
    return MESSAGE_TYPE;
}

/// Message version
std::string Origin::getMessageVersion() const noexcept
{
    return MESSAGE_VERSION;
}

//  Convert message
std::string Origin::toMessage() const
{
    auto obj = ::toJSONObject(*this);
    auto v = nlohmann::json::to_cbor(obj);
    std::string result(v.begin(), v.end());
    return result; 
}

void Origin::fromMessage(const std::string &message)
{
    if (message.empty()){throw std::invalid_argument("Message is empty");}
    fromMessage(message.data(), message.size());   
}

void Origin::fromMessage(const char *messageIn, const size_t length)
{
    if (length == 0){throw std::invalid_argument("No data");}
    if (messageIn == nullptr)
    {
        throw std::invalid_argument("Message is NULL");
    }
    auto message = reinterpret_cast<const uint8_t *> (messageIn);
    *this = ::fromCBORMessage(message, length);
}


/// Iterators
Origin::iterator Origin::begin()
{
    return pImpl->mArrivals.begin();
}

Origin::const_iterator Origin::begin() const noexcept
{
    return pImpl->mArrivals.begin();
}

Origin::const_iterator Origin::cbegin() const noexcept
{
    return pImpl->mArrivals.cbegin();
}

Origin::iterator Origin::end()
{
    return pImpl->mArrivals.end();
}

Origin::const_iterator Origin::end() const noexcept
{
    return pImpl->mArrivals.cend();
}

Origin::const_iterator Origin::cend() const noexcept
{
    return pImpl->mArrivals.cend();
}
