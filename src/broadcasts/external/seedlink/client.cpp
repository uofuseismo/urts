#include <umps/logging/standardOut.hpp>
#include <libslink.h>
#include <slplatform.h>
#include "urts/broadcasts/external/seedlink/client.hpp"
#include "urts/broadcasts/external/seedlink/clientOptions.hpp"

using namespace URTS::Broadcasts::External::SEEDLink;

class Client::ClientImpl
{
public:
    ~ClientImpl()
    {
        disconnect();
    }
    void disconnect()
    {
        if (mSEEDLinkConnection)
        {
            if (mSEEDLinkConnection->link != -1)
            {
                sl_disconnect(mSEEDLinkConnection);
            }
            mSEEDLinkConnection = nullptr;
        }
    }
//private:
    SLCD *mSEEDLinkConnection{nullptr};
    class ClientOptions mOptions;
};
    
/// Constructor
Client::Client() :
    pImpl(std::make_unique<ClientImpl> ())
{
}

/// Constructor
Client::~Client() = default;
