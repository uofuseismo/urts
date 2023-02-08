#include <string>
#include <filesystem>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <umps/authentication/zapOptions.hpp>
#include "urts/services/scalable/packetCache/serviceOptions.hpp"
#include "urts/broadcasts/internal/dataPacket/subscriberOptions.hpp"
#include "private/isEmpty.hpp"

using namespace URTS::Services::Scalable::PacketCache;
namespace UDP = URTS::Broadcasts::Internal::DataPacket;
namespace UAuth = UMPS::Authentication;

class ServiceOptions::ServiceOptionsImpl
{
public:
    UDP::SubscriberOptions mSubscriberOptions;
    UAuth::ZAPOptions mReplierZAPOptions;
    std::string mReplierAddress;
    std::chrono::milliseconds mReplierPollingTimeOut{10};
    int mReplierSendHighWaterMark{8192};
    int mReplierReceiveHighWaterMark{4096};
    int mMaxPackets{300};
};

/// Constructor
ServiceOptions::ServiceOptions() :
    pImpl(std::make_unique<ServiceOptionsImpl> ())
{
}

/// Copy constructor
ServiceOptions::ServiceOptions(const ServiceOptions &options)
{
    *this = options;
}

/// Move constructor
ServiceOptions::ServiceOptions(ServiceOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment
ServiceOptions& ServiceOptions::operator=(const ServiceOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<ServiceOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
ServiceOptions& ServiceOptions::operator=(ServiceOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Reset class
void ServiceOptions::clear() noexcept
{
    pImpl = std::make_unique<ServiceOptionsImpl> ();
}

/// Destructor
ServiceOptions::~ServiceOptions() = default;

/// Max packets
void ServiceOptions::setMaximumNumberOfPackets(const int maxPackets)
{
    if (maxPackets < 1)
    {
        throw std::invalid_argument(
            "Maximum number of packet smust be positive");
    }
    pImpl->mMaxPackets = maxPackets;
}

int ServiceOptions::getMaximumNumberOfPackets() const noexcept
{
    return pImpl->mMaxPackets;
}

/// Data packet subscriber options
void ServiceOptions::setDataPacketSubscriberOptions(
    const UDP::SubscriberOptions &options)
{
    if (!options.haveAddress())
    {
        throw std::invalid_argument("Address not set");
    }
    pImpl->mSubscriberOptions = options;
}

UDP::SubscriberOptions ServiceOptions::getDataPacketSubscriberOptions() const
{
    if (!haveDataPacketSubscriberOptions())
    {
        throw std::runtime_error("Data packet subscriber options not set");
    }
    return pImpl->mSubscriberOptions;
}
bool ServiceOptions::haveDataPacketSubscriberOptions() const noexcept
{
    return pImpl->mSubscriberOptions.haveAddress();
}

/// Sets the backend address
void ServiceOptions::setReplierAddress(const std::string &address)
{
    if (::isEmpty(address)){throw std::invalid_argument("Address is empty");}
    pImpl->mReplierAddress = address;
}

std::string ServiceOptions::getReplierAddress() const
{
    if (!haveReplierAddress())
    {
        throw std::runtime_error("Replier address not set");
    }
    return pImpl->mReplierAddress; 
}

bool ServiceOptions::haveReplierAddress() const noexcept
{
    return !pImpl->mReplierAddress.empty();
}

/// ZAP Options
void ServiceOptions::setReplierZAPOptions(
    const UAuth::ZAPOptions &zapOptions) noexcept
{
    pImpl->mReplierZAPOptions = zapOptions;
}

UAuth::ZAPOptions ServiceOptions::getReplierZAPOptions() const noexcept
{
    return pImpl->mReplierZAPOptions;
}

/// Time out
void ServiceOptions::setReplierPollingTimeOut(
    const std::chrono::milliseconds &timeOut)
{
    if (timeOut.count() < 0)
    {
        throw std::invalid_argument("Time out cannot be negative");
    }
    pImpl->mReplierPollingTimeOut = timeOut;
}

std::chrono::milliseconds 
ServiceOptions::getReplierPollingTimeOut() const noexcept
{
    return pImpl->mReplierPollingTimeOut;
}

/// High water mark
void ServiceOptions::setReplierSendHighWaterMark(const int highWaterMark)
{
    pImpl->mReplierSendHighWaterMark = highWaterMark;
}

int ServiceOptions::getReplierSendHighWaterMark() const noexcept
{
    return pImpl->mReplierSendHighWaterMark;
}

void ServiceOptions::setReplierReceiveHighWaterMark(const int highWaterMark)
{
    pImpl->mReplierReceiveHighWaterMark = highWaterMark;
}

int ServiceOptions::getReplierReceiveHighWaterMark() const noexcept
{
    return pImpl->mReplierReceiveHighWaterMark;
}

/*
void ServiceOptions::parseInitializationFile(const std::string &iniFile,
                                             const std::string &section)
{
    if (!std::filesystem::exists(iniFile))
    {   
        throw std::invalid_argument("Initialization file: "
                                  + iniFile + " does not exist");
    }   
    ServiceOptions options;
    boost::property_tree::ptree propertyTree;
    boost::property_tree::ini_parser::read_ini(iniFile, propertyTree);
    // Capped collections
    auto maxPackets
        = propertyTree.get<int> (section + ".maxPackets",
                                 options.getMaximumNumberOfPackets()); 
    options.setMaximumNumberOfPackets(maxPackets);
    // Broadcast address
    auto broadcastAddress = propertyTree.get<std::string>
                            (section + ".broadcastAddress");

    auto recvHWM
        = propertyTree.get<int> (section + ".broadcastReceiveHighWaterMark", 0);
    // Replier Connection
    auto replierAddress = propertyTree.get<std::string>
                          (section + ".replierAddress");
    if (!replierAddress.empty()){options.setReplierAddress(replierAddress);}
 
    auto sendHWM
        = propertyTree.get<int> (section + ".replierSendHighWaterMark",
                                 options.getReplierSendHighWaterMark());
    options.setReplierSendHighWaterMark(sendHWM);

    recvHWM = propertyTree.get<int> (section + ".replierReceiveHighWaterMark",
                                     options.getReplierReceiveHighWaterMark());
    options.setReplierReceiveHighWaterMark(recvHWM);

    auto iTimeOut
       = propertyTree.get<int64_t> (section + ".replierPollingTimeOut",
                                    options.getReplierPollingTimeOut().count());
    std::chrono::milliseconds timeOut{iTimeOut};
    options.setReplierPollingTimeOut(timeOut);
    // Got everything and didn't throw -> copy to this
    *this = std::move(options);
}
*/
