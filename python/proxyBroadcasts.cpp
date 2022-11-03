#include <string>
#include <vector>
#include <chrono>
#include <urts/proxyBroadcasts/dataPacket/publisher.hpp>
#include <urts/proxyBroadcasts/dataPacket/publisherOptions.hpp>
#include <urts/proxyBroadcasts/dataPacket/subscriber.hpp>
#include <urts/proxyBroadcasts/dataPacket/subscriberOptions.hpp>
#include <urts/proxyBroadcasts/dataPacket/dataPacket.hpp>
#include <umps/messaging/context.hpp>
#include <umps/logging/log.hpp>
#include <umps/messageFormats/staticUniquePointerCast.hpp>
#include <python/messaging.hpp>
#include "proxyBroadcasts.hpp"

using namespace URTS::Python::ProxyBroadcasts;
namespace UPB = URTS::ProxyBroadcasts;

///--------------------------------------------------------------------------///
///                               Data Packet                                ///
///--------------------------------------------------------------------------///
/// Constructor
DataPacket::DataPacket::DataPacket() :
    pImpl(std::make_unique<UPB::DataPacket::DataPacket> ()) 
{
}

/// Copy constructor
DataPacket::DataPacket::DataPacket(const DataPacket &packet)
{
    *this = packet;
}

/// Construct from native class
DataPacket::DataPacket::DataPacket(const UPB::DataPacket::DataPacket &packet)
{
    *this = packet;
}

/// Move constructor
DataPacket::DataPacket::DataPacket(DataPacket &&packet) noexcept
{
    *this = packet;
}

/// Copy assignment
DataPacket::DataPacket&
DataPacket::DataPacket::operator=(const DataPacket &packet)
{
    if (&packet == this){return *this;}
    pImpl = std::make_unique<UPB::DataPacket::DataPacket> (*packet.pImpl);
    return *this;
}

/// Copy from native class
DataPacket::DataPacket&
DataPacket::DataPacket::operator=(const UPB::DataPacket::DataPacket &packet)
{
    pImpl = std::make_unique<UPB::DataPacket::DataPacket> (packet);
    return *this;
}

/// Move assignment
DataPacket::DataPacket&
DataPacket::DataPacket::operator=(DataPacket &&packet) noexcept
{
    if (&packet == this){return *this;}
    pImpl = std::move(packet.pImpl);
    return *this;
}

const UPB::DataPacket::DataPacket&
DataPacket::DataPacket::getNativeClassReference() const noexcept
{
    return *pImpl;
}

UPB::DataPacket::DataPacket
DataPacket::DataPacket::getNativeClass() const noexcept
{
    return *pImpl;
}

/// Network
void DataPacket::DataPacket::setNetwork(const std::string &network)
{
    pImpl->setNetwork(network);
}

std::string DataPacket::DataPacket::getNetwork() const
{
    return pImpl->getNetwork();
}

/// Station
void DataPacket::DataPacket::setStation(const std::string &station)
{
    pImpl->setStation(station);
}

std::string DataPacket::DataPacket::getStation() const
{
    return pImpl->getStation();
}

/// Channel
void DataPacket::DataPacket::setChannel(const std::string &channel)
{
    pImpl->setChannel(channel);
}

std::string DataPacket::DataPacket::getChannel() const
{
    return pImpl->getChannel();
}

/// Location code
void DataPacket::DataPacket::setLocationCode(const std::string &location)
{
    pImpl->setLocationCode(location);
}

std::string DataPacket::DataPacket::getLocationCode() const
{
    return pImpl->getLocationCode();
}

/// Sampling rate
void DataPacket::DataPacket::setSamplingRate(const double samplingRate)
{
    pImpl->setSamplingRate(samplingRate);
}

double DataPacket::DataPacket::getSamplingRate() const
{
    return pImpl->getSamplingRate();
}

/// Start time
void DataPacket::DataPacket::setStartTime(
    const std::chrono::microseconds &time) noexcept
{
    pImpl->setStartTime(time);
}

std::chrono::microseconds DataPacket::DataPacket::getStartTime() const noexcept
{
    return pImpl->getStartTime();
}

/// Data
void DataPacket::DataPacket::setData(
    const pybind11::array_t<double, pybind11::array::c_style | pybind11::array::forcecast> &x) 
{
    std::vector<double> xWork(x.size());
    std::memcpy(xWork.data(), x.data(), xWork.size()*sizeof(double));
    pImpl->setData(std::move(xWork)); 
}

std::vector<double> DataPacket::DataPacket::getData() const noexcept
{
    std::vector<double> result;
    result.resize(pImpl->getNumberOfSamples());
    const auto srcPtr = pImpl->getDataPointer();
    auto dstPtr = result.data();
    std::copy(srcPtr, srcPtr + result.size(), dstPtr);
    return result;
}

int DataPacket::DataPacket::getNumberOfSamples() const noexcept
{
    return pImpl->getNumberOfSamples();
}

/// Base class
void DataPacket::DataPacket::fromBaseClass(UMPS::MessageFormats::IMessage &message)
{
    if (message.getMessageType() != pImpl->getMessageType())
    {   
        throw std::invalid_argument("Expecting message type: "
                                    + pImpl->getMessageType()
                                    + " but given: "
                                    + message.getMessageType());
    }   
    pImpl = UMPS::MessageFormats::static_unique_pointer_cast<UPB::DataPacket::DataPacket> (message.clone());
}

std::unique_ptr<UMPS::Python::MessageFormats::IMessage> DataPacket::DataPacket::clone(
        const std::unique_ptr<UMPS::MessageFormats::IMessage> &message) const
{
    if (message->getMessageType() != pImpl->getMessageType())
    {   
        throw std::invalid_argument("Expecting: " + pImpl->getMessageType()
                                    + " but got: " + message->getMessageType());
    }   
    auto copy = UMPS::MessageFormats::static_unique_pointer_cast
                <UPB::DataPacket::DataPacket> (message->clone());
    return std::make_unique<DataPacket> (*copy);
}

std::unique_ptr<UMPS::Python::MessageFormats::IMessage>
DataPacket::DataPacket::createInstance() const
{
    return std::make_unique<DataPacket> (); 
}

std::unique_ptr<UMPS::MessageFormats::IMessage>
DataPacket::DataPacket::getInstanceOfBaseClass() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> message
        = pImpl->createInstance();
    return message;
}

/// Message type
std::string DataPacket::DataPacket::getMessageType() const noexcept
{
    return pImpl->getMessageType();
}

/// Reset class
void DataPacket::DataPacket::clear() noexcept
{
    pImpl->clear();
}

/// Destructor
DataPacket::DataPacket::~DataPacket() = default;


///--------------------------------------------------------------------------///
///                               Data Packet Publisher                      ///
///--------------------------------------------------------------------------///
DataPacket::Publisher::Publisher() :
    mPublisher(std::make_unique<UPB::DataPacket::Publisher> ())
{
}

DataPacket::Publisher::Publisher(
    UMPS::Python::Messaging::Context &context)
{
    auto umpsContext = context.getSharedPointer();
    mPublisher = std::make_unique<UPB::DataPacket::Publisher> (umpsContext);
}

DataPacket::Publisher::~Publisher() = default;

///--------------------------------------------------------------------------///
///                            Initialization                                ///
///--------------------------------------------------------------------------///

///--------------------------------------------------------------------------///
///                            Initialization                                ///
///--------------------------------------------------------------------------///
void URTS::Python::ProxyBroadcasts::initialize(pybind11::module &m)
{
    pybind11::module pb = m.def_submodule("ProxyBroadcasts");
    pb.attr("__doc__") = "Proxy broadcasts in URTS.";
    //---------------------------------Data Packet----------------------------//
    pybind11::module dpm = pb.def_submodule("DataPacket");
    dpm.attr("__doc__") = "A proxy broadcast for data packets.";
    //--------------------------------Data Packet Message---------------------//
    pybind11::class_<URTS::Python::ProxyBroadcasts::DataPacket::DataPacket>
        dataPacket(dpm, "DataPacket");
    dataPacket.attr("__doc__") = "Defines a data packet message format.";
    dataPacket.def(pybind11::init<> ());
    dataPacket.doc() = R""""(
This is an URTS data packet; a container for sending snippets of continuous waveform data.

Required Properties:
    sampling_rate : float
        The sampling rate in Hz.
    network : str
        The network code - e.g., UU.
    station : str
        The station code - e.g., FORK.  
    channel : str
        The channel code - e.g., EHZ.
    location_code : str
        The location code - e.g., 00.

Optional Properties:
    data : np.array
        The time series. 
    start_time : int
        The UTC start time of the packet in microseconds since the
        epoch (Jan 1, 1970).

Read-Only Properties:
    number_of_samples : int
        The number of samples in the time series.
    message_type : str
        The message type.
)"""";
    dataPacket.def("__copy__", [](const DataPacket::DataPacket &self)
    {
        return DataPacket::DataPacket(self);
    });
    dataPacket.def_property_readonly("message_type",
                                     &DataPacket::DataPacket::getMessageType);
    dataPacket.def_property("network",
                            &DataPacket::DataPacket::getNetwork,
                            &DataPacket::DataPacket::setNetwork);
    dataPacket.def_property("station",
                            &DataPacket::DataPacket::getStation,
                            &DataPacket::DataPacket::setStation);
    dataPacket.def_property("channel",
                            &DataPacket::DataPacket::getChannel,
                            &DataPacket::DataPacket::setChannel);
    dataPacket.def_property("location_code",
                            &DataPacket::DataPacket::getLocationCode,
                            &DataPacket::DataPacket::setLocationCode);
    dataPacket.def_property("sampling_rate",
                            &DataPacket::DataPacket::getSamplingRate,
                            &DataPacket::DataPacket::setSamplingRate);
    dataPacket.def_property_readonly("number_of_samples",
                                     &DataPacket::DataPacket::getNumberOfSamples);
    dataPacket.def_property("data",
                            &DataPacket::DataPacket::getData,
                            &DataPacket::DataPacket::setData);
    dataPacket.def_property("start_time",
                            &DataPacket::DataPacket::getStartTime,
                            &DataPacket::DataPacket::setStartTime);
    dataPacket.def("clear",
                   &DataPacket::DataPacket::clear,
                   "Resets the class and releases memory.");
}
