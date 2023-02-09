#include <mutex>
#include <thread>
#include <uussmlmodels/detectors/uNetThreeComponentP/inference.hpp>
#include <uussmlmodels/detectors/uNetThreeComponentP/preprocessing.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/messaging/routerDealer/reply.hpp>
#include <umps/messaging/routerDealer/replyOptions.hpp>
#include <umps/messageFormats/failure.hpp>
#include "urts/services/scalable/detectors/uNetThreeComponentP/service.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/serviceOptions.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/inferenceRequest.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/inferenceResponse.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/preprocessingRequest.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/preprocessingResponse.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/processingRequest.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/processingResponse.hpp"
//#include "private/threadSafeQueue.hpp"

namespace URouterDealer = UMPS::Messaging::RouterDealer;
using namespace URTS::Services::Scalable::Detectors::UNetThreeComponentP;
namespace UModels = UUSSMLModels::Detectors::UNetThreeComponentP;

namespace
{

/*
template<typename U>
void evaluateModel(const std::vector<U> &vertical,
                   const std::vector<U> &north,
                   const std::vector<U> &east,
                   const UModels::Inference &inference,
                   InferenceResponse *response,
                   std::shared_ptr<UMPS::Logging::ILog> &logger)
{
    try
    {
        auto probabilitySignal
            = inference.predictProbability(vertical, north, east);
        response->setProbabilitySignal(std::move(probabilitySignal));
        response->setReturnCode(InferenceResponse::Success);
    }
    catch (const std::exception &e)
    {
        if (logger != nullptr)
        {
            logger->error("Failed to evaluate model.  Failed with "
                        + std::string{e.what()});
        }
        response->setReturnCode(InferenceResponse::AlgorithmFailure);
    }
}

void evaluateModel(const InferenceRequest &request,
                   const UModels::Inference &inference,
                   InferenceResponse *response,
                   std::shared_ptr<UMPS::Logging::ILog> &logger)
{
    const auto &vertical = request.getVerticalSignalReference();
    const auto &north    = request.getNorthSignalReference();
    const auto &east     = request.getEastSignalReference();
    evaluateModel(vertical, north, east, inference, response, logger);
}

std::unique_ptr<UMPS::MessageFormats::IMessage>
preprocessSignal(const PreprocessingRequest &request,
                 UModels::Preprocessing &preprocess,
                 std::shared_ptr<UMPS::Logging::ILog> &logger)
{
    const auto &vertical = request.getVerticalSignalReference();
    const auto &north    = request.getNorthSignalReference();
    const auto &east     = request.getEastSignalReference();
    // Process data
    auto [verticalResult, northResult, eastResult]
         = preprocess.process(vertical, north, east);
    // Set data on response
    PreprocessingResponse response;
    response.setSamplingRate(UModels::Preprocessing::getTargetSamplingRate());
    try
    {
        response.setVerticalNorthEastSignal(std::move(verticalResult),
                                            std::move(northResult),
                                            std::move(eastResult));
        response.setReturnCode(PreprocessingResponse::Success);
    }
    catch (const std::exception &e)
    {
        if (logger != nullptr)
        {
            logger->error("Failed to perform preprocessing.  Failed with: "
                        + std::string{e.what()});
        }
        response.setReturnCode(PreprocessingResponse::AlgorithmFailure); 
    }
    return response.clone();
}
*/
 
}

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
    /// Destructor
    ~ServiceImpl()
    {
        stop();
    }
    /// @brief Stops the threads
    void stop()
    {
        mLogger->debug("UNetThreeComponentP service stopping threads...");
        setRunning(false);
        if (mReplier != nullptr){mReplier->stop();}
    }
    /// @brief Starts the service
    void start()
    {
        stop();
        setRunning(true); 
        std::lock_guard<std::mutex> lockGuard(mMutex);
        mLogger->debug("Starting replier service...");
        mReplier->start();
    }
    /// @result True indicates the threads should keep running
    [[nodiscard]] bool keepRunning() const
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        return mKeepRunning;
    }
    /// @brief Toggles this as running or not running
    void setRunning(const bool running)
    {
        std::lock_guard<std::mutex> lockGuard(mMutex);
        mKeepRunning = running;
    }
    // Respond to processing requests
    [[nodiscard]] std::unique_ptr<UMPS::MessageFormats::IMessage>
        callback(const std::string &messageType,
                 const void *messageContents, const size_t length) noexcept
    {
        mLogger->debug("Request received");
        //-----------Everything -> Process data then perform inference--------//
        ProcessingRequest processingRequest;
        if (messageType == processingRequest.getMessageType())
        {
            mLogger->debug("Processing request received");
            ProcessingResponse response;
            try
            {
                processingRequest.fromMessage(
                    reinterpret_cast<const char *> (messageContents), length);
                response.setIdentifier(processingRequest.getIdentifier());
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to upnack preprocessing request: "
                             + std::string{e.what()});
                response.setReturnCode(ProcessingResponse::InvalidMessage);
                return response.clone();
            }
            // Process the data
            std::tuple<std::vector<double>,
                       std::vector<double>,
                       std::vector<double>> processedSignals;
            try
            {
                const auto &vertical
                    = processingRequest.getVerticalSignalReference();
                const auto &north
                    = processingRequest.getNorthSignalReference();
                const auto &east
                    = processingRequest.getEastSignalReference();
                auto samplingRate = processingRequest.getSamplingRate();
                // Process data
                processedSignals
                    = mPreprocess.process(vertical, north, east, samplingRate);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to preprocess data.  Failed with "
                             + std::string{e.what()});
                response.setReturnCode(ProcessingResponse::PreprocessingFailure);
                return response.clone();
            }
            // Extract signals
            const auto &verticalProcessed = std::get<0> (processedSignals);
            const auto &northProcessed    = std::get<1> (processedSignals);
            const auto &eastProcessed     = std::get<2> (processedSignals);
            // Check resulting signal lengths
            auto strategy = processingRequest.getInferenceStrategy();
            if (strategy == ProcessingRequest::InferenceStrategy::SlidingWindow)
            {
                if (static_cast<int> (verticalProcessed.size()) <
                    mInference.getMinimumSignalLength())
                {
                    mLogger->error("Signal too small to perform inference");
                    response.setReturnCode(
                       ProcessingResponse::ReturnCode::ProcessedSignalTooSmall);
                    return response.clone();
                }
            }
            else
            {
                if (!mInference.isValidSignalLength(verticalProcessed.size()))
                {
                    mLogger->error("Invalid signal length");
                    response.setReturnCode(
                    ProcessingResponse::ReturnCode::InvalidProcessedSignalLength
                    );
                    return response.clone();
                }
            }
            try
            {
                if (strategy ==
                    ProcessingRequest::InferenceStrategy::SlidingWindow) 
                {
                    auto probabilitySignal
                        = mInference.predictProbabilitySlidingWindow(verticalProcessed,
                                                                     northProcessed,
                                                                     eastProcessed);
                    response.setProbabilitySignal(std::move(probabilitySignal));
                }
                else
                {
                    auto probabilitySignal
                        = mInference.predictProbability(verticalProcessed,
                                                        northProcessed,
                                                        eastProcessed);
                    response.setProbabilitySignal(std::move(probabilitySignal));
                }
                response.setReturnCode(ProcessingResponse::ReturnCode::Success);
            }
            catch (const std::exception &e) 
            {
                mLogger->error("Failed to evaluate model.  Failed with "
                             + std::string{e.what()});
                response.setReturnCode(ProcessingResponse::InferenceFailure);
            }
            return response.clone();
        }
        //-----------------------------------Inference------------------------//
        InferenceRequest inferenceRequest;
        if (messageType == inferenceRequest.getMessageType())
        {
            mLogger->debug("Inference request received");
            InferenceResponse response;
            try
            {
                inferenceRequest.fromMessage(
                    reinterpret_cast<const char *> (messageContents), length);
                response.setIdentifier(inferenceRequest.getIdentifier());
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to upnack preprocessing request: "
                             + std::string{e.what()});
                response.setReturnCode(InferenceResponse::InvalidMessage);
                return response.clone();
            }
            // Ensure the signals are set
            if (!inferenceRequest.haveSignals())
            {
                response.setReturnCode(InferenceResponse::InvalidMessage);
                return response.clone();
            }
            // Process the data
            try
            {
                const auto &vertical
                    = inferenceRequest.getVerticalSignalReference();
                const auto &north
                    = inferenceRequest.getNorthSignalReference();
                const auto &east
                    = inferenceRequest.getEastSignalReference();
                auto strategy = inferenceRequest.getInferenceStrategy();
                if (strategy ==
                    InferenceRequest::InferenceStrategy::SlidingWindow) 
                {
                    auto probabilitySignal
                        = mInference.predictProbabilitySlidingWindow(vertical,
                                                                     north,
                                                                     east);
                    response.setProbabilitySignal(std::move(probabilitySignal));
                }
                else
                {
                    auto probabilitySignal
                        = mInference.predictProbability(vertical, north, east);
                    response.setProbabilitySignal(std::move(probabilitySignal));
                }
                response.setReturnCode(InferenceResponse::Success);
            }
            catch (const std::exception &e) 
            {
                mLogger->error("Failed to evaluate model.  Failed with "
                             + std::string{e.what()});
                response.setReturnCode(InferenceResponse::AlgorithmFailure);
            }
            return response.clone();
        }
        //---------------------------Preprocessing----------------------------//
        PreprocessingRequest preprocessingRequest;
        if (messageType == preprocessingRequest.getMessageType())
        {
            mLogger->debug("Preprocessing request received");
            PreprocessingResponse response;
            try
            {
                preprocessingRequest.fromMessage(
                    reinterpret_cast<const char *> (messageContents), length);
                response.setIdentifier(preprocessingRequest.getIdentifier());
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to upnack preprocessing request: "
                             + std::string{e.what()});
                response.setReturnCode(PreprocessingResponse::InvalidMessage);
                return response.clone();
            }
            // Ensure the signals are set
            if (!preprocessingRequest.haveSignals())
            {
                response.setReturnCode(PreprocessingResponse::InvalidMessage);
                return response.clone();
            }
            // Process the data
            try
            {
                const auto &vertical
                    = preprocessingRequest.getVerticalSignalReference();
                const auto &north
                    = preprocessingRequest.getNorthSignalReference();
                const auto &east
                    = preprocessingRequest.getEastSignalReference();
                auto samplingRate = preprocessingRequest.getSamplingRate();
                // Process data
                auto [verticalResult, northResult, eastResult]
                    = mPreprocess.process(vertical, north, east, samplingRate);
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
                response.setReturnCode(PreprocessingResponse::AlgorithmFailure);
                return response.clone();
            }
            response.setReturnCode(PreprocessingResponse::Success);
            return response.clone();
        }
        // No idea -> Send something back so they don't wait forever
        mLogger->error("Unhandled message type: " + messageType);
        UMPS::MessageFormats::Failure response;
        response.setDetails("Unhandled message type");
        return response.clone();
    }
///private:
    mutable std::mutex mMutex;
    std::shared_ptr<UMPS::Logging::ILog> mLogger{nullptr};
    std::unique_ptr<URouterDealer::Reply> mReplier{nullptr};
    UModels::Preprocessing mPreprocess;
    UModels::Inference mInference;
    std::thread mResponseThread;
    const double mTargetSamplingRate{
        UModels::Preprocessing::getTargetSamplingRate()
    };
    const int mMinimumSignalLength{
        UModels::Inference::getMinimumSignalLength()
    };
    bool mKeepRunning{false};
    bool mInitialized{false};
};

/// Constructor
Service::Service() :
    pImpl(std::make_unique<ServiceImpl> (nullptr, nullptr))
{
}

/// Constructor with logger
Service::Service(std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ServiceImpl> (nullptr, logger))
{
}

/// Consructor with context and logger
Service::Service(std::shared_ptr<UMPS::Messaging::Context> &context,
                 std::shared_ptr<UMPS::Logging::ILog> &logger) :
    pImpl(std::make_unique<ServiceImpl> (context, logger))
{
}

/// Destructor
Service::~Service() = default;

/// Initialized?
bool Service::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}
