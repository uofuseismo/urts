#include <vector>
#include "urts/services/scalable/uNetDetector/threeComponent/inferenceResponse.hpp"
#include "urts/services/scalable/uNetDetector/threeComponent/preprocessingRequest.hpp"
#include "urts/services/scalable/uNetDetector/threeComponent/preprocessingResponse.hpp"
#include <gtest/gtest.h>

namespace
{

using namespace URTS::Services::Scalable::UNetDetector::ThreeComponent;

TEST(ServicesScalableUNetDetectorThreeComponent, PreprocessingRequest)
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
              "URTS::Services::Scalable::UNetDetector::ThreeComponent::PreprocessingRequest");
}

TEST(ServicesScalableUNetDetectorThreeComponent, PreprocessingResponse)
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
              "URTS::Services::Scalable::UNetDetector::ThreeComponent::PreprocessingResponse");
    EXPECT_FALSE(response.haveReturnCode());
}

TEST(ServicesScalableUNetDetectorThreeComponent, InferenceResponse)
{
    const std::vector<float> probabilitySignal{1, 2, 3, 4, 5}; 
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
              "URTS::Services::Scalable::UNetDetector::ThreeComponent::InferenceResponse");
    EXPECT_FALSE(response.haveReturnCode());
}

}
