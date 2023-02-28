#include <vector>
#include <thread>
#include <fstream>
#include <filesystem>
#include <umps/authentication/zapOptions.hpp>
#include <umps/messaging/routerDealer/proxy.hpp>
#include <umps/messaging/routerDealer/proxyOptions.hpp>
#include <uussmlmodels/pickers/cnnOneComponentP/inference.hpp>
#include <uussmlmodels/pickers/cnnOneComponentP/preprocessing.hpp>
#include "urts/services/scalable/pickers/cnnOneComponentP/inferenceRequest.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/inferenceResponse.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/preprocessingRequest.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/preprocessingResponse.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/processingRequest.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/processingResponse.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/requestorOptions.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/requestor.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/serviceOptions.hpp"
#include "urts/services/scalable/pickers/cnnOneComponentP/service.hpp"
#include <gtest/gtest.h>

namespace
{

using namespace URTS::Services::Scalable::Pickers::CNNOneComponentP;

TEST(ServicesScalablePickersCNNOneComponentP, PreprocessingRequest)
{
    const std::vector<double> vertical{1, 2, 3, 4, 5};
    const double samplingRate{50};
    const int64_t identifier{5021};
    PreprocessingRequest request;

    EXPECT_NO_THROW(request.setSamplingRate(samplingRate));
    EXPECT_NO_THROW(request.setVerticalSignal(vertical));
    request.setIdentifier(identifier);
 
    PreprocessingRequest copy;
    EXPECT_NO_THROW(copy.fromMessage(request.toMessage()));
    EXPECT_EQ(copy.getIdentifier(), identifier);
    EXPECT_NEAR(copy.getSamplingRate(), samplingRate, 1.e-14);
    EXPECT_TRUE(copy.haveSignal());
    const auto vr = copy.getVerticalSignalReference();
    EXPECT_EQ(vertical.size(), vr.size()); 
    for (int i = 0; i < static_cast<int> (vr.size()); ++i)
    {
        EXPECT_NEAR(vertical.at(i), vr.at(i), 1.e-14);
    }

    auto v = copy.getVerticalSignal();
    EXPECT_EQ(vertical.size(), v.size()); 
    for (int i = 0; i < static_cast<int> (v.size()); ++i)
    {   
        EXPECT_NEAR(vertical.at(i), v.at(i), 1.e-14);
    } 

    request.clear();
    EXPECT_NEAR(request.getSamplingRate(), 100, 1.e-14);
    EXPECT_EQ(request.getIdentifier(), 0);
    EXPECT_EQ(request.getMessageType(),
              "URTS::Services::Scalable::Pickers::CNNOneComponentP::PreprocessingRequest");
}

TEST(ServicesScalablePickersCNNOneComponentP, PreprocessingResponse)
{
    const std::vector<double> vertical{1, 2, 3, 4, 5};
    const double samplingRate{50};
    const int64_t identifier{5021};
    const PreprocessingResponse::ReturnCode
        returnCode{PreprocessingResponse::ReturnCode::Success};
    PreprocessingResponse response;

    EXPECT_NO_THROW(response.setSamplingRate(samplingRate));
    EXPECT_NO_THROW(response.setVerticalSignal(vertical));
    response.setIdentifier(identifier);
    response.setReturnCode(returnCode);

    PreprocessingResponse copy;
    EXPECT_NO_THROW(copy.fromMessage(response.toMessage()));
    EXPECT_EQ(copy.getIdentifier(), identifier);
    EXPECT_EQ(copy.getReturnCode(), returnCode);
    EXPECT_NEAR(copy.getSamplingRate(), samplingRate, 1.e-14);
    EXPECT_TRUE(copy.haveSignal());
    auto v = copy.getVerticalSignal();
    EXPECT_EQ(vertical.size(), v.size());
    for (int i = 0; i < static_cast<int> (v.size()); ++i)
    {
        EXPECT_NEAR(vertical.at(i), v.at(i), 1.e-14);
    }

    response.clear();
    EXPECT_NEAR(response.getSamplingRate(), 100, 1.e-14);
    EXPECT_EQ(response.getIdentifier(), 0);
    EXPECT_EQ(response.getMessageType(),
              "URTS::Services::Scalable::Pickers::CNNOneComponentP::PreprocessingResponse");
    EXPECT_FALSE(response.haveReturnCode());
}

TEST(ServicesScalablePickersCNNOneComponentP, InferenceRequest)
{
    std::vector<double> vertical(400);
    std::fill(vertical.begin(), vertical.end(), 1);
    const int64_t identifier{1026};
    InferenceRequest request;

    EXPECT_EQ(request.getExpectedSignalLength(), 400);
    EXPECT_NO_THROW(request.setVerticalSignal(vertical));
    request.setIdentifier(identifier);

    InferenceRequest copy;
    EXPECT_NO_THROW(copy.fromMessage(request.toMessage()));
    EXPECT_EQ(copy.getIdentifier(), identifier);
    EXPECT_NEAR(copy.getSamplingRate(), 100, 1.e-14);
    EXPECT_TRUE(copy.haveSignal());
    const auto vr = copy.getVerticalSignalReference();
    EXPECT_EQ(vertical.size(), vr.size());
    for (int i = 0; i < static_cast<int> (vr.size()); ++i)
    {
        EXPECT_NEAR(vertical.at(i), vr.at(i), 1.e-14);
    }

    auto v = copy.getVerticalSignal();
    EXPECT_EQ(vertical.size(), v.size());
    for (int i = 0; i < static_cast<int> (v.size()); ++i)
    {
        EXPECT_NEAR(vertical.at(i), v.at(i), 1.e-14);
    }

    request.clear();
    EXPECT_NEAR(request.getSamplingRate(), 100, 1.e-14);
    EXPECT_FALSE(request.haveSignal());
    EXPECT_EQ(request.getIdentifier(), 0);
    EXPECT_EQ(request.getMessageType(),
              "URTS::Services::Scalable::Pickers::CNNOneComponentP::InferenceRequest");
}
                                                                                              

TEST(ServicesScalablePickersCNNOneComponentP, InferenceResponse)
{
    const double correction{-0.3};
    const int64_t identifier{5024};
    const InferenceResponse::ReturnCode
        returnCode{InferenceResponse::ReturnCode::Success};
    InferenceResponse response;

    EXPECT_NO_THROW(response.setCorrection(correction));
    response.setIdentifier(identifier);
    response.setReturnCode(returnCode);

    InferenceResponse copy;
    EXPECT_NO_THROW(copy.fromMessage(response.toMessage()));
    EXPECT_EQ(copy.getIdentifier(), identifier);
    EXPECT_EQ(copy.getReturnCode(), returnCode);
    EXPECT_TRUE(copy.haveCorrection());
    EXPECT_NEAR(copy.getCorrection(), correction, 1.e-7);

    response.clear();
    EXPECT_EQ(response.getIdentifier(), 0); 
    EXPECT_EQ(response.getMessageType(),
              "URTS::Services::Scalable::Pickers::CNNOneComponentP::InferenceResponse");
    EXPECT_FALSE(response.haveReturnCode());
}


TEST(ServicesScalablePickersCNNOneComponentP, ProcessingRequest)
{
    std::vector<double> vertical(400);
    std::fill(vertical.begin(), vertical.end(), 1); 
    const int64_t identifier{1026};
    const double samplingRate{50};
    ProcessingRequest request;

    EXPECT_EQ(request.getExpectedSignalLength(), 400);
    EXPECT_NO_THROW(request.setSamplingRate(samplingRate));
    EXPECT_NO_THROW(request.setVerticalSignal(vertical));
    request.setIdentifier(identifier);

    ProcessingRequest copy;
    EXPECT_NO_THROW(copy.fromMessage(request.toMessage()));
    EXPECT_EQ(copy.getIdentifier(), identifier);
    EXPECT_NEAR(copy.getSamplingRate(), samplingRate, 1.e-14);
    EXPECT_TRUE(copy.haveSignal());
    const auto vr = copy.getVerticalSignalReference();
    EXPECT_EQ(vertical.size(), vr.size());
    for (int i = 0; i < static_cast<int> (vr.size()); ++i)
    {
        EXPECT_NEAR(vertical.at(i), vr.at(i), 1.e-14);
    }

    auto v = copy.getVerticalSignal();
    EXPECT_EQ(vertical.size(), v.size());
    for (int i = 0; i < static_cast<int> (v.size()); ++i)
    {
        EXPECT_NEAR(vertical.at(i), v.at(i), 1.e-14);
    }

    request.clear();
    EXPECT_NEAR(request.getSamplingRate(), 100, 1.e-14);
    EXPECT_FALSE(request.haveSignal());
    EXPECT_EQ(request.getIdentifier(), 0);
    EXPECT_EQ(request.getMessageType(),
              "URTS::Services::Scalable::Pickers::CNNOneComponentP::ProcessingRequest");
}

TEST(ServicesScalablePickersCNNOneComponentP, ProcessingResponse)
{
    const double correction{0.3};
    const int64_t identifier{5025};
    const ProcessingResponse::ReturnCode
        returnCode{ProcessingResponse::ReturnCode::Success};
    ProcessingResponse response;

    EXPECT_NO_THROW(response.setCorrection(correction));
    response.setIdentifier(identifier);
    response.setReturnCode(returnCode);

    ProcessingResponse copy;
    EXPECT_NO_THROW(copy.fromMessage(response.toMessage()));
    EXPECT_EQ(copy.getIdentifier(), identifier);
    EXPECT_EQ(copy.getReturnCode(), returnCode);
    EXPECT_TRUE(copy.haveCorrection());
    EXPECT_NEAR(copy.getCorrection(), correction, 1.e-7);

    response.clear();
    EXPECT_EQ(response.getIdentifier(), 0); 
    EXPECT_EQ(response.getMessageType(),
              "URTS::Services::Scalable::Pickers::CNNOneComponentP::ProcessingResponse");
    EXPECT_FALSE(response.haveReturnCode());
}

TEST(ServicesScalablePickersCNNOneComponentP, ServiceOptions)
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

TEST(ServicesScalablePickersCNNOneComponentP, RequestorOptions)
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
    "/usr/local/share/UUSSMLModels/pickersCNNOneComponentP.onnx"
}; 
std::vector<double> zProcRef;
std::vector<double> vertical(400, 0); 
double correctionReference{0};

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
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
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
    std::this_thread::sleep_for(std::chrono::milliseconds{1500});
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
    EXPECT_NO_THROW(preprocessingRequest.setVerticalSignal(vertical));
    std::unique_ptr<PreprocessingResponse> preprocessingResponse{nullptr};
    EXPECT_NO_THROW(preprocessingResponse
        = client.request(preprocessingRequest));
    std::vector<double> zProc;
    if (preprocessingResponse != nullptr)
    {
        EXPECT_EQ(preprocessingResponse->getIdentifier(), 2);
        zProc = preprocessingResponse->getVerticalSignal();
        EXPECT_EQ(zProc.size(), zProcRef.size());
        double error = 0;
        for (int i = 0; i < static_cast<int> (zProc.size()); ++i)
        {
            error = std::max(error, std::abs(zProc.at(i) - zProcRef.at(i)));
        } 
        EXPECT_NEAR(error, 0, 1.e-7);
    }
    // Follow on with an inference test
    InferenceRequest inferenceRequest;
    inferenceRequest.setIdentifier(3);
    inferenceRequest.setVerticalSignal(zProcRef);
    std::unique_ptr<InferenceResponse> inferenceResponse{nullptr};
    EXPECT_NO_THROW(inferenceResponse = client.request(inferenceRequest));
    if (inferenceResponse != nullptr)
    {
        EXPECT_EQ(inferenceResponse->getIdentifier(), 3);
        auto correction = inferenceResponse->getCorrection();
        EXPECT_NEAR(correction, correctionReference, 1.e-7);
    }
}

TEST(ServicesScalablePickersCNNOneComponentP, Service)
{
    // Do this manually
    {
        UUSSMLModels::Pickers::CNNOneComponentP::Preprocessing preprocessing;
        UUSSMLModels::Pickers::CNNOneComponentP::Inference inference;
        EXPECT_NO_THROW(inference.load(weightsFile));

        zProcRef = preprocessing.process(vertical);
        correctionReference = inference.predict(zProcRef);
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
