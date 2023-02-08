#include <vector>
#include <fstream>
#include <filesystem>
#include <umps/authentication/zapOptions.hpp>
#include "urts/services/scalable/detectors/uNetThreeComponentP/inferenceRequest.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/inferenceResponse.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/preprocessingRequest.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/preprocessingResponse.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/processingRequest.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/processingResponse.hpp"
#include "urts/services/scalable/detectors/uNetThreeComponentP/serviceOptions.hpp"
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

}
