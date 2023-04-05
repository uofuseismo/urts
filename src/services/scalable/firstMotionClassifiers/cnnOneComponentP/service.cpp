#include <iostream>
#include <mutex>
#include <thread>
#ifndef NDEBUG
#include <cassert>
#endif
#include <uussmlmodels/firstMotionClassifiers/cnnOneComponentP/inference.hpp>
#include <uussmlmodels/firstMotionClassifiers/cnnOneComponentP/preprocessing.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/messaging/routerDealer/reply.hpp>
#include <umps/messaging/routerDealer/replyOptions.hpp>
#include <umps/messageFormats/failure.hpp>
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/service.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/serviceOptions.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/inferenceRequest.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/inferenceResponse.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/preprocessingRequest.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/preprocessingResponse.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/processingRequest.hpp"
#include "urts/services/scalable/firstMotionClassifiers/cnnOneComponentP/processingResponse.hpp"

namespace URouterDealer = UMPS::Messaging::RouterDealer;
using namespace URTS::Services::Scalable::FirstMotionClassifiers::CNNOneComponentP;
namespace UModels = UUSSMLModels::FirstMotionClassifiers::CNNOneComponentP;

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
            response.setFirstMotion(ProcessingResponse::FirstMotion::Unknown);
            try
            {
                processingRequest.fromMessage(
                    reinterpret_cast<const char *> (messageContents), length);
                response.setIdentifier(processingRequest.getIdentifier());
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to unpack preprocessing request: "
                             + std::string{e.what()});
                response.setReturnCode(ProcessingResponse::InvalidMessage);
                return response.clone();
            }
            // Process the data
            std::vector<double> verticalProcessed;
            try
            {
                const auto &vertical
                    = processingRequest.getVerticalSignalReference();
                auto samplingRate = processingRequest.getSamplingRate();
                // Process data
                verticalProcessed = mPreprocess.process(vertical, samplingRate);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to preprocess data.  Failed with "
                             + std::string{e.what()});
                response.setReturnCode(ProcessingResponse::PreprocessingFailure);
                return response.clone();
            }
            // Check resulting signal lengths
            auto nSamples = static_cast<int> (verticalProcessed.size());
            if (nSamples != mExpectedSignalLength)
            {
                
                if (nSamples < mExpectedSignalLength)
                {
                    mLogger->error("Signal too small after processing");
                    response.setReturnCode(
                    ProcessingResponse::ReturnCode::InvalidProcessedSignalLength
                    );
                    return response.clone();
                }
                else
                {
                    // This signal is a bit too long.  Trim it so the pick which
                    // should be at the center is in the center.
                    auto halfWindow
                        = static_cast<int> (mExpectedSignalLength/2);
                    auto halfIndex = static_cast<int> (nSamples/2);
                    int i1 = std::max(0, halfIndex - halfWindow);
                    int i2 = i1 + mExpectedSignalLength;
#ifndef DEBUG
                    assert(i2 - i1 == mExpectedSignalLength);
#endif
                    std::vector<double> tempSignal(mExpectedSignalLength);
                    std::copy(verticalProcessed.data() + i1,
                              verticalProcessed.data() + i2,
                              tempSignal.data()); 
                    verticalProcessed = std::move(tempSignal);
#ifndef NDEBUG
                    assert(static_cast<int> (verticalProcessed.size()) ==
                           mExpectedSignalLength);
#endif
                }
            }
            try
            {
                auto threshold = processingRequest.getThreshold();
                auto probabilities
                    = mInference->predictProbability(verticalProcessed);
                auto firstMotion
                    = UModels::convertProbabilityToClass(
                          std::get<0> (probabilities),
                          std::get<1> (probabilities),
                          std::get<2> (probabilities),
                          threshold);
                response.setProbabilities(probabilities);
                response.setFirstMotion(
                    static_cast<ProcessingResponse::FirstMotion> (firstMotion));
                response.setReturnCode(ProcessingResponse::Success);
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
            response.setFirstMotion(
                InferenceResponse::FirstMotion::Unknown);
            try
            {
                inferenceRequest.fromMessage(
                    reinterpret_cast<const char *> (messageContents), length);
                response.setIdentifier(inferenceRequest.getIdentifier());
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to unpack preprocessing request: "
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
                    = inferenceRequest.getVerticalSignalReference();
                auto threshold = inferenceRequest.getThreshold();
                auto probabilities = mInference->predictProbability(vertical);
                auto firstMotion
                    = UModels::convertProbabilityToClass(
                          std::get<0> (probabilities),
                          std::get<1> (probabilities),
                          std::get<2> (probabilities),
                          threshold);
                response.setProbabilities(probabilities);
                response.setFirstMotion(
                    static_cast<InferenceResponse::FirstMotion> (firstMotion)
                );
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
                mLogger->error("Failed to unpack preprocessing request: "
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
                    = preprocessingRequest.getVerticalSignalReference();
                auto samplingRate = preprocessingRequest.getSamplingRate();
                // Process data
                auto verticalResult
                    = mPreprocess.process(vertical, samplingRate);
                // Set data on response
                response.setSamplingRate(mTargetSamplingRate);
                response.setVerticalSignal(std::move(verticalResult));
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
    const int mExpectedSignalLength{
        UModels::Inference::getExpectedSignalLength()
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

