#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <nlohmann/json.hpp>
#include "urts/broadcasts/internal/origin/origin.hpp"
#include "urts/broadcasts/internal/origin/arrival.hpp"
#include "urts/broadcasts/internal/pick/uncertaintyBound.hpp"
#include "database/aqms/utilities.hpp"

#define MESSAGE_TYPE "URTS::Broadcasts::Internal::Origin"
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
    obj["MonitoringRegion"] = static_cast<int> (origin.getMonitoringRegion());
    obj["Algorithms"] = origin.getAlgorithms();
    obj["PreviousIdentifiers"] = origin.getPreviousIdentifiers();
    const auto &arrivals = origin.getArrivalsReference();
    if (!arrivals.empty())
    {
        nlohmann::json arrivalObjects;
        for (const auto &arrival : arrivals)
        {
             nlohmann::json arrivalObject;
             arrivalObject["Identifier"] = arrival.getIdentifier();
             arrivalObject["Network"] = arrival.getNetwork();
             arrivalObject["Station"] = arrival.getStation();
             arrivalObject["Channel"] = arrival.getChannel();
             arrivalObject["LocationCode"] = arrival.getLocationCode();
             arrivalObject["Time"]
                = static_cast<int64_t> (arrival.getTime().count());
             arrivalObject["Phase"] = static_cast<int> (arrival.getPhase());
             arrivalObject["OriginalChannels"] = arrival.getOriginalChannels();
             arrivalObject["ProcessingAlgorithms"]
                = arrival.getProcessingAlgorithms();
             arrivalObject["FirstMotion"]
                = static_cast<int> (arrival.getFirstMotion());
             arrivalObject["ReviewStatus"]
                = static_cast<int> (arrival.getReviewStatus());
             auto originIdentifier = arrival.getOriginIdentifier();
             if (originIdentifier)
             {
                 arrivalObject["OriginIdentifier"] = *originIdentifier;
             }
             auto residual = arrival.getResidual();
             if (residual){arrivalObject["Residual"] = *residual;}
             auto snr = arrival.getSignalToNoiseRatio();
             if (snr){arrivalObject["SignalToNoiseRatio"] = *snr;}
             auto uncertainty = arrival.getLowerAndUpperUncertaintyBound();
             if (uncertainty)
             {
                 nlohmann::json uncertaintyObject;
                 uncertaintyObject["LowerPercentile"]
                    = uncertainty->first.getPercentile();
                 uncertaintyObject["LowerPerturbation"]
                    = static_cast<int64_t>
                      (uncertainty->first.getPerturbation().count());
                 uncertaintyObject["UpperPercentile"]
                    = uncertainty->second.getPercentile();
                 uncertaintyObject["UpperPerturbation"]
                    = static_cast<int64_t>
                      (uncertainty->second.getPerturbation().count());
                 arrivalObject["Uncertainty"] = uncertaintyObject;
             }
             arrivalObjects.push_back(std::move(arrivalObject));
        }
        obj["Arrivals"] = arrivalObjects;
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
    origin.setMonitoringRegion(
        static_cast<Origin::MonitoringRegion> 
            (obj["MonitoringRegion"].template get<int> ())
    );
    origin.setAlgorithms(
        (obj["Algorithms"].template get<std::vector<std::string>> ())
    ); 
    origin.setPreviousIdentifiers(
        (obj["PreviousIdentifiers"].template get<std::vector<int64_t>> ())
    );
    if (obj.contains("Arrivals"))
    {
        std::vector<Arrival> arrivals;
        for (const auto &arrivalObject : obj["Arrivals"])
        {
            Arrival arrival;
            arrival.setIdentifier(
                arrivalObject["Identifier"].template get<int64_t> ());
            arrival.setNetwork(
                arrivalObject["Network"].template get<std::string> ());
            arrival.setStation(
                arrivalObject["Station"].template get<std::string> ());
            arrival.setChannel(
                arrivalObject["Channel"].template get<std::string> ());
            arrival.setLocationCode(
                arrivalObject["LocationCode"].template get<std::string> ());
            arrival.setTime(
                std::chrono::microseconds {
                   arrivalObject["Time"].template get<int64_t> ()
                });
            arrival.setPhase(
                static_cast<Arrival::Phase> (
                   arrivalObject["Phase"].template get<int> ()));
            arrival.setOriginalChannels(
                arrivalObject["OriginalChannels"].template
                   get<std::vector<std::string>> ());
            arrival.setProcessingAlgorithms(
                arrivalObject["ProcessingAlgorithms"].template
                   get<std::vector<std::string>> ());
            arrival.setFirstMotion(
                static_cast<Arrival::FirstMotion> (
                   arrivalObject["FirstMotion"].template get<int> ())); 
            arrival.setReviewStatus(
                static_cast<Arrival::ReviewStatus> (
                   arrivalObject["ReviewStatus"].template get<int> ()));
            if (arrivalObject.contains("OriginIdentifier"))
            {
                arrival.setOriginIdentifier(
                   arrivalObject["OriginIdentifier"].template get<int64_t> ());
            }
            if (arrivalObject.contains("Residual"))
            {
                arrival.setResidual(
                   arrivalObject["Residual"].template get<double> ());
            }
            if (arrivalObject.contains("SignalToNoiseRatio"))
            {
                arrival.setSignalToNoiseRatio(
                   arrivalObject["SignalToNoiseRatio"].template get<double> ());
            }
            if (arrivalObject.contains("Uncertainty"))
            {
                auto uncertaintyObject = arrivalObject["Uncertainty"];
                URTS::Broadcasts::Internal::Pick::UncertaintyBound
                    lowerBound, upperBound;   
                lowerBound.setPercentile(
                    uncertaintyObject["LowerPercentile"].template
                        get<double> ());
                lowerBound.setPerturbation(
                    std::chrono::microseconds
                    {
                        uncertaintyObject["LowerPerturbation"].template
                            get<int64_t> ()
                    });
                upperBound.setPercentile(
                    uncertaintyObject["UpperPercentile"].template
                        get<double> ());
                upperBound.setPerturbation(
                    std::chrono::microseconds
                    {
                        uncertaintyObject["UpperPerturbation"].template
                            get<int64_t> ()
                    });
                arrival.setLowerAndUpperUncertaintyBound(
                    std::pair {lowerBound, upperBound});
            }
            arrivals.push_back(std::move(arrival));
        }
        if (!arrivals.empty()){origin.setArrivals(std::move(arrivals));}
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
    std::vector<int64_t> mPreviousIdentifiers;
    std::chrono::microseconds mTime{0};
    double mDepth{0};
    double mLatitude{0};
    double mLongitude{0};
    int64_t mIdentifier{0};
    Origin::ReviewStatus mReviewStatus{Origin::ReviewStatus::Automatic};
    Origin::MonitoringRegion
         mMonitoringRegion{Origin::MonitoringRegion::Unknown};
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
    // Purge any matching previous identifiers
    if (!pImpl->mPreviousIdentifiers.empty())
    {
        pImpl->mPreviousIdentifiers.erase(
            std::remove(pImpl->mPreviousIdentifiers.begin(),
                        pImpl->mPreviousIdentifiers.end(),
                        identifier),
            pImpl->mPreviousIdentifiers.end());
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
        if (!arrival.haveChannel())
        {
            throw std::invalid_argument("Channel not set");
        }
        if (!arrival.haveLocationCode())
        {
            throw std::invalid_argument("Location code not set");
        }
        if (!arrival.haveTime()){throw std::invalid_argument("Time not set");}
        if (!arrival.havePhase()){throw std::invalid_argument("Phase not set");}
    }
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

/// Monitoring region
void Origin::setMonitoringRegion(const MonitoringRegion region) noexcept
{
    pImpl->mMonitoringRegion = region;
}

Origin::MonitoringRegion Origin::getMonitoringRegion() const noexcept
{
    return pImpl->mMonitoringRegion;
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

/// Sets the previous origin identifiers
void Origin::setPreviousIdentifiers(const std::vector<int64_t> &identifiersIn)
{
    // Not a lot to do
    if (identifiersIn.empty())
    {
        pImpl->mPreviousIdentifiers.clear();
        return;
    } 
    // Only save the unique identifiers
    auto identifiers = identifiersIn;
    std::sort(identifiers.begin(), identifiers.end());
    auto identifiersLast
         = std::unique(identifiers.begin(), identifiers.end());
    identifiers.erase(identifiersLast, identifiers.end());

    pImpl->mPreviousIdentifiers.clear();
    pImpl->mPreviousIdentifiers.reserve(identifiers.size());
    // Make sure we don't make the current identifier a previous identifier
    if (haveIdentifier())
    {
        auto currentIdentifier = getIdentifier();
        for (const auto &identifier : identifiers)
        {
            if (identifier != currentIdentifier)
            {
                pImpl->mPreviousIdentifiers.push_back(identifier);
            } 
        }
    }
    else
    {
        // Nothing to check to add them all
        pImpl->mPreviousIdentifiers = identifiers;
    }
}

std::vector<int64_t> Origin::getPreviousIdentifiers() const noexcept
{
    return pImpl->mPreviousIdentifiers;
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
