#include <mutex>
#include <thread>
#include <uussmlmodels/detectors/uNetOneComponentP/inference.hpp>
#include <uussmlmodels/detectors/uNetOneComponentP/preprocessing.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/messaging/routerDealer/reply.hpp>
#include <umps/messaging/routerDealer/replyOptions.hpp>
#include <umps/messageFormats/failure.hpp>
#include "urts/services/scalable/detectors/uNetOneComponentP/service.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/serviceOptions.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/inferenceRequest.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/inferenceResponse.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/preprocessingRequest.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/preprocessingResponse.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/processingRequest.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/processingResponse.hpp"

namespace URouterDealer = UMPS::Messaging::RouterDealer;
using namespace URTS::Services::Scalable::Detectors::UNetOneComponentP;
namespace UModels = UUSSMLModels::Detectors::UNetOneComponentP;

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
        mLogger->debug("Inference service stopping threads...");
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
        //-----------Everything: Process data then perform inference----------//
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
            std::vector<double> processedSignal;
            try
            {
                const auto &vertical
                    = processingRequest.getSignalReference();
                auto samplingRate = processingRequest.getSamplingRate();
                // Process data
                processedSignal
                    = mPreprocess.process(vertical, samplingRate);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to preprocess data.  Failed with "
                             + std::string{e.what()});
                response.setReturnCode(ProcessingResponse::PreprocessingFailure);
                return response.clone();
            }
            // Check resulting signal lengths
            auto strategy = processingRequest.getInferenceStrategy();
            if (strategy == ProcessingRequest::InferenceStrategy::SlidingWindow)
            {
                if (static_cast<int> (processedSignal.size()) <
                    mInference->getMinimumSignalLength())
                {
                    mLogger->error("Signal too small to perform inference");
                    response.setReturnCode(
                       ProcessingResponse::ReturnCode::ProcessedSignalTooSmall);
                    return response.clone();
                }
            }
            else
            {
                if (!mInference->isValidSignalLength(processedSignal.size()))
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
                        = mInference->predictProbabilitySlidingWindow(
                             processedSignal);
                    response.setProbabilitySignal(std::move(probabilitySignal));
                }
                else
                {
                    auto probabilitySignal
                        = mInference->predictProbability(processedSignal);
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
            if (!inferenceRequest.haveSignal())
            {
                response.setReturnCode(InferenceResponse::InvalidMessage);
                return response.clone();
            }
            // Process the data
            try
            {
                const auto &vertical
                    = inferenceRequest.getSignalReference();
                auto strategy = inferenceRequest.getInferenceStrategy();
                if (strategy ==
                    InferenceRequest::InferenceStrategy::SlidingWindow) 
                {
                    auto probabilitySignal
                        = mInference->predictProbabilitySlidingWindow(vertical);
                    response.setProbabilitySignal(std::move(probabilitySignal));
                }
                else
                {
                    auto probabilitySignal
                        = mInference->predictProbability(vertical);
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
            if (!preprocessingRequest.haveSignal())
            {
                response.setReturnCode(PreprocessingResponse::InvalidMessage);
                return response.clone();
            }
            // Process the data
            try
            {
                const auto &vertical
                    = preprocessingRequest.getSignalReference();
                auto samplingRate = preprocessingRequest.getSamplingRate();
                // Process data
                auto verticalResult
                    = mPreprocess.process(vertical, samplingRate);
                // Set data on response
                response.setSamplingRate(mTargetSamplingRate);
                response.setSignal(std::move(verticalResult));
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
    std::unique_ptr<UModels::Inference> mInference{nullptr};
    UModels::Preprocessing mPreprocess;
    ServiceOptions mOptions;
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

/// Running?
bool Service::isRunning() const noexcept
{
    return pImpl->keepRunning();
}

/// Stop service
void Service::stop()
{
    pImpl->mLogger->debug("Stopping service...");
    pImpl->stop();
}

/// Start service
void Service::start()
{
    if (!isInitialized())
    {   
        throw std::runtime_error("Inference service not initialized");
    }   
    pImpl->mLogger->debug("Starting service...");
    pImpl->start();
}

/// Initialize the class
void Service::initialize(const ServiceOptions &options)
{
    if (!options.haveModelWeightsFile())
    {
        throw std::invalid_argument("Model weights file not set");
    }
    if (!options.haveAddress())
    {
        throw std::invalid_argument("Replier address not set");
    }
    // Ensure the service is stopped
    stop(); // Ensure the service is stopped
    // Initialize the inference engine
    auto inferenceDevice{UModels::Inference::Device::CPU};
    if (options.getDevice() == ServiceOptions::Device::GPU)
    {
        pImpl->mLogger->debug("Attempting to use GPU...");
        inferenceDevice = UModels::Inference::Device::GPU;
    }
    pImpl->mInference = std::make_unique<UModels::Inference> (inferenceDevice);
    pImpl->mInference->load(options.getModelWeightsFile(),
                            UModels::Inference::ModelFormat::ONNX); 
    // Create the replier
    pImpl->mLogger->debug("Creating replier...");
    UMPS::Messaging::RouterDealer::ReplyOptions replierOptions;
    replierOptions.setAddress(options.getAddress());
    replierOptions.setZAPOptions(options.getZAPOptions());
    replierOptions.setPollingTimeOut(options.getPollingTimeOut());
    replierOptions.setSendHighWaterMark(options.getSendHighWaterMark());
    replierOptions.setReceiveHighWaterMark(
        options.getReceiveHighWaterMark());
    replierOptions.setCallback(std::bind(&ServiceImpl::callback,
                                         &*this->pImpl,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3));
    pImpl->mReplier->initialize(replierOptions); 
    std::this_thread::sleep_for(std::chrono::milliseconds {10});
    // Initialized?
    pImpl->mInitialized = pImpl->mInference->isInitialized() &&
                          pImpl->mReplier->isInitialized();
    if (pImpl->mInitialized)
    {
        pImpl->mLogger->debug("Service initialized!");
        pImpl->mOptions = options;
    }
    else
    {
        pImpl->mLogger->error("Failed to initialize service.");
    }
}

