#include <iostream>
#include <cmath>
#include <vector>
#ifndef NDEBUG
#include <cassert>
#endif
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "urts/broadcasts/internal/probabilityPacket/probabilityPacket.hpp"
#include "thresholdDetector.hpp"
#include "thresholdDetectorOptions.hpp"
#include "triggerWindow.hpp"

using namespace URTS::Modules::Pickers;

namespace
{

/// @brief Defines what to do with the next packet.
enum class NextPacketCategory
{
    Normal, /*!< Packet overlaps or is on time.  Recommend using this packet
                 starting with the start sample. */
    Gap,    /*!< Packet represents a data gap.  Recommend resetting the module
                 and using this packet starting at the first sample. */
    Expired, /*!< Packet expired.  Recommend discarding this packet 
                  altogether. */
    AlgorithmicFailure /*!< Sometimes bad things happen.
                            Log the error and email me. */
};

/// @brief Rounds a number to the nearest digit.  For example,
///        if digit = 2, and x = 203.8 then this will be rounded to the
///        nearest 100 (10^2) which in this case is 200.
template<typename T>
[[nodiscard]] [[maybe_unused]]
T roundToNearestDigit(const T xIn, const int digit)
{
    T result = xIn;
    if (digit == 0){return std::round(result);}
    double xScale = std::pow(10., digit);
    result = static_cast<T> (std::round(xIn/xScale)*xScale);
    return result;
}

[[nodiscard]]
int64_t computeTimeTolerance(const int64_t samplingPeriodInMicroSeconds)
{
    return static_cast<int64_t> (std::round(samplingPeriodInMicroSeconds/4.));
}

/// @brief Non-strict equality comparitor.
/// @result True indicates a is within tol of b.
template<class T>
[[nodiscard]] [[maybe_unused]]
bool isClose(const T a, const T b, const T tol)
{
    return (std::abs(a - b) < tol);
}

/// @brief Does the unsavory task of finding the sample in the next packet
///        to use in your real-time algorithm.  This can deal with gaps,
///        overlapping, and expired packets.
/// @param[in] packetStartTime     Start time of packet.
/// @param[in] packetEndTime       End time of packet.
/// @param[in] lastEvaluationTime  Timestamp of last sample in real-time
///                                algorithm.
/// @param[in] nSamples  Number of samples in packet.
/// @param[in] samplingPeriodMuS   The sampling period in miroseconds.
/// @param[in] timeToleranceMuS    Even with integer arithmetic strict 
///                                equality between the start time of the
///                                packet and desired next time is unrealizble.
///                                Data loggers and time is hard.
/// @param[in] gapSizeInSamples    The number of samples allowed between this
///                                and the next packet (using a sampling period
///                                given by samplingPeriodMuS).  After this 
///                                point a gap is declared.
/// @result result.first is the start index to use in the next packet for
///         copying, updating, etc. and result.second is a packet descriptor.
[[nodiscard]] [[maybe_unused]]
std::pair<int, NextPacketCategory>
   getStartSample(const std::chrono::microseconds &packetStartTime,
                  const std::chrono::microseconds &packetEndTime,
                  const std::chrono::microseconds &lastEvaluationTime,
                  const int nSamples,
                  const int64_t samplingPeriodMuS,
                  const int64_t timeToleranceMuS,
                  const int gapSizeInSamples = 5)
{
    // Initialize result
    NextPacketCategory category{NextPacketCategory::Normal};
    int iStart = 0;
    // Round the desired next time
    int64_t desiredNextTimeMuS = lastEvaluationTime.count()
                               + samplingPeriodMuS;
    desiredNextTimeMuS = ::roundToNearestDigit(desiredNextTimeMuS, 1);
    int64_t iStartTimeMuS = packetStartTime.count();

    if (packetEndTime.count() < desiredNextTimeMuS - timeToleranceMuS)
    {
        category = NextPacketCategory::Expired;
        iStart = nSamples; // Way to prevent updates in ensuing for loop 
        return std::pair{iStart, category};
    }
    // Data is right on time
    if (::isClose(iStartTimeMuS, desiredNextTimeMuS, timeToleranceMuS))
    {
        category = NextPacketCategory::Normal;
        iStart = 0;
    }
    // Gap exists - reset and try again
    else if (iStartTimeMuS > desiredNextTimeMuS + samplingPeriodMuS)
    {
        // Definite gap
        if (iStartTimeMuS > desiredNextTimeMuS
                          + std::max(0, gapSizeInSamples - 1)*samplingPeriodMuS)
        {
            // Should probably reset detector and start at beginning of this
            // signal
            category = NextPacketCategory::Gap;
            iStart = 0;
        }
        else // It's within tolerance
        {
            category = NextPacketCategory::Normal;
            iStart = 0;
        }
    }
    // Overlap
    else
    {
        // Usually can get to about one sample with rounding error
        // but for expediency we guess in the vicinity of the expected
        // sample.
        std::array<int, 3> offsets{-1, -1, -1};
        auto dt = static_cast<double> (desiredNextTimeMuS
                                     - iStartTimeMuS);
        // First guess is usually right
        offsets.at(0) = static_cast<int>
                        (std::round(dt/samplingPeriodMuS));
        // Note these bounds ensure next two guesses are usable 
        offsets.at(0) = std::min(nSamples - 2, offsets.at(0));
        offsets.at(0) = std::max(1, offsets.at(0));
        // However, it could be the previous sample
        offsets.at(1) = offsets.at(0) - 1;
        // Or it could be the next sample
        offsets.at(2) = offsets.at(0) + 1;
        int64_t iT1Est = 0;
        int offset =-1;
        for (int i = 0; i < static_cast<int> (offsets.size()); ++i)
        {
            iT1Est = iStartTimeMuS + offsets[i]*samplingPeriodMuS;
            if (::isClose(desiredNextTimeMuS, iT1Est, timeToleranceMuS))
            {
                offset = offsets[i];
                break;
            }
         }
         iT1Est = iStartTimeMuS + offset*samplingPeriodMuS;
         // Guessing failed - try the brute force approach
         if (!::isClose(desiredNextTimeMuS, iT1Est, timeToleranceMuS))
         {
             std::string msg = "Doing it slow way: "
                             + std::to_string(desiredNextTimeMuS) + " "
                             + std::to_string(roundToNearestDigit(iT1Est, 1));
             std::cerr << msg << std::endl;
             bool lFound = false;
             for (int i = 0; i < nSamples; ++i)
             {
                 int64_t time = iStartTimeMuS + i*samplingPeriodMuS;
                 if (::isClose(desiredNextTimeMuS, time, timeToleranceMuS))
                 {
                     iStart = i;
                     category = NextPacketCategory::Normal;
                     lFound = true;
                     break;
                 }
             }
             // Algorithmic failure - lets log it and get out of here
             if (!lFound)
             {
                 std::cerr << "Algorithmic failure" << std::endl;
                 iStart = nSamples;
                 category = NextPacketCategory::AlgorithmicFailure;
             }
         }
         else
         {
             iStart = offset;
             category = NextPacketCategory::Normal;
         }
    }
    return std::pair{iStart, category};
}

/// @brief Toggles the detector state.
enum class State
{   
    Off = 0,  /*!< Indicates the detector is waiting to exceed the
                   on threshold */
    On = 1    /*!< Indicates the detector has exceeded the on threshold and
                   is waiting to drop below the off threshold. */
}; 
}

class ThresholdDetector::ThresholdDetectorImpl
{
public:
    TriggerWindow<double> mCurrentTriggerWindow;
    ThresholdDetectorOptions mOptions;
    std::chrono::microseconds mLastEvaluationTime{-2208988800000000}; // 1900
    std::chrono::microseconds mMaxTriggerDuration{10000000}; // 10 s
    State mState{State::Off};
    bool mInitialized{false};
};

/// C'tor
ThresholdDetector::ThresholdDetector() :
    pImpl(std::make_unique<ThresholdDetectorImpl> ())
{
}

/// Copy c'tor
ThresholdDetector::ThresholdDetector(const ThresholdDetector &detector)
{
    *this = detector;
}

/// Move c'tor
ThresholdDetector::ThresholdDetector(ThresholdDetector &&detector) noexcept
{
    *this = std::move(detector);
}

/// Copy assignment
ThresholdDetector&
ThresholdDetector::operator=(const ThresholdDetector &detector)
{
    if (&detector == this){return *this;}
    pImpl = std::make_unique<ThresholdDetectorImpl> (*detector.pImpl);
    return *this;
} 

/// Move assignment
ThresholdDetector&
ThresholdDetector::operator=(ThresholdDetector &&detector) noexcept
{
    if (&detector == this){return *this;}
    pImpl = std::move(detector.pImpl);
    return *this;
}

/// Reset class
void ThresholdDetector::clear() noexcept
{
    pImpl = std::make_unique<ThresholdDetectorImpl> ();
}

/// Destructor
ThresholdDetector::~ThresholdDetector() = default;

/// Initialize
void ThresholdDetector::initialize(const ThresholdDetectorOptions &options)
{
    if (!options.haveOnThreshold())
    {
        throw std::runtime_error("On threshold not set");
    }
    if (!options.haveOffThreshold())
    {
        throw std::runtime_error("Off threshold not set");
    }
    pImpl->mOptions = options;
    pImpl->mInitialized = true;
}

/// Initialized?
bool ThresholdDetector::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// Apply the detector
void ThresholdDetector::apply(
    const URTS::Broadcasts::Internal::DataPacket::DataPacket &packet,
    std::vector<TriggerWindow<double>> *triggerWindows)
{
    if (!packet.haveSamplingRate())
    {   
        throw std::invalid_argument("Sampling rate not set");
    }
    apply(packet.getDataReference(),
          packet.getSamplingRate(),
          packet.getStartTime(),
          triggerWindows);
}

void ThresholdDetector::apply(
    const URTS::Broadcasts::Internal::ProbabilityPacket::
                                      ProbabilityPacket &packet,
    std::vector<TriggerWindow<double>> *triggerWindows)
{
    if (!packet.haveSamplingRate())
    {
        throw std::invalid_argument("Sampling rate not set");
    }
    apply(packet.getDataReference(),
          packet.getSamplingRate(),
          packet.getStartTime(),
          triggerWindows);
}

void ThresholdDetector::apply(
    const std::vector<double> &signal,
    const double samplingRate,
    const std::chrono::microseconds &startTime,
    std::vector<TriggerWindow<double>> *triggerWindows)
{
    if (triggerWindows == nullptr)
    {   
        throw std::invalid_argument("Trigger windows is NULL");
    }
    triggerWindows->clear();
    if (signal.empty()){return;}
    if (!isInitialized())
    {
        throw std::invalid_argument("Class not initialized");
    }
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("Sampling rate not positive");
    }
    auto nSamples = static_cast<int> (signal.size());
    // Figure out the sample at which to commence the threshold evaluation
    auto iSamplingPeriodInMicroSeconds
        = static_cast<int64_t> (std::round(1000000./samplingRate));
    auto endTime = startTime
                 + (nSamples - 1)
                  *std::chrono::microseconds {iSamplingPeriodInMicroSeconds};
    auto timeTolerance = ::computeTimeTolerance(iSamplingPeriodInMicroSeconds);
    auto [iStart, category]
        = ::getStartSample(startTime,
                           endTime,
                           pImpl->mLastEvaluationTime,
                           nSamples,
                           iSamplingPeriodInMicroSeconds,
                           timeTolerance,
                           pImpl->mOptions.getMinimumGapSize());
    if (category != NextPacketCategory::Normal)
    {
        if (category == NextPacketCategory::Expired)
        {
            return;
        }
        else if (category == NextPacketCategory::Gap)
        {
            resetInitialConditions();
        }
        else
        {
            throw std::runtime_error("Algorithmic failure");
        }
    }
    // Update the detector
    //auto signal = packet.getDataPointer();
    auto iStartTimeMuS = startTime.count();
    auto onThreshold = pImpl->mOptions.getOnThreshold();
    auto offThreshold = pImpl->mOptions.getOffThreshold();
    auto maxTriggerDuration = pImpl->mOptions.getMaximumTriggerDuration();
    bool checkTriggerDuration = (maxTriggerDuration.count() > 0);
    std::pair<std::chrono::microseconds, double> startPair, maxPair;
    if (pImpl->mState == State::On)
    {
        startPair = pImpl->mCurrentTriggerWindow.getStart();
        maxPair = pImpl->mCurrentTriggerWindow.getMaximum();
    }
    for (int i = iStart; i < nSamples; ++i)
    {
        int64_t iNow = iStartTimeMuS
                     + i*iSamplingPeriodInMicroSeconds;
        iNow = ::roundToNearestDigit(iNow, 1);
        std::chrono::microseconds tNow{iNow};
        pImpl->mLastEvaluationTime = tNow;
        // Looking to start the window
        if (pImpl->mState == State::Off)
        {
            if (signal[i] >= onThreshold)
            {
                // Window starts - create it and change state
                pImpl->mCurrentTriggerWindow = TriggerWindow<double> {};
                startPair.first = tNow;
                startPair.second = signal[i]; 
                maxPair = startPair;
                pImpl->mCurrentTriggerWindow.setStart(startPair);
                // To simplify algorithm initialize the maximum
                pImpl->mCurrentTriggerWindow.setMaximum(maxPair);
                pImpl->mState = State::On;
            }
        }
        else // Looking to end the window
        {
            // Terminate window?
            if (signal[i] < offThreshold)
            {
#ifndef NDEBUG
                assert(pImpl->mCurrentTriggerWindow.haveStart());
                assert(pImpl->mCurrentTriggerWindow.haveMaximum());
#endif
                // Set the end
                std::pair<std::chrono::microseconds, double>
                    endPair{tNow, signal[i]};
                pImpl->mCurrentTriggerWindow.setEnd(endPair);
                // Handle one edge case - last sample is biggest.
                // This can really only happen if the off threshold is
                // bigger than the on threshold which is an inherently
                // screwy scenario.
                if (endPair.second > maxPair.second)
                {
                    pImpl->mCurrentTriggerWindow.setMaximum(endPair);
                }
                // Now add it to the triggers and change state
                triggerWindows->push_back(pImpl->mCurrentTriggerWindow);
                pImpl->mState = State::Off;
            }
            else
            {
                // Business as ususal
                if (signal[i] > maxPair.second)
                {
                    maxPair.first = tNow;
                    maxPair.second = signal[i];
                    pImpl->mCurrentTriggerWindow.setMaximum(maxPair);
                }
                // Trigger expired -> kill it
                if (checkTriggerDuration &&
                    tNow - startPair.first > maxTriggerDuration)
                {
                    pImpl->mState = State::Off;
                }
            }
        }
    } // Loop on samples
}

/// Set initial conditions
void ThresholdDetector::setInitialConditions()
{
    resetInitialConditions();
}

/// Reset initial conditions
void ThresholdDetector::resetInitialConditions()
{   
    if (!isInitialized()){throw std::runtime_error("Class not initialized");}
    pImpl->mCurrentTriggerWindow.clear();
    pImpl->mLastEvaluationTime = std::chrono::microseconds{-2208988800000000};
    pImpl->mState = State::Off; 
}

