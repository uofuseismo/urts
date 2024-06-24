#include <chrono>
#include <vector>
#include <cstdbool>
#include <algorithm>
#include "urts/services/scalable/packetCache/wigginsInterpolator.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "wiggins.hpp"

using namespace URTS::Services::Scalable::PacketCache;
namespace UDP = URTS::Broadcasts::Internal::DataPacket;

namespace
{
std::vector<std::pair<int64_t, int64_t>> createGapStartEnd(
    const int64_t gapTolerance,
    const std::vector<std::pair<int64_t, int64_t>> &packetStartEndTimes,
    const bool lSorted)
{
    auto nPackets = static_cast<int> (packetStartEndTimes.size());
    std::vector<std::pair<int64_t,int64_t>> gapStartEnd;
    gapStartEnd.reserve(nPackets);
    if (lSorted)
    {
        for (int i = 1; i < nPackets; ++i)
        {
            auto t0 = packetStartEndTimes[i - 1].second;
            auto t1 = packetStartEndTimes[i].first;
#ifndef NDEBUG
            assert(t1 - t0 >= 0);
#else
            if (t1 < t0)
            {
                throw std::runtime_error(
                    "::createGapStartEnd algorhmic failure t1 < t0; "
                  + std::to_string(t1.count()) + " "
                  + std::to_string(t0.count()));
            }
#endif
            if (t1 - t0 > gapTolerance)
            {
                gapStartEnd.push_back( std::pair{t0, t1} );
            }
        }
    }
    else
    {
        // For each packet...
        for (int i = 0; i < nPackets; ++i)
        {
            // Hunt for the adjacent packet
            auto t0 = packetStartEndTimes[i].second; 
            //if (i > 1){t0 = packetStartEndTimes[i - 1].second;}
            std::pair<int64_t, int64_t> nearestPacket;
            nearestPacket.first = t0;
            auto dMin = std::numeric_limits<int64_t>::max(); 
            for (int j = 0; j < nPackets; ++j)
            {
                if (i == j){continue;}
                auto t1 = packetStartEndTimes[j].first; 
                if (t1 < t0){continue;}
                if (t1 - t0 < dMin)
                {
                    dMin = t1 - t0;
                    nearestPacket.second = t1;
                }
            }
            // Check if the adjacent packet exceeds the gap tolerance
            if (nearestPacket.second - nearestPacket.first > gapTolerance)
            {
                gapStartEnd.push_back(nearestPacket);
            }
        }
    }
    return gapStartEnd;
}

void fillGapPointer(const size_t nNewSamples,
                    const int64_t targetSamplingPeriodMicroSeconds,
                    const std::vector<int64_t> &timesToEvaluate,
                    const std::vector<std::pair<int64_t, int64_t>> &gapStartEnd,
                    std::vector<int8_t> *gapIndicator,
                    bool *haveGaps)
{
    *haveGaps = false;
    // Less than this difference we're basically collocating
    auto targetDt2
         = static_cast<int64_t> (0.5*targetSamplingPeriodMicroSeconds);
    // Fill the default result
    gapIndicator->resize(nNewSamples, 0);
    if (gapStartEnd.empty()){return;} //No gaps
    // Fill gaps...
    const auto *__restrict__ times = timesToEvaluate.data();
    auto *__restrict__ gapPointer = gapIndicator->data();
    for (const auto &gap : gapStartEnd)
    {
        auto t0 = gap.first;
        auto t1 = gap.second; 
        // Locate the times between these samples
        auto startPointer = std::lower_bound(timesToEvaluate.begin(),
                                             timesToEvaluate.end(), t0);
        size_t iStart = std::distance(timesToEvaluate.begin(), startPointer);
        while (iStart > 0)
        {
            if (times[iStart] < t0){break;}
            iStart = iStart - 1;
        } 
        auto endPointer = std::lower_bound(timesToEvaluate.begin(),
                                           timesToEvaluate.end(), t1);
        size_t iEnd = std::distance(timesToEvaluate.begin(), endPointer);
        iEnd = std::min(iEnd, nNewSamples);
        while (iEnd < nNewSamples)
        {
            if (times[iEnd] > t1){break;}
            iEnd = iEnd + 1;
        }
        for (auto it = iStart; it < iEnd; ++it)
        {
            // The gap tolerance has been exceeded since this defined as gap.
            // As long as this isn't the input datapoint then this is in the
            // gap.
            if (t0 + targetDt2 < times[it] && times[it] < t1 - targetDt2)
            {
                 gapPointer[it] = 1;
                 *haveGaps = true;
            }
        }
    }
}
                
}

class WigginsInterpolator::WigginsInterpolatorImpl
{
public:
    void clearSignal() noexcept
    {
        mSignal.clear();
        mGapIndicator.clear();
        mStartTime = std::chrono::microseconds{0};
        mEndTime = std::chrono::microseconds{0};
        mHaveGaps = false;
    }
    std::vector<double> mSignal;
    std::vector<int8_t> mGapIndicator;
    double mTargetSamplingRate{100};
    std::chrono::microseconds mGapTolerance{50000};
    std::chrono::microseconds mStartTime{0};
    std::chrono::microseconds mEndTime{0};
    bool mHaveGaps{false};
};

/// C'tor
WigginsInterpolator::WigginsInterpolator() :
    pImpl(std::make_unique<WigginsInterpolatorImpl> ())
{
}

/// Copy c'tor
WigginsInterpolator::WigginsInterpolator(const WigginsInterpolator &wiggins)
{
    *this = wiggins;
}

/// Move c'tor
WigginsInterpolator::WigginsInterpolator(WigginsInterpolator &&wiggins) noexcept
{
    *this = std::move(wiggins);
}

/// Copy assignment
WigginsInterpolator&
WigginsInterpolator::operator=(const WigginsInterpolator &wiggins)
{
    if (&wiggins == this){return *this;}
    pImpl = std::make_unique<WigginsInterpolatorImpl> (*wiggins.pImpl);
    return *this;
}

/// Move assignment
WigginsInterpolator&
WigginsInterpolator::operator=(WigginsInterpolator &&wiggins) noexcept
{
    if (&wiggins == this){return *this;}
    pImpl = std::move(wiggins.pImpl);
    return *this;
}

/// Destructor
WigginsInterpolator::~WigginsInterpolator() = default;

/// Target sampling rate
void WigginsInterpolator::setTargetSamplingRate(const double samplingRate)
{
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("Sampling rate must be positive");
    }
    pImpl->mTargetSamplingRate = samplingRate;
}

double WigginsInterpolator::getTargetSamplingRate() const noexcept
{
    return pImpl->mTargetSamplingRate;
}

/// Gap tolerance
void WigginsInterpolator::setGapTolerance(
    const std::chrono::microseconds &tolerance) noexcept
{
    pImpl->mGapTolerance = tolerance;
} 

std::chrono::microseconds WigginsInterpolator::getGapTolerance() const noexcept
{
    return pImpl->mGapTolerance;
}

/// Number of samples in interpolated signal
int WigginsInterpolator::getNumberOfSamples() const noexcept
{
    return static_cast<int> (pImpl->mSignal.size());
}

/// Clear signal
void WigginsInterpolator::clearSignal() noexcept
{
    pImpl->clearSignal();
}

/// Reset class
void WigginsInterpolator::clear() noexcept
{
    pImpl = std::make_unique<WigginsInterpolatorImpl> ();
}

/// Interpolate
void WigginsInterpolator::interpolate(
    const std::vector<UDP::DataPacket> &packets,
    const std::chrono::microseconds &startTime,
    const std::chrono::microseconds &endTime)
{
    interpolate(packets.size(), packets.data(), startTime, endTime);
}

void WigginsInterpolator::interpolate(
    const int nPackets, const UDP::DataPacket packets[],
    const std::chrono::microseconds &desiredInterpolationStartTime,
    const std::chrono::microseconds &desiredInterpolationEndTime)
{
    clearSignal();
    if (nPackets < 1){return;} // Nothing to do
    if (packets == nullptr){throw std::invalid_argument("packets is NULL");}
    if (desiredInterpolationStartTime > desiredInterpolationEndTime)
    {
        throw std::invalid_argument(
            "Desired interpolation start time exceeds end time");
    }
    // Do all packets have sampling rates
    for (int ip = 0; ip < nPackets; ++ip)
    {
        if (!packets[ip].haveSamplingRate())
        {
            throw std::invalid_argument(
               "Sampling rate must be set for all packets");
        }
    }
    // Is there data?
    std::vector<int> packetSamplePtr(nPackets + 1);
    int nSamples = 0;
    packetSamplePtr[0] = 0;
    for (int iPacket = 0; iPacket < nPackets; ++iPacket)
    {
        nSamples = nSamples + packets[iPacket].getNumberOfSamples();
        packetSamplePtr[iPacket + 1] = nSamples;
    }
    if (nSamples < 1)
    {
        throw std::invalid_argument("No samples in packets");
    }
    if (nSamples < 2)
    {
        throw std::invalid_argument("At least two samples required");
    }
    // Are the packets in order?
    auto isSorted
        = std::is_sorted(packets, packets + nPackets,
                         [](const UDP::DataPacket &lhs,
                            const UDP::DataPacket &rhs)
                         {
                            return lhs.getEndTime() < rhs.getStartTime();
                         });
    // Create abscissas and values at abscissas
    std::vector<double> data(nSamples);
    std::vector<int64_t> times(nSamples);
    std::vector<std::pair<int64_t, int64_t>> startEndTimes;
    startEndTimes.reserve(nPackets);
    for (int iPacket = 0; iPacket < nPackets; ++iPacket)
    {   
        const auto *__restrict__ dataPtr = packets[iPacket].getDataPointer();
        auto i0 = packetSamplePtr[iPacket];
        auto i1 = packetSamplePtr[iPacket + 1]; // Exclusive
        auto nSamplesInPacket = i1 - i0; 
        if (nSamplesInPacket > 0)
        {
            std::copy(dataPtr, dataPtr + nSamplesInPacket, &data[i0]);
            auto samplingRate = packets[iPacket].getSamplingRate();
            auto samplingPeriodMicroSeconds
                = static_cast<double> (1000000./samplingRate);
            auto t0 = packets[iPacket].getStartTime().count();
            auto t1 = packets[iPacket].getEndTime().count();
            startEndTimes.push_back(std::pair{t0, t1});
            auto *__restrict__ timesPtr = &times[i0];
            for (int i = 0; i < nSamplesInPacket; ++i)
            {
                timesPtr[i] = t0 + i*samplingPeriodMicroSeconds;
            }
        }
    }
    // Create the interpolation times
    int64_t time0, time1;
    if (isSorted)
    {
        time0 = times.front();
        time1 = times.back();
    }
    else
    {
        auto tMinMax = std::minmax_element(times.begin(), times.end());
        time0 = *tMinMax.first;
        time1 = *tMinMax.second;
    }
    if (desiredInterpolationStartTime.count() > time0 &&
        desiredInterpolationStartTime.count() < time1)
    {

    }
    // Prevent user from trying to start interpolation after signal ends
    if (desiredInterpolationStartTime.count() < time1)
    {
        time0 = std::max(time0, desiredInterpolationStartTime.count());
    }
    // Prevent user from trying to end interpolation before signal starts
    if (desiredInterpolationEndTime.count() >= time0)
    {
        time1 = std::min(time1, desiredInterpolationEndTime.count()); 
    }
    auto targetSamplingRate = pImpl->mTargetSamplingRate;
    auto targetSamplingPeriodMicroSeconds
        = static_cast<int64_t> (std::round(1000000./targetSamplingRate));
    auto spaceEstimate
        = static_cast<int>
          (std::round((time1 - time0)
                     /static_cast<double> (targetSamplingPeriodMicroSeconds)));
    std::vector<int64_t> timesToEvaluate;
    // Now figure out the actual end time.  The idea is after the space estimate
    // to start the next loop definitely before the end of the desired 
    // interpolation time.  When we exceed that desired time we call it a day
    // and break.  Barring weird overflow, we won't spend much time in this
    // loop.
    int nNewSamples = spaceEstimate - 1;
    while (true)
    {
        auto interpolationTime
            = time0 + nNewSamples*targetSamplingPeriodMicroSeconds;
        if (interpolationTime > time1){break;}
        nNewSamples = nNewSamples + 1;
    }
    nNewSamples = std::max(nNewSamples, 1); // Interpolate at `start point'
    // Fill the interpolation times 
    timesToEvaluate.resize(nNewSamples);
    auto *__restrict__ timesToEvaluatePtr = timesToEvaluate.data();
    for (int i = 0; i < nNewSamples; ++i)
    {
        timesToEvaluatePtr[i] = time0 + i*targetSamplingPeriodMicroSeconds;
    }
#ifndef NDEBUG
    assert(timesToEvaluate.front() >= time0);
    assert(timesToEvaluate.back() <= time1);
    assert(timesToEvaluate.front() <= timesToEvaluate.back());
#endif
    /* 
    // A very crude way to do this
    timesToEvaluate.reserve(std::max(1, spaceEstimate));
    int i = 0;
    while (true)
    {
       auto interpolationTime = time0 + i*targetSamplingPeriodMicroSeconds;
       if (interpolationTime > time1){break;}
       timesToEvaluate.push_back(interpolationTime);
       i = i + 1;
    }
    */
    // Package into the result
    auto checkSorting = !isSorted;
    pImpl->mSignal = ::weightedAverageSlopes(times, data, timesToEvaluate,
                                             checkSorting);
    if (!pImpl->mSignal.empty())
    {
        auto nNewSamples = pImpl->mSignal.size(); 
        pImpl->mStartTime = std::chrono::microseconds{time0};
        pImpl->mEndTime   = std::chrono::microseconds{timesToEvaluate.back()}; 
        // Perform gap check
        auto gapStartEnd = ::createGapStartEnd(pImpl->mGapTolerance.count(),
                                               startEndTimes,
                                               isSorted);
        ::fillGapPointer(nNewSamples,
                         targetSamplingPeriodMicroSeconds,
                         timesToEvaluate,
                         gapStartEnd,
                         &pImpl->mGapIndicator,
                         &pImpl->mHaveGaps);
    }
}

// Get signal
std::vector<double> WigginsInterpolator::getSignal() const noexcept
{
    return pImpl->mSignal;
}

const double *WigginsInterpolator::getSignalPointer() const noexcept
{
    return pImpl->mSignal.data();
}

const std::vector<double> 
&WigginsInterpolator::getSignalReference() const noexcept
{
    return pImpl->mSignal;
}

// Get gap indicator
std::vector<int8_t> WigginsInterpolator::getGapIndicator() const noexcept
{
    return pImpl->mGapIndicator;
}

const int8_t *WigginsInterpolator::getGapIndicatorPointer() const noexcept
{
    return pImpl->mGapIndicator.data();
}

const std::vector<int8_t>
&WigginsInterpolator::getGapIndicatorReference() const noexcept
{
    return pImpl->mGapIndicator;
}

// Are there gaps?
bool WigginsInterpolator::haveGaps() const noexcept
{
    return pImpl->mHaveGaps;
}

/// Start time
std::chrono::microseconds WigginsInterpolator::getStartTime() const noexcept
{
    return pImpl->mStartTime;
}

/// End time
std::chrono::microseconds WigginsInterpolator::getEndTime() const noexcept
{
    return pImpl->mEndTime;
}
