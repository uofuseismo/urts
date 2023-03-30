#ifndef URTS_MODULES_DETECTOR_PROPERTIES_HPP
#define URTS_MODULES_DETECTOR_PROPERTIES_HPP
#include <cmath>
#include <chrono>
#include <uussmlmodels/detectors/uNetThreeComponentP/inference.hpp>
#include <uussmlmodels/detectors/uNetThreeComponentS/inference.hpp>
namespace
{
struct P3CDetectorProperties
{
    P3CDetectorProperties()
    {    
        mDetectorWindowDuration
            = std::chrono::microseconds {
                static_cast<int64_t> (std::round(mExpectedLength/mSamplingRate*1.e6))
              };
    }    
    std::chrono::microseconds mDetectorWindowDuration{10080000};
    double mSamplingRate{UUSSMLModels::Detectors::UNetThreeComponentP::Inference::getSamplingRate()};
    int mExpectedLength{UUSSMLModels::Detectors::UNetThreeComponentP::Inference::getExpectedSignalLength()};
    int mWindowStart{UUSSMLModels::Detectors::UNetThreeComponentP::Inference::getCentralWindowStartEndIndex().first};
    int mWindowEnd{UUSSMLModels::Detectors::UNetThreeComponentP::Inference::getCentralWindowStartEndIndex().second}; 
};

struct S3CDetectorProperties
{
    S3CDetectorProperties()
    {    
        mDetectorWindowDuration
            = std::chrono::microseconds {
                static_cast<int64_t> (std::round(mExpectedLength/mSamplingRate*1.e6))
              };
    }    
    std::chrono::microseconds mDetectorWindowDuration{10080000};
    double mSamplingRate{UUSSMLModels::Detectors::UNetThreeComponentS::Inference::getSamplingRate()};
    int mExpectedLength{UUSSMLModels::Detectors::UNetThreeComponentS::Inference::getExpectedSignalLength()};
    int mWindowStart{UUSSMLModels::Detectors::UNetThreeComponentS::Inference::getCentralWindowStartEndIndex().first};
    int mWindowEnd{UUSSMLModels::Detectors::UNetThreeComponentS::Inference::getCentralWindowStartEndIndex().second}; 
};

bool operator==(const P3CDetectorProperties &lhs,
                const S3CDetectorProperties &rhs)
{
    if (lhs.mDetectorWindowDuration != rhs.mDetectorWindowDuration){return false;}
    if (std::abs(lhs.mSamplingRate - rhs.mSamplingRate) > 1.e-4){return false;}
    if (lhs.mExpectedLength != rhs.mExpectedLength){return false;}
    if (lhs.mWindowStart != rhs.mWindowStart){return false;}
    if (lhs.mWindowEnd != rhs.mWindowEnd){return false;}
    return true;
}
}
#endif
