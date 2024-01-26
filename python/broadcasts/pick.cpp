#include <string>
#include <cmath>
#include <vector>
#include <chrono>
#include <pybind11/stl.h>
#include <pybind11/chrono.h>
#include <urts/broadcasts/internal/pick/publisher.hpp>
#include <urts/broadcasts/internal/pick/publisherOptions.hpp>
#include <urts/broadcasts/internal/pick/subscriber.hpp>
#include <urts/broadcasts/internal/pick/subscriberOptions.hpp>
#include <urts/broadcasts/internal/pick/pick.hpp>
#include <urts/broadcasts/internal/pick/uncertaintyBound.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/messaging/context.hpp>
#include <umps/logging/log.hpp>
#include <umps/messageFormats/staticUniquePointerCast.hpp>
#include <python/messaging.hpp>
#include <python/authentication.hpp>
#include "pick.hpp"

using namespace URTS::Python::Broadcasts::Internal::Pick;
namespace UPick = URTS::Broadcasts::Internal::Pick;
///--------------------------------------------------------------------------///
///                               Uncertainty Bound                          ///
///--------------------------------------------------------------------------///
/// Constructor
UncertaintyBound::UncertaintyBound() :
    pImpl(std::make_unique<UPick::UncertaintyBound> ())
{
}

/// Copy constructor
UncertaintyBound::UncertaintyBound(const UncertaintyBound &bound)
{
    *this = bound;
}

/// Construct from native class
UncertaintyBound::UncertaintyBound(const UPick::UncertaintyBound &bound)
{
    *this = bound;
}

/// Move constructor
UncertaintyBound::UncertaintyBound(UncertaintyBound &&bound) noexcept
{
    *this = std::move(bound);
}

/// Copy assignment
UncertaintyBound& UncertaintyBound::operator=(const UncertaintyBound &bound)
{
    if (&bound == this){return *this;}
    pImpl = std::make_unique<UPick::UncertaintyBound> (*bound.pImpl);
    return *this;
}

/// Copy from native class
UncertaintyBound&
UncertaintyBound::operator=(const UPick::UncertaintyBound &bound)
{
    pImpl = std::make_unique<UPick::UncertaintyBound> (bound);
    return *this;
}

/// Move assignment
UncertaintyBound& UncertaintyBound::operator=(UncertaintyBound &&bound) noexcept
{
    if (&bound == this){return *this;}
    pImpl = std::move(bound.pImpl);
    return *this;
}

/// Perturbation
void UncertaintyBound::setPerturbation(const double perturbation)
{
    std::chrono::microseconds perturbationMuS
    {
        static_cast<int64_t> (std::round(perturbation*1.e6))
    };
    pImpl->setPerturbation(perturbationMuS);
}

double UncertaintyBound::getPerturbation() const
{
    return pImpl->getPerturbation().count()*1.e-6;
}

/// Percentile
void UncertaintyBound::setPercentile(const double percentile)
{
    pImpl->setPercentile(percentile);
}

double UncertaintyBound::getPercentile() const
{
    return pImpl->getPercentile();
}

const UPick::UncertaintyBound&
UncertaintyBound::getNativeClassReference() const noexcept
{
    return *pImpl;
}

/// Reset class
void UncertaintyBound::clear() noexcept
{
    pImpl = std::make_unique<UPick::UncertaintyBound> ();
}

/// Destructor
UncertaintyBound::~UncertaintyBound() = default;

///--------------------------------------------------------------------------///
///                               Data Packet                                ///
///--------------------------------------------------------------------------///
/// Constructor
Pick::Pick() :
    pImpl(std::make_unique<UPick::Pick> ()) 
{
}

/// Copy constructor
Pick::Pick(const Pick &pick)
{
    *this = pick;
}

/// Construct from native class
Pick::Pick(const UPick::Pick &pick)
{
    *this = pick;
}

/// Move constructor
Pick::Pick(Pick &&pick) noexcept
{
    *this = std::move(pick);
}

/// Copy assignment
Pick& Pick::operator=(const Pick &pick)
{
    if (&pick == this){return *this;}
    pImpl = std::make_unique<UPick::Pick> (*pick.pImpl);
    return *this;
}

/// Copy from native class
Pick& Pick::operator=(const UPick::Pick &pick)
{
    pImpl = std::make_unique<UPick::Pick> (pick);
    return *this;
}

/// Move assignment
Pick& Pick::operator=(Pick &&pick) noexcept
{
    if (&pick == this){return *this;}
    pImpl = std::move(pick.pImpl);
    return *this;
}

const UPick::Pick&
Pick::getNativeClassReference() const noexcept
{
    return *pImpl;
}

UPick::Pick
Pick::getNativeClass() const noexcept
{
    return *pImpl;
}

/// Network
void Pick::setNetwork(const std::string &network)
{
    pImpl->setNetwork(network);
}

std::string Pick::getNetwork() const
{
    return pImpl->getNetwork();
}

/// Station
void Pick::setStation(const std::string &station)
{
    pImpl->setStation(station);
}

std::string Pick::getStation() const
{
    return pImpl->getStation();
}

/// Channel
void Pick::setChannel(const std::string &channel)
{
    pImpl->setChannel(channel);
}

std::string Pick::getChannel() const
{
    return pImpl->getChannel();
}

/// Location code
void Pick::setLocationCode(const std::string &location)
{
    pImpl->setLocationCode(location);
}

std::string Pick::getLocationCode() const
{
    return pImpl->getLocationCode();
}

/// Phase hint
void Pick::setPhaseHint(const std::string &hint)
{
    pImpl->setPhaseHint(hint);
}

std::string Pick::getPhaseHint() const
{
    return pImpl->getPhaseHint();
}

/// Start time
void Pick::setTime(const double time) noexcept
{
    pImpl->setTime(time);
}

double Pick::getTime() const
{
    auto time = pImpl->getTime();
    return time.count()*1.e-6;
}

/// First motion
void Pick::setFirstMotion(
    const URTS::Broadcasts::Internal::Pick::Pick::FirstMotion fm) noexcept
{
    pImpl->setFirstMotion(fm);
}

URTS::Broadcasts::Internal::Pick::Pick::FirstMotion
Pick::getFirstMotion() const noexcept
{
    return pImpl->getFirstMotion();
}

/// Processing algorithms
std::vector<std::string> Pick::getProcessingAlgorithms() const
{
    return pImpl->getProcessingAlgorithms();
}

void Pick::setProcessingAlgorithms(const std::vector<std::string> &algorithms)
{
    pImpl->setProcessingAlgorithms(algorithms);
}

/// Original channels
std::vector<std::string> Pick::getOriginalChannels() const
{
    return pImpl->getOriginalChannels();
}

void Pick::setOriginalChannels(const std::vector<std::string> &channels)
{
    pImpl->setOriginalChannels(channels);
}

/// Identifier
void Pick::setIdentifier(const uint64_t identifier) noexcept
{
    pImpl->setIdentifier(identifier);
}

uint64_t Pick::getIdentifier() const
{
    return pImpl->getIdentifier();
}

/// Review status
void Pick::setReviewStatus(
    const URTS::Broadcasts::Internal::Pick::Pick::ReviewStatus status) noexcept
{
    pImpl->setReviewStatus(status);
}

URTS::Broadcasts::Internal::Pick::Pick::ReviewStatus
Pick::getReviewStatus() const noexcept
{
    return pImpl->getReviewStatus();
}

/// Uncertainty bounds
void Pick::setLowerAndUpperUncertaintyBound(
    const std::pair<::UncertaintyBound, ::UncertaintyBound> &lowerAndUpperBound)
{
    URTS::Broadcasts::Internal::Pick::UncertaintyBound
        lowerBound{lowerAndUpperBound.first.getNativeClassReference()};
    URTS::Broadcasts::Internal::Pick::UncertaintyBound
        upperBound{lowerAndUpperBound.second.getNativeClassReference()};
    pImpl->setLowerAndUpperUncertaintyBound(
        std::pair {lowerBound, upperBound} );
}

std::pair<::UncertaintyBound, ::UncertaintyBound>
Pick::getLowerAndUpperUncertaintyBound() const
{
    auto bounds = pImpl->getLowerAndUpperUncertaintyBound();
    return std::pair {::UncertaintyBound {bounds.first},
                      ::UncertaintyBound {bounds.second}};
}

/*
/// Data
void DataPacket::setData(
    const pybind11::array_t<double, pybind11::array::c_style | pybind11::array::forcecast> &x) 
{
    std::vector<double> xWork(x.size());
    std::memcpy(xWork.data(), x.data(), xWork.size()*sizeof(double));
    pImpl->setData(std::move(xWork)); 
}

std::vector<double> DataPacket::getData() const noexcept
{
    std::vector<double> result;
    result.resize(pImpl->getNumberOfSamples());
    const auto srcPtr = pImpl->getDataPointer();
    auto dstPtr = result.data();
    std::copy(srcPtr, srcPtr + result.size(), dstPtr);
    return result;
}

int DataPacket::getNumberOfSamples() const noexcept
{
    return pImpl->getNumberOfSamples();
}
*/

/// Base class
void Pick::fromBaseClass(UMPS::MessageFormats::IMessage &message)
{
    if (message.getMessageType() != pImpl->getMessageType())
    {   
        throw std::invalid_argument("Expecting message type: "
                                    + pImpl->getMessageType()
                                    + " but given: "
                                    + message.getMessageType());
    }   
    pImpl = UMPS::MessageFormats::static_unique_pointer_cast<UPick::Pick> (message.clone());
}

std::unique_ptr<UMPS::Python::MessageFormats::IMessage> Pick::clone(
        const std::unique_ptr<UMPS::MessageFormats::IMessage> &message) const
{
    if (message->getMessageType() != pImpl->getMessageType())
    {   
        throw std::invalid_argument("Expecting: " + pImpl->getMessageType()
                                    + " but got: " + message->getMessageType());
    }   
    auto copy = UMPS::MessageFormats::static_unique_pointer_cast
                <UPick::Pick> (message->clone());
    return std::make_unique<Pick> (*copy);
}

std::unique_ptr<UMPS::Python::MessageFormats::IMessage>
Pick::createInstance() const
{
    return std::make_unique<Pick> (); 
}

std::unique_ptr<UMPS::MessageFormats::IMessage>
Pick::getInstanceOfBaseClass() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> message
        = pImpl->createInstance();
    return message;
}

/// Message type
std::string Pick::getMessageType() const noexcept
{
    return pImpl->getMessageType();
}

/// Reset class
void Pick::clear() noexcept
{
    pImpl->clear();
}

/// Destructor
Pick::~Pick() = default;


///--------------------------------------------------------------------------///
///                               Pick Publisher                             ///
///--------------------------------------------------------------------------///
Publisher::Publisher() :
    mPublisher(std::make_unique<UPick::Publisher> ())
{
}

Publisher::Publisher(
    UMPS::Python::Messaging::Context &context)
{
    auto umpsContext = context.getSharedPointer();
    mPublisher = std::make_unique<UPick::Publisher> (umpsContext);
}

Publisher::~Publisher() = default;

///--------------------------------------------------------------------------///
///                             Pick Subscriber                              ///
///--------------------------------------------------------------------------///

Subscriber::Subscriber() :
    mSubscriber(std::make_unique<UPick::Subscriber> ()) 
{
}

Subscriber::Subscriber(
    UMPS::Python::Messaging::Context &context)
{
    auto umpsContext = context.getSharedPointer();
    mSubscriber = std::make_unique<UPick::Subscriber> (umpsContext);
}

Subscriber::Subscriber(
    UMPS::Python::Logging::ILog &logger)
{
    auto umpsLogger = logger.getSharedPointer();
    mSubscriber = std::make_unique<UPick::Subscriber> (umpsLogger);
}

Subscriber::Subscriber(
    UMPS::Python::Messaging::Context &context,
    UMPS::Python::Logging::ILog &logger)
{
    auto umpsContext = context.getSharedPointer();
    auto umpsLogger = logger.getSharedPointer();
    mSubscriber = std::make_unique<UPick::Subscriber> (umpsContext, umpsLogger);
}

void Subscriber::initialize(const SubscriberOptions &options)
{
    auto nativeClass = options.getNativeClassReference();
    mSubscriber->initialize(nativeClass);
}

std::optional<Pick> Subscriber::receive() const
{
    auto umpsPick = mSubscriber->receive();
    if (umpsPick != nullptr)
    {
        return Pick{*umpsPick};
    }
    return {};
}

bool Subscriber::isInitialized() const noexcept
{
    return mSubscriber->isInitialized();
}

Subscriber::~Subscriber() = default;

///--------------------------------------------------------------------------///
///                           Subscriber Options                             ///
///--------------------------------------------------------------------------///
/// Constructor
SubscriberOptions::SubscriberOptions() :
    pImpl(std::make_unique<UPick::SubscriberOptions> ())
{
}

/// Copy constructor
SubscriberOptions::SubscriberOptions(const SubscriberOptions &options)
{
    *this = options;
}

/// Construct from native class
SubscriberOptions::SubscriberOptions(const UPick::SubscriberOptions &options)
{
    *this = options;
}

/// Move constructor
SubscriberOptions::SubscriberOptions(SubscriberOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment
SubscriberOptions&
SubscriberOptions::operator=(const SubscriberOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<UPick::SubscriberOptions> (*options.pImpl);
    return *this;
}

/// Copy from native class
SubscriberOptions&
SubscriberOptions::operator=(const UPick::SubscriberOptions &options)
{
    pImpl = std::make_unique<UPick::SubscriberOptions> (options);
    return *this;
}

/// Move assignment
SubscriberOptions&
SubscriberOptions::operator=(SubscriberOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

const UPick::SubscriberOptions&
SubscriberOptions::getNativeClassReference() const noexcept
{
    return *pImpl;
}

/// ZAP options
void SubscriberOptions::setZAPOptions(
    const UMPS::Python::Authentication::ZAPOptions &options)
{
    auto nativeClass = options.getNativeClassReference();
    pImpl->setZAPOptions(nativeClass);
}

UMPS::Python::Authentication::ZAPOptions SubscriberOptions::getZAPOptions() const
{
    return UMPS::Python::Authentication::ZAPOptions {pImpl->getZAPOptions()};
}

/// Time out
void SubscriberOptions::setTimeOut(const std::chrono::milliseconds &timeOut)
{
    pImpl->setTimeOut(timeOut);
}

std::chrono::milliseconds SubscriberOptions::getTimeOut() const
{
    return pImpl->getTimeOut();
}

/// HWM
void SubscriberOptions::setHighWaterMark(const int hwm)
{
    pImpl->setHighWaterMark(hwm);
}

int SubscriberOptions::getHighWaterMark() const
{
    return pImpl->getHighWaterMark();
}

/// Reset class
void SubscriberOptions::clear() noexcept
{
    pImpl = std::make_unique<UPick::SubscriberOptions> ();
}

/// Address
void SubscriberOptions::setAddress(const std::string &address)
{
    pImpl->setAddress(address);
}

std::string SubscriberOptions::getAddress() const
{
    return pImpl->getAddress();
}

/// Destructor
SubscriberOptions::~SubscriberOptions() = default;

///--------------------------------------------------------------------------///
///                            Initialization                                ///
///--------------------------------------------------------------------------///
void URTS::Python::Broadcasts::Internal::Pick::initialize(pybind11::module &m)
{
    //---------------------------------Pick-----------------------------------//
    pybind11::module dpm = m.def_submodule("Pick");
    dpm.attr("__doc__") = "A proxy broadcast for picks.";
    //-------------------------------Uncertainty Bound------------------------//
    pybind11::class_<URTS::Python::Broadcasts::Internal::Pick::UncertaintyBound>
        uncertainty(dpm, "UncertaintyBound");
    uncertainty.attr("__doc__") = "Defines an uncertainty for a phase arrival.";
    uncertainty.doc() = R""""(
This defines an uncertainty bound for a phase arrival.

Properties
    percentile : float
        The percentile of the pick.  For example, if this pick is +/- 1 
        standard deviation and this is the lower bound, then this would be
        ~15.87.  This must be in the range [0,100].
    perturbation : float
        The perturbation in seconds to add to the pick that defines the
        lower or upper bound.  When the percentile is less than 50 this
        likely will be negative.
)"""";
    uncertainty.def("__copy__", [](const UncertaintyBound &self)
    {
        return UncertaintyBound(self);
    });
    uncertainty.def_property("percentile",
                             &UncertaintyBound::getPercentile,
                             &UncertaintyBound::setPercentile);
    uncertainty.def_property("perturbation",
                             &UncertaintyBound::getPerturbation,
                             &UncertaintyBound::setPerturbation);
    uncertainty.def("clear",
                    &UncertaintyBound::clear,
                    "Resets the class.");
    //--------------------------------Pick Message----------------------------//
    pybind11::class_<URTS::Python::Broadcasts::Internal::Pick::Pick>
        pick(dpm, "Pick");
    pick.attr("__doc__") = "Defines an URTS pick.";
    pick.def(pybind11::init<> ());
    pick.doc() = R""""(
This is an URTS pick; a container for picks (unassociated) seismic phase arrivals.

Required Properties:
    time : float
        The time (UTC) of the pick in seconds since the epoch.
    network : str
        The network code - e.g., UU.
    station : str
        The station code - e.g., FORK.  
    channel : str
        The channel code - e.g., EHZ.
    location_code : str
        The location code - e.g., 00.
    identifer : int
        The (unique) pick identification number.

Optional Properties:
    phase_hint : str
        A phase hint - e.g., P or S.
    first_motion 
        For P picks the first motion can be up, down, or unknown.
    processing_algorithms
        The processing algorithms used to make this pick.
    lower_and_upper_uncertainty_bound : UncertaintyBound 
        
Read-Only Properties:
    message_type : str
        The message type.
)"""";
    pybind11::enum_<URTS::Broadcasts::Internal::Pick::Pick::FirstMotion> (pick, "FirstMotion")
        .value("Up", URTS::Broadcasts::Internal::Pick::Pick::FirstMotion::Up,
               "The first motion is up.")
        .value("Down", URTS::Broadcasts::Internal::Pick::Pick::FirstMotion::Down,
               "The first motion is down.")
        .value("Unknown", URTS::Broadcasts::Internal::Pick::Pick::FirstMotion::Unknown,
               "The first motion is unknown.");
    pybind11::enum_<URTS::Broadcasts::Internal::Pick::Pick::ReviewStatus> (pick, "ReviewStatus")
        .value("Automatic", URTS::Broadcasts::Internal::Pick::Pick::ReviewStatus::Automatic,
               "This pick was generated by an algorithm and is not reviewed.")
        .value("Manual", URTS::Broadcasts::Internal::Pick::Pick::ReviewStatus::Manual,
               "This pick was generated by a human."); 
    pick.def("__copy__", [](const Pick &self)
    {
        return Pick(self);
    });
    pick.def_property_readonly("message_type",
                               &Pick::getMessageType);
    pick.def_property("network",
                      &Pick::getNetwork,
                      &Pick::setNetwork);
    pick.def_property("station",
                      &Pick::getStation,
                      &Pick::setStation);
    pick.def_property("channel",
                      &Pick::getChannel,
                      &Pick::setChannel);
    pick.def_property("location_code",
                      &Pick::getLocationCode,
                      &Pick::setLocationCode);
    pick.def_property("time",
                      &Pick::getTime,
                      &Pick::setTime);
    pick.def_property("phase_hint",
                      &Pick::getPhaseHint,
                      &Pick::setPhaseHint);
    pick.def_property("first_motion",
                      &Pick::getFirstMotion,
                      &Pick::setFirstMotion);
    pick.def_property("original_channels",
                      &Pick::getOriginalChannels,
                      &Pick::setOriginalChannels);
    pick.def_property("processing_algorithms",
                      &Pick::getProcessingAlgorithms,
                      &Pick::setProcessingAlgorithms);
    pick.def_property("lower_and_upper_uncertainty_bound",
                      &Pick::getLowerAndUpperUncertaintyBound,
                      &Pick::setLowerAndUpperUncertaintyBound);
    pick.def_property("review_status",
                      &Pick::getReviewStatus,
                      &Pick::setReviewStatus);
    pick.def_property("identifier",
                      &Pick::getIdentifier,
                      &Pick::setIdentifier);
    pick.def_property_readonly("message_type",
                               &Pick::getMessageType);

    pick.def("clear",
             &Pick::Pick::clear,
            "Resets the class and releases memory.");
    //----------------------------SubscriberOptions---------------------------//
    pybind11::class_<URTS::Python::Broadcasts::Internal::Pick::SubscriberOptions>
        subscriberOptions(dpm, "SubscriberOptions");
    subscriberOptions.attr("__doc__") = "Defines the options from which to initialize an URTS pick message subscriber.";
    subscriberOptions.doc() = R""""(
This defines the options influencing the behavior of the pick broadcast subscriber.

Required Properties:
    address : str
        The address of the pick broadcast to which to connect.

Optional Properties:
    zap_options : UMPS::Python::Authentication::ZAPOptions
        The ZeroMQ authentication protocol options.
    time_out 
        The number of milliseconds to wait before the receiving thread
        returns.  If this is negative then this will wait forever.
        If this is 0 then this will return immediately.
    high_water_mark : int
        The approximate number of messages to attempt to cache on the socket.
        If this is 0 then this will attempt to hold onto "infinite" messages.
)"""";

    subscriberOptions.def(pybind11::init<> ());
    subscriberOptions.def_property("address",
                                   &SubscriberOptions::getAddress,
                                   &SubscriberOptions::setAddress);
    subscriberOptions.def_property("zap_options",
                                   &SubscriberOptions::getZAPOptions,
                                   &SubscriberOptions::setZAPOptions);
    subscriberOptions.def_property("time_out",
                                   &SubscriberOptions::getTimeOut,
                                   &SubscriberOptions::setTimeOut);
    subscriberOptions.def_property("high_water_mark",
                                   &SubscriberOptions::getHighWaterMark,
                                   &SubscriberOptions::setHighWaterMark);
    subscriberOptions.def("clear",
                          &SubscriberOptions::clear,
                          "Resets the class and releases memory.");
    //--------------------------------Subscriber------------------------------//
    pybind11::class_<URTS::Python::Broadcasts::Internal::Pick::Subscriber>
        subscriber(dpm, "Subscriber");
    subscriber.attr("__doc__") = "Defines an URTS pick message subscriber.";
    subscriber.def(pybind11::init<> ());
    subscriber.def(pybind11::init<UMPS::Python::Logging::ILog &> ());
    subscriber.def(pybind11::init<UMPS::Python::Messaging::Context &> ());
    subscriber.def(pybind11::init<UMPS::Python::Messaging::Context &,
                                  UMPS::Python::Logging::ILog &> ());
    subscriberOptions.doc() = R""""(
This is the pick broadcast subscriber.

Read-Only Properties
    initialized : bool
       Indicates the class is connected and ready to receiver picks.
)"""";
    subscriber.def_property_readonly("initilialized",
                                     &Subscriber::isInitialized);
    subscriber.def("initialize",
                   &Subscriber::initialize,
                   "Initializes the class and connects the subscriber to the broadcast.");
    subscriber.def("receive",
                   &Subscriber::receive,
                   "Receives a pick from the broadcast.  If this times out the result will be None.");
}
