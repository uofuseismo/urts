#include <iostream>
#include <string>
#include <filesystem>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <umps/authentication/zapOptions.hpp>
#include "urts/services/standalone/incrementer/serviceOptions.hpp"
#include "private/isEmpty.hpp"

using namespace URTS::Services::Standalone::Incrementer;
namespace UAuth = UMPS::Authentication;

class ServiceOptions::ServiceOptionsImpl
{
public:
    UAuth::ZAPOptions mZAPOptions;
    std::filesystem::path mSqlite3FileName{
        std::string(std::getenv("HOME"))
      + "/.local/share/URTS/tables/incrementer.sqlite3"};
    std::string mAddress;
    std::chrono::milliseconds mPollingTimeOut{10};
    int64_t mInitialValue{0};
    int mSendHighWaterMark{1024};
    int mReceiveHighWaterMark{1024};
    int mIncrement{1};
    bool mDeleteIfExists{false};
};

/// C'tor
ServiceOptions::ServiceOptions() :
    pImpl(std::make_unique<ServiceOptionsImpl> ())
{
}

/// Copy assignment
ServiceOptions::ServiceOptions(const ServiceOptions &options)
{
    *this = options;
}

/// Move assignment
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

/// Clear
void ServiceOptions::clear() noexcept
{
    pImpl = std::make_unique<ServiceOptionsImpl> ();
}

/// Destructor
ServiceOptions::~ServiceOptions() = default;

/// Name
void ServiceOptions::setSqlite3FileName(const std::string &fileName)
{
    if (::isEmpty(fileName))
    {
        throw std::invalid_argument("File name is empty");
    }
    std::filesystem::path filePath{fileName};
    if (!std::filesystem::exists(filePath))
    {
        auto parentPath = filePath.parent_path();
        if (!parentPath.empty())
        {
            if (!std::filesystem::exists(parentPath))
            {
                if (!std::filesystem::create_directories(parentPath))
                {
                    throw std::runtime_error("Failed to create path: "
                                           + std::string{parentPath});
                }
            }
        }
    }
    pImpl->mSqlite3FileName = fileName;
}

std::string ServiceOptions::getSqlite3FileName() const noexcept
{
    return pImpl->mSqlite3FileName;
}

/// Sets the backend address
void ServiceOptions::setAddress(const std::string &address)
{
    if (::isEmpty(address)){throw std::invalid_argument("Address is empty");}
    pImpl->mAddress = address;
}

std::string ServiceOptions::getAddress() const
{
    if (!haveAddress()){throw std::runtime_error("Address not set");}
    return pImpl->mAddress; 
}

bool ServiceOptions::haveAddress() const noexcept
{
    return !pImpl->mAddress.empty();
}

/// Delete SQLite3 file if it exists
void ServiceOptions::toggleDeleteSqlite3FileIfExists(
    const bool deleteIfExists) noexcept
{
    pImpl->mDeleteIfExists = deleteIfExists;
}

bool ServiceOptions::deleteSqlite3FileIfExists() const noexcept
{
    return pImpl->mDeleteIfExists;
}

/// Incrementer increment
void ServiceOptions::setIncrement(const int increment)
{
    if (increment < 1)
    {
        throw std::invalid_argument("Increment must be positive"); 
    }
    pImpl->mIncrement = increment;
}

int ServiceOptions::getIncrement() const noexcept
{
    return pImpl->mIncrement;
}

/// Initial value
void ServiceOptions::setInitialValue(const int32_t value) noexcept
{
    pImpl->mInitialValue = static_cast<int64_t> (value);
}

int64_t ServiceOptions::getInitialValue() const noexcept
{
    return pImpl->mInitialValue;
}

/// ZAP Options
void ServiceOptions::setZAPOptions(const UAuth::ZAPOptions &zapOptions) noexcept
{
    pImpl->mZAPOptions = zapOptions;
}

UAuth::ZAPOptions ServiceOptions::getZAPOptions() const noexcept
{
    return pImpl->mZAPOptions;
}

/// Time out
void ServiceOptions::setPollingTimeOut(
    const std::chrono::milliseconds &timeOut)
{
    if (timeOut.count() < 0)
    {
        throw std::invalid_argument("Time out cannot be negative");
    }
    pImpl->mPollingTimeOut = timeOut;
}
 
std::chrono::milliseconds ServiceOptions::getPollingTimeOut() const noexcept
{
    return pImpl->mPollingTimeOut;
}

/// High water mark
void ServiceOptions::setSendHighWaterMark(const int highWaterMark)
{
    pImpl->mSendHighWaterMark = highWaterMark;
}

int ServiceOptions::getSendHighWaterMark() const noexcept
{
    return pImpl->mSendHighWaterMark;
}

void ServiceOptions::setReceiveHighWaterMark(const int highWaterMark)
{
    pImpl->mReceiveHighWaterMark = highWaterMark;
}

int ServiceOptions::getReceiveHighWaterMark() const noexcept
{
    return pImpl->mReceiveHighWaterMark;
}

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

    // Incrementer details
    auto sqlite3FileName
        = propertyTree.get<std::string> (section + ".sqlite3FileName",
                                         options.getSqlite3FileName());
    options.setSqlite3FileName(sqlite3FileName);

    auto increment = propertyTree.get<int> (section + ".increment", 1); 
    options.setIncrement(increment);

    auto initialValue = propertyTree.get<int> (section + ".initialValue", 0); 
    options.setInitialValue(initialValue);

    // Connection
    auto address = propertyTree.get<std::string> (section + ".address");
    if (!address.empty()){options.setAddress(address);}
 
    auto sendHWM = propertyTree.get<int> (section + ".sendHighWaterMark",
                                          options.getSendHighWaterMark());
    options.setSendHighWaterMark(sendHWM);

    auto recvHWM = propertyTree.get<int> (section + ".receiveHighWaterMark",
                                          options.getReceiveHighWaterMark());
    options.setReceiveHighWaterMark(recvHWM);

    auto iTimeOut
        = propertyTree.get<int64_t> (section + ".pollingTimeOut",
                                     options.getPollingTimeOut().count());
    std::chrono::milliseconds timeOut{iTimeOut};
    options.setPollingTimeOut(timeOut);
    // Got everything and didn't throw -> copy to this
    *this = std::move(options);
}
