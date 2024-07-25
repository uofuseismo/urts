#include <vector>
#include <thread>
#include <fstream>
#include <filesystem>
#include <umps/authentication/zapOptions.hpp>
#include <umps/messaging/routerDealer/proxy.hpp>
#include <umps/messaging/routerDealer/proxyOptions.hpp>
#include <uussmlmodels/detectors/uNetOneComponentP/inference.hpp>
#include <uussmlmodels/detectors/uNetOneComponentP/preprocessing.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include "urts/services/scalable/detectors/uNetOneComponentP/inferenceRequest.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/inferenceResponse.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/preprocessingRequest.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/preprocessingResponse.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/processingRequest.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/processingResponse.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/requestorOptions.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/requestor.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/serviceOptions.hpp"
#include "urts/services/scalable/detectors/uNetOneComponentP/service.hpp"

namespace
{

using namespace URTS::Services::Scalable::Detectors::UNetOneComponentP;

TEST_CASE("URTS::Services::Scalbale::Detectors::UNetOneComponentP", "[PreprocessingRequest]")
{
    const std::vector<double> vertical{1, 2, 3, 4, 5};
    const double samplingRate{50};
    const int64_t identifier{5021};
    PreprocessingRequest request;

    REQUIRE_NOTHROW(request.setSamplingRate(samplingRate));
    REQUIRE_NOTHROW(request.setSignal(vertical));
    request.setIdentifier(identifier);
 
    PreprocessingRequest copy;
    REQUIRE_NOTHROW(copy.fromMessage(request.toMessage()));
    REQUIRE(copy.getIdentifier() == identifier);
    REQUIRE(std::abs(copy.getSamplingRate() - samplingRate) < 1.e-14);
    REQUIRE(copy.haveSignal());
    const auto vr = copy.getSignalReference();
    REQUIRE(vertical.size() == vr.size()); 
    for (int i = 0; i < static_cast<int> (vr.size()); ++i)
    {
        REQUIRE(std::abs(vertical.at(i) - vr.at(i)) < 1.e-14);
    }

    auto v = copy.getSignal();
    REQUIRE(vertical.size() == v.size()); 
    for (int i = 0; i < static_cast<int> (v.size()); ++i)
    {
        REQUIRE(std::abs(vertical.at(i) - v.at(i)) < 1.e-14);
    } 

    SECTION("clear")
    {
        request.clear();
        REQUIRE(std::abs(request.getSamplingRate() - 100) < 1.e-14);
        REQUIRE(request.getIdentifier() == 0);
        REQUIRE(request.getMessageType() ==
              "URTS::Services::Scalable::Detectors::UNetOneComponentP::PreprocessingRequest");
    }
}

TEST_CASE("URTS::Services::Scalbale::Detectors::UNetOneComponentP", "[PreprocessingResponse]")
{
    const std::vector<double> vertical{1, 2, 3, 4, 5};
    const double samplingRate{50};
    const int64_t identifier{5021};
    const PreprocessingResponse::ReturnCode
        returnCode{PreprocessingResponse::ReturnCode::Success};
    PreprocessingResponse response;

    REQUIRE_NOTHROW(response.setSamplingRate(samplingRate));
    REQUIRE_NOTHROW(response.setSignal(vertical));
    response.setIdentifier(identifier);
    response.setReturnCode(returnCode);

    PreprocessingResponse copy;
    REQUIRE_NOTHROW(copy.fromMessage(response.toMessage()));
    REQUIRE(copy.getIdentifier() == identifier);
    REQUIRE(copy.getReturnCode() == returnCode);
    REQUIRE(std::abs(copy.getSamplingRate() - samplingRate) < 1.e-14);
    REQUIRE(copy.haveSignal());
    auto v = copy.getSignal();
    REQUIRE(vertical.size() == v.size());
    for (int i = 0; i < static_cast<int> (v.size()); ++i)
    {
        REQUIRE(std::abs(vertical.at(i) - v.at(i)) < 1.e-14);
    }

    SECTION("clear")
    {
        response.clear();
        REQUIRE(std::abs(response.getSamplingRate() - 100) < 1.e-14);
        REQUIRE(response.getIdentifier() == 0);
        REQUIRE(response.getMessageType() ==
              "URTS::Services::Scalable::Detectors::UNetOneComponentP::PreprocessingResponse");
        REQUIRE_FALSE(response.haveReturnCode());
    }
}

TEST_CASE("URTS::Services::Scalbale::Detectors::UNetOneComponentP", "[InferenceRequest]")
{
    std::vector<double> vertical(1008 + 128);
    std::fill(vertical.begin(), vertical.end(), 1);
    const int64_t identifier{1026};
    auto fixedWindow = InferenceRequest::InferenceStrategy::FixedWindow;
    auto slidingWindow = InferenceRequest::InferenceStrategy::SlidingWindow;
    InferenceRequest request;

    REQUIRE(InferenceRequest::getMinimumSignalLength() ==
            UUSSMLModels::Detectors::UNetOneComponentP::Inference::getMinimumSignalLength());
    REQUIRE(std::abs(InferenceRequest::getSamplingRate()
                   - UUSSMLModels::Detectors::UNetOneComponentP::Inference::getSamplingRate()) < 1.e-14);
    for (int i = 0; i < 2020; ++i)
    {
        REQUIRE(InferenceRequest::isValidSignalLength(i)
              == UUSSMLModels::Detectors::UNetOneComponentP::Inference::isValidSignalLength(i));
    }
    REQUIRE(request.getMinimumSignalLength() == 1008);
    REQUIRE_NOTHROW(request.setSignal(vertical, fixedWindow));
    REQUIRE_NOTHROW(request.setSignal(vertical, slidingWindow));
    request.setIdentifier(identifier);

    SECTION("copy")
    {
        InferenceRequest copy;
        REQUIRE_NOTHROW(copy.fromMessage(request.toMessage()));
        REQUIRE(copy.getIdentifier() == identifier);
        REQUIRE(std::abs(copy.getSamplingRate() - 100) < 1.e-14);
        REQUIRE(copy.getInferenceStrategy() == slidingWindow);
        REQUIRE(copy.haveSignal());
        const auto vr = copy.getSignalReference();
        REQUIRE(vertical.size() == vr.size());
        for (int i = 0; i < static_cast<int> (vr.size()); ++i)
        {
            REQUIRE(std::abs(vertical.at(i) - vr.at(i)) < 1.e-14);
        }
    }
    auto v = request.getSignal();
    REQUIRE(vertical.size() == v.size());
    for (int i = 0; i < static_cast<int> (v.size()); ++i)
    {
        REQUIRE(std::abs(vertical.at(i) -  v.at(i)) < 1.e-14);
    }

    SECTION("clear")
    {
        request.clear();
        REQUIRE(std::abs(request.getSamplingRate() - 100) < 1.e-14);
        REQUIRE_FALSE(request.haveSignal());
        REQUIRE(request.getIdentifier() == 0);
        REQUIRE(request.getMessageType() ==
              "URTS::Services::Scalable::Detectors::UNetOneComponentP::InferenceRequest");
    }
}
                                                                                              
TEST_CASE("URTS::Services::Scalbale::Detectors::UNetOneComponentP", "[InferenceResponse]")
{
    const std::vector<double> probabilitySignal{1, 2, 3, 4, 5}; 
    const double samplingRate{250};
    const int64_t identifier{5024};
    const InferenceResponse::ReturnCode
        returnCode{InferenceResponse::ReturnCode::Success};
    InferenceResponse response;

    REQUIRE_NOTHROW(response.setSamplingRate(samplingRate));
    REQUIRE_NOTHROW(response.setProbabilitySignal(probabilitySignal));
    response.setIdentifier(identifier);
    response.setReturnCode(returnCode);

    SECTION("copy")
    {
        InferenceResponse copy;
        REQUIRE_NOTHROW(copy.fromMessage(response.toMessage()));
        REQUIRE(copy.getIdentifier() == identifier);
        REQUIRE(copy.getReturnCode() == returnCode);
        REQUIRE(std::abs(copy.getSamplingRate() - samplingRate) < 1.e-14);
        REQUIRE(copy.haveProbabilitySignal());
        auto p = copy.getProbabilitySignal();
        REQUIRE(probabilitySignal.size() == p.size());
        for (int i = 0; i < static_cast<int> (p.size()); ++i)
        {   
            REQUIRE(std::abs(probabilitySignal.at(i) - p.at(i)) < 1.e-7);
        }
    }

    SECTION("clear")
    {
        response.clear();
        REQUIRE(std::abs(response.getSamplingRate() - 100) < 1.e-14);
        REQUIRE(response.getIdentifier() == 0); 
        REQUIRE(response.getMessageType() ==
              "URTS::Services::Scalable::Detectors::UNetOneComponentP::InferenceResponse");
        REQUIRE_FALSE(response.haveReturnCode());
    }
}

TEST_CASE("URTS::Services::Scalbale::Detectors::UNetOneComponentP", "[ProcessingRequest]")
{
    std::vector<double> vertical(2*1008 + 128);
    std::fill(vertical.begin(), vertical.end(), 1); 
    const int64_t identifier{1026};
    const double samplingRate{50};
    auto fixedWindow = ProcessingRequest::InferenceStrategy::FixedWindow;
    auto slidingWindow = ProcessingRequest::InferenceStrategy::SlidingWindow;
    ProcessingRequest request;

    REQUIRE(request.getMinimumSignalLength() == 1008);
    REQUIRE_NOTHROW(request.setSamplingRate(samplingRate));
    REQUIRE_NOTHROW(request.setSignal(vertical, fixedWindow));
    REQUIRE_NOTHROW(request.setSignal(vertical, slidingWindow));
    request.setIdentifier(identifier);

    ProcessingRequest copy;
    REQUIRE_NOTHROW(copy.fromMessage(request.toMessage()));
    REQUIRE(copy.getIdentifier() == identifier);
    REQUIRE(std::abs(copy.getSamplingRate() - samplingRate) < 1.e-14);
    REQUIRE(copy.getInferenceStrategy() == slidingWindow);
    REQUIRE(copy.haveSignal());
    const auto vr = copy.getSignalReference();
    REQUIRE(vertical.size() == vr.size());
    for (int i = 0; i < static_cast<int> (vr.size()); ++i)
    {
        REQUIRE(std::abs(vertical.at(i) - vr.at(i)) < 1.e-14);
    }

    auto v = copy.getSignal();
    REQUIRE(vertical.size() == v.size());
    for (int i = 0; i < static_cast<int> (v.size()); ++i)
    {
        REQUIRE(std::abs(vertical.at(i) - v.at(i)) < 1.e-14);
    }

    SECTION("clear")
    {
        request.clear();
        REQUIRE(std::abs(request.getSamplingRate() - 100) < 1.e-14);
        REQUIRE_FALSE(request.haveSignal());
        REQUIRE(request.getIdentifier() == 0);
        REQUIRE(request.getMessageType() ==
              "URTS::Services::Scalable::Detectors::UNetOneComponentP::ProcessingRequest");
    }
}

TEST_CASE("URTS::Services::Scalbale::Detectors::UNetOneComponentP", "[ProcessingResponse]")
{
    const std::vector<double> probabilitySignal{1, 2, 3, 4, 5}; 
    const double samplingRate{125};
    const int64_t identifier{5025};
    const ProcessingResponse::ReturnCode
        returnCode{ProcessingResponse::ReturnCode::Success};
    ProcessingResponse response;

    REQUIRE_NOTHROW(response.setSamplingRate(samplingRate));
    REQUIRE_NOTHROW(response.setProbabilitySignal(probabilitySignal));
    response.setIdentifier(identifier);
    response.setReturnCode(returnCode);

    ProcessingResponse copy;
    REQUIRE_NOTHROW(copy.fromMessage(response.toMessage()));
    REQUIRE(copy.getIdentifier() == identifier);
    REQUIRE(copy.getReturnCode() == returnCode);
    REQUIRE(std::abs(copy.getSamplingRate() - samplingRate) < 1.e-14);
    REQUIRE(copy.haveProbabilitySignal());
    auto p = copy.getProbabilitySignal();
    REQUIRE(probabilitySignal.size() == p.size());
    for (int i = 0; i < static_cast<int> (p.size()); ++i)
    {
        REQUIRE(std::abs(probabilitySignal.at(i) - p.at(i)) < 1.e-7);
    }

    response.clear();
    SECTION("clear")
    {
        REQUIRE(std::abs(response.getSamplingRate() - 100) < 1.e-14);
        REQUIRE(response.getIdentifier() == 0); 
        REQUIRE(response.getMessageType() ==
              "URTS::Services::Scalable::Detectors::UNetOneComponentP::ProcessingResponse");
        REQUIRE_FALSE(response.haveReturnCode());
    }
}

TEST_CASE("URTS::Services::Scalbale::Detectors::UNetOneComponentP", "[ServiceOptions]")
{
    const std::string address{"tcp://127.0.0.1:5550"};
    const int sendHWM{105};
    const int recvHWM{106};
    const std::chrono::milliseconds pollTimeOut{145};
    ServiceOptions::Device device{ServiceOptions::Device::GPU};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    // Create a dumby weights file
    const std::string weightsFile{"dumbyWeights.onnx"};
    if (std::filesystem::exists(weightsFile))
    {
        std::filesystem::remove(weightsFile);
    }
    std::ofstream weightsFileHandle;
    weightsFileHandle.open(weightsFile);
    weightsFileHandle.close();

    ServiceOptions options;
    REQUIRE_NOTHROW(options.setModelWeightsFile(weightsFile));
    options.setDevice(device);
    REQUIRE_NOTHROW(options.setAddress(address));
    REQUIRE_NOTHROW(options.setSendHighWaterMark(sendHWM));
    REQUIRE_NOTHROW(options.setReceiveHighWaterMark(recvHWM));
    REQUIRE_NOTHROW(options.setPollingTimeOut(pollTimeOut));
    options.setZAPOptions(zapOptions);
   
    ServiceOptions copy(options);
    REQUIRE(copy.getModelWeightsFile() == weightsFile);
    REQUIRE(copy.getDevice() == device);
    REQUIRE(copy.getSendHighWaterMark() == sendHWM);
    REQUIRE(copy.getReceiveHighWaterMark() == recvHWM);
    REQUIRE(copy.getPollingTimeOut() == pollTimeOut);
    REQUIRE(copy.getZAPOptions().getSecurityLevel() ==
              zapOptions.getSecurityLevel());

    // Clean up  
    if (std::filesystem::exists(weightsFile))
    {
        std::filesystem::remove(weightsFile);
    }

    SECTION("clear")
    {
        options.clear();
        REQUIRE_FALSE(options.haveModelWeightsFile());
        REQUIRE_FALSE(options.haveAddress());
        REQUIRE(options.getDevice() == ServiceOptions::Device::CPU); 
        REQUIRE(options.getSendHighWaterMark() == 8192);
        REQUIRE(options.getReceiveHighWaterMark() == 4096);
        REQUIRE(options.getPollingTimeOut() == std::chrono::milliseconds {10});
        REQUIRE(options.getZAPOptions().getSecurityLevel() ==
                UMPS::Authentication::SecurityLevel::Grasslands);
    }
}

TEST_CASE("URTS::Services::Scalbale::Detectors::UNetOneComponentP", "[RequestorOptions]")
{
    const std::string address{"tcp://127.0.0.1:5550"};
    const int sendHWM{105};
    const int recvHWM{106};
    const std::chrono::milliseconds sendTimeOut{120};
    const std::chrono::milliseconds recvTimeOut{145};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    RequestorOptions options;
    REQUIRE_NOTHROW(options.setAddress(address));
    REQUIRE_NOTHROW(options.setSendHighWaterMark(sendHWM));
    REQUIRE_NOTHROW(options.setReceiveHighWaterMark(recvHWM));
    options.setSendTimeOut(sendTimeOut);
    options.setReceiveTimeOut(recvTimeOut);
    options.setZAPOptions(zapOptions);

    RequestorOptions copy(options);
    REQUIRE(copy.getAddress() == address);
    REQUIRE(copy.getSendHighWaterMark() == sendHWM);
    REQUIRE(copy.getReceiveHighWaterMark() == recvHWM);
    REQUIRE(copy.getSendTimeOut() == sendTimeOut); 
    REQUIRE(copy.getReceiveTimeOut() == recvTimeOut);
    REQUIRE(copy.getZAPOptions().getSecurityLevel() ==
              zapOptions.getSecurityLevel());

    SECTION("clear")
    {
        options.clear();
        REQUIRE(options.getSendHighWaterMark() == 4096);
        REQUIRE(options.getReceiveHighWaterMark() == 8192);
        REQUIRE(options.getSendTimeOut() == std::chrono::milliseconds {0});
        REQUIRE(options.getReceiveTimeOut() == std::chrono::milliseconds {3000});
        zapOptions.setGrasslandsClient();
        REQUIRE(options.getZAPOptions().getSecurityLevel() ==
                zapOptions.getSecurityLevel());
    }
}

///--------------------------------------------------------------------------///
///                               Communication Test                         ///
///--------------------------------------------------------------------------///
const std::string frontendAddress{"tcp://127.0.0.1:5555"};
const std::string backendAddress{"tcp://127.0.0.1:5556"};
const std::string weightsFile{
    "/usr/local/share/UUSSMLModels/detectorsUNetOneComponentP.onnx"
}; 
std::vector<double> zProcRef, pRef, pRefWindow;
std::vector<double> vertical(1008, 0); 

void proxy()
{
    UMPS::Messaging::RouterDealer::ProxyOptions options;
    options.setFrontendAddress(frontendAddress);
    options.setBackendAddress(backendAddress);
    UMPS::Messaging::RouterDealer::Proxy proxy;
    REQUIRE_NOTHROW(proxy.initialize(options));
    std::thread proxyThread(&UMPS::Messaging::RouterDealer::Proxy::start,
                            &proxy);
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    REQUIRE(proxy.isRunning());
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    REQUIRE_NOTHROW(proxy.stop());
    proxyThread.join();
}

void server()
{
    ServiceOptions options;
    options.setAddress(backendAddress);
    options.setModelWeightsFile(weightsFile);
    Service service;
    REQUIRE_NOTHROW(service.initialize(options));
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    REQUIRE(service.isInitialized());
    REQUIRE_NOTHROW(service.start());
    std::this_thread::sleep_for(std::chrono::milliseconds{1500});
    service.stop();
}

void client()
{
    RequestorOptions options;
    options.setAddress(frontendAddress);
    Requestor client;
    REQUIRE_NOTHROW(client.initialize(options));
    REQUIRE(client.isInitialized());

    const double samplingRate{100};

    // Do a processing request
    PreprocessingRequest preprocessingRequest;
    preprocessingRequest.setIdentifier(2);
    REQUIRE_NOTHROW(preprocessingRequest.setSamplingRate(samplingRate));
    REQUIRE_NOTHROW(preprocessingRequest.setSignal(vertical));
    std::unique_ptr<PreprocessingResponse> preprocessingResponse{nullptr};
    REQUIRE_NOTHROW(preprocessingResponse
        = client.request(preprocessingRequest));
    std::vector<double> zProc, nProc, eProc;
    if (preprocessingResponse != nullptr)
    {
        REQUIRE(preprocessingResponse->getIdentifier() == 2);
        zProc = preprocessingResponse->getSignal();
        REQUIRE(zProc.size() == zProcRef.size());
        double error = 0;
        for (int i = 0; i < static_cast<int> (zProc.size()); ++i)
        {
            error = std::max(error, std::abs(zProc.at(i) - zProcRef.at(i)));
        } 
        REQUIRE(std::abs(error - 0) < 1.e-7);
    }
    // Follow on with an inference test
    InferenceRequest inferenceRequest;
    inferenceRequest.setIdentifier(3);
    inferenceRequest.setSignal(zProcRef,
        InferenceRequest::InferenceStrategy::SlidingWindow);
    std::unique_ptr<InferenceResponse> inferenceResponse{nullptr};
    inferenceResponse = client.request(inferenceRequest);
    if (inferenceResponse != nullptr)
    {
        REQUIRE(inferenceResponse->getIdentifier() == 3);
        auto probabilitySignal = inferenceResponse->getProbabilitySignal();
        REQUIRE(probabilitySignal.size() == pRefWindow.size());
        double error = 0;
        for (int i = 0; i < static_cast<int> (probabilitySignal.size()); ++i)
        {
            error = std::max(error, std::abs(probabilitySignal[i]
                                           - pRefWindow[i]));
        }
        REQUIRE(std::abs(error -  0) < 1.e-7);
    }
    // Repeat with fixed window
    inferenceRequest.setSignal(zProcRef,
        InferenceRequest::InferenceStrategy::FixedWindow);
    inferenceResponse = client.request(inferenceRequest);
    if (inferenceResponse != nullptr)
    {
        REQUIRE(inferenceResponse->getIdentifier() == 3); 
        auto probabilitySignal = inferenceResponse->getProbabilitySignal();
        REQUIRE(probabilitySignal.size() == pRef.size());
        double error = 0;
        for (int i = 0; i < static_cast<int> (probabilitySignal.size()); ++i)
        {
            error = std::max(error, std::abs(probabilitySignal[i]
                                           - pRef[i]));
        }
        REQUIRE(std::abs(error -  0) < 1.e-7);
    }


    // Do a sliding window (usual) then fixed window test
    ProcessingRequest processingRequest;
    processingRequest.setIdentifier(1);
    REQUIRE_NOTHROW(processingRequest.setSamplingRate(samplingRate));
    REQUIRE_NOTHROW(
        processingRequest.setSignal(vertical,
            ProcessingRequest::InferenceStrategy::SlidingWindow)
    );
    std::unique_ptr<ProcessingResponse> processingResponse{nullptr};
    processingResponse = client.request(processingRequest);
    if (processingResponse != nullptr)
    {
        REQUIRE(processingResponse->getIdentifier() == 1); 
        auto probabilitySignal = processingResponse->getProbabilitySignal();
        double error = 0;
        for (int i = 0; i < static_cast<int> (probabilitySignal.size()); ++i)
        {
            error = std::max(error, std::abs(probabilitySignal[i]
                                           - pRefWindow[i]));
        }
        REQUIRE(std::abs(error -  0) < 1.e-7);
    }
    REQUIRE_NOTHROW(
        processingRequest.setSignal(vertical,
            ProcessingRequest::InferenceStrategy::FixedWindow)
    );
    REQUIRE_NOTHROW(processingResponse = client.request(processingRequest));
    if (processingResponse != nullptr)
    {
        auto probabilitySignal = processingResponse->getProbabilitySignal();
        REQUIRE(processingResponse->getIdentifier() == 1);
        REQUIRE(pRef.size() == probabilitySignal.size());
        double error = 0; 
        for (int i = 0; i < static_cast<int> (probabilitySignal.size()); ++i)
        {
            error = std::max(error, std::abs(probabilitySignal[i]
                                           - pRef[i]));
        }
        REQUIRE(std::abs(error -  0) < 1.e-7);
    }
}

TEST_CASE("URTS::Services::Scalbale::Detectors::UNetOneComponentP", "[Service][.][integration]")
{
    // Do this manually
    {
        UUSSMLModels::Detectors::UNetOneComponentP::Preprocessing
             preprocessing;
        UUSSMLModels::Detectors::UNetOneComponentP::Inference inference;
        REQUIRE_NOTHROW(inference.load(weightsFile));

        zProcRef = preprocessing.process(vertical);
        pRefWindow = inference.predictProbabilitySlidingWindow(zProcRef);
        pRef = inference.predictProbability(zProcRef);
    }
    // Start the proxy
    auto proxyThread = std::thread(proxy);
    std::this_thread::sleep_for(std::chrono::milliseconds {50});
    // Connect / initialize the server
    auto serverThread = std::thread(server);
    // Let the clients rip
    std::this_thread::sleep_for(std::chrono::milliseconds {500});
    auto clientThread1 = std::thread(client);
    auto clientThread2 = std::thread(client);

    clientThread1.join();
    clientThread2.join();
    serverThread.join();
    proxyThread.join();
}

}
