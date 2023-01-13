#include <string>
#include "urts/broadcasts/external/seedlink/clientOptions.hpp"

using namespace URTS::Broadcasts::External::SEEDLink;

class ClientOptions::ClientOptionsImpl
{
public:
    std::string mAddress{"rtserve.iris.washington.edu"};
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
