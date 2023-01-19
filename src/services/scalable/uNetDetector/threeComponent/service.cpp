#include <mutex>
#include <thread>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/messaging/routerDealer/reply.hpp>
#include <umps/messaging/routerDealer/replyOptions.hpp>
#include <umps/messageFormats/failure.hpp>
//#include "urts/services/scalable/uNetDetector/threeComponent/service.hpp"
//#include "urts/services/scalable/uNetDetector/threeComponent/serviceOptions.hpp"
#include "urts/services/scalable/uNetDetector/threeComponent/preprocessingRequest.hpp"
/*
#include "urts/services/scalable/packetCache/bulkDataRequest.hpp"
#include "urts/services/scalable/packetCache/bulkDataResponse.hpp"
#include "urts/services/scalable/packetCache/sensorRequest.hpp"
#include "urts/services/scalable/packetCache/sensorResponse.hpp"
#include "urts/broadcasts/internal/dataPacket/subscriber.hpp"
#include "urts/broadcasts/internal/dataPacket/subscriberOptions.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "private/threadSafeQueue.hpp"
*/

using namespace URTS::Services::Scalable::UNetDetector::ThreeComponent;

class Service::ServiceImpl
{
public:
    /// @brief Constructor
    ServiceImpl(std::shared_ptr<UMPS::Messaging::Context> responseContext = nullptr,
                const std::shared_ptr<UMPS::Logging::ILog> &logger = nullptr)
    {
        if (logger == nullptr)
        {   
            mLogger = std::make_shared<UMPS::Logging::StandardOut> ();
        }
        else
        {
            mLogger = logger;
        }
        mReplier
            = std::make_unique<URouterDealer::Reply> (responseContext, mLogger);
    }
    // Respond to data requests
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage>
        callback(const std::string &messageType,
                 const void *messageContents, const size_t length) noexcept
    {
        // Get data
        mLogger->debug("Request received");
        PreprocessingRequest preprocessingRequest;
        if (messageType == preprocessingRequest)
        {
            mLogger->debug("Preprocessing request received");
            PreprocessingResponse response;
            try
            {
                preprocessingRequest.fromMessage(messageContents, length);
            }
            catch (const std::exception &e)
            {
                response.setResponse(PreprocessingResponse::InvalidMessage);
                return response.clone();
            }
            response.setResponse(PreprocessingResponse::Success);
            response.clone();
        }
        // Send something back so they don't wait forever
        mLogger->error("Unhandled message type: " + messageType);
        UMPS::MessageFormats::Failure response;
        response.setDetails("Unhandled message type");
        return response.clone();
    }

};
