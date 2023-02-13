#include <vector>
#include <thread>
#include <fstream>
#include <filesystem>
#include <umps/authentication/zapOptions.hpp>
#include <umps/messaging/routerDealer/proxy.hpp>
#include <umps/messaging/routerDealer/proxyOptions.hpp>
#include <uussmlmodels/detectors/uNetThreeComponentP/inference.hpp>
#include <uussmlmodels/detectors/uNetThreeComponentP/preprocessing.hpp>
#include "urts/services/scalable/detectors/uNetThreeComponentP/inferenceRequest.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/inferenceResponse.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/preprocessingRequest.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/preprocessingResponse.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/processingRequest.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/processingResponse.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/requestorOptions.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/requestor.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/serviceOptions.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/service.hpp"
#include <gtest/gtest.h>

namespace
{

using namespace URTS::Services::Scalable::Detectors::UNetThreeComponentP;

TEST(ServicesScalableDetectorsUNetThreeComponentP, PreprocessingRequest)
{
    const std::vector<double> vertical{1, 2, 3, 4, 5};
    const std::vector<double> north{6, 7, 8, 9, 10};
    const std::vector<double> east{11, 12, 13, 14, 15};
    const double samplingRate{50};
    const int64_t identifier{5021};
    PreprocessingRequest request;

    EXPECT_NO_THROW(request.setSamplingRate(samplingRate));
    EXPECT_NO_THROW(request.setVerticalNorthEastSignal(vertical, north, east));
    request.setIdentifier(identifier);
 
    PreprocessingRequest copy;
    EXPECT_NO_THROW(copy.fromMessage(request.toMessage()));
    EXPECT_EQ(copy.getIdentifier(), identifier);
    EXPECT_NEAR(copy.getSamplingRate(), samplingRate, 1.e-14);
    EXPECT_TRUE(copy.haveSignals());
    const auto vr = copy.getVerticalSignalReference();
    const auto nr = copy.getNorthSignalReference();
    const auto er = copy.getEastSignalReference();
    EXPECT_EQ(vertical.size(), vr.size()); 
    EXPECT_EQ(north.size(),    nr.size());
    EXPECT_EQ(east.size(),     er.size());
    for (int i = 0; i < static_cast<int> (vr.size()); ++i)
    {
        EXPECT_NEAR(vertical.at(i), vr.at(i), 1.e-14);
        EXPECT_NEAR(north.at(i),    nr.at(i), 1.e-14);
        EXPECT_NEAR(east.at(i),     er.at(i), 1.e-14);
    }

    auto v = copy.getVerticalSignal();
    auto n = copy.getNorthSignal();
    auto e = copy.getEastSignal();
    EXPECT_EQ(vertical.size(), v.size()); 
    EXPECT_EQ(north.size(),    n.size());
    EXPECT_EQ(east.size(),     e.size());
    for (int i = 0; i < static_cast<int> (v.size()); ++i)
    {   
        EXPECT_NEAR(vertical.at(i), v.at(i), 1.e-14);
        EXPECT_NEAR(north.at(i),    n.at(i), 1.e-14);
        EXPECT_NEAR(east.at(i),     e.at(i), 1.e-14);
    } 

    request.clear();
    EXPECT_NEAR(request.getSamplingRate(), 100, 1.e-14);
    EXPECT_EQ(request.getIdentifier(), 0);
    EXPECT_EQ(request.getMessageType(),
              "URTS::Services::Scalable::Detectors::UNetThreeComponentP::PreprocessingRequest");
}

TEST(ServicesScalableDetectorsUNetThreeComponentP, PreprocessingResponse)
{
    const std::vector<double> vertical{1, 2, 3, 4, 5};
    const std::vector<double> north{6, 7, 8, 9, 10};
    const std::vector<double> east{11, 12, 13, 14, 15};
    const double samplingRate{50};
    const int64_t identifier{5021};
    const PreprocessingResponse::ReturnCode
        returnCode{PreprocessingResponse::ReturnCode::Success};
    PreprocessingResponse response;

    EXPECT_NO_THROW(response.setSamplingRate(samplingRate));
    EXPECT_NO_THROW(response.setVerticalNorthEastSignal(vertical, north, east));
    response.setIdentifier(identifier);
    response.setReturnCode(returnCode);

    PreprocessingResponse copy;
    EXPECT_NO_THROW(copy.fromMessage(response.toMessage()));
    EXPECT_EQ(copy.getIdentifier(), identifier);
    EXPECT_EQ(copy.getReturnCode(), returnCode);
    EXPECT_NEAR(copy.getSamplingRate(), samplingRate, 1.e-14);
    EXPECT_TRUE(copy.haveSignals());
    auto v = copy.getVerticalSignal();
    auto n = copy.getNorthSignal();
    auto e = copy.getEastSignal();
    EXPECT_EQ(vertical.size(), v.size());
    EXPECT_EQ(north.size(),    n.size());
    EXPECT_EQ(east.size(),     e.size());
    for (int i = 0; i < static_cast<int> (v.size()); ++i)
    {
        EXPECT_NEAR(vertical.at(i), v.at(i), 1.e-14);
        EXPECT_NEAR(north.at(i),    n.at(i), 1.e-14);
        EXPECT_NEAR(east.at(i),     e.at(i), 1.e-14);
    }

    response.clear();
    EXPECT_NEAR(response.getSamplingRate(), 100, 1.e-14);
    EXPECT_EQ(response.getIdentifier(), 0);
    EXPECT_EQ(response.getMessageType(),
              "URTS::Services::Scalable::Detectors::UNetThreeComponentP::PreprocessingResponse");
    EXPECT_FALSE(response.haveReturnCode());
}

TEST(ServicesScalableDetectorsUNetThreeComponentP, InferenceRequest)
{
    std::vector<double> vertical(1008 + 128);
    std::vector<double> north(1008 + 128);
    std::vector<double> east(1008 + 128);
    std::fill(vertical.begin(), vertical.end(), 1);
    std::fill(north.begin(), north.end(), 2);
    std::fill(east.begin(), east.end(), 3);
    const int64_t identifier{1026};
    auto fixedWindow = InferenceRequest::InferenceStrategy::FixedWindow;
    auto slidingWindow = InferenceRequest::InferenceStrategy::SlidingWindow;
    InferenceRequest request;

    EXPECT_EQ(request.getMinimumSignalLength(), 1008);
    EXPECT_NO_THROW(request.setVerticalNorthEastSignal(vertical, north, east,
                                                       fixedWindow));
    EXPECT_NO_THROW(request.setVerticalNorthEastSignal(vertical, north, east,
                                                       slidingWindow));
    request.setIdentifier(identifier);

    InferenceRequest copy;
    EXPECT_NO_THROW(copy.fromMessage(request.toMessage()));
    EXPECT_EQ(copy.getIdentifier(), identifier);
    EXPECT_NEAR(copy.getSamplingRate(), 100, 1.e-14);
    EXPECT_EQ(copy.getInferenceStrategy(), slidingWindow);
    EXPECT_TRUE(copy.haveSignals());
    const auto vr = copy.getVerticalSignalReference();
    const auto nr = copy.getNorthSignalReference();
    const auto er = copy.getEastSignalReference();
    EXPECT_EQ(vertical.size(), vr.size());
    EXPECT_EQ(north.size(),    nr.size());
    EXPECT_EQ(east.size(),     er.size());
    for (int i = 0; i < static_cast<int> (vr.size()); ++i)
    {
        EXPECT_NEAR(vertical.at(i), vr.at(i), 1.e-14);
        EXPECT_NEAR(north.at(i),    nr.at(i), 1.e-14);
        EXPECT_NEAR(east.at(i),     er.at(i), 1.e-14);
    }

    auto v = copy.getVerticalSignal();
    auto n = copy.getNorthSignal();
    auto e = copy.getEastSignal();
    EXPECT_EQ(vertical.size(), v.size());
    EXPECT_EQ(north.size(),    n.size());
    EXPECT_EQ(east.size(),     e.size());
    for (int i = 0; i < static_cast<int> (v.size()); ++i)
    {
        EXPECT_NEAR(vertical.at(i), v.at(i), 1.e-14);
        EXPECT_NEAR(north.at(i),    n.at(i), 1.e-14);
        EXPECT_NEAR(east.at(i),     e.at(i), 1.e-14);
    }

    request.clear();
    EXPECT_NEAR(request.getSamplingRate(), 100, 1.e-14);
    EXPECT_FALSE(request.haveSignals());
    EXPECT_EQ(request.getIdentifier(), 0);
    EXPECT_EQ(request.getMessageType(),
              "URTS::Services::Scalable::Detectors::UNetThreeComponentP::InferenceRequest");
}
                                                                                              

TEST(ServicesScalableDetectorsUNetThreeComponentP, InferenceResponse)
{
    const std::vector<double> probabilitySignal{1, 2, 3, 4, 5}; 
    const double samplingRate{250};
    const int64_t identifier{5024};
    const InferenceResponse::ReturnCode
        returnCode{InferenceResponse::ReturnCode::Success};
    InferenceResponse response;

    EXPECT_NO_THROW(response.setSamplingRate(samplingRate));
    EXPECT_NO_THROW(response.setProbabilitySignal(probabilitySignal));
    response.setIdentifier(identifier);
    response.setReturnCode(returnCode);

    InferenceResponse copy;
    EXPECT_NO_THROW(copy.fromMessage(response.toMessage()));
    EXPECT_EQ(copy.getIdentifier(), identifier);
    EXPECT_EQ(copy.getReturnCode(), returnCode);
    EXPECT_NEAR(copy.getSamplingRate(), samplingRate, 1.e-14);
    EXPECT_TRUE(copy.haveProbabilitySignal());
    auto p = copy.getProbabilitySignal();
    EXPECT_EQ(probabilitySignal.size(), p.size());
    for (int i = 0; i < static_cast<int> (p.size()); ++i)
    {   
        EXPECT_NEAR(probabilitySignal.at(i), p.at(i), 1.e-7);
    }

    response.clear();
    EXPECT_NEAR(response.getSamplingRate(), 100, 1.e-14);
    EXPECT_EQ(response.getIdentifier(), 0); 
    EXPECT_EQ(response.getMessageType(),
              "URTS::Services::Scalable::Detectors::UNetThreeComponentP::InferenceResponse");
    EXPECT_FALSE(response.haveReturnCode());
}


TEST(ServicesScalableDetectorsUNetThreeComponentP, ProcessingRequest)
{
    std::vector<double> vertical(2*1008 + 128);
    std::vector<double> north(2*1008 + 128);
    std::vector<double> east(2*1008 + 128);
    std::fill(vertical.begin(), vertical.end(), 1); 
    std::fill(north.begin(), north.end(), 2); 
    std::fill(east.begin(), east.end(), 3); 
    const int64_t identifier{1026};
    const double samplingRate{50};
    auto fixedWindow = ProcessingRequest::InferenceStrategy::FixedWindow;
    auto slidingWindow = ProcessingRequest::InferenceStrategy::SlidingWindow;
    ProcessingRequest request;

    EXPECT_EQ(request.getMinimumSignalLength(), 1008);
    EXPECT_NO_THROW(request.setSamplingRate(samplingRate));
    EXPECT_NO_THROW(request.setVerticalNorthEastSignal(vertical, north, east,
                                                       fixedWindow));
    EXPECT_NO_THROW(request.setVerticalNorthEastSignal(vertical, north, east,
                                                       slidingWindow));
    request.setIdentifier(identifier);

    ProcessingRequest copy;
    EXPECT_NO_THROW(copy.fromMessage(request.toMessage()));
    EXPECT_EQ(copy.getIdentifier(), identifier);
    EXPECT_NEAR(copy.getSamplingRate(), samplingRate, 1.e-14);
    EXPECT_EQ(copy.getInferenceStrategy(), slidingWindow);
    EXPECT_TRUE(copy.haveSignals());
    const auto vr = copy.getVerticalSignalReference();
    const auto nr = copy.getNorthSignalReference();
    const auto er = copy.getEastSignalReference();
    EXPECT_EQ(vertical.size(), vr.size());
    EXPECT_EQ(north.size(),    nr.size());
    EXPECT_EQ(east.size(),     er.size());
    for (int i = 0; i < static_cast<int> (vr.size()); ++i)
    {
        EXPECT_NEAR(vertical.at(i), vr.at(i), 1.e-14);
        EXPECT_NEAR(north.at(i),    nr.at(i), 1.e-14);
        EXPECT_NEAR(east.at(i),     er.at(i), 1.e-14);
    }

    auto v = copy.getVerticalSignal();
    auto n = copy.getNorthSignal();
    auto e = copy.getEastSignal();
    EXPECT_EQ(vertical.size(), v.size());
    EXPECT_EQ(north.size(),    n.size());
    EXPECT_EQ(east.size(),     e.size());
    for (int i = 0; i < static_cast<int> (v.size()); ++i)
    {
        EXPECT_NEAR(vertical.at(i), v.at(i), 1.e-14);
        EXPECT_NEAR(north.at(i),    n.at(i), 1.e-14);
        EXPECT_NEAR(east.at(i),     e.at(i), 1.e-14);
    }

    request.clear();
    EXPECT_NEAR(request.getSamplingRate(), 100, 1.e-14);
    EXPECT_FALSE(request.haveSignals());
    EXPECT_EQ(request.getIdentifier(), 0);
    EXPECT_EQ(request.getMessageType(),
              "URTS::Services::Scalable::Detectors::UNetThreeComponentP::ProcessingRequest");
}

TEST(ServicesScalableDetectorsUNetThreeComponentP, ProcessingResponse)
{
    const std::vector<double> probabilitySignal{1, 2, 3, 4, 5}; 
    const double samplingRate{125};
    const int64_t identifier{5025};
    const ProcessingResponse::ReturnCode
        returnCode{ProcessingResponse::ReturnCode::Success};
    ProcessingResponse response;

    EXPECT_NO_THROW(response.setSamplingRate(samplingRate));
    EXPECT_NO_THROW(response.setProbabilitySignal(probabilitySignal));
    response.setIdentifier(identifier);
    response.setReturnCode(returnCode);

    ProcessingResponse copy;
    EXPECT_NO_THROW(copy.fromMessage(response.toMessage()));
    EXPECT_EQ(copy.getIdentifier(), identifier);
    EXPECT_EQ(copy.getReturnCode(), returnCode);
    EXPECT_NEAR(copy.getSamplingRate(), samplingRate, 1.e-14);
    EXPECT_TRUE(copy.haveProbabilitySignal());
    auto p = copy.getProbabilitySignal();
    EXPECT_EQ(probabilitySignal.size(), p.size());
    for (int i = 0; i < static_cast<int> (p.size()); ++i)
    {
        EXPECT_NEAR(probabilitySignal.at(i), p.at(i), 1.e-7);
    }

    response.clear();
    EXPECT_NEAR(response.getSamplingRate(), 100, 1.e-14);
    EXPECT_EQ(response.getIdentifier(), 0); 
    EXPECT_EQ(response.getMessageType(),
              "URTS::Services::Scalable::Detectors::UNetThreeComponentP::ProcessingResponse");
    EXPECT_FALSE(response.haveReturnCode());
}

TEST(ServicesScalableDetectorsUNetThreeComponentP, ServiceOptions)
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
    EXPECT_NO_THROW(options.setModelWeightsFile(weightsFile));
    options.setDevice(device);
    EXPECT_NO_THROW(options.setAddress(address));
    EXPECT_NO_THROW(options.setSendHighWaterMark(sendHWM));
    EXPECT_NO_THROW(options.setReceiveHighWaterMark(recvHWM));
    EXPECT_NO_THROW(options.setPollingTimeOut(pollTimeOut));
    options.setZAPOptions(zapOptions);
   
    ServiceOptions copy(options);
    EXPECT_EQ(copy.getModelWeightsFile(), weightsFile);
    EXPECT_EQ(copy.getDevice(), device);
    EXPECT_EQ(copy.getSendHighWaterMark(), sendHWM);
    EXPECT_EQ(copy.getReceiveHighWaterMark(), recvHWM);
    EXPECT_EQ(copy.getPollingTimeOut(), pollTimeOut);
    EXPECT_EQ(copy.getZAPOptions().getSecurityLevel(),
              zapOptions.getSecurityLevel());

    // Clean up  
    if (std::filesystem::exists(weightsFile))
    {
         std::filesystem::remove(weightsFile);
    }

    options.clear();
    EXPECT_FALSE(options.haveModelWeightsFile());
    EXPECT_FALSE(options.haveAddress());
    EXPECT_EQ(options.getDevice(), ServiceOptions::Device::CPU); 
    EXPECT_EQ(options.getSendHighWaterMark(), 8192);
    EXPECT_EQ(options.getReceiveHighWaterMark(), 4096);
    EXPECT_EQ(options.getPollingTimeOut(), std::chrono::milliseconds {10});
    EXPECT_EQ(options.getZAPOptions().getSecurityLevel(),
              UMPS::Authentication::SecurityLevel::Grasslands);
}

TEST(ServicesScalableDetectorsUNetThreeComponentP, RequestorOptions)
{
    const std::string address{"tcp://127.0.0.1:5550"};
    const int sendHWM{105};
    const int recvHWM{106};
    const std::chrono::milliseconds sendTimeOut{120};
    const std::chrono::milliseconds recvTimeOut{145};
    UMPS::Authentication::ZAPOptions zapOptions;
    zapOptions.setStrawhouseClient();

    RequestorOptions options;
    EXPECT_NO_THROW(options.setAddress(address));
    EXPECT_NO_THROW(options.setSendHighWaterMark(sendHWM));
    EXPECT_NO_THROW(options.setReceiveHighWaterMark(recvHWM));
    options.setSendTimeOut(sendTimeOut);
    options.setReceiveTimeOut(recvTimeOut);
    options.setZAPOptions(zapOptions);

    RequestorOptions copy(options);
    EXPECT_EQ(copy.getAddress(), address);
    EXPECT_EQ(copy.getSendHighWaterMark(), sendHWM);
    EXPECT_EQ(copy.getReceiveHighWaterMark(), recvHWM);
    EXPECT_EQ(copy.getSendTimeOut(), sendTimeOut); 
    EXPECT_EQ(copy.getReceiveTimeOut(), recvTimeOut);
    EXPECT_EQ(copy.getZAPOptions().getSecurityLevel(),
              zapOptions.getSecurityLevel());

    options.clear();
    EXPECT_EQ(options.getSendHighWaterMark(), 4096);
    EXPECT_EQ(options.getReceiveHighWaterMark(), 8192);
    EXPECT_EQ(options.getSendTimeOut(), std::chrono::milliseconds {0});
    EXPECT_EQ(options.getReceiveTimeOut(), std::chrono::milliseconds {3000});
    zapOptions.setGrasslandsClient();
    EXPECT_EQ(options.getZAPOptions().getSecurityLevel(),
              zapOptions.getSecurityLevel());
}

///--------------------------------------------------------------------------///
///                               Communication Test                         ///
///--------------------------------------------------------------------------///
const std::string frontendAddress{"tcp://127.0.0.1:5555"};
const std::string backendAddress{"tcp://127.0.0.1:5556"};
const std::string weightsFile{
    "/usr/local/share/UUSSMLModels/detectorsUNetThreeComponentP.onnx"
}; 
std::vector<double> zProcRef, nProcRef, eProcRef, pRef, pRefWindow;
std::vector<double> vertical(1008, 0); 
std::vector<double> north(1008, -1);
std::vector<double> east(1008, 1); 


void proxy()
{
    UMPS::Messaging::RouterDealer::ProxyOptions options;
    options.setFrontendAddress(frontendAddress);
    options.setBackendAddress(backendAddress);
    UMPS::Messaging::RouterDealer::Proxy proxy;
    EXPECT_NO_THROW(proxy.initialize(options));
    std::thread proxyThread(&UMPS::Messaging::RouterDealer::Proxy::start,
                            &proxy);
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    EXPECT_TRUE(proxy.isRunning());
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    EXPECT_NO_THROW(proxy.stop());
    proxyThread.join();
}

void server()
{
    ServiceOptions options;
    options.setAddress(backendAddress);
    options.setModelWeightsFile(weightsFile);
    Service service;
    EXPECT_NO_THROW(service.initialize(options));
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    EXPECT_TRUE(service.isInitialized());
    EXPECT_NO_THROW(service.start());
    std::this_thread::sleep_for(std::chrono::milliseconds{2000});
    service.stop();
}

void client()
{
    RequestorOptions options;
    options.setAddress(frontendAddress);
    Requestor client;
    EXPECT_NO_THROW(client.initialize(options));
    EXPECT_TRUE(client.isInitialized());

    const double samplingRate{100};

    // Do a processing request
    PreprocessingRequest preprocessingRequest;
    preprocessingRequest.setIdentifier(2);
    EXPECT_NO_THROW(preprocessingRequest.setSamplingRate(samplingRate));
    EXPECT_NO_THROW(preprocessingRequest.setVerticalNorthEastSignal(vertical,
                                                                    north,
                                                                    east));
    std::unique_ptr<PreprocessingResponse> preprocessingResponse{nullptr};
    EXPECT_NO_THROW(preprocessingResponse
        = client.request(preprocessingRequest));
    std::vector<double> zProc, nProc, eProc;
    if (preprocessingResponse != nullptr)
    {
        EXPECT_EQ(preprocessingResponse->getIdentifier(), 2);
        zProc = preprocessingResponse->getVerticalSignal();
        nProc = preprocessingResponse->getNorthSignal();
        eProc = preprocessingResponse->getEastSignal();
        EXPECT_EQ(zProc.size(), zProcRef.size());
        double error = 0;
        for (int i = 0; i < static_cast<int> (zProc.size()); ++i)
        {
            error = std::max(error, std::abs(zProc.at(i) - zProcRef.at(i)));
            error = std::max(error, std::abs(nProc.at(i) - nProcRef.at(i)));
            error = std::max(error, std::abs(eProc.at(i) - eProcRef.at(i)));
        } 
        EXPECT_NEAR(error, 0, 1.e-7);
    }
    // Follow on with an inference test
    InferenceRequest inferenceRequest;
    inferenceRequest.setIdentifier(3);
    inferenceRequest.setVerticalNorthEastSignal(zProcRef, nProcRef, eProcRef,
        InferenceRequest::InferenceStrategy::SlidingWindow);
    std::unique_ptr<InferenceResponse> inferenceResponse{nullptr};
    EXPECT_NO_THROW(
         inferenceResponse = client.request(inferenceRequest)
    );
    if (inferenceResponse != nullptr)
    {
        EXPECT_EQ(inferenceResponse->getIdentifier(), 3);
        auto probabilitySignal = inferenceResponse->getProbabilitySignal();
        EXPECT_EQ(probabilitySignal.size(), pRefWindow.size());
        double error = 0;
        for (int i = 0; i < static_cast<int> (probabilitySignal.size()); ++i)
        {
            error = std::max(error, std::abs(probabilitySignal[i]
                                           - pRefWindow[i]));
        }
        EXPECT_NEAR(error, 0, 1.e-7);
    }
    // Repeat with fixed window
    inferenceRequest.setVerticalNorthEastSignal(zProcRef, nProcRef, eProcRef,
        InferenceRequest::InferenceStrategy::FixedWindow);
    EXPECT_NO_THROW(
         inferenceResponse = client.request(inferenceRequest)
    );
    if (inferenceResponse != nullptr)
    {
        EXPECT_EQ(inferenceResponse->getIdentifier(), 3); 
        auto probabilitySignal = inferenceResponse->getProbabilitySignal();
        EXPECT_EQ(probabilitySignal.size(), pRef.size());
        double error = 0;
        for (int i = 0; i < static_cast<int> (probabilitySignal.size()); ++i)
        {
            error = std::max(error, std::abs(probabilitySignal[i]
                                           - pRef[i]));
        }
        EXPECT_NEAR(error, 0, 1.e-7);
    }


    // Do a sliding window (usual) then fixed window test
    ProcessingRequest processingRequest;
    processingRequest.setIdentifier(1);
    EXPECT_NO_THROW(processingRequest.setSamplingRate(samplingRate));
    EXPECT_NO_THROW(
        processingRequest.setVerticalNorthEastSignal(vertical, north, east,
            ProcessingRequest::InferenceStrategy::SlidingWindow)
    );
    std::unique_ptr<ProcessingResponse> processingResponse{nullptr};
    EXPECT_NO_THROW(processingResponse = client.request(processingRequest));
    if (processingResponse != nullptr)
    {
        EXPECT_EQ(processingResponse->getIdentifier(), 1); 
        auto probabilitySignal = processingResponse->getProbabilitySignal();
        double error = 0;
        for (int i = 0; i < static_cast<int> (probabilitySignal.size()); ++i)
        {
            error = std::max(error, std::abs(probabilitySignal[i]
                                           - pRefWindow[i]));
        }
        EXPECT_NEAR(error, 0, 1.e-7);
    }
    EXPECT_NO_THROW(
        processingRequest.setVerticalNorthEastSignal(vertical, north, east,
            ProcessingRequest::InferenceStrategy::FixedWindow)
    );
    EXPECT_NO_THROW(processingResponse = client.request(processingRequest));
    if (processingResponse != nullptr)
    {
        auto probabilitySignal = processingResponse->getProbabilitySignal();
        EXPECT_EQ(processingResponse->getIdentifier(), 1);
        EXPECT_EQ(pRef.size(), probabilitySignal.size());
        double error = 0; 
        for (int i = 0; i < static_cast<int> (probabilitySignal.size()); ++i)
        {
            error = std::max(error, std::abs(probabilitySignal[i]
                                           - pRef[i]));
        }
        EXPECT_NEAR(error, 0, 1.e-7);
    }
}

TEST(ServicesScalableDetectorsUNetThreeComponentP, Service)
{
    // Do this manually
    {
        UUSSMLModels::Detectors::UNetThreeComponentP::Preprocessing
             preprocessing;
        UUSSMLModels::Detectors::UNetThreeComponentP::Inference inference;
        EXPECT_NO_THROW(inference.load(weightsFile));

        auto result = preprocessing.process(vertical, north, east);
        zProcRef = std::get<0> (result);
        nProcRef = std::get<1> (result);
        eProcRef = std::get<2> (result);
        pRefWindow = inference.predictProbabilitySlidingWindow(zProcRef,
                                                               nProcRef,
                                                               eProcRef);
        pRef = inference.predictProbability(zProcRef, nProcRef, eProcRef);
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
