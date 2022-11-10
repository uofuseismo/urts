#include <string>
#include <umps/messageFormats/message.hpp>
#include <umps/messageFormats/messages.hpp>
#include <umps/messageFormats/failure.hpp>
#include <umps/messageFormats/staticUniquePointerCast.hpp>
#include <umps/messaging/routerDealer/requestOptions.hpp>
#include <umps/messaging/routerDealer/request.hpp>
#include <umps/authentication/zapOptions.hpp>
#include "urts/services/standalone/incrementer/requestor.hpp"
#include "urts/services/standalone/incrementer/requestorOptions.hpp"
#include "urts/services/standalone/incrementer/itemsRequest.hpp"
#include "urts/services/standalone/incrementer/itemsResponse.hpp"
#include "urts/services/standalone/incrementer/incrementRequest.hpp"
#include "urts/services/standalone/incrementer/incrementResponse.hpp"

using namespace URTS::Services::Standalone::Incrementer;
namespace UCI = UMPS::Services::ConnectionInformation;
namespace URouterDealer = UMPS::Messaging::RouterDealer;
namespace UMF = UMPS::MessageFormats;

namespace
{
[[nodiscard]] UMF::Messages createMessageFormats()
{
    std::unique_ptr<UMPS::MessageFormats::IMessage> incrementResponse
        = std::make_unique<IncrementResponse> ();
    std::unique_ptr<UMPS::MessageFormats::IMessage> itemsResponse
        = std::make_unique<ItemsResponse> ();
    std::unique_ptr<UMPS::MessageFormats::IMessage> failureResponse
        = std::make_unique<UMF::Failure> ();
    UMPS::MessageFormats::Messages messageFormats;
    messageFormats.add(incrementResponse);
    messageFormats.add(itemsResponse);
    messageFormats.add(failureResponse);
    return messageFormats;
}
}

class Requestor::RequestorImpl
{
public:
    RequestorImpl(std::shared_ptr<UMPS::Messaging::Context> context,
                  std::shared_ptr<UMPS::Logging::ILog> logger)
    {
        mRequestor = std::make_unique<URouterDealer::Request> (context, logger);
    }
    std::unique_ptr<URouterDealer::Request> mRequestor;
    RequestorOptions mRequestOptions;
    UMF::Failure mFailureMessage;
};

/// C'tor
Requestor::Requestor() :
    pImpl(std::make_unique<RequestorImpl> (nullptr, nullptr))
{
}

Requestor::Requestor(std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<RequestorImpl> (nullptr, logger))
{
}

Requestor::Requestor(std::shared_ptr<UMPS::Messaging::Context> &context) :
    pImpl(std::make_unique<RequestorImpl> (context, nullptr))
{
}

Requestor::Requestor(std::shared_ptr<UMPS::Messaging::Context> &context,
                     std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<RequestorImpl> (context, logger))
{
}

/// Move c'tor
Requestor::Requestor(Requestor &&request) noexcept
{
    *this = std::move(request);
}

/// Move assignment
Requestor& Requestor::operator=(Requestor &&request) noexcept
{
    if (&request == this){return *this;}
    pImpl = std::move(request.pImpl);
    return *this;
}

/// Destructor
Requestor::~Requestor() = default;

/// Initialize
void Requestor::initialize(const RequestorOptions &options)
{
    if (!options.haveAddress())
    {
        throw std::invalid_argument("Address not set");
    }
    URouterDealer::RequestOptions requestOptions;
    requestOptions.setAddress(options.getAddress());
    requestOptions.setZAPOptions(options.getZAPOptions());
    requestOptions.setMessageFormats(createMessageFormats());
    requestOptions.setSendHighWaterMark(options.getSendHighWaterMark());
    requestOptions.setReceiveHighWaterMark(options.getReceiveHighWaterMark());
    requestOptions.setSendTimeOut(options.getSendTimeOut());
    requestOptions.setReceiveTimeOut(options.getReceiveTimeOut());
    // Initialize the request socket
    pImpl->mRequestor->initialize(requestOptions);
    // Save the options
    pImpl->mRequestOptions = options;
}

/// Initialized?
bool Requestor::isInitialized() const noexcept
{
    return pImpl->mRequestor->isInitialized();
}


/// Disconnect
void Requestor::disconnect()
{
    pImpl->mRequestor->disconnect();
}

/// Request items on which to increment
std::unique_ptr<ItemsResponse> Requestor::request(const ItemsRequest &request)
{
    auto message = pImpl->mRequestor->request(request);
    if (message->getMessageType() == pImpl->mFailureMessage.getMessageType())
    {
        auto failureMessage = UMF::static_unique_pointer_cast<UMF::Failure>
                              (std::move(message));
        auto errorMessage
            = "Failure message received for items request.  Failed with: "
            + failureMessage->getDetails();
        throw std::runtime_error(errorMessage);
    }
    auto response = UMF::static_unique_pointer_cast<ItemsResponse>
                    (std::move(message));
    return response;
}

/// Request increment on item
std::unique_ptr<IncrementResponse>
    Requestor::request(const IncrementRequest &request)
{
    if (!request.haveItem()){throw std::invalid_argument("Item not set");}
    auto message = pImpl->mRequestor->request(request);
    if (message->getMessageType() == pImpl->mFailureMessage.getMessageType())
    {
        auto failureMessage = UMF::static_unique_pointer_cast<UMF::Failure>
                              (std::move(message));
        auto errorMessage
            = "Failure message received for increment request.  Failed with: "
            + failureMessage->getDetails();
        throw std::runtime_error(errorMessage);
    }
    auto response = UMF::static_unique_pointer_cast<IncrementResponse>
                    (std::move(message));
    return response;
}
