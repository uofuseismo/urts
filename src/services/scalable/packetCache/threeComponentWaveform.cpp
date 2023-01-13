#include <chrono>
#include <cmath>
#ifndef NDEBUG
#include <cassert>
#endif
#include "urts/services/scalable/packetCache/threeComponentWaveform.hpp"
#include "urts/services/scalable/packetCache/singleComponentWaveform.hpp"
#include "urts/services/scalable/packetCache/dataResponse.hpp"
#include "checkPackets.hpp"

using namespace URTS::Services::Scalable::PacketCache;

class ThreeComponentWaveform::ThreeComponentWaveformImpl
{
public:
    SingleComponentWaveform mVerticalComponent; // Vertical component
    SingleComponentWaveform mNorthComponent; // North or 1 component
    SingleComponentWaveform mEastComponent; // East or 2 component
};

/// Constructor
ThreeComponentWaveform::ThreeComponentWaveform() :
    pImpl(std::make_unique<ThreeComponentWaveformImpl> ())
{
}

/// Copy constructor
ThreeComponentWaveform::ThreeComponentWaveform(
    const ThreeComponentWaveform &waveforms)
{
    *this = waveforms;
}

/// Copy constructor
ThreeComponentWaveform::ThreeComponentWaveform(
    ThreeComponentWaveform &&waveforms) noexcept
{
    *this = std::move(waveforms);
}

/// Release memory with signals
void ThreeComponentWaveform::clearSignal() noexcept
{
    pImpl->mVerticalComponent.clearSignal();
    pImpl->mNorthComponent.clearSignal();
    pImpl->mEastComponent.clearSignal();
}

/// Reset class
void ThreeComponentWaveform::clear() noexcept
{
    pImpl = std::make_unique<ThreeComponentWaveformImpl> ();
}

/// Destructor
ThreeComponentWaveform::~ThreeComponentWaveform() = default;

/// Copy assignment
ThreeComponentWaveform&
ThreeComponentWaveform::operator=(const ThreeComponentWaveform &waveforms)
{
    if (&waveforms == this){return *this;}
    pImpl = std::make_unique<ThreeComponentWaveformImpl> (*waveforms.pImpl); 
    return *this;
}

/// Move assignment
ThreeComponentWaveform&
ThreeComponentWaveform::operator=(ThreeComponentWaveform &&waveforms) noexcept
{
    if (&waveforms == this){return *this;}
    pImpl = std::move(waveforms.pImpl); 
    return *this;
}

/// Sampling rate
void ThreeComponentWaveform::setNominalSamplingRate(const double samplingRate)
{
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("Sampling rate must be positive");
    }
    pImpl->mVerticalComponent.setNominalSamplingRate(samplingRate);
    pImpl->mNorthComponent.setNominalSamplingRate(samplingRate);
    pImpl->mEastComponent.setNominalSamplingRate(samplingRate);
}

double ThreeComponentWaveform::getNominalSamplingRate() const noexcept
{
#ifndef NDEBUG
    assert(std::abs(pImpl->mVerticalComponent.getNominalSamplingRate() -
                    pImpl->mNorthComponent.getNominalSamplingRate()) < 1.e-10);
    assert(std::abs(pImpl->mVerticalComponent.getNominalSamplingRate() -
                    pImpl->mEastComponent.getNominalSamplingRate()) < 1.e-10);
#endif
    return pImpl->mVerticalComponent.getNominalSamplingRate();
}

std::chrono::microseconds
ThreeComponentWaveform::getNominalSamplingPeriod() const noexcept
{
#ifndef NDEBUG
    assert(pImpl->mVerticalComponent.getNominalSamplingPeriod() == 
           pImpl->mNorthComponent.getNominalSamplingPeriod());
    assert(pImpl->mVerticalComponent.getNominalSamplingPeriod() ==       
           pImpl->mEastComponent.getNominalSamplingPeriod());
#endif
    return pImpl->mVerticalComponent.getNominalSamplingPeriod();
}

/// Gap tolerance
void ThreeComponentWaveform::setGapTolerance(
    const std::chrono::microseconds &gapTolerance) noexcept
{
    pImpl->mVerticalComponent.setGapTolerance(gapTolerance);
    pImpl->mNorthComponent.setGapTolerance(gapTolerance);
    pImpl->mEastComponent.setGapTolerance(gapTolerance);
}

std::chrono::microseconds
ThreeComponentWaveform::getGapTolerance() const noexcept
{
#ifndef NDEBUG
    assert(pImpl->mVerticalComponent.getGapTolerance() ==
           pImpl->mNorthComponent.getGapTolerance());
    assert(pImpl->mVerticalComponent.getGapTolerance() ==
           pImpl->mEastComponent.getGapTolerance());
#endif
    return pImpl->mVerticalComponent.getGapTolerance();
}

/// Network code
void ThreeComponentWaveform::setNetwork(const std::string &network)
{
    pImpl->mVerticalComponent.setNetwork(network);
    pImpl->mNorthComponent.setNetwork(network);
    pImpl->mEastComponent.setNetwork(network);
}

std::string ThreeComponentWaveform::getNetwork() const
{
    if (!haveNetwork()){throw std::runtime_error("Network code not set");}
#ifndef NDEBUG
    assert(pImpl->mVerticalComponent.getNetwork() ==
           pImpl->mNorthComponent.getNetwork());
    assert(pImpl->mVerticalComponent.getNetwork() ==
           pImpl->mEastComponent.getNetwork());
#endif
    return pImpl->mVerticalComponent.getNetwork();
}

bool ThreeComponentWaveform::haveNetwork() const noexcept
{
    return pImpl->mVerticalComponent.haveNetwork() &&
           pImpl->mNorthComponent.haveNetwork() &&
           pImpl->mEastComponent.haveNetwork();
}

/// Station name
void ThreeComponentWaveform::setStation(const std::string &station)
{
    pImpl->mVerticalComponent.setStation(station);
    pImpl->mNorthComponent.setStation(station);
    pImpl->mEastComponent.setStation(station);
}

std::string ThreeComponentWaveform::getStation() const
{
    if (!haveStation()){throw std::runtime_error("Station name not set");}
#ifndef NDEBUG
    assert(pImpl->mVerticalComponent.getStation() ==
           pImpl->mNorthComponent.getStation());
    assert(pImpl->mVerticalComponent.getStation() ==
           pImpl->mEastComponent.getStation());
#endif
    return pImpl->mVerticalComponent.getStation();
}

bool ThreeComponentWaveform::haveStation() const noexcept
{
    return pImpl->mVerticalComponent.haveStation() &&
           pImpl->mNorthComponent.haveStation() &&
           pImpl->mEastComponent.haveStation();
}

/// Vertical channel
void ThreeComponentWaveform::setVerticalChannel(const std::string &channel)
{
    pImpl->mVerticalComponent.setChannel(channel);
}

std::string ThreeComponentWaveform::getVerticalChannel() const
{
    if (!haveVerticalChannel())
    {
        throw std::runtime_error("Vertical channel not set");
    }
    return pImpl->mVerticalComponent.getChannel();
}

bool ThreeComponentWaveform::haveVerticalChannel() const noexcept
{
    return pImpl->mVerticalComponent.haveChannel();
}

/// North channel
void ThreeComponentWaveform::setNorthChannel(const std::string &channel)
{
    pImpl->mNorthComponent.setChannel(channel);
}

std::string ThreeComponentWaveform::getNorthChannel() const
{
    if (!haveNorthChannel())
    {
        throw std::runtime_error("North channel not set");
    }
    return pImpl->mNorthComponent.getChannel();
}

bool ThreeComponentWaveform::haveNorthChannel() const noexcept
{
    return pImpl->mNorthComponent.haveChannel();
}

/// East channel
void ThreeComponentWaveform::setEastChannel(const std::string &channel)
{
    pImpl->mEastComponent.setChannel(channel);
}

std::string ThreeComponentWaveform::getEastChannel() const
{
    if (!haveEastChannel())
    {
        throw std::runtime_error("East channel not set");
    }
    return pImpl->mEastComponent.getChannel();
}

bool ThreeComponentWaveform::haveEastChannel() const noexcept
{
    return pImpl->mEastComponent.haveChannel();
}

/// Location code
void ThreeComponentWaveform::setLocationCode(const std::string &location)
{
    pImpl->mVerticalComponent.setLocationCode(location);
    pImpl->mNorthComponent.setLocationCode(location);
    pImpl->mEastComponent.setLocationCode(location);
}

std::string ThreeComponentWaveform::getLocationCode() const
{
    if (!haveLocationCode()){throw std::runtime_error("Location code not set");}
#ifndef NDEBUG
    assert(pImpl->mVerticalComponent.getLocationCode() ==
           pImpl->mNorthComponent.getLocationCode());
    assert(pImpl->mVerticalComponent.getLocationCode() ==
           pImpl->mEastComponent.getLocationCode());
#endif
    return pImpl->mVerticalComponent.getLocationCode();
}

bool ThreeComponentWaveform::haveLocationCode() const noexcept
{
    return pImpl->mVerticalComponent.haveLocationCode() &&
           pImpl->mNorthComponent.haveLocationCode() &&
           pImpl->mEastComponent.haveLocationCode();
}

/// Number of samples
int ThreeComponentWaveform::getNumberOfSamples() const noexcept
{
#ifndef NDEBUG
    assert(pImpl->mVerticalComponent.getNumberOfSamples() ==
           pImpl->mNorthComponent.getNumberOfSamples());
    assert(pImpl->mVerticalComponent.getNumberOfSamples() ==
           pImpl->mEastComponent.getNumberOfSamples());
#endif
    return pImpl->mVerticalComponent.getNumberOfSamples();
}

/// Get the start time
std::chrono::microseconds ThreeComponentWaveform::getStartTime() const noexcept
{
#ifndef NDEBUG
    assert(pImpl->mVerticalComponent.getStartTime() ==
           pImpl->mNorthComponent.getStartTime());
    assert(pImpl->mVerticalComponent.getStartTime() ==
           pImpl->mEastComponent.getStartTime());
#endif
    return pImpl->mVerticalComponent.getStartTime();
}

/// Get the end time
std::chrono::microseconds ThreeComponentWaveform::getEndTime() const noexcept
{
#ifndef NDEBUG
    assert(pImpl->mVerticalComponent.getEndTime() ==
           pImpl->mNorthComponent.getEndTime());
    assert(pImpl->mVerticalComponent.getEndTime() ==
           pImpl->mEastComponent.getEndTime());
#endif
    return pImpl->mVerticalComponent.getEndTime();
}

/// Process data
void ThreeComponentWaveform::set(const DataResponse &verticalComponent,
                                 const DataResponse &northComponent,
                                 const DataResponse &eastComponent)
{
    clearSignal();
    int nVerticalPackets = verticalComponent.getNumberOfPackets();
    int nNorthPackets    = northComponent.getNumberOfPackets();
    int nEastPackets     = eastComponent.getNumberOfPackets();
    // Nothing to do
    if (nVerticalPackets < 1 || nNorthPackets < 1 || nEastPackets < 1)
    {
        return;
    }
    // Do some checks and figure out the start/end time to interpolate
    const auto &verticalPacketsReference
         = verticalComponent.getPacketsReference();
    const auto &northPacketsReference = northComponent.getPacketsReference();
    const auto &eastPacketsReference = eastComponent.getPacketsReference();
    auto [t0VerticalPackets, t1VerticalPackets]
        = ::checkPacketsAndGetStartEndTime(verticalPacketsReference);
    auto [t0NorthPackets, t1NorthPackets]
        = ::checkPacketsAndGetStartEndTime(northPacketsReference);
    auto [t0EastPackets, t1EastPackets]
        = ::checkPacketsAndGetStartEndTime(eastPacketsReference);
    // Ensure names make sense -> indicative that the nominal sampling rate
    // will be potentially problematic
    auto network = verticalPacketsReference.at(0).getNetwork();
    auto station = verticalPacketsReference.at(0).getStation();
    auto channel = verticalPacketsReference.at(0).getChannel();
    auto locationCode = verticalPacketsReference.at(0).getStation();
    if (network != northPacketsReference.at(0).getNetwork() ||
        network != eastPacketsReference.at(0).getNetwork())
    {
        throw std::invalid_argument("Vertical/north/east network codes differ");
    }
    if (station != northPacketsReference.at(0).getStation() ||
        station != eastPacketsReference.at(0).getStation())
    {
        throw std::invalid_argument("Vertical/north/east station names differ");
    }
    if (locationCode != northPacketsReference.at(0).getLocationCode() ||
        locationCode != eastPacketsReference.at(0).getLocationCode())
    {
        throw std::invalid_argument(
            "Vertical/north/east location codes differ");
    }
    if (channel == northPacketsReference.at(0).getChannel() ||
        channel == eastPacketsReference.at(0).getChannel())
    {
        throw std::invalid_argument(
            "Vertical/north/east channel codes are the same");
    }
    // Now figure out the start/end time of the interpolation
    auto t0 = std::max(t0VerticalPackets,
                       std::max(t0NorthPackets, t0EastPackets));
    auto t1 = std::min(t1VerticalPackets,
                       std::min(t1NorthPackets, t1EastPackets)); 
    // Finally, we can interpolate
    pImpl->mVerticalComponent.set(verticalComponent, t0, t1);
    pImpl->mNorthComponent.set(northComponent, t0, t1);
    pImpl->mEastComponent.set(eastComponent, t0, t1);
#ifndef NDEBUG
    // Algorithmic checks on the names
    assert(haveNetwork());
    assert(haveStation());
    assert(haveVerticalChannel());
    assert(haveNorthChannel());
    assert(haveEastChannel());
    assert(haveLocationCode());
#endif
    // Ensure all the resulting signals are the same size and start at the
    // same time
    if ( (pImpl->mVerticalComponent.getNumberOfSamples() !=
          pImpl->mNorthComponent.getNumberOfSamples()) ||
         (pImpl->mVerticalComponent.getNumberOfSamples() !=
          pImpl->mEastComponent.getNumberOfSamples()) )
    {
        clearSignal();
        throw std::runtime_error(
            "Vertical/north/east component have different interpolated length");
    }
    if ( (pImpl->mVerticalComponent.getStartTime() !=
          pImpl->mNorthComponent.getStartTime()) ||
         (pImpl->mVerticalComponent.getStartTime() !=
          pImpl->mEastComponent.getStartTime()) )
    {
        clearSignal();
        throw std::runtime_error(
            "Vertical/north/east component have different start times");
    }
}
