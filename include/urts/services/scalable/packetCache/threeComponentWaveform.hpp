#ifndef URTS_SERVICES_SCALABLE_PACKET_CACHE_THREE_COMPONENT_WAVEFORM_HPP
#define URTS_SERVICES_SCALABLE_PACKET_CACHE_THREE_COMPONENT_WAVEFORM_HPP
#include <memory>
#include <string>
#include <vector>
#include <chrono>
namespace URTS
{
 namespace Broadcasts::Internal::DataPacket
 {
  class DataPacket;
 }
 namespace Services::Scalable::PacketCache
 {
  class DataResponse;
  class SingleComponentWaveform;
 }
}
namespace URTS::Services::Scalable::PacketCache
{
/// @class ThreeComponentWaveform "singleComponentWaveform.hpp" "urts/services/scalable/packetCache/singleComponentWaveform.hpp"
/// @brief This class converts a sequence of packets for a three-component
///        station to three continuous waveforms that have the same start time,
///        length, and sampling rate.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class ThreeComponentWaveform
{
public:
    /// @name Constructors
    /// @{

    /// @brief Default constructor.
    ThreeComponentWaveform();
    /// @brief Constructor with a given nominal sampling rate.
    /// @param[in] samplingRate  The nominal sampling rate in Hz.
    explicit ThreeComponentWaveform(double samplingRate);
    /// @brief Constructor with a given gap tolerance.
    /// @param[in] gapTolerance  The gap tolerance in microseconds.
    explicit ThreeComponentWaveform(const std::chrono::microseconds &gapTolerance) noexcept;
    /// @brief Constructor with a given gap tolerance and sampling rate.
    /// @param[in] samplingRate   The nominal sampling rate in Hz.
    /// @param[in] gapTolerance   The gap tolerance in microseconds.
    ThreeComponentWaveform(double samplingRate,
                           const std::chrono::microseconds &gapTolerance);
    /// @brief Copy constructor.
    /// @param[in] waveform  The waveform from which to initialize this class.
    ThreeComponentWaveform(const ThreeComponentWaveform &waveform);
    /// @brief Move constructor.
    /// @param[in,out] waveform  The waveform from which to initialize this
    ///                          class.  On exit, waveform's behavior is 
    ///                          undefined.
    ThreeComponentWaveform(ThreeComponentWaveform &&waveform) noexcept;
    /// @}
    
    /// @name Properties
    /// @{

    /// @brief Sets the nominal sampling rate in Hz.  This is associated with
    ///        the station's metadata.  This may (slightly) different than a
    ///        packet's sampling rate.
    /// @param[in] samplingRate  The sampling rate in Hz.
    void setNominalSamplingRate(double samplingRate);
    /// @result The nominal sampling rate in Hz.
    /// @note By default this is 100 Hz.
    [[nodiscard]] double getNominalSamplingRate() const noexcept;
    /// @result The nominal sampling period in microseconds
    [[nodiscard]] std::chrono::microseconds getNominalSamplingPeriod() const noexcept;

    /// @brief Sets the gap tolerance between packets.
    /// @param[in] gapTolerance  If the time between the end of a packet and the
    ///                          start of the subsequent packet exceeds this
    ///                          time then the samples interpolated between the
    ///                          packets will be denoted as extrapolated data
    ///                          gaps.
    /// @note Zero or negative effectively disables this.  In which case any
    ///       sample interpolated between a packet is being extrapolated in a
    ///       gap.
    void setGapTolerance(const std::chrono::microseconds &gapTolerance) noexcept;
    /// @result The gap tolerance.  By default this is 50000 microseconds 
    ///         (0.05 seconds).  This is 5 samples at the nominal sampling
    ///         rate of 100 Hz.
    [[nodiscard]] std::chrono::microseconds getGapTolerance() const noexcept;
    /// @}

    /// @name Packet Setters
    /// @{

    /// @brief Sets the data response and interpolates to a continuous signal.
    /// @param[in] verticalComponent  The data response from the packet cache
    ///                               for the vertical component.
    /// @param[in] northComponent     The data response from the packet cache
    ///                               for the north (or 1) component.
    /// @param[in] eastComponent      The data response from the packet cache
    ///                               for the east (or 2) component.
    /// @note This will interpolate the packets to the nominal sampling rate
    ///       and set the corresponding channel naming information.
    /// @throws std::invalid_argument if the naming information cannot be
    ///         determined, there are no packets, or the packets correspond
    ///         to different channels.
    /// @throws std::runtime_error if an error occurs during interpolation.
    void set(const DataResponse &verticalComponent,
             const DataResponse &northComponent,
             const DataResponse &eastComponent);
    /// @}

    /// @name Waveform Getters
    /// @{

    /// @result The number of samples in the three-component signal.
    [[nodiscard]] int getNumberOfSamples() const noexcept;
    /// @result The start time (UTC) of the signal since the epoch.
    [[nodiscard]] std::chrono::microseconds getStartTime() const noexcept;
    /// @result The end time (UTC) of the signal since the epoch.
    [[nodiscard]] std::chrono::microseconds getEndTime() const noexcept;

    /// @result The interpolated signal on the vertical channel.
    [[nodiscard]] std::vector<double> getVerticalSignal() const noexcept;
    /// @result A reference to the interpolated signal on the vertical channel.
    /// @note This exists for performance reasons.  When possible use
    ///       \c getVerticalSignal().
    [[nodiscard]] const std::vector<double> &getVerticalSignalReference() const noexcept;

    /// @result The interpolated signal on the north channel.
    [[nodiscard]] std::vector<double> getNorthSignal() const noexcept;
    /// @result A reference to the interpolated signal on the north channel.
    /// @note This exists for performance reasons.  When possible use
    ///       \c getNorthSignal().
    [[nodiscard]] const std::vector<double> &getNorthSignalReference() const noexcept;

    /// @result The interpolated signal on the east channel.
    [[nodiscard]] std::vector<double> getEastSignal() const noexcept;
    /// @result A reference to the interpolated signal on the east channel.
    /// @note This exists for performance reasons.  When possible use
    ///       \c getEastSignal().
    [[nodiscard]] const std::vector<double> &getEastSignalReference() const noexcept;

    /// @result An array of true/false that indicates whether or not the 
    ///         interpolated signal fell between packet endpoints.   
    [[nodiscard]] std::vector<int8_t> getGapIndicator() const noexcept;
    /// @result A pointer to a signal that indicates whether or not the
    ///         sample in the interpolated signal was within a packet (false) or
    ///         between packets (true) - i.e., extrapolated.
    /// @note This exists for performance reasons.  When possibleu se
    ///       \c getGapIndicator().
    [[nodiscard]] const std::vector<int8_t> &getGapIndicatorReference() const noexcept;
    /// @result True indicates that there are non-zeros in the gapIndicator.
    [[nodiscard]] bool haveGaps() const noexcept;
    /// @}

    /// @name Channel Naming Information
    /// @{

    /// @brief Sets the network code.
    /// @param[in] network  The network code - e.g., UU.
    /// @throws std::invalid_argument if the network code is empty.
    void setNetwork(const std::string &network);
    /// @result The network code.
    /// @throws std::runtime_error if \c haveNetwork() is false. 
    [[nodiscard]] std::string getNetwork() const;
    /// @result True indicates the network code was set.
    [[nodiscard]] bool haveNetwork() const noexcept;

    /// @brief Sets the station name.
    /// @param[in] station  The station name - e.g., FORK.
    /// @throws std::invalid_argument if the station name is empty.
    void setStation(const std::string &station);
    /// @result The station name.
    /// @throws std::runtime_error if \c haveStation() is false. 
    [[nodiscard]] std::string getStation() const;
    /// @result True indicates the station name was set.
    [[nodiscard]] bool haveStation() const noexcept;

    /// @brief Sets the vertical channel code.
    /// @param[in] verticalChannel  The vertical channel code - e.g., HHZ.
    /// @throws std::invalid_argument if the channel code is empty.
    void setVerticalChannel(const std::string &verticalChannel);
    /// @result The vertical channel code.
    /// @throws std::runtime_error if \c haveVerticalChannel() is false. 
    [[nodiscard]] std::string getVerticalChannel() const;
    /// @result True indicates the vertical channel code was set.
    [[nodiscard]] bool haveVerticalChannel() const noexcept;

    /// @brief Sets the north (or 1) channel code.
    /// @param[in] northChannel  The north channel code - e.g., HHN or HH1.
    /// @throws std::invalid_argument if the channel code is empty.
    void setNorthChannel(const std::string &northChannel);
    /// @result The north channel code.
    /// @throws std::runtime_error if \c haveNorthChannel() is false. 
    [[nodiscard]] std::string getNorthChannel() const;
    /// @result True indicates the north channel code was set.
    [[nodiscard]] bool haveNorthChannel() const noexcept;

    /// @brief Sets the east (or 2) channel code.
    /// @param[in] eastChannel  The north channel code - e.g., HHE or HH2.
    /// @throws std::invalid_argument if the channel code is empty.
    void setEastChannel(const std::string &eastChannel);
    /// @result The east channel code.
    /// @throws std::runtime_error if \c haveEastChannel() is false. 
    [[nodiscard]] std::string getEastChannel() const;
    /// @result True indicates the east channel code was set.
    [[nodiscard]] bool haveEastChannel() const noexcept;

    /// @brief Sets the location code.
    /// @param[in] locationCode  The location code - e.g., 01.
    void setLocationCode(const std::string &locationCode);
    /// @result The location code.
    /// @throw std::invalid_argument if \c haveLocationCode() is false.
    [[nodiscard]] std::string getLocationCode() const;
    /// @result True indicates the location code was set.
    [[nodiscard]] bool haveLocationCode() const noexcept;
    /// @}

    /// @name Operators
    /// @{

    /// @brief Copy assignment operator.
    /// @param[in] waveform  The waveform to copy to this.
    /// @result A copy of the input waveform.
    ThreeComponentWaveform& operator=(const ThreeComponentWaveform &waveform);
    /// @brief Move assignment operator.
    /// @param[in,out] waveform  The waveform whose memory will be moved to
    ///                          this.  On exit, waveform's behavior is
    ///                          undefined.
    /// @result The memory from waveform moved to this.
    ThreeComponentWaveform& operator=(ThreeComponentWaveform &&waveform) noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Releases memory associated with signals.
    void clearSignal() noexcept;
    /// @brief Releases memory and restores the defaults.
    void clear() noexcept;
    /// @brief Destructor.
    ~ThreeComponentWaveform();
    /// @}
private:
    class ThreeComponentWaveformImpl;
    std::unique_ptr<ThreeComponentWaveformImpl> pImpl;
};
}
#endif
