#include <cmath>
#include "urts/services/scalable/packetCache/threeComponentWaveform.hpp"
#include "urts/services/scalable/packetCache/singleComponentWaveform.hpp"

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
