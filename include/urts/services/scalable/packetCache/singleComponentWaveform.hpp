#ifndef URTS_SERVICES_SCALABLE_PACKET_CACHE_SINGLE_COMPONENT_WAVEFORM_HPP
#define URTS_SERVICES_SCALABLE_PACKET_CACHE_SINGLE_COMPONENT_WAVEFORM_HPP
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
 }
}
namespace URTS::Services::Scalable::PacketCache
{
/// @class SingleComponentWaveform "singleComponentWaveform.hpp" "urts/services/scalable/packetCache/singleComponentWaveform.hpp"
/// @brief This class converts a sequence of packets to a continuous waveform
///        sampled at the channel's nominal sampling rate.  This is accomplished
///        via a (Wiggins) time-domain interpolation method.
/// @copyright Ben Baker (University of Utah) distributed under the MIT license.
class SingleComponentWaveform
{
public:
    /// @name Constructors
    /// @{

    /// @brief Default constructor.
    SingleComponentWaveform();
    /// @brief Constructor with a given nominal sampling rate.
    /// @param[in] samplingRate  The nominal sampling rate in Hz.
    explicit SingleComponentWaveform(double samplingRate);
    /// @brief Constructor with a given gap tolerance.
    /// @param[in] gapTolerance  The gap tolerance in microseconds.
    explicit SingleComponentWaveform(const std::chrono::microseconds &gapTolerance) noexcept;
    /// @brief Constructor with a given gap tolerance and sampling rate.
    /// @param[in] samplingRate   The nominal sampling rate in Hz.
    /// @param[in] gapTolerance   The gap tolerance in microseconds.
    SingleComponentWaveform(double samplingRate,
                            const std::chrono::microseconds &gapTolerance);
    /// @brief Copy constructor.
    /// @param[in] waveform  The waveform from which to initialize this class.
    SingleComponentWaveform(const SingleComponentWaveform &waveform);
    /// @brief Move constructor.
    /// @param[in,out] waveform  The waveform from which to initialize this
    ///                          class.  On exit, waveform's behavior is 
    ///                          undefined.
    SingleComponentWaveform(SingleComponentWaveform &&waveform) noexcept;
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
    /// @param[in] dataResponse  The data response from the packet cache.
    /// @param[in] startTime     The start time (UTC) in microseconds since the
    ///                          epoch of the interpolation.  If this is less
    ///                          than the minimum start time of all the packets
    ///                          then the interpolation will start at the
    ///                          minimum start time of all packets.
    /// @param[in] endTime       The end time (UTC) in microseconds since the
    ///                          epoch of the interpolation.  If this is greater
    ///                          than the maximum end time of all the packets
    ///                          then the interpolation will end at the maximum
    ///                          end time of all packets.
    /// @note This will interpolate the packets to the nominal sampling rate
    ///       and set the corresponding channel naming information.
    /// @throws std::invalid_argument if the naming information cannot be
    ///         determined, there are no packets, or the packets correspond
    ///         to different channels.
    /// @throws std::runtime_error if an error occurs during interpolation.
    void set(const DataResponse &dataResponse,
             const std::chrono::microseconds &startTime = std::chrono::microseconds {-631152000000000},
             const std::chrono::microseconds &endtime   = std::chrono::microseconds {5680281600000000});
    /// @}

    /// @name Waveform Getters
    /// @{

    /// @result The interpolated signal.
    [[nodiscard]] std::vector<double> getSignal() const noexcept;
    /// @result A reference to the interpolated signal.
    /// @note This exists for performance reasons.  When possible use
    ///       \c getSignal().
    [[nodiscard]] const std::vector<double> &getSignalReference() const noexcept;

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

    /// @result The number of samples in the three-component signal.
    [[nodiscard]] int getNumberOfSamples() const noexcept;
    /// @result The start time (UTC) of the signal since the epoch.
    [[nodiscard]] std::chrono::microseconds getStartTime() const noexcept;
    /// @result The end time (UTC) of the signal since the epoch.
    [[nodiscard]] std::chrono::microseconds getEndTime() const noexcept;
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

    /// @brief Sets the channel code.
    /// @param[in] channel  The channel code - e.g., HHZ.
    /// @throws std::invalid_argument if the channel code is empty.
    void setChannel(const std::string &channel);
    /// @result The channel code.
    /// @throws std::runtime_error if \c haveChannel() is false. 
    [[nodiscard]] std::string getChannel() const;
    /// @result True indicates the channel code was set.
    [[nodiscard]] bool haveChannel() const noexcept;

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
    SingleComponentWaveform& operator=(const SingleComponentWaveform &waveform);
    /// @brief Move assignment operator.
    /// @param[in,out] waveform  The waveform whose memory will be moved to
    ///                          this.  On exit, waveform's behavior is
    ///                          undefined.
    /// @result The memory from waveform moved to this.
    SingleComponentWaveform& operator=(SingleComponentWaveform &&waveform) noexcept;
    /// @}

    /// @name Destructors
    /// @{

    /// @brief Releases memory associated with signals.
    void clearSignal() noexcept;
    /// @brief Releases memory and restores the defaults.
    void clear() noexcept;
    /// @brief Destructor.
    ~SingleComponentWaveform();
    /// @}
private:
    class SingleComponentWaveformImpl;
    std::unique_ptr<SingleComponentWaveformImpl> pImpl;
};
}
#endif
