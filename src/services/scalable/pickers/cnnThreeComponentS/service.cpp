#include <mutex>
#include <thread>
#ifndef NDEBUG
#include <cassert>
#endif
#include <uussmlmodels/pickers/cnnThreeComponentS/inference.hpp>
#include <uussmlmodels/pickers/cnnThreeComponentS/preprocessing.hpp>
#include <umps/authentication/zapOptions.hpp>
#include <umps/logging/standardOut.hpp>
#include <umps/messaging/context.hpp>
#include <umps/messaging/routerDealer/reply.hpp>
#include <umps/messaging/routerDealer/replyOptions.hpp>
#include <umps/messageFormats/failure.hpp>
#include "urts/services/scalable/pickers/cnnThreeComponentS/service.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS/serviceOptions.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS/inferenceRequest.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS/inferenceResponse.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS/preprocessingRequest.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS/preprocessingResponse.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS/processingRequest.hpp"
#include "urts/services/scalable/pickers/cnnThreeComponentS/processingResponse.hpp"

namespace URouterDealer = UMPS::Messaging::RouterDealer;
using namespace URTS::Services::Scalable::Pickers::CNNThreeComponentS;
namespace UModels = UUSSMLModels::Pickers::CNNThreeComponentS;

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
                processedSignals = mPreprocess.process(vertical, north, east,
                                                       samplingRate);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to preprocess data.  Failed with "
                             + std::string{e.what()});
                response.setReturnCode(ProcessingResponse::PreprocessingFailure);
                return response.clone();
            }
            // Extract signals
            auto verticalProcessed = std::move(std::get<0> (processedSignals));
            auto northProcessed    = std::move(std::get<1> (processedSignals));
            auto eastProcessed     = std::move(std::get<2> (processedSignals));
            // Ensure signals have same size
#ifndef NDEBUG
            assert(verticalProcessed.size() == northProcessed.size());
            assert(verticalProcessed.size() == eastProcessed.size());
#endif
            // Deal with potentially long signal
            auto nSamples = static_cast<int> (verticalProcessed.size());
            if (nSamples != mExpectedSignalLength)
            {
    
                if (nSamples < mExpectedSignalLength)
                {
                    mLogger->error("Signals too small after processing");
                    response.setReturnCode(
                    ProcessingResponse::ReturnCode::InvalidProcessedSignalLength
                    );
                    return response.clone();
                }
                else
                {
                    // This signal is a bit too long.  Trim it so the pick which
                    // should be at the center is in the center.  Note, we have
                    // already ensured the signals have the same size.
                    auto halfWindow
                        = static_cast<int> (mExpectedSignalLength/2);
                    auto halfIndex = static_cast<int> (nSamples/2);
                    int i1 = std::max(0, halfIndex - halfWindow);
                    int i2 = i1 + mExpectedSignalLength;
#ifndef DEBUG
                    assert(i2 - i1 == mExpectedSignalLength);
#endif
                    std::vector<double> tempSignalVertical(mExpectedSignalLength);
                    std::copy(verticalProcessed.data() + i1, 
                              verticalProcessed.data() + i2, 
                              tempSignalVertical.data()); 
                    verticalProcessed = std::move(tempSignalVertical);

                    std::vector<double> tempSignalNorth(mExpectedSignalLength);
                    std::copy(northProcessed.data() + i1, 
                              northProcessed.data() + i2, 
                              tempSignalNorth.data()); 
                    northProcessed = std::move(tempSignalNorth);

                    std::vector<double> tempSignalEast(mExpectedSignalLength);
                    std::copy(eastProcessed.data() + i1,   
                              eastProcessed.data() + i2,   
                              tempSignalEast.data());
                    eastProcessed = std::move(tempSignalEast);
#ifndef NDEBUG
                    assert(static_cast<int> (verticalProcessed.size()) ==
                           mExpectedSignalLength);
                    assert(static_cast<int> (northProcessed.size()) ==
                           mExpectedSignalLength);
                    assert(static_cast<int> (eastProcessed.size()) ==
                           mExpectedSignalLength);
#endif
                }
            }
            try
            {
                 auto correction = mInference->predict(verticalProcessed,
                                                       northProcessed,
                                                       eastProcessed);
                 response.setCorrection(correction);
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
                auto correction = mInference->predict(vertical, north, east);
                response.setCorrection(correction);
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
                response.setReturnCode(PreprocessingResponse::Success);
            }
            catch (const std::exception &e)
            {
                mLogger->error("Failed to preprocess data: "
                             + std::string{e.what()});
                response.setReturnCode(PreprocessingResponse::AlgorithmFailure);
            }
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

