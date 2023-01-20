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
#include "urts/services/scalable/uNetDetector/threeComponent/inferenceRequest.hpp"
#include "urts/services/scalable/uNetDetector/threeComponent/inferenceResponse.hpp"
#include "urts/services/scalable/uNetDetector/threeComponent/preprocessingRequest.hpp"
#include "urts/services/scalable/uNetDetector/threeComponent/preprocessingResponse.hpp"
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
    // Respond to processing requests
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage>
        callback(const std::string &messageType,
                 const void *messageContents, const size_t length) noexcept
    {
        mLogger->debug("Request received");
        // Inference
        InferenceRequest inferenceRequest;
        if (messageType == inferenceRequest.getMessageType())
        {
            mLogger->debug("Inference request received");
            InferenceResponse response;
            try
            {
                inferenceRequest.fromMessage(messageContents, length);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to upnack preprocessing request: "
                             + std::string{e.what()});
                response.setResponse(InferenceResponse::InvalidMessage);
                return response.clone();
            }
            return response.clone();
        }
        // Preprocessing
        PreprocessingRequest preprocessingRequest;
        if (messageType == preprocessingRequest.getMessageType())
        {
            mLogger->debug("Preprocessing request received");
            PreprocessingResponse response;
            try
            {
                preprocessingRequest.fromMessage(messageContents, length);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to upnack preprocessing request: "
                             + std::string{e.what()});
                response.setResponse(PreprocessingResponse::InvalidMessage);
                return response.clone();
            }
            // Ensure the signals are set
            if (!response.haveSignals())
            {
                response.setResponse(PreprocessingResponse::InvalidMessage);
                return response.clone();
            }
            // Process the data
            try
            {
                const auto vertical = request.getVerticalSignalReference();
                const auto north    = request.getNorthSignalReference();
                const auto east     = request.getEastSignalReference();
                // Process data
                std::vector<double> verticalResult;
                std::vector<double> northResult;
                std::vector<double> eastResult;
                // Set data on response
                response.setSamplingRate(mTargetSamplingRate);
                response.setVerticalNorthEastSignal(std::move(verticalResult),
                                                    std::move(northResult),
                                                    std::move(eastResult));
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to preprocess data: "
                             + std::string{e.what()});
                response.setResponse(PreprocessingResponse::AlgorithmFailure);
                return response.clone();
            }
            response.setResponse(PreprocessingResponse::Success);
            response.clone();
        }
        // No idea -> Send something back so they don't wait forever
        mLogger->error("Unhandled message type: " + messageType);
        UMPS::MessageFormats::Failure response;
        response.setDetails("Unhandled message type");
        return response.clone();
    }
///private:
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<URouterDealer::Reply> mReplier{nullptr};
    class UModels::ProcessData mPreprocess;
    const double mTargetSamplingRate{UModels::ProcessData::getTargetSamplingRate()};
    const int mMinimumSignalLength{1008};
};
