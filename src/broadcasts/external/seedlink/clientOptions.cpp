#include <string>
#include <filesystem>
#include "urts/broadcasts/external/seedlink/clientOptions.hpp"

using namespace URTS::Broadcasts::External::SEEDLink;

class ClientOptions::ClientOptionsImpl
{
public:
    std::string mAddress{"rtserve.iris.washington.edu"};
    std::filesystem::path mStateFile;
    int mSEEDRecordSize{512};
    int mMaxQueueSize{8192};
    uint16_t mStateFileInterval{100};
    uint16_t mPort{18000};
};

/// Constructor
ClientOptions::ClientOptions() :
    pImpl(std::make_unique<ClientOptionsImpl> ())
{
}

/// Copy constructor
ClientOptions::ClientOptions(const ClientOptions &options)
{
    *this = options;
}

/// Move constructor
ClientOptions::ClientOptions(ClientOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment
ClientOptions& ClientOptions::operator=(const ClientOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<ClientOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
ClientOptions& ClientOptions::operator=(ClientOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Address
void ClientOptions::setAddress(const std::string &address)
{
    if (address.empty()){throw std::invalid_argument("Address is empty");}
    pImpl->mAddress = address;
}

std::string ClientOptions::getAddress() const noexcept
{
    return pImpl->mAddress;
}

/// Port
void ClientOptions::setPort(const uint16_t port) noexcept
{
    pImpl->mPort = port;
}

uint16_t ClientOptions::getPort() const noexcept
{
    return pImpl->mPort;
}

/// Destructor
ClientOptions::~ClientOptions() = default;

/// Reset class
void ClientOptions::clear() noexcept
{
    pImpl = std::make_unique<ClientOptionsImpl> ();
}

/// Sets a SEEDLink state file
void ClientOptions::setStateFile(const std::string &stateFileName)
{
    if (stateFileName.empty())
    {
        pImpl->mStateFile.clear();
        return;
    }
    std::filesystem::path stateFile(stateFileName); 
    auto parentPath = stateFile.parent_path();
    if (!parentPath.empty())
    {
        if (!std::filesystem::exists(parentPath))
        {
            if (!std::filesystem::create_directories(parentPath))
            {
                throw std::runtime_error("Failed to create state file path");
            }
        }
    }
    pImpl->mStateFile = stateFile;
}

std::string ClientOptions::getStateFile() const
{
    if (!haveStateFile()){throw std::runtime_error("State file not set");}
    return pImpl->mStateFile;
}

bool ClientOptions::haveStateFile() const noexcept
{
    return !pImpl->mStateFile.empty();
}

/// State file interval
void ClientOptions::setStateFileUpdateInterval(const uint16_t interval) noexcept
{
    pImpl->mStateFileInterval = interval;
}

uint16_t ClientOptions::getStateFileUpdateInterval() const noexcept
{
    return pImpl->mStateFileInterval;
}

/// The SEEDLink record size
void ClientOptions::setSEEDRecordSize(const int recordSize)
{
    if (recordSize != 512 && recordSize != 256 && recordSize != 128)
    {
        throw std::invalid_argument("Record size " + std::to_string(recordSize)
                               + " is invalid.  Can only use 128, 256, or 512");
    }
    pImpl->mSEEDRecordSize = recordSize;
}

int ClientOptions::getSEEDRecordSize() const noexcept
{
    return pImpl->mSEEDRecordSize;
}

/// Maximum internal queue size
void ClientOptions::setMaximumInternalQueueSize(const int maxSize)
{
    if (maxSize < 1)
    {
        throw std::invalid_argument(
            "Maximum internal queue size must be postive");
    }
    pImpl->mMaxQueueSize = maxSize;
}

int ClientOptions::getMaximumInternalQueueSize() const noexcept
{
    return pImpl->mMaxQueueSize;
}
