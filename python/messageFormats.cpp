#include <vector>
#include <cstring>
#include <urts/messageFormats/dataPacket.hpp>
#include <umps/messageFormats/message.hpp>
#include <umps/messageFormats/staticUniquePointerCast.hpp>
#include "messageFormats.hpp"


using namespace URTS::Python::MessageFormats;
namespace UMF = URTS::MessageFormats;

///--------------------------------------------------------------------------///
///                               Data Packet                                ///
///--------------------------------------------------------------------------///
/// Constructor
DataPacket::DataPacket() :
    pImpl(std::make_unique<UMF::DataPacket> ())
{
}

/// Copy constructor
DataPacket::DataPacket(const DataPacket &packet)
{
    *this = packet;
}

/// Construct from native class
DataPacket::DataPacket(const UMF::DataPacket &packet)
{
    *this = packet;
}

/// Move constructor
DataPacket::DataPacket(DataPacket &&packet) noexcept
{
    *this = packet;
}

/// Copy assignment
DataPacket& DataPacket::operator=(const DataPacket &packet)
{
    if (&packet == this){return *this;}
    pImpl = std::make_unique<UMF::DataPacket> (*packet.pImpl);
    return *this;
}

/// Copy from native class
DataPacket& DataPacket::operator=(const UMF::DataPacket &packet)
{
    pImpl = std::make_unique<UMF::DataPacket> (packet);
    return *this;
}

/// Move assignment
DataPacket& DataPacket::operator=(DataPacket &&packet) noexcept
{
    if (&packet == this){return *this;}
    pImpl = std::move(packet.pImpl);
    return *this;
}

const URTS::MessageFormats::DataPacket&
DataPacket::getNativeClassReference() const noexcept
{
    return *pImpl;
}

URTS::MessageFormats::DataPacket DataPacket::getNativeClass() const noexcept
{
    return *pImpl;
}

/// Network
void DataPacket::setNetwork(const std::string &network)
{
    pImpl->setNetwork(network);
}

std::string DataPacket::getNetwork() const
{
    return pImpl->getNetwork();
}

/// Station
void DataPacket::setStation(const std::string &station)
{
    pImpl->setStation(station);
}

std::string DataPacket::getStation() const
{
    return pImpl->getStation();
}

/// Channel
void DataPacket::setChannel(const std::string &channel)
{
    pImpl->setChannel(channel);
}

std::string DataPacket::getChannel() const
{
    return pImpl->getChannel();
}

/// Location code
void DataPacket::setLocationCode(const std::string &location)
{
    pImpl->setLocationCode(location);
}

std::string DataPacket::getLocationCode() const
{
    return pImpl->getLocationCode();
}

/// Sampling rate
void DataPacket::setSamplingRate(const double samplingRate)
{
    pImpl->setSamplingRate(samplingRate);
}

double DataPacket::getSamplingRate() const
{
    return pImpl->getSamplingRate();
}

/// Start time
void DataPacket::setStartTime(const std::chrono::microseconds &time) noexcept
{
    pImpl->setStartTime(time);
}

std::chrono::microseconds DataPacket::getStartTime() const noexcept
{
    return pImpl->getStartTime();
}

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

/// Base class
void DataPacket::fromBaseClass(UMPS::MessageFormats::IMessage &message)
{
    if (message.getMessageType() != pImpl->getMessageType())
    {   
        throw std::invalid_argument("Expecting message type: "
                                    + pImpl->getMessageType()
                                    + " but given: "
                                    + message.getMessageType());
    }   
    pImpl = UMPS::MessageFormats::static_unique_pointer_cast<UMF::DataPacket>
            (message.clone());
}

std::unique_ptr<UMPS::Python::MessageFormats::IMessage> DataPacket::clone(
        const std::unique_ptr<UMPS::MessageFormats::IMessage> &message) const
{
    if (message->getMessageType() != pImpl->getMessageType())
    {
        throw std::invalid_argument("Expecting: " + pImpl->getMessageType()
                                    + " but got: " + message->getMessageType());
    }
    auto copy = UMPS::MessageFormats::static_unique_pointer_cast
                <URTS::MessageFormats::DataPacket> (message->clone());
    return std::make_unique<DataPacket> (*copy);
}

std::unique_ptr<UMPS::Python::MessageFormats::IMessage>
DataPacket::createInstance() const
{
    return std::make_unique<DataPacket> ();
}

std::unique_ptr<UMPS::MessageFormats::IMessage>
DataPacket::getInstanceOfBaseClass() const noexcept
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> message
        = pImpl->createInstance();
    return message;
}
/// Message type
std::string DataPacket::getMessageType() const noexcept
{
    return pImpl->getMessageType();
}

/// Reset class
void DataPacket::clear() noexcept
{
    pImpl->clear();
}

/// Destructor
DataPacket::~DataPacket() = default;

///--------------------------------------------------------------------------///
///                            Initialization                                ///
///--------------------------------------------------------------------------///
void URTS::Python::MessageFormats::initialize(pybind11::module &m)
{
    pybind11::module mfm = m.def_submodule("MessageFormats");
    mfm.attr("__doc__") = "Core message formats used in URTS.";
    //---------------------------------Data Packet----------------------------//
    pybind11::class_<URTS::Python::MessageFormats::DataPacket>
        dataPacket(mfm, "DataPacket");
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
    dataPacket.def("__copy__", [](const DataPacket &self)
    {
        return DataPacket(self);
    });
    dataPacket.def_property_readonly("message_type",
                                     &DataPacket::getMessageType);
    dataPacket.def_property("network",
                            &DataPacket::getNetwork,
                            &DataPacket::setNetwork);
    dataPacket.def_property("station",
                            &DataPacket::getStation,
                            &DataPacket::setStation);
    dataPacket.def_property("channel",
                            &DataPacket::getChannel,
                            &DataPacket::setChannel);
    dataPacket.def_property("location_code",
                            &DataPacket::getLocationCode,
                            &DataPacket::setLocationCode);
    dataPacket.def_property("sampling_rate",
                            &DataPacket::getSamplingRate,
                            &DataPacket::setSamplingRate);
    dataPacket.def_property_readonly("number_of_samples",
                                     &DataPacket::getNumberOfSamples);
    dataPacket.def_property("data",
                            &DataPacket::getData,
                            &DataPacket::setData);
    dataPacket.def_property("start_time",
                            &DataPacket::getStartTime,
                            &DataPacket::setStartTime);
    dataPacket.def("clear",
                   &DataPacket::clear,
                   "Resets the class and releases memory.");
}
