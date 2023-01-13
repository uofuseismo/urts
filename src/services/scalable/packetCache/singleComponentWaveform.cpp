#include <string>
#include <algorithm>
#include <cmath>
#ifndef NDEBUG
#include <cassert>
#endif
#include "urts/services/scalable/packetCache/singleComponentWaveform.hpp"
#include "urts/services/scalable/packetCache/dataResponse.hpp"
#include "urts/services/scalable/packetCache/wigginsInterpolator.hpp"
#include "urts/broadcasts/internal/dataPacket/dataPacket.hpp"
#include "checkPackets.hpp"

#define NOMINAL_SAMPLING_RATE 100
#define GAP_TOLERANCE_MUSEC 50000

using namespace URTS::Services::Scalable::PacketCache;

namespace
{
/// @brief Converts an input string to an upper-case string with no blanks.
/// @param[in] s  The string to convert.
/// @result The input string without blanks and in all capital letters.
std::string convertString(const std::string &s) 
{
    auto temp = s;
    temp.erase(std::remove(temp.begin(), temp.end(), ' '), temp.end());
    std::transform(temp.begin(), temp.end(), temp.begin(), ::toupper);
    return temp;
}
/// @brief Converts a sampling rate in Hz to a sampling period in microseconds.
/// @param[in] samplingRate  The sampling rate in Hz.
/// @result The corresponding sampling period in microseconds.
[[nodiscard]] [[maybe_unused]]
std::chrono::microseconds
    samplingRateToSamplingPeriodInMicroSeconds(const double samplingRate)
{
    auto idt = static_cast<int64_t> (std::round(1000000./samplingRate));
    return std::chrono::microseconds {idt}; 
}
}


class SingleComponentWaveform::SingleComponentWaveformImpl
{
public:
    SingleComponentWaveformImpl()
    {
        mInterpolator.setTargetSamplingRate(NOMINAL_SAMPLING_RATE);
        mInterpolator.setGapTolerance(
            std::chrono::microseconds {GAP_TOLERANCE_MUSEC});
    }
    WigginsInterpolator mInterpolator;
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    std::chrono::microseconds mNominalSamplingPeriod{
        ::samplingRateToSamplingPeriodInMicroSeconds(NOMINAL_SAMPLING_RATE)};
};

/// Constructor
SingleComponentWaveform::SingleComponentWaveform() :
    SingleComponentWaveform(NOMINAL_SAMPLING_RATE,
                          std::chrono::microseconds {GAP_TOLERANCE_MUSEC})
{
}

/// Constructor with sampling rate
SingleComponentWaveform::SingleComponentWaveform(const double samplingRate) :
    SingleComponentWaveform(samplingRate,
                          std::chrono::microseconds {GAP_TOLERANCE_MUSEC})
{
}

/// Constructor with gap tolerance
SingleComponentWaveform::SingleComponentWaveform(
    const std::chrono::microseconds &gapTolerance) noexcept :
    SingleComponentWaveform(NOMINAL_SAMPLING_RATE, gapTolerance)
{
    setGapTolerance(gapTolerance);
}

/// Constructor with sampling rate
SingleComponentWaveform::SingleComponentWaveform(
    const double samplingRate,
    const std::chrono::microseconds &gapTolerance) :
    pImpl(std::make_unique<SingleComponentWaveformImpl> ())
{
    setNominalSamplingRate(samplingRate);
    setGapTolerance(gapTolerance);
}


/// Copy constructor
SingleComponentWaveform::SingleComponentWaveform(
    const SingleComponentWaveform &waveform)
{
    *this = waveform;
}

/// Move constructor
SingleComponentWaveform::SingleComponentWaveform(
    SingleComponentWaveform &&waveform) noexcept
{
    *this = std::move(waveform);
}

/// Copy assignment
SingleComponentWaveform&
SingleComponentWaveform::operator=(const SingleComponentWaveform &waveform)
{
    if (&waveform == this){return *this;}
    pImpl = std::make_unique<SingleComponentWaveformImpl> (*waveform.pImpl);
    return *this;
}

/// Move assignment
SingleComponentWaveform&
SingleComponentWaveform::operator=(SingleComponentWaveform &&waveform) noexcept
{
    if (&waveform == this){return *this;}
    pImpl = std::move(waveform.pImpl);
    return *this;
}

/// Reset class
void SingleComponentWaveform::clear() noexcept
{
    pImpl = std::make_unique<SingleComponentWaveformImpl> ();
}

/// Release memory associated with signals
void SingleComponentWaveform::clearSignal() noexcept
{
    pImpl->mInterpolator.clearSignal();
}

/// Destructor
SingleComponentWaveform::~SingleComponentWaveform() = default;

/// Network
void SingleComponentWaveform::setNetwork(const std::string &networkIn)
{
    auto s = ::convertString(networkIn);
    if (s.empty()){throw std::invalid_argument("Network is empty");}
    pImpl->mNetwork = s;
}

std::string SingleComponentWaveform::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network not set");}
    return pImpl->mNetwork;
}

bool SingleComponentWaveform::haveNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void SingleComponentWaveform::setStation(const std::string &stationIn)
{
    auto s = ::convertString(stationIn);
    if (s.empty()){throw std::invalid_argument("Station is empty");}
    pImpl->mStation = s;
}

std::string SingleComponentWaveform::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station not set");}
    return pImpl->mStation;
}

bool SingleComponentWaveform::haveStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void SingleComponentWaveform::setChannel(const std::string &channelIn)
{
    auto s = ::convertString(channelIn);
    if (s.empty()){throw std::invalid_argument("Channel is empty");}
    pImpl->mChannel = s;
}

std::string SingleComponentWaveform::getChannel() const
{
    if (!haveChannel()){throw std::runtime_error("Channel not set");}
    return pImpl->mChannel;
}

bool SingleComponentWaveform::haveChannel() const noexcept
{
    return !pImpl->mChannel.empty();
}


/// Location code
void SingleComponentWaveform::setLocationCode(const std::string &locationIn)
{
    auto s = ::convertString(locationIn);
    if (s.empty()){throw std::invalid_argument("Location code is empty");}
    pImpl->mLocationCode = s;
}

std::string SingleComponentWaveform::getLocationCode() const
{
    if (!haveLocationCode()){throw std::runtime_error("Location code not set");}
    return pImpl->mLocationCode;
}

bool SingleComponentWaveform::haveLocationCode() const noexcept
{
    return !pImpl->mLocationCode.empty();
}

/// Sampling rate
void SingleComponentWaveform::setNominalSamplingRate(const double samplingRate)
{
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("Sampling rate must be positive");
    }
    pImpl->mNominalSamplingPeriod
        = ::samplingRateToSamplingPeriodInMicroSeconds(samplingRate);
    pImpl->mInterpolator.setTargetSamplingRate(samplingRate);
}

double SingleComponentWaveform::getNominalSamplingRate() const noexcept
{
    return pImpl->mInterpolator.getTargetSamplingRate();
}

std::chrono::microseconds
SingleComponentWaveform::getNominalSamplingPeriod() const noexcept
{
    return pImpl->mNominalSamplingPeriod;
}

/// Gap tolerance
void SingleComponentWaveform::setGapTolerance(
    const std::chrono::microseconds &gapTolerance) noexcept
{
    pImpl->mInterpolator.setGapTolerance(gapTolerance);
}

std::chrono::microseconds
SingleComponentWaveform::getGapTolerance() const noexcept
{
    return pImpl->mInterpolator.getGapTolerance();
}

/// Set packets
void SingleComponentWaveform::set(const DataResponse &response,
                                  const std::chrono::microseconds &startTime,
                                  const std::chrono::microseconds &endTime)
{
    pImpl->mInterpolator.clearSignal();
    if (endTime < startTime)
    {
        throw std::invalid_argument("Start time cannot exceed end time");
    }
    int nPackets = response.getNumberOfPackets();
    if (nPackets < 1){return;} // Nothing to do
    const auto &packetsReference = response.getPacketsReference();
#ifndef NDEBUG
    assert(static_cast<int> (packetsReference.size()) == nPackets);
#endif
    /// Preliminary checks on packets
    auto [t0Packets, t1Packets]
        = ::checkPacketsAndGetStartEndTime(packetsReference);
    auto network = packetsReference.at(0).getNetwork();
    auto station = packetsReference.at(0).getStation();
    auto channel = packetsReference.at(0).getChannel();
    auto locationCode = packetsReference.at(0).getLocationCode();
    /*
    auto t0Packets = packetsReference[0].getStartTime();
    auto t1Packets = packetsReference[0].getEndTime();
    auto network = packetsReference[0].getNetwork();
    auto station = packetsReference[0].getStation();
    auto channel = packetsReference[0].getChannel();
    auto locationCode = packetsReference[0].getLocationCode(); 
    if (!packetsReference[0].haveSamplingRate())
    {
        throw std::invalid_argument("Sampling rate not set for first packet");
    }
    for (int i = 1; i < nPackets; ++i)
    {
        t0Packets = std::min(t0Packets, packetsReference[i].getStartTime());
        t1Packets = std::max(t1Packets, packetsReference[i].getEndTime());
        if (!packetsReference[i].haveSamplingRate())
        {
            throw std::invalid_argument("Sampling rate not set for packet "
                                      + std::to_string(i + 1));
        }
        if (network != packetsReference[i].getNetwork())
        {
            throw std::invalid_argument("Inconsistent network codes");
        }
        if (station != packetsReference[i].getStation())
        {
            throw std::invalid_argument("Inconsistent station names");
        }
        if (channel != packetsReference[i].getChannel())
        {
            throw std::invalid_argument("Inconsistent channel codes");
        }
        if (locationCode != packetsReference[i].getLocationCode())
        {
            throw std::invalid_argument("Inconsistent location codes");
        }
    }
    */
    if (startTime > t1Packets)
    {
        auto errorMessage = "Desired interpolation start time ("
                          + std::to_string(startTime.count())
                          + ") exceeds last sample in data query ("
                          + std::to_string(t1Packets.count())
                          + ")"; 
        throw std::invalid_argument(errorMessage);
    }
    if (endTime < t0Packets)
    {
        auto errorMessage = "Desired interpolation end time ("
                          + std::to_string(endTime.count())
                          + ") is than first sample in data query ("
                          + std::to_string(t0Packets.count())
                          + ")";
    }
    /// Set the appropriate start/end interpolation time
    auto t0Interpolate = std::max(startTime, t0Packets);
    auto t1Interpolate = std::min(endTime,   t1Packets);
    /// Set my data
    setNetwork(network);
    setStation(station);
    setChannel(channel);
    setLocationCode(locationCode);
    /// Interpolate
    pImpl->mInterpolator.interpolate(packetsReference,
                                     t0Interpolate, t1Interpolate);
}

/// Number of samples
int SingleComponentWaveform::getNumberOfSamples() const noexcept
{
    return pImpl->mInterpolator.getNumberOfSamples();
}

/// Get the start time
std::chrono::microseconds SingleComponentWaveform::getStartTime() const noexcept
{
    return pImpl->mInterpolator.getStartTime();
}

/// Get the end time
std::chrono::microseconds SingleComponentWaveform::getEndTime() const noexcept
{
    return pImpl->mInterpolator.getEndTime();
}
